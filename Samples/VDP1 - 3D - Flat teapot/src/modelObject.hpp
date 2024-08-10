#pragma once

#include <srl.hpp>

/** @brief Detect whether object has size function
 * @tparam T Object type
 */
template<typename T>
concept HasLoadSizeFunction = requires {
    { std::declval<T>().LoadSize() } -> std::same_as<size_t>;
};

/** @brief Get object pointer from stream buffer
 * @tparam T Object type
 * @param iterator Stream buffer
 * @param count Number of objects
 * @return T* Object pointer
 */
template<typename T>
T* GetAndIterate(char*& iterator, size_t count = 1)
{
    T* ptr = reinterpret_cast<T*>(iterator);

    if constexpr (HasLoadSizeFunction<T>)
    {
        iterator += ptr->LoadSize() * count;
    }
    else
    {
        iterator += (sizeof(T) * count);
    }

    return ptr;
}
    
/** @brief Model object
 */
class ModelObject
{
private:

	/** @brief Model file header
	 */
	struct ModelHeader
	{
		/** @brief Number of meshes inside the model file
		 */
		size_t MeshCount;

		/** @brief Number of textures inside the mesh file
		 */
		size_t TextureCount;
	};

	/** @brief Texture header, textures are always RGB1555
	 */
	struct TextureHeader
	{
		/** @brief Width of the texture
		 */
		uint16_t Width;

		/** @brief Height of the texture
		 */
		uint16_t Height;

        /** @brief Object size
         * @return Object size
         */
		size_t LoadSize() const
		{
			return sizeof(TextureHeader) + (sizeof(SRL::Types::HighColor) * (Width * Height));
		}

        /** @brief Object data
         * @return The data pointer
         */
		SRL::Types::HighColor* Data() const
		{
			return (SRL::Types::HighColor*)(((char*)this) + sizeof(TextureHeader));
		}
	};

	/** @brief Mesh data header
	 */
	struct MeshHeader
	{
		/** @brief Number of points in the mesh
		 */
		size_t PointCount;

		/** @brief Number of polygons in the mesh
		 */
		size_t PolygonCount;
	};

	/** @brief Face attributes
	 */
	struct Attribute
	{
		/** @brief Indicates whether a texture is applied to this polygon
		 */
		uint8_t HasTexture : 1;

		/** @brief Indicates whether this polygon has a mesh effect applied to it
		 */
		uint8_t HasMeshEffect : 1;

		/** @brief Reserved for future use
		 */
		uint8_t ReservedForFlags : 6;

		/** @brief Reserved for future use
		 */
		uint8_t Reserved;

		/** @brief This field is set if HasTexture field is false
		 */
		SRL::Types::HighColor BaseColor;

		/** @brief Index of a texture to use if HasTexture field is true
		 */
		int32_t Texture;
	};

	/** @brief Loaded mesh data
	 */
	SRL::Types::Mesh* meshes;

	/** @brief Number of loaded meshes
	 */
	size_t meshCount;

	/** @brief Index of first loaded texture
	 */
	int32_t startTextureIndex;

	/** @brief Number of loaded textures
	 */
	size_t textureCount;

public:
	/** @brief Initializes a new model object from a file
	 * @param modelFile Model file
	 */
	ModelObject(const char* modelFile)
	{
        SRL::Cd::File file = SRL::Cd::File(modelFile);

		char* fileBuffer = new char[file.Size.Bytes];
        file.LoadBytes(0, file.Size.Bytes, fileBuffer);

		char* iterator = fileBuffer;

		uint16_t lastTextureIndex = SRL::VDP1::GetTextureCount();
		ModelHeader* header = GetAndIterate<ModelHeader>(iterator);

		// Set defaults
		this->startTextureIndex = -1;
		this->textureCount = header->TextureCount;
		this->meshCount = header->MeshCount;

		this->meshes = new SRL::Types::Mesh[this->meshCount];

		for (size_t meshIndex = 0; meshIndex < this->meshCount; meshIndex++)
		{
			// Get mesh header
			MeshHeader* meshHeader = GetAndIterate<MeshHeader>(iterator);

			SRL::Types::Mesh mesh = SRL::Types::Mesh(meshHeader->PointCount, meshHeader->PolygonCount);
            
            SRL::Types::Vector3D* points = GetAndIterate<SRL::Types::Vector3D>(iterator, meshHeader->PointCount);
            slDMACopy(points, mesh.Vertices, sizeof(SRL::Types::Vector3D) * meshHeader->PointCount);

            SRL::Types::Polygon* faces = GetAndIterate<SRL::Types::Polygon>(iterator, meshHeader->PolygonCount);
            slDMACopy(faces, mesh.Faces, sizeof(SRL::Types::Polygon) * meshHeader->PolygonCount);

			for (size_t attributeIndex = 0; attributeIndex < meshHeader->PolygonCount; attributeIndex++)
			{
				// Read mesh attributes
				Attribute* attributeHeader = GetAndIterate<Attribute>(iterator);

				// Set attributes
				uint16_t textureIndex = No_Texture;
				uint16_t color = attributeHeader->BaseColor;

				if (attributeHeader->HasTexture)
				{
					textureIndex = lastTextureIndex + attributeHeader->Texture;
					color = No_Palet;
				}

				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wnarrowing"
				mesh.Attributes[attributeIndex] = SRL::Types::Attribute(
					SRL::Types::Attribute::FaceVisibility::SingleSided,
					SRL::Types::Attribute::SortMode::Maximum,
					textureIndex,
					color,
					CL32KRGB | No_Gouraud,
					CL32KRGB | (attributeHeader->HasMeshEffect != 1 ? MESHon : MESHoff),
					(attributeHeader->HasTexture != 0 ? sprNoflip : sprPolygon),
					UseLight);
				#pragma GCC diagnostic pop
			}

			this->meshes[meshIndex] = std::move(mesh);
		}

		// Load textures
		for (size_t textureIndex = 0; textureIndex < this->textureCount; textureIndex++)
		{
			// Get header
			TextureHeader* textureHeader = GetAndIterate<TextureHeader>(iterator);

			// Get texture data
			int32_t spriteIndex = SRL::VDP1::TryLoadTexture(textureHeader->Width, textureHeader->Height, SRL::CRAM::TextureColorMode::RGB555, 0, textureHeader->Data());
		}

		// Free the read file
		delete fileBuffer;
	}

	/** @brief Destroy the Model object and free its resources, textures must be freed separately
	 */
	~ModelObject()
	{
		delete[] this->meshes;
		this->meshCount = 0;
	}

	/** @brief Draw specified mesh
	 * @param mesh Mesh index
	 */
	void Draw(size_t mesh)
	{
		if (mesh < this->meshCount)
		{
            SRL::Scene3D::DrawMesh(this->meshes[mesh]);
		}
	}

	/** @brief Draw all loaded meshes
	 */
	void Draw()
	{
		for (size_t mesh = 0; mesh < this->meshCount; mesh++)
		{
            SRL::Scene3D::DrawMesh(this->meshes[mesh]);
		}
	}

	/** @brief Get index of the first texture loaded
	 * @return Index of first texture or -1 if model has no textures
	 */
	constexpr int32_t GetFirstTextureIndex()
	{
		return this->startTextureIndex;
	}

	/** @brief Gets number of loaded meshes
	 * @return Number of loaded meshes
	 */
	constexpr size_t GetMeshCount()
	{
		return this->meshCount;
	}
};

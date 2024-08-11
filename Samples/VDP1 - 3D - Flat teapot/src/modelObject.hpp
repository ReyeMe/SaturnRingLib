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
        /** @brief Mesh type, 0 = PDATA, 1 = XPDATA 
         */
        size_t Type;

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
    void* meshes;

    /** @brief Number of loaded meshes
     */
    size_t meshCount;

    /** @brief Index of first loaded texture
     */
    int32_t startTextureIndex;

    /** @brief Number of loaded textures
     */
    size_t textureCount;

    /** @brief Mesh type
     */
    uint32_t type;

    /** @brief Offset in gouraud table
     */
    size_t gouraudOffset;

    /** @brief Load flat mesh entry
     * @param iterator Stream buffer
     * @param entryId Entry index
     * @param header File header
     */
    void LoadFlatMesh(char* iterator, size_t entryId, ModelHeader* header)
    {
        // Get mesh header
        MeshHeader* meshHeader = GetAndIterate<MeshHeader>(iterator);
        uint16_t lastTextureIndex = SRL::VDP1::GetTextureCount();

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

        ((SRL::Types::Mesh*)this->meshes)[entryId] = std::move(mesh);
    }

    /** @brief Load smooth mesh entry
     * @param iterator Stream buffer
     * @param entryId Entry index
     * @param header File header
     */
    void LoadSmoothMesh(char* iterator, size_t entryId, ModelHeader* header)
    {
        // Get mesh header
        MeshHeader* meshHeader = GetAndIterate<MeshHeader>(iterator);
        uint16_t lastTextureIndex = SRL::VDP1::GetTextureCount();

        SRL::Types::SmoothMesh mesh = SRL::Types::SmoothMesh(meshHeader->PointCount, meshHeader->PolygonCount);
        
        SRL::Types::Vector3D* points = GetAndIterate<SRL::Types::Vector3D>(iterator, meshHeader->PointCount);
        slDMACopy(points, mesh.Vertices, sizeof(SRL::Types::Vector3D) * meshHeader->PointCount);

        SRL::Types::Polygon* faces = GetAndIterate<SRL::Types::Polygon>(iterator, meshHeader->PolygonCount);
        slDMACopy(faces, mesh.Faces, sizeof(SRL::Types::Polygon) * meshHeader->PolygonCount);
        int gouraudIndex = 0xe000 + this->gouraudOffset;

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
                gouraudIndex++,
                CL32KRGB | (attributeHeader->HasMeshEffect != 1 ? MESHon : MESHoff) | CL_Gouraud,
                (attributeHeader->HasTexture != 0 ? sprNoflip : sprPolygon),
                UseGouraud);
            #pragma GCC diagnostic pop
        }

        // Mesh contains XPDATA normals
        SRL::Types::Vector3D* vertexNormals = GetAndIterate<SRL::Types::Vector3D>(iterator, meshHeader->PointCount);
        slDMACopy(vertexNormals, mesh.Normals, sizeof(SRL::Types::Vector3D) * meshHeader->PointCount);

        ((SRL::Types::SmoothMesh*)this->meshes)[entryId] = std::move(mesh);
    }

public:

    /** @brief Initializes a new model object from a file
     * @param modelFile Model file
     * @param gouraudTableStart Offset in gouraud table (used only with smooth meshes)
     */
    ModelObject(const char* modelFile, size_t gouraudTableStart = 0)
    {
        SRL::Cd::File file = SRL::Cd::File(modelFile);

        char* fileBuffer = new char[file.Size.Bytes];
        file.LoadBytes(0, file.Size.Bytes, fileBuffer);

        char* iterator = fileBuffer;
        
        ModelHeader* header = GetAndIterate<ModelHeader>(iterator);

        // Set defaults
        this->startTextureIndex = -1;
        this->textureCount = header->TextureCount;
        this->meshCount = header->MeshCount;
        this->type = header->Type;
        this->gouraudOffset = gouraudOffset;

        this->meshes = header->Type == 1 ? (void*)new SRL::Types::SmoothMesh[this->meshCount] : (void*)new SRL::Types::Mesh[this->meshCount];

        if (header->Type == 1)
        {
            for (size_t meshIndex = 0; meshIndex < this->meshCount; meshIndex++)
            {
                this->LoadSmoothMesh(iterator, meshIndex, header);
            }
        }
        else
        {
            for (size_t meshIndex = 0; meshIndex < this->meshCount; meshIndex++)
            {
                this->LoadFlatMesh(iterator, meshIndex, header);
            }
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
        if (this->type == 0)
        {
            delete[] (SRL::Types::Mesh*)this->meshes;
        }
        else
        {
            delete[] (SRL::Types::SmoothMesh*)this->meshes;
        }

        this->meshCount = 0;
    }

    /** @brief Draw specified mesh
     * @note Used only with flat type mesh data
     * @param mesh Mesh index
     */
    void Draw(size_t mesh)
    {
        if (mesh < this->meshCount && this->type == 0)
        {
            SRL::Scene3D::DrawMesh(((SRL::Types::Mesh*)this->meshes)[mesh]);
        }
    }

    /** @brief Draw specified mesh
     * @note Used only with smooth type mesh data
     * @param mesh Mesh index
     * @param light Light direction, used only with smooth type mesh data
     */
    void Draw(size_t mesh, SRL::Types::Vector3D& light)
    {
        if (mesh < this->meshCount && this->type == 1)
        {
            SRL::Scene3D::DrawSmoothMesh(((SRL::Types::SmoothMesh*)this->meshes)[mesh], light);
        }
    }

    /** @brief Draw all loaded meshes
     * @note Used only with flat type mesh data
     */
    void Draw()
    {
        if (this->type == 0)
        {
            for (size_t mesh = 0; mesh < this->meshCount; mesh++)
            {
                SRL::Scene3D::DrawMesh(((SRL::Types::Mesh*)this->meshes)[mesh]);
            }
        }
    }

    /** @brief Draw all loaded meshes
     * @note Used only with smooth type mesh data
     * @param light Light direction
     */
    void Draw(SRL::Types::Vector3D& light)
    {
        if (this->type == 1)
        {
            for (size_t mesh = 0; mesh < this->meshCount; mesh++)
            {
                SRL::Scene3D::DrawSmoothMesh(((SRL::Types::SmoothMesh*)this->meshes)[mesh], light);
            }
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

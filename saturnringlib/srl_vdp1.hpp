#pragma once

#include "srl_base.hpp"

namespace SRL
{
	/** @brief VDP1 control functions
	 */
	class VDP1
	{
	private:
		/** @brief Pointer to the last free space in the heap
		 */
		inline static  Uint32 HeapPointer = 0;

	public:
		/** @brief VDP1 front bugffer address
		 */
		inline static const Uint32 FrontBuffer = 0x25C80000;

		/** @brief VDP1 ram address
		 */
		inline static const Uint32 RAM = 0x25C00000;

		/** @brief End location of the user area
		 */
		inline static const Uint32 UserAreaEnd = 0x25C7FEF8;

		/** @brief Texture color mode
		 */
		enum class TextureColorMode : Uint16
		{
			/** @brief 256 color image
			 */
			Paletted = 4,

			/** @brief RGB555 image
			 */
			RGB555 = 5
		};

		/** @brief VDP1 texture
		 */
		struct Texture
		{
			/** @brief Texture width
			 */
			Uint16 Width;

			/** @brief Texture height
			 */
			Uint16 Height;
			
			/** @brief Address of the texture
			 */
			Uint16 Address;

			/** @brief Size of the texture for hardwarre
			 * @note Width is divided by 8
			 */
			Uint16 Size;

			/** @brief Construct a new Texture object
			 */
			Texture() : Width(0), Height(0), Address(0), Size(0)
			{
				// Do nothing
			}

			/** @brief Construct a new Texture object
			 * @param width Texture width (must be divisible by 8)
			 * @param height Texture height
			 */
			Texture(const Uint16 width, const Uint16 height) : Width(width), Height(height), Address(0)
			{
				this->Size = ((width & 0x1f8) << 5) | height;
			}

			/** @brief Construct a new Texture object
			 * @param width Texture width (must be divisible by 8)
			 * @param height Texture height
			 * @param address Texture address
			 */
			Texture(const Uint16 width, const Uint16 height, const Uint16 address) : Width(width), Height(height), Address(address)
			{
				this->Size = ((width & 0x1f8) << 5) | height;
			}
		};

		/** @brief Metadata for texture
		 */
		struct TextureMetadata
		{
			/** @brief Texture color mode
			 */
			VDP1::TextureColorMode ColorMode; 

			/** @brief Identifier of the palette (not used in RGB555)
			 */
			Uint16 PaletteId;

			/** @brief Pointer to texture
			 */
			VDP1::Texture* Texture;

			/** @brief Construct a new Texture Metadata object
			 */
			TextureMetadata() : ColorMode(VDP1::TextureColorMode::RGB555), Texture(nullptr)
			{
				// Do nothing
			}

			/** @brief Construct a new Texture Metadata object
			 * @param colorMode Texture color mode
			 * @param palette Id of the pallet (not used in RGB555 mode)
			 */
			TextureMetadata(VDP1::Texture* texture, VDP1::TextureColorMode colorMode, Uint16 palette) : ColorMode(colorMode), Texture(texture), PaletteId(palette)
			{
				// Do nothing
			}

			/** @brief Get texture image data
			 * @return Pointer to image data
			 */
			void* GetData()
			{
				if (this->Texture != nullptr)
				{
					return (void*)(VDP1::RAM + (this->Texture->Address << 3));
				}

				return nullptr;
			}
		};

		/** @brief Texture heap
		 */
		inline static Texture Textures[SRL_MAX_TEXTURES] = { Texture() };

		/** @brief Texture metadata
		 */
		inline static TextureMetadata Metadata[SRL_MAX_TEXTURES] = { TextureMetadata() };

		/** @brief Get free avaialbe memory left for textures on VDP1
		 * @return Number of bytes left 
		 */
		inline static size_t GetAvailableMemory()
		{
			if (VDP1::HeapPointer == 0)
			{
				return VDP1::UserAreaEnd - (VDP1::RAM + 0x1f);
			}

			VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
			return VDP1::UserAreaEnd - (VDP1::RAM + AdjCG(previous.Address << 3, previous.Width, previous.Height, previous.Size));
		}

		/** @brief Try to load a texture
		 * @param width Texture width
		 * @param height Texture height
		 * @param colorMode Color mode
		 * @param palette Pallet start identifier in color RAM (not used in RGB555 mode)
		 * @param data Texture data
		 * @return Index of the loaded texture
		 */
		inline static Uint32 TryLoadTexture(const Uint16 width, const Uint16 height, const VDP1::TextureColorMode colorMode, const Uint16 palette, void* data)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				Uint16 address = (Uint16)CGADDRESS;

				if (VDP1::HeapPointer > 0)
				{
					VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
					address = AdjCG(previous.Address << 3, previous.Width, previous.Height, previous.Size);
				}

				// Create texture entry
				VDP1::Textures[VDP1::HeapPointer] = VDP1::Texture(width, height, address >> 3);

				// Create metadata entry
				VDP1::TextureMetadata metadata = VDP1::TextureMetadata(&VDP1::Textures[VDP1::HeapPointer], colorMode, palette);
				VDP1::Metadata[VDP1::HeapPointer] = metadata;

				// Copy data over to the VDP1
				slDMACopy(data, metadata.GetData(), (Uint32)(((width * height) << 2) >> (Uint32)colorMode));

				// Increase heap pointer
				return VDP1::HeapPointer++;
			}

			// There is no free space left
			return -1;
		}

		/** @brief Fully reset texture heap
		 */
		inline static  void ResetTextureHeap()
		{
			VDP1::HeapPointer = 0;
		}
		
		/** @brief Reset texture heap to specified index
		 * @param index Index to reset to (texture on this index will be overwritten on next TryLoadTexture(); call)
		 */
		inline static  void ResetTextureHeap(const Uint32 index)
		{
			VDP1::HeapPointer = VDP1::HeapPointer > index ? index : VDP1::HeapPointer;
		}
	};
}

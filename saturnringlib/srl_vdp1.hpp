#pragma once

#include "srl_base.hpp"
#include "srl_bitmap.hpp"
#include "srl_debug.hpp"

namespace SRL
{
	/** @brief VDP1 control functions
	 */
	class VDP1
	{
	private:
		/** @brief Pointer to the last free space in the heap
		 */
		inline static Uint16 HeapPointer = 0;

	public:
		/** @brief VDP1 front bugffer address
		 */
		inline static const Uint32 FrontBuffer = 0x25C80000;

		/** @brief End location of the user area
		 */
		inline static const Uint32 UserAreaEnd = 0x25C7FEF8;

		/** @brief Texture color mode
		 */
		enum class TextureColorMode : Uint16
		{
			/** @brief 16 color image
			 */
			Paletted16 = 3,

			/** @brief 256 color image
			 */
			Paletted = 2,

			/** @brief 32k color image
			 */
			RGB555 = 1
		};

		/** @brief VDP1 texture
		 */
		struct Texture : public SRL::SGL::SglType<Texture, TEXTURE>
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
			/** @brief Pointer to texture
			 */
			VDP1::Texture* Texture;

			/** @brief Texture color mode
			 */
			VDP1::TextureColorMode ColorMode; 

			/** @brief Identifier of the palette (not used in RGB555)
			 */
			Uint16 PaletteId;

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
			TextureMetadata(VDP1::Texture* texture, VDP1::TextureColorMode colorMode, Uint32 palette) : ColorMode(colorMode), Texture(texture), PaletteId(palette)
			{
				// Do nothing
			}

			/** @brief Get texture image data
			 * @return Pointer to image data
			 */
			Uint8* GetData()
			{
				if (this->Texture != nullptr)
				{
					return (Uint8*)(SpriteVRAM + (this->Texture->Address << 3));
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
				return VDP1::UserAreaEnd - (SpriteVRAM + 0x1f);
			}

			VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
			return VDP1::UserAreaEnd - (SpriteVRAM + AdjCG(previous.Address << 3, previous.Width, previous.Height, (Uint16)VDP1::Metadata[VDP1::HeapPointer - 1].ColorMode));
		}

		/** @brief Try to load a texture
		 * @param width Texture width
		 * @param height Texture height
		 * @param colorMode Color mode
		 * @param palette Pallet start identifier in color RAM (not used in RGB555 mode)
		 * @param data Texture data
		 * @return Index of the loaded texture
		 */
		inline static Sint32 TryLoadTexture(const Uint16 width, const Uint16 height, const VDP1::TextureColorMode colorMode, const Uint16 palette, void* data)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				Uint32 address = CGADDRESS;

				if (VDP1::HeapPointer > 0)
				{
					VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
					address = AdjCG(previous.Address << 3, previous.Width, previous.Height, (Uint16)VDP1::Metadata[VDP1::HeapPointer - 1].ColorMode);
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

		/** @brief Try to load a texture
		 * @param bitmap Texture to load
		 * @param paletteHandler Palette loader handling (expects index of the palette in CRAM as result, only needed for loading paletted image)
		 * @return Index of the loaded texture
		 */
		inline static Sint32 TryLoadTexture(SRL::Bitmap::IBitmap* bitmap, Uint16 (*paletteHandler)(SRL::Bitmap::Palette*) = nullptr)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				Uint16 palette = 0;
				SRL::Bitmap::BitmapInfo info = bitmap->GetInfo();
				Uint32 address = CGADDRESS;

				if (info.Palette != nullptr)
				{
					if (paletteHandler != nullptr)
					{
						palette = paletteHandler(info.Palette);
					}
					else
					{
						// Palette loader not specified
						return -1;
					}
				}

				return VDP1::TryLoadTexture(info.Width, info.Height, (VDP1::TextureColorMode)info.ColorMode, palette, bitmap->GetData());
			}

			// There is no free space left
			return -1;
		}

		/** @brief Fully reset texture heap
		 */
		inline static void ResetTextureHeap()
		{
			VDP1::HeapPointer = 0;
		}
		
		/** @brief Reset texture heap to specified index
		 * @param index Index to reset to (texture on this index will be overwritten on next TryLoadTexture(); call)
		 */
		inline static void ResetTextureHeap(const Uint16 index)
		{
			VDP1::HeapPointer = VDP1::HeapPointer > index ? index : VDP1::HeapPointer;
		}
	};
}

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
		inline static uint16_t HeapPointer = 0;

        /** @brief Get the number of bits to shift to the right
         * @param colorMode Color mode
         * @return Number of bits to shift
         */
        inline static uint8_t GetSizeShifter(CRAM::TextureColorMode colorMode)
        {
            uint16_t pixelSizeShifter = 0;

            switch (colorMode)
            {
            case CRAM::TextureColorMode::Paletted256:
            case CRAM::TextureColorMode::Paletted128:
            case CRAM::TextureColorMode::Paletted64:
                pixelSizeShifter = 1;
                break;
            
            case CRAM::TextureColorMode::Paletted16:
                pixelSizeShifter = 2;
                break;

            default:
                break;
            }

            return pixelSizeShifter;
        }

	public:

		/** @brief VDP1 front buffer address
		 */
		inline static const uint32_t FrontBuffer = 0x25C80000;

		/** @brief End location of the user area
		 */
		inline static const uint32_t UserAreaEnd = 0x25C7FEF8;

		/** @brief VDP1 texture
		 */
		struct Texture : public SRL::SGL::SglType<Texture, TEXTURE>
		{
			/** @brief Texture width
			 */
			uint16_t Width;

			/** @brief Texture height
			 */
			uint16_t Height;
			
			/** @brief Address of the texture
			 */
			uint16_t Address;

			/** @brief Size of the texture for hardware
			 * @note Width is divided by 8
			 */
			uint16_t Size;

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
			Texture(const uint16_t width, const uint16_t height) : Width(width), Height(height), Address(0)
			{
				this->Size = ((width & 0x1f8) << 5) | height;
			}

			/** @brief Construct a new Texture object
			 * @param width Texture width (must be divisible by 8)
			 * @param height Texture height
			 * @param address Texture address
			 */
			Texture(const uint16_t width, const uint16_t height, const uint16_t address) : Width(width), Height(height), Address(address)
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
			CRAM::TextureColorMode ColorMode; 

			/** @brief Identifier of the palette (not used in RGB555)
			 */
			uint16_t PaletteId;

			/** @brief Construct a new Texture Metadata object
			 */
			TextureMetadata() : ColorMode(CRAM::TextureColorMode::RGB555), Texture(nullptr)
			{
				// Do nothing
			}

			/** @brief Construct a new Texture Metadata object
             * @param texture Texture info
			 * @param colorMode Texture color mode
			 * @param palette Id of the pallet (not used in RGB555 mode)
			 */
			TextureMetadata(VDP1::Texture* texture, CRAM::TextureColorMode colorMode, uint32_t palette) : ColorMode(colorMode), Texture(texture), PaletteId(palette)
			{
				// Do nothing
			}

			/** @brief Get texture image data
			 * @return Pointer to image data
			 */
			uint8_t* GetData()
			{
				if (this->Texture != nullptr)
				{
					return (uint8_t*)(SpriteVRAM + (this->Texture->Address << 3));
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

		/** @brief Get free available memory left for textures on VDP1
		 * @return Number of bytes left 
		 */
		inline static size_t GetAvailableMemory()
		{
			if (VDP1::HeapPointer == 0)
			{
				return VDP1::UserAreaEnd - (SpriteVRAM + 0x1f);
			}

			VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
			return VDP1::UserAreaEnd - (SpriteVRAM + AdjCG(previous.Address << 3, previous.Width, previous.Height, (uint16_t)VDP1::Metadata[VDP1::HeapPointer - 1].ColorMode));
		}

        /** @brief Get the start location of the gouraud table
         * @return HighColor* Start location of the gouraud table
         */
        inline static Types::HighColor* GetGouraudTable()
        {
            return (Types::HighColor*)(SpriteVRAM + 0x70000);
        }

		/** @brief Try to load a texture
		 * @param width Texture width
		 * @param height Texture height
		 * @param colorMode Color mode
		 * @param palette Palette start identifier in color RAM (not used in RGB555 mode)
		 * @param data Texture data
		 * @return Index of the loaded texture
		 */
		inline static int32_t TryLoadTexture(const uint16_t width, const uint16_t height, const CRAM::TextureColorMode colorMode, const uint16_t palette, void* data)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				uint32_t address = CGADDRESS;

				if (VDP1::HeapPointer > 0)
				{
					VDP1::Texture previous = VDP1::Textures[VDP1::HeapPointer - 1];
					address = AdjCG(previous.Address << 3, previous.Width, previous.Height, VDP1::GetSizeShifter(VDP1::Metadata[VDP1::HeapPointer - 1].ColorMode));
				}

				// Create texture entry
				VDP1::Textures[VDP1::HeapPointer] = VDP1::Texture(width, height, address >> 3);

				// Create metadata entry
				VDP1::TextureMetadata metadata = VDP1::TextureMetadata(&VDP1::Textures[VDP1::HeapPointer], colorMode, palette);
				VDP1::Metadata[VDP1::HeapPointer] = metadata;

				// Copy data over to the VDP1
				slDMACopy(data, metadata.GetData(), (uint32_t)(((width * height) << 1) >> VDP1::GetSizeShifter(colorMode)));

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
		inline static int32_t TryLoadTexture(SRL::Bitmap::IBitmap* bitmap, int16_t (*paletteHandler)(SRL::Bitmap::BitmapInfo*) = nullptr)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				int16_t palette = 0;
				SRL::Bitmap::BitmapInfo info = bitmap->GetInfo();

				if (info.Palette != nullptr)
				{
					if (paletteHandler != nullptr)
					{
						palette = paletteHandler(&info);

                        if (palette == -1)
                        {
                            return -1;
                        }
					}
					else
					{
						// Palette loader not specified
						return -1;
					}
				}

				return VDP1::TryLoadTexture(info.Width, info.Height, (CRAM::TextureColorMode)info.ColorMode, palette, bitmap->GetData());
			}

			// There is no free space left
			return -1;
		}

		/** @brief Try to load a texture
		 * @param bitmap Texture to load
		 * @param palette Color palette number
		 * @return Index of the loaded texture
		 */
		inline static int32_t TryLoadTexture(SRL::Bitmap::IBitmap* bitmap, const int16_t& palette)
		{
			if (VDP1::HeapPointer < SRL_MAX_TEXTURES)
			{
				SRL::Bitmap::BitmapInfo info = bitmap->GetInfo();
				return VDP1::TryLoadTexture(info.Width, info.Height, (CRAM::TextureColorMode)info.ColorMode, palette, bitmap->GetData());
			}

			// There is no free space left
			return -1;
		}

        /** @brief Get the number of currently loaded textures
         *  @return Number of currently loaded textures
         */
        inline static uint16_t GetTextureCount()
        {
            return VDP1::HeapPointer;
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
		inline static void ResetTextureHeap(const uint16_t index)
		{
			VDP1::HeapPointer = VDP1::HeapPointer > index ? index : VDP1::HeapPointer;
		}
	};
}

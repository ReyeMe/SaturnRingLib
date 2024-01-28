#pragma once

#include "../srl_debug.hpp"
#include "srl_bitmap.hpp"
#include "../srl_cd.hpp"

/*
 * This TGA loader is loosely based on TGA loader from yaul by:
 * Israel Jacquez <mrkotfw@gmail.com>
 * David Oberhollenzer
 */

namespace SRL::Bitmap
{
	/** @brief TGA image handling
	 */
	struct TGA : IBitmap
	{
	private:
		/** @brief Size of the header
		 */
		constexpr inline static const Uint32 HeaderSize = 18;

		/** @brief Known types
		 */
		enum class TgaTypes
		{
			TgaNone = 0,
			TgaPaletted = 1,
			TgaTrueColor = 2,
			TgaGrayscale = 3,
			TgaRlePaletted = 9,
			TgaRleTrueColor = 10,
			TgaRleGrayscale = 11
		};

		/** @brief TGA data point
		 */
		struct TgaPoint
		{
			/** @brief X coordinate
			 */
			Uint16 X;

			/** @brief Y coordinate
			 */
			Uint16 Y;

			/** @brief Construct a new Tga Point object
			 */
			TgaPoint()
			{
				// Do nothing
			}

			/** @brief Construct a new Tga Point object
			 * @param data little endian data
			 */
			TgaPoint(Uint8* data)
			{
				this->X = SRL::Utils::Endianity::Swap<Uint16>(*(data++));
				data++;

				this->Y = SRL::Utils::Endianity::Swap<Uint16>(*(data++));
				data++;
			}
		};

		/** @brief Palette description
		 */
		struct TgaPalette
		{
			/** @brief Start of the palette data
			 */
			Uint16 PaletteStart;

			/** @brief Length of the palette data
			 */
			Uint16 PaletteLength;

			/** @brief Color bit depth of the palette entry
			 */
			Uint8 PaletteColorDepth;

			/** @brief Construct a new Tga Palette object
			 */
			TgaPalette()
			{
				// Do nothing
			}

			/** @brief Construct a new Tga Palette object
			 * @param data little endian data
			 */
			TgaPalette(Uint8* data)
			{
				this->PaletteStart = SRL::Utils::Endianity::Swap<Uint16>(*(data++));
				data++;

				this->PaletteLength = SRL::Utils::Endianity::Swap<Uint16>(*(data++));
				data++;

				this->PaletteColorDepth = *(data++);
			}
		};

		/** @brief Image description
		 */
		struct TgaImage
		{
			/** @brief Image origin point
			 */
			TGA::TgaPoint Origin;

			/** @brief Image size
			 */
			TGA::TgaPoint Size;

			/** @brief Color bit depth
			 */
			Uint8 PixelColorDepth;

			/** @brief Image descriptor
			 */
			Uint8 Descriptor;

			/** @brief Construct a new Tga Image object
			 */
			TgaImage()
			{
				// Do nothing
			}

			/** @brief Construct a new Tga Image object
			 * @param data little endian data
			 */
			TgaImage(Uint8* data)
			{
				this->Origin = TGA::TgaPoint(data);
				this->Size = TGA::TgaPoint(data);
				this->PixelColorDepth = *(data++);
				this->Descriptor = *(data++);
			}
		};

		/** @brief TGA image header
		 */
		struct TgaHeader
		{
			/** @brief Image identifier length
			 */
			Uint8 ImageIdLength;

			/** @brief Has palette
			 */
			Uint8 HasPalette;

			/** @brief Image type
			 */
			Uint8 ImageType;

			/** @brief Palette description
			 */
			TGA::TgaPalette Palette;

			/** @brief Image description
			 */
			TGA::TgaImage Image;

			/** @brief Construct a new Tga Header object
			 * @param data little endian data
			 */
			TgaHeader(Uint8* data)
			{
				this->ImageIdLength = *(data++);
				this->HasPalette = *(data++);
				this->ImageType = *(data++);
				this->Palette = TGA::TgaPalette(data);
				this->Image = TGA::TgaImage(data);
			}
		};

		/** @brief Height of the bitmap
		 */
		Uint16 height;

		/** @brief Image data
		 */
		Uint8* imageData;

		/** @brief Color palette
		 */
		Bitmap::Palette* palette;

		/** @brief Width of the bitmap
		 */
		Uint16 width;

		/** @brief Check if format is wrong or not
		 * @param header Tga header
		 * @return True if format is supported
		 */
		inline static bool IsFormatValid(const TGA::TgaHeader& header)
		{
			Uint8 bitDepth = header.Image.PixelColorDepth >> 3;

			switch (header.ImageType)
			{
			case ((Uint8)TGA::TgaTypes::TgaGrayscale):
			case ((Uint8)TGA::TgaTypes::TgaRleGrayscale):
				if (bitDepth != 1 || header.HasPalette) return false;
				break;
			
			case ((Uint8)TGA::TgaTypes::TgaTrueColor):
			case ((Uint8)TGA::TgaTypes::TgaRleTrueColor):
				if ((bitDepth != 2 && bitDepth != 3 && bitDepth != 4) || header.HasPalette) return false;
				break;
			
			case ((Uint8)TGA::TgaTypes::TgaPaletted):
			case ((Uint8)TGA::TgaTypes::TgaRlePaletted):
				if (header.Palette.PaletteStart >= header.Palette.PaletteLength || !header.HasPalette) return false;

				switch (header.Palette.PaletteColorDepth)
				{
				case 15:
				case 16:
				case 24:
				case 32:
					break;
				
				default:
					return false;
				}

				break;

			default:
				return false;
			}

			return true;
		}

		/** @brief Get offset to data block
		 * @param header TGA header
		 * @return Offset to the data block
		 */
		constexpr inline static Uint32 ImageDataOffset(const TGA::TgaHeader& header)
		{
			return TGA::HeaderSize + header.ImageIdLength + (header.Palette.PaletteLength * (header.Palette.PaletteColorDepth >> 3));
		}

		/** @brief Get offset to palette block
		 * @param header TGA header
		 * @return Offset to the palette block
		 */
		constexpr inline static Uint32 ImagePaletteOffset(const TGA::TgaHeader& header)
		{
			return TGA::HeaderSize + header.ImageIdLength;
		}

		/** @brief Decode paletted image
		 * @param data Image data
		 */
		void DecodePaletted(Uint8* stream, const TGA::TgaHeader& header)
		{
			// Allocated space for image data
			this->imageData = (Uint8*)Memory::Malloc(this->width * this->height);
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));


		}
		
		/** @brief Decode paletted image with RLE compression
		 * @param data Image data
		 */
		void DecodePalettedRle(Uint8* stream, const TGA::TgaHeader& header)
		{
			// Allocated space for image data
			this->imageData = (Uint8*)Memory::Malloc(this->width * this->height);
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));

		}
		
		/** @brief Decode true color image
		 * @param data Image data
		 */
		void DecodeTrueColor(Uint8* stream, const TGA::TgaHeader& header)
		{
			// Allocated space for image data
			this->imageData = (Uint8*)Memory::Malloc(this->width * this->height * sizeof(SRL::Types::SaturnColor));
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));

		}
		
		/** @brief Decode true color image with RLE compression
		 * @param data Image data
		 */
		void DecodeTrueColorRle(Uint8* stream, const TGA::TgaHeader& header)
		{
			// Allocated space for image data
			this->imageData = (Uint8*)Memory::Malloc(this->width * this->height * sizeof(SRL::Types::SaturnColor));
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));

		}

	public:

		/** @brief Construct RGB555 TGA image from file
		 * @param filename TGA file name
		 */
		TGA(char* filename) : imageData(nullptr), palette(nullptr)
		{
			Cd::File tgaFile = Cd::File(filename);
			
			// Open file
			if (tgaFile.Open())
			{
				// Load file
				Uint8* stream = (Uint8*)Memory::Malloc(tgaFile.Size);
				tgaFile.Read(0, tgaFile.Size, stream);
				Uint8* data = stream;

				// Close the file
				tgaFile.Close();

				// Load header, this is a bit complicated since the header not only is not alligned, but is also little endian
				TgaHeader header = TgaHeader(data);

				// Lets check whether the header makes sense
				if (header.Image.Size.X == 0 || header.Image.Size.Y == 0 ||
					header.Image.Size.X > 512 || header.Image.Size.Y > 482)
				{
					// Image has no size or is too big
					SRL::Debug::Assert("Image has no size or is too big!");
				}
				
				// Check format
				if (!this->IsFormatValid(header))
				{
					// We do not know how to read this type
					SRL::Debug::Assert("Image is of unsupported type!");
				}

				// Set TGA object stuff
				this->width = header.Image.Size.X;
				this->height = header.Image.Size.Y;

				// Data stream should now be pointing to after the header
				switch (header.ImageType)
				{
				case ((Uint8)TGA::TgaTypes::TgaPaletted):
					this->DecodePaletted(stream, header);
					break;

				case ((Uint8)TGA::TgaTypes::TgaRlePaletted):
					this->DecodePalettedRle(stream, header);
					break;
				
				case ((Uint8)TGA::TgaTypes::TgaTrueColor):
					this->DecodeTrueColor(stream, header);
					break;

				case ((Uint8)TGA::TgaTypes::TgaRleTrueColor):
					this->DecodeTrueColorRle(stream, header);
					break;
				
				default:
					SRL::Debug::Assert("Image is of unsupported type!\nCould not decode the image.");
					break;
				}

				// Clear allocated memory
				Memory::Free(stream);
			}
		}

		/** @brief Destroy the TGA image
		 */
		~TGA()
		{
			if (this->imageData != nullptr)
			{
				delete this->imageData;
			}

			if (this->palette != nullptr)
			{
				delete this->palette->Colors;
				delete this->palette;
			}
		}

		/** @brief Get image data
		 * @return Pointer to image data
		 */
		Uint8* GetData() override
		{
			return this->imageData;
		}

		/** @brief Get image info
		 * @return image info
		 */
		BitmapInfo GetInfo() override
		{
			if (this->palette != nullptr)
			{
				return BitmapInfo(this->width, this->height, this->palette);
			}

			return BitmapInfo(this->width, this->height);
		}
	};
}
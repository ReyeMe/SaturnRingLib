#pragma once

#include "srl_debug.hpp"
#include "srl_bitmap.hpp"
#include "srl_cd.hpp"

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
			Sint16 X;

			/** @brief Y coordinate
			 */
			Sint16 Y;
		};

		/** @brief Palette description
		 */
		struct TgaPalette
		{
			/** @brief Start of the palette data
			 */
			Sint16 PaletteStart;

			/** @brief Length of the palette data
			 */
			Sint16 PaletteLength;

			/** @brief Color bit depth of the palette entry
			 */
			Sint8 PaletteColorDepth;

			/** @brief 
			 */
			Sint8 Reserved;
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
			Sint8 PixelColorDepth;

			/** @brief Image descriptor
			 */
			Sint8 Descriptor;
		};

		/** @brief TGA image header
		 */
		struct TgaHeader
		{
			/** @brief Image identifier length
			 */
			Sint8 ImageIdLength;

			/** @brief Has palette
			 */
			Sint8 HasPalette;

			/** @brief Image type
			 */
			Sint8 ImageType;

			/** @brief Palette description
			 */
			TGA::TgaPalette Palette;

			/** @brief Image description
			 */
			TGA::TgaImage Image;
		};

		/** @brief Run lenght encoding packet
		 */
		struct TgaRlePacket
		{
			enum class PacketType : Uint8
			{
				/** @brief Count contains number of pixels after the packet
				 */
				RawPacket = 0,

				/** @brief Count contains number of repetitions of a color specified after the packet
				 */
				RlePacket = 1
			};

			/** @brief Type of the packet
			 */
			PacketType Type:1;

			/** @brief Number of pixels or repetitions
			 */
			Uint8 Count:7;
		};
		
		/** @brief Color palette
		 */
		Bitmap::Palette* palette;

		/** @brief Width of the bitmap
		 */
		size_t width;

		/** @brief Height of the bitmap
		 */
		size_t height;

		/** @brief Image data
		 */
		Uint8* imageData;

		/** @brief Deserialize number
		 * @param buf Value buffer
		 * @return Deserialited value
		 */
		constexpr inline static Uint16 DeserializeUint16(Uint8 *buf)
		{
			return (*(buf + 1) << 8) | *(buf);
		}

		/** @brief Deserialize number
		 * @param buf Value buffer
		 * @return Deserialited value
		 */
		constexpr inline static Uint32 DeserializeUint32(Uint8 *buf)
		{
			return (*(buf + 3) << 24) | (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
		}

		/** @brief Deserialize number
		 * @param buf Value buffers
		 * @return Deserialited value
		 */
		constexpr inline static Uint32 DeserializeUint24(Uint8 *buf)
		{
			return (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
		}

		/** @brief Check if format is wrong or not
		 * @param header Tga header
		 * @return True if format is supported
		 */
		static bool IsFormatValid(const TGA::TgaHeader* header)
		{
			Uint8 bitDepth = header->Image.PixelColorDepth >> 3;

			switch (header->ImageType)
			{
			case ((Sint8)TGA::TgaTypes::TgaGrayscale):
			case ((Sint8)TGA::TgaTypes::TgaRleGrayscale):
				if (bitDepth != 1 || header->HasPalette) return false;
				break;
			
			case ((Sint8)TGA::TgaTypes::TgaTrueColor):
			case ((Sint8)TGA::TgaTypes::TgaRleTrueColor):
				if ((bitDepth != 2 && bitDepth != 3 && bitDepth != 4) || header->HasPalette) return false;
				break;
			
			case ((Sint8)TGA::TgaTypes::TgaPaletted):
			case ((Sint8)TGA::TgaTypes::TgaRlePaletted):
				if (header->Palette.PaletteStart >= header->Palette.PaletteLength || !header->HasPalette) return false;

				switch (header->Palette.PaletteColorDepth)
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
		constexpr inline static Uint32 ImageDataOffset(const TGA::TgaHeader* header)
		{
			return TGA::HeaderSize + header->ImageIdLength + (header->Palette.PaletteLength * (header->Palette.PaletteColorDepth >> 3));
		}

		/** @brief Get offset to palette block
		 * @param header TGA header
		 * @return Offset to the palette block
		 */
		constexpr inline static Uint32 ImagePaletteOffset(const TGA::TgaHeader* header)
		{
			return TGA::HeaderSize + header->ImageIdLength;
		}

		/** @brief Decode image header
		 * @param stream File stream
		 * @param header File header
		 * @param transparentColor defines a color that should be changed to be transparent
		 */
		static Bitmap::Palette* DecodePalette(Uint8* stream, const TGA::TgaHeader* header, Sint32 transparentColor)
		{
			Uint8* buffer = (stream + TGA::ImagePaletteOffset(header));
			Bitmap::Palette* palette = new Bitmap::Palette(header->Palette.PaletteLength);

			for (Sint32 index = 0; index < header->Palette.PaletteLength; index++)
			{
				if (index != transparentColor)
				{
					switch (header->Palette.PaletteColorDepth >> 3)
					{
					case 2:
						palette->Colors[index] = SRL::Types::SaturnColor::FromARGB15(TGA::DeserializeUint16(&buffer[index << 1]));
						break;
					
					// Loading RGB24 is a tad slower thx to the multiplication
					case 3:
						palette->Colors[index] = SRL::Types::SaturnColor::FromRGB24(TGA::DeserializeUint24(&buffer[index * 3]));
						break;
						
					default:
					case 4:
						palette->Colors[index] = SRL::Types::SaturnColor::FromARGB32(TGA::DeserializeUint32(&buffer[index << 2]));
						break;
					}
				}
				else
				{
					palette->Colors[index] = SRL::Types::SaturnColor();
				}
			}

			return palette;
		}

		/** @brief Decode paletted image
		 * @param stream File stream
		 * @param header File header
		 */
		inline void DecodePaletted(Uint8* stream, const TGA::TgaHeader* header)
		{
			// Allocated space for image data
			Uint32 pixels = this->width * this->height;
			this->imageData = new Uint8[pixels];
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));

			// Read palette data
			if (header->Palette.PaletteLength - 1 <= 16)
			{
				// 16 color palette
				for (Uint32 index = 0; index < pixels; index += 2)
				{
					this->imageData[index] = ((buffer[index] && 0x0f) << 4) | (buffer[index + 1] && 0x0f);
				}
			}
			else
			{
				// 256 color palette
				for (Uint32 index = 0; index < pixels; index++)
				{
					this->imageData[index] = buffer[index];
				}
			}
		}
		
		/** @brief Decode true color image
		 * @param data Image data
		 * @param header File header
		 * @param transparentColor defines a color that should be changed to be transparent
		 */
		inline void DecodeTrueColor(Uint8* stream, const TGA::TgaHeader* header, SRL::Types::SaturnColor transparentColor)
		{
			// Allocated space for image data
			Uint32 size = this->width * this->height;
			this->imageData = (Uint8*)new SRL::Types::SaturnColor[size];
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));
			Uint8 depth = header->Image.PixelColorDepth >> 3;

			// Read image data
			for (Uint32 index = 0; index < size; index++)
			{
				Uint8* pixelData = buffer + (depth * index);
				SRL::Types::SaturnColor color;

				switch (depth)
				{
				case 2:
					color = SRL::Types::SaturnColor::FromARGB15(TGA::DeserializeUint16(pixelData));
					break;
				
				case 3:
					color = SRL::Types::SaturnColor::FromRGB24(TGA::DeserializeUint24(pixelData));
					break;
					
				default:
				case 4:
					color = SRL::Types::SaturnColor::FromARGB32(TGA::DeserializeUint32(pixelData));
					break;
				}
				
				((SRL::Types::SaturnColor*)this->imageData)[index] = color != transparentColor ? color : SRL::Types::SaturnColor();
			}
		}
		
		/** @brief Decode true color image with RLE compression
		 * @param data Image data
		 * @param header File header
		 * @param transparentColor defines a color that should be changed to be transparent
		 */
		inline void DecodeTrueColorRle(Uint8* stream, const TGA::TgaHeader* header, SRL::Types::SaturnColor transparentColor)
		{
			// Allocated space for image data
			Uint32 size = this->width * this->height;
			this->imageData = (Uint8*)new SRL::Types::SaturnColor[size];
			Uint8* buffer = (stream + TGA::ImageDataOffset(header));
			Uint8 depth = header->Image.PixelColorDepth >> 3;
			SRL::Types::SaturnColor fill;

			for (Uint32 pixel = 0; pixel < size;)
			{
				TgaRlePacket* packet = (TgaRlePacket*)(buffer++);

				switch (packet->Type)
				{
				case TgaRlePacket::PacketType::RawPacket:
					for (int packed = 0; packed <= packet->Count; packed++)
					{
						SRL::Types::SaturnColor color;

						// Read pixel
						switch (depth)
						{
						case 2:
							color = SRL::Types::SaturnColor::FromARGB15(TGA::DeserializeUint16(buffer));
							break;
						
						case 3:
							color = SRL::Types::SaturnColor::FromRGB24(TGA::DeserializeUint24(buffer));
							break;
							
						default:
						case 4:
							color = SRL::Types::SaturnColor::FromARGB32(TGA::DeserializeUint32(buffer));
							break;
						}

						((SRL::Types::SaturnColor*)this->imageData)[pixel] = color != transparentColor ? color : SRL::Types::SaturnColor();

						// Move to next pixel data
						buffer += depth;

						// Move to next pixel
						pixel++;
					}

					break;
				
				default:
				case TgaRlePacket::PacketType::RlePacket:
					// Read pixel color
					switch (depth)
					{
					case 2:
						fill = SRL::Types::SaturnColor::FromARGB15(TGA::DeserializeUint16(buffer));
						break;
					
					case 3:
						fill = SRL::Types::SaturnColor::FromRGB24(TGA::DeserializeUint24(buffer));
						break;
						
					default:
					case 4:
						fill= SRL::Types::SaturnColor::FromARGB32(TGA::DeserializeUint32(buffer));
						break;
					}
		
					fill = fill != transparentColor ? fill : SRL::Types::SaturnColor();

					// Repeat the pixel color
					for (int repeater = 0; repeater <= packet->Count; repeater++)
					{
						((SRL::Types::SaturnColor*)this->imageData)[pixel++] = fill;
					}

					// Move to next pixel data
					buffer += depth;
					break;
				}
			}
		}

	public:

		/** @brief Settings for TGA loader
		 */
		struct LoaderSettings
		{
			/** @brief Defines a index of color in palette that should be changed to be transparent
			 * @note Used only with paletted images
			 */
			Sint32 TransparentColorIndex;
			
			/** @brief Defines a color that should be changed to be transparent
			 * @note Used only with RGB images
			 */
			SRL::Types::SaturnColor TransparentColor;

			/** @brief Construct a new loader settings object
			 */
			LoaderSettings() : TransparentColor(SRL::Types::SaturnColor()), TransparentColorIndex(-1)
			{
				// Do nothing
			}
		};

	private:

		void LoadData(Cd::File* file, LoaderSettings* settings)
		{
			Uint8* stream = new Uint8[file->Size.Bytes + 1];
			
			// Open file
			if (file->OpenBatch(0, file->Size.Bytes, stream) >= 0)
			{
				// Load file
				Uint8* data = stream;

				// Load header, this is a bit complicated since the header not only is not alligned, but is also little endian
				TgaHeader header;
				header.ImageIdLength = *(data);
				header.HasPalette = *(data + 1);
				header.ImageType = *(data + 2);
				header.Palette.PaletteStart = TGA::DeserializeUint16(data + 3);
				header.Palette.PaletteLength = TGA::DeserializeUint16(data + 5);
				header.Palette.PaletteColorDepth = *(data + 7);
				header.Image.Origin.X = TGA::DeserializeUint16(data + 8);
				header.Image.Origin.Y = TGA::DeserializeUint16(data + 10);
				header.Image.Size.X = TGA::DeserializeUint16(data + 12);
				header.Image.Size.Y = TGA::DeserializeUint16(data + 14);
				header.Image.PixelColorDepth = *(data + 16);
				header.Image.Descriptor = *(data + 17);
				
				// Lets check whether the header makes sense
				if (header.Image.Size.X == 0 || header.Image.Size.Y == 0 ||
					header.Image.Size.X > 512 || header.Image.Size.Y > 482)
				{
					// Image has no size or is too big
					SRL::Debug::Assert("Image has no size or is too big!\nWidth=%d\nHeight=%d", header.Image.Size.X, header.Image.Size.Y);
				}

				// Check format
				if (!TGA::IsFormatValid(&header))
				{
					// We do not know how to read this type
					SRL::Debug::Assert("Image is of unsupported type!");
				}

				// Set TGA object stuff
				this->width = (size_t)header.Image.Size.X;
				this->height = (size_t)header.Image.Size.Y;

				// Data stream should now be pointing to after the header
				switch (header.ImageType)
				{
				case ((Sint8)TGA::TgaTypes::TgaPaletted):
					this->palette = TGA::DecodePalette(stream, &header, settings->TransparentColorIndex);
					this->DecodePaletted(stream, &header);
					break;

				case ((Sint8)TGA::TgaTypes::TgaTrueColor):
					this->DecodeTrueColor(stream, &header, settings->TransparentColor);
					break;

				case ((Sint8)TGA::TgaTypes::TgaRleTrueColor):
					this->DecodeTrueColorRle(stream, &header, settings->TransparentColor);
					break;
				
				default:
					SRL::Debug::Assert("Image is of unsupported type!\nCould not decode the image.");
					break;
				}

				// Clear allocated memory
				delete stream;
			}
		}

	public:

		/** @brief Construct RGB555 TGA image from file
		 * @param filename TGA file name
		 * @param settings TGA loader settings
		 */
		TGA(Cd::File* data, TGA::LoaderSettings settings = TGA::LoaderSettings()) : imageData(nullptr), palette(nullptr)
		{
			this->LoadData(data, &settings);
		}

		/** @brief Construct RGB555 TGA image from file
		 * @param filename TGA file name
		 * @param settings TGA loader settings
		 */
		TGA(const char* filename, TGA::LoaderSettings settings = TGA::LoaderSettings()) : imageData(nullptr), palette(nullptr)
		{
			Cd::File file = Cd::File(filename);
			this->LoadData(&file, &settings);
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
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
        constexpr inline static const uint32_t HeaderSize = 18;

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
            int16_t X;

            /** @brief Y coordinate
             */
            int16_t Y;
        };

        /** @brief Palette description
         */
        struct TgaPalette
        {
            /** @brief Start of the palette data
             */
            int16_t PaletteStart;

            /** @brief Length of the palette data
             */
            int16_t PaletteLength;

            /** @brief Color bit depth of the palette entry
             */
            int8_t PaletteColorDepth;

            /** @brief 
             */
            int8_t Reserved;
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
            int8_t PixelColorDepth;

            /** @brief Image descriptor
             */
            int8_t Descriptor;
        };

        /** @brief TGA image header
         */
        struct TgaHeader
        {
            /** @brief Image identifier length
             */
            int8_t ImageIdLength;

            /** @brief Has palette
             */
            int8_t HasPalette;

            /** @brief Image type
             */
            int8_t ImageType;

            /** @brief Palette description
             */
            TGA::TgaPalette Palette;

            /** @brief Image description
             */
            TGA::TgaImage Image;
        };

        /** @brief Run length encoding packet
         */
        struct TgaRlePacket
        {
            enum class PacketType : uint8_t
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
            uint8_t Count:7;
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
        uint8_t* imageData;

        /** @brief Deserialize number
         * @param buf Value buffer
         * @return Deserialized value
         */
        constexpr inline static uint16_t Deserializeuint16_t(uint8_t *buf)
        {
            return (*(buf + 1) << 8) | *(buf);
        }

        /** @brief Deserialize number
         * @param buf Value buffer
         * @return Deserialized value
         */
        constexpr inline static uint32_t Deserializeuint32_t(uint8_t *buf)
        {
            return (*(buf + 3) << 24) | (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
        }

        /** @brief Deserialize number
         * @param buf Value buffers
         * @return Deserialized value
         */
        constexpr inline static uint32_t DeserializeUint24(uint8_t *buf)
        {
            return (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
        }

        /** @brief Check if format is wrong or not
         * @param header Tga header
         * @return True if format is supported
         */
        static bool IsFormatValid(const TGA::TgaHeader* header)
        {
            uint8_t bitDepth = header->Image.PixelColorDepth >> 3;

            switch (header->ImageType)
            {
            case ((int8_t)TGA::TgaTypes::TgaGrayscale):
            case ((int8_t)TGA::TgaTypes::TgaRleGrayscale):
                if (bitDepth != 1 || header->HasPalette) return false;
                break;
            
            case ((int8_t)TGA::TgaTypes::TgaTrueColor):
            case ((int8_t)TGA::TgaTypes::TgaRleTrueColor):
                if ((bitDepth != 2 && bitDepth != 3 && bitDepth != 4) || header->HasPalette) return false;
                break;
            
            case ((int8_t)TGA::TgaTypes::TgaPaletted):
            case ((int8_t)TGA::TgaTypes::TgaRlePaletted):
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
        constexpr inline static uint32_t ImageDataOffset(const TGA::TgaHeader* header)
        {
            return TGA::HeaderSize + header->ImageIdLength + (header->Palette.PaletteLength * (header->Palette.PaletteColorDepth >> 3));
        }

        /** @brief Get offset to palette block
         * @param header TGA header
         * @return Offset to the palette block
         */
        constexpr inline static uint32_t ImagePaletteOffset(const TGA::TgaHeader* header)
        {
            return TGA::HeaderSize + header->ImageIdLength;
        }

        /** @brief Decode image header
         * @param stream File stream
         * @param header File header
         * @param transparentColor defines a color that should be changed to be transparent
         */
        static Bitmap::Palette* DecodePalette(uint8_t* stream, const TGA::TgaHeader* header, int32_t transparentColor)
        {
            uint8_t* buffer = (stream + TGA::ImagePaletteOffset(header));
            Bitmap::Palette* palette = new Bitmap::Palette(header->Palette.PaletteLength);
            uint8_t depth = header->Palette.PaletteColorDepth >> 3;

            for (int32_t index = 0; index < header->Palette.PaletteLength; index++)
            {
                if (index != transparentColor)
                {
                    switch (depth)
                    {
                    case 2:
                        palette->Colors[index] = SRL::Types::HighColor::FromARGB15(TGA::Deserializeuint16_t(&buffer[index << 1]));
                        break;
                    
                    // Loading RGB24 is a tad slower thx to the multiplication
                    case 3:
                        palette->Colors[index] = SRL::Types::HighColor::FromRGB24(TGA::DeserializeUint24(&buffer[index * 3]));
                        break;
                        
                    default:
                    case 4:
                        palette->Colors[index] = SRL::Types::HighColor::FromARGB32(TGA::Deserializeuint32_t(&buffer[index << 2]));
                        break;
                    }
                }
                else
                {
                    palette->Colors[index] = SRL::Types::HighColor();
                }
            }

            return palette;
        }

        /** @brief Decode paletted image
         * @param stream File stream
         * @param header File header
         */
        inline void DecodePaletted(uint8_t* stream, const TGA::TgaHeader* header)
        {
            // Allocated space for image data
            uint32_t pixels = this->width * this->height;
            this->imageData = new uint8_t[pixels];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));

            // Read palette data
            if (header->Palette.PaletteLength <= 16)
            {
                // 16 color palette
                for (uint32_t index = 0; index < pixels; index++)
                {
                    this->imageData[index] = ((buffer[index << 1] & 0x0f) << 4) | (buffer[(index << 1) + 1] & 0x0f);
                }
            }
            else
            {
                // 256 color palette
                for (uint32_t index = 0; index < pixels; index++)
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
        inline void DecodeTrueColor(uint8_t* stream, const TGA::TgaHeader* header, SRL::Types::HighColor transparentColor)
        {
            // Allocated space for image data
            uint32_t size = this->width * this->height;
            this->imageData = (uint8_t*)new SRL::Types::HighColor[size];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            uint8_t depth = header->Image.PixelColorDepth >> 3;

            // Read image data
            for (uint32_t index = 0; index < size; index++)
            {
                uint8_t* pixelData = buffer + (depth * index);
                SRL::Types::HighColor color;

                switch (depth)
                {
                case 2:
                    color = SRL::Types::HighColor::FromARGB15(TGA::Deserializeuint16_t(pixelData));
                    break;
                
                case 3:
                    color = SRL::Types::HighColor::FromRGB24(TGA::DeserializeUint24(pixelData));
                    break;
                    
                default:
                case 4:
                    color = SRL::Types::HighColor::FromARGB32(TGA::Deserializeuint32_t(pixelData));
                    break;
                }
                
                ((SRL::Types::HighColor*)this->imageData)[index] = color != transparentColor ? color : SRL::Types::HighColor();
            }
        }
        
        /** @brief Decode true color image with RLE compression
         * @param data Image data
         * @param header File header
         * @param transparentColor defines a color that should be changed to be transparent
         */
        inline void DecodeTrueColorRle(uint8_t* stream, const TGA::TgaHeader* header, SRL::Types::HighColor transparentColor)
        {
            // Allocated space for image data
            uint32_t size = this->width * this->height;
            this->imageData = (uint8_t*)new SRL::Types::HighColor[size];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            uint8_t depth = header->Image.PixelColorDepth >> 3;
            SRL::Types::HighColor fill;

            for (uint32_t pixel = 0; pixel < size;)
            {
                TgaRlePacket* packet = (TgaRlePacket*)(buffer++);

                switch (packet->Type)
                {
                case TgaRlePacket::PacketType::RawPacket:
                    for (int packed = 0; packed <= packet->Count; packed++)
                    {
                        SRL::Types::HighColor color;

                        // Read pixel
                        switch (depth)
                        {
                        case 2:
                            color = SRL::Types::HighColor::FromARGB15(TGA::Deserializeuint16_t(buffer));
                            break;
                        
                        case 3:
                            color = SRL::Types::HighColor::FromRGB24(TGA::DeserializeUint24(buffer));
                            break;
                            
                        default:
                        case 4:
                            color = SRL::Types::HighColor::FromARGB32(TGA::Deserializeuint32_t(buffer));
                            break;
                        }

                        ((SRL::Types::HighColor*)this->imageData)[pixel] = color != transparentColor ? color : SRL::Types::HighColor();

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
                        fill = SRL::Types::HighColor::FromARGB15(TGA::Deserializeuint16_t(buffer));
                        break;
                    
                    case 3:
                        fill = SRL::Types::HighColor::FromRGB24(TGA::DeserializeUint24(buffer));
                        break;
                        
                    default:
                    case 4:
                        fill= SRL::Types::HighColor::FromARGB32(TGA::Deserializeuint32_t(buffer));
                        break;
                    }
        
                    fill = fill != transparentColor ? fill : SRL::Types::HighColor();

                    // Repeat the pixel color
                    for (int repeater = 0; repeater <= packet->Count; repeater++)
                    {
                        ((SRL::Types::HighColor*)this->imageData)[pixel++] = fill;
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
            int32_t TransparentColorIndex;
            
            /** @brief Defines a color that should be changed to be transparent
             * @note Used only with RGB images
             */
            SRL::Types::HighColor TransparentColor;

            /** @brief Construct a new loader settings object
             */
            LoaderSettings() : TransparentColor(SRL::Types::HighColor()), TransparentColorIndex(-1)
            {
                // Do nothing
            }
        };

    private:

        /** @brief Load image data
         * @param file Image file
         * @param settings Loader settings
         */
        void LoadData(Cd::File* file, LoaderSettings* settings)
        {
            uint8_t* stream = new uint8_t[file->Size.Bytes + 1];
            int32_t read = file->LoadBytes(0, file->Size.Bytes, stream);
            
            // Open file
            if (read == file->Size.Bytes)
            {
                // Load file
                uint8_t* data = stream;

                // Load header, this is a bit complicated since the header not only is not aligned, but is also little endian
                TgaHeader header;
                header.ImageIdLength = *(data);
                header.HasPalette = *(data + 1);
                header.ImageType = *(data + 2);
                header.Palette.PaletteStart = TGA::Deserializeuint16_t(data + 3);
                header.Palette.PaletteLength = TGA::Deserializeuint16_t(data + 5);
                header.Palette.PaletteColorDepth = *(data + 7);
                header.Image.Origin.X = TGA::Deserializeuint16_t(data + 8);
                header.Image.Origin.Y = TGA::Deserializeuint16_t(data + 10);
                header.Image.Size.X = TGA::Deserializeuint16_t(data + 12);
                header.Image.Size.Y = TGA::Deserializeuint16_t(data + 14);
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
                case ((int8_t)TGA::TgaTypes::TgaPaletted):
                    this->palette = TGA::DecodePalette(stream, &header, settings->TransparentColorIndex);
                    this->DecodePaletted(stream, &header);
                    break;

                case ((int8_t)TGA::TgaTypes::TgaTrueColor):
                    this->DecodeTrueColor(stream, &header, settings->TransparentColor);
                    break;

                case ((int8_t)TGA::TgaTypes::TgaRleTrueColor):
                    this->DecodeTrueColorRle(stream, &header, settings->TransparentColor);
                    break;
                
                default:
                    SRL::Debug::Assert("Image is of unsupported type!\nCould not decode the image.");
                    break;
                }
            }
            else
            {
                SRL::Debug::Assert("File size does not match!.");
            }

            // Clear allocated memory
            delete stream;
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

            if (file.Exists())
            {
                this->LoadData(&file, &settings);
            }
            else
            {
                SRL::Debug::Assert("File '%s' is missing!", filename);
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
        uint8_t* GetData() override
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
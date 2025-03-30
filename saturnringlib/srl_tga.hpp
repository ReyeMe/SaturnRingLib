#pragma once

#include "srl_debug.hpp"
#include "srl_bitmap.hpp"
#include "srl_cd.hpp"
#include "srl_endian.hpp"

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
        enum class TgaTypes : int8_t
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

        /** @brief For loop range
         */
        struct ForRange
        {
            /** @brief Loop start
             */
            int32_t Start;

            /** @brief Loop end
             */
            int32_t End;

            /** @brief Loop step
             */
            int32_t Step;
        };

        /** @brief
         */
        enum TgaOrigin
        {
            BottomLeft = 0x00,
            BottomRight = 0x01,
            TopLeft = 0x02,
            TopRight = 0x03,
        };

#pragma pack(push, 1)
        /** @brief Image descriptor
         */
        struct TgaDescriptor
        {
            /** @brief How image pixel data is organized
             */
            uint8_t DataStorageMode: 2;

            /** @brief Where image pixel data start
             */
            uint8_t Origin:2;

            /** @brief Number of attribute bits associated with each pixel
             */
            uint8_t PixelAttributeBits:4;

        };
#pragma pack(pop)

#pragma pack(push, 1)
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
            union
            {
                int8_t Value;
                TgaDescriptor Data;
            } Descriptor;
        };
#pragma pack(pop)

#pragma pack(push, 1)
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
#pragma pack(pop)

#pragma pack(push, 1)
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
#pragma pack(pop)

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

        /** @brief Get color from ARGB value
         * @param argb ARGB value
         * @return Color value
         */
        constexpr inline static SRL::Types::HighColor ParseArgb(uint32_t argb)
        {
            if (((argb >> 24) & 0xff) < 128 && (argb & 0x00ffffff) == 0)
            {
                return SRL::Types::HighColor();
            }

            return SRL::Types::HighColor::FromARGB32(argb | 0xff000000);
        }

        /** @brief Check if format is wrong or not
         * @param header Tga header
         * @return True if format is supported
         */
        static bool IsFormatValid(const TGA::TgaHeader* header)
        {
            uint8_t bitDepth = header->Image.PixelColorDepth >> 3;

            switch (static_cast<TgaTypes>(header->ImageType))
            {
            case TGA::TgaTypes::TgaGrayscale:
            case TGA::TgaTypes::TgaRleGrayscale:
                if (bitDepth != 1 || header->HasPalette) return false;
                break;

            case TGA::TgaTypes::TgaTrueColor:
            case TGA::TgaTypes::TgaRleTrueColor:
                if ((bitDepth != 2 && bitDepth != 3 && bitDepth != 4) || header->HasPalette) return false;
                break;

            case TGA::TgaTypes::TgaPaletted:
            case TGA::TgaTypes::TgaRlePaletted:
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
        Bitmap::Palette* DecodePalette(uint8_t* stream, const TGA::TgaHeader* header, int32_t transparentColor)
        {
            uint8_t* buffer = (stream + TGA::ImagePaletteOffset(header));
            Bitmap::Palette* palette = autonew Bitmap::Palette(header->Palette.PaletteLength);
            uint8_t depth = header->Palette.PaletteColorDepth >> 3;
            uint8_t* pixelData = buffer;

            for (int32_t index = 0; index < header->Palette.PaletteLength; index++)
            {
                if (index != transparentColor)
                {
                    SRL::Types::HighColor color = SRL::Types::HighColor();

                    switch (depth)
                    {
                    case 2:
                        color = SRL::Types::HighColor::FromARGB15(SRL::Endian::DeserializeUint16(pixelData));
                        break;

                    case 3:
                        color = SRL::Types::HighColor::FromRGB24(SRL::Endian::DeserializeUint24(pixelData));
                        break;

                    default:
                    case 4:
                        color = TGA::ParseArgb(SRL::Endian::DeserializeUint32(pixelData));
                        break;
                    }

                    palette->Colors[index] = color;
                }
                else
                {
                    palette->Colors[index] = SRL::Types::HighColor();
                }

                pixelData += depth;
            }

            return palette;
        }

        /** @brief Decode paletted image
         * @param stream File stream
         * @param header File header
         * @param xLoop Range of width loop
         * @param yLoop Range of height loop
         */
        inline void DecodePaletted(uint8_t* stream, const TGA::TgaHeader* header, ForRange& xLoop, ForRange& yLoop)
        {
            // Allocated space for image data
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            int32_t xLocation = xLoop.Start;
            int32_t yLocation = yLoop.Start;

            // Read palette data
            if (header->Palette.PaletteLength <= 16)
            {
                uint32_t pixels = (this->width * this->height) >> 1;
                this->imageData = autonew uint8_t[pixels];

                // 16 color palette
                for (uint32_t index = 0; index < pixels; index++)
                {
                    uint32_t location = (yLocation * this->width) + xLocation;
                    xLocation += xLoop.Step;
                    uint32_t location2 = (yLocation * this->width) + xLocation;
                    xLocation += xLoop.Step;

                    this->imageData[index] = ((buffer[location] & 0x0f) << 4) | (buffer[location2] & 0x0f);

                    if (xLocation == xLoop.End)
                    {
                        xLocation = xLoop.Start;
                        yLocation += yLoop.Step;
                    }
                }
            }
            else
            {
                uint32_t pixels = this->width * this->height;
                this->imageData = autonew uint8_t[pixels];

                // 256 color palette
                for (uint32_t index = 0; index < pixels; index++)
                {
                    uint32_t location = (yLocation * this->width) + xLocation;
                    this->imageData[location] = buffer[index];
                    xLocation += xLoop.Step;

                    if (xLocation == xLoop.End)
                    {
                        xLocation = xLoop.Start;
                        yLocation += yLoop.Step;
                    }
                }
            }
        }

        /** @brief Decode paletted image
         * @param stream File stream
         * @param header File header
         * @param xLoop Range of width loop
         * @param yLoop Range of height loop
         */
        inline void DecodeRlePaletted(uint8_t* stream, const TGA::TgaHeader* header, ForRange& xLoop, ForRange& yLoop)
        {
            // Allocated space for image data
            size_t size = this->width * this->height;
            this->imageData = autonew uint8_t[size];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            int32_t xLocation = xLoop.Start;
            int32_t yLocation = yLoop.Start;
            size_t fill = 0;

            // Read image data
            for (uint32_t pixel = 0; pixel < size;)
            {
                TgaRlePacket* packet = (TgaRlePacket*)(buffer++);

                switch (packet->Type)
                {

                // Decompress pixel colors
                case TgaRlePacket::PacketType::RawPacket:
                    for (int packed = 0; packed <= packet->Count; packed++)
                    {
                        if (header->Palette.PaletteLength <= 16)
                        {
                            xLocation += xLoop.Step;
                            uint32_t location = (yLocation * this->width) + xLocation;
                            xLocation += xLoop.Step;

                            // 16 color palette
                            this->imageData[location >> 1] = ((buffer[0] & 0x0f) << 4) | (buffer[1] & 0x0f);
                            buffer+=2;
                        }
                        else
                        {
                            uint32_t location = (yLocation * this->width) + xLocation;
                            xLocation += xLoop.Step;

                            // 256 color palette
                            this->imageData[location] = *(buffer++);
                        }

                        pixel++;

                        if (xLocation == xLoop.End)
                        {
                            xLocation = xLoop.Start;
                            yLocation += yLoop.Step;
                        }
                    }

                    break;

                // Repeat the pixel color
                default:
                case TgaRlePacket::PacketType::RlePacket:
                    for (int repeater = 0; repeater <= packet->Count; repeater++)
                    {
                        if (header->Palette.PaletteLength <= 16)
                        {
                            xLocation += xLoop.Step;
                            uint32_t location = (yLocation * this->width) + xLocation;
                            xLocation += xLoop.Step;

                            // 16 color palette
                            this->imageData[location >> 1] = ((buffer[0] & 0x0f) << 4) | (buffer[1] & 0x0f);
                        }
                        else
                        {
                            uint32_t location = (yLocation * this->width) + xLocation;
                            xLocation += xLoop.Step;

                            // 256 color palette
                            this->imageData[location] = *buffer;
                        }

                        pixel++;

                        if (xLocation == xLoop.End)
                        {
                            xLocation = xLoop.Start;
                            yLocation += yLoop.Step;
                        }
                    }

                    // Move to next pixel data
                    buffer++;
                    break;
                }
            }
        }

        /** @brief Decode true color image
         * @param data Image data
         * @param header File header
         * @param xLoop Range of width loop
         * @param yLoop Range of height loop
         * @param transparentColor defines a color that should be changed to be transparent
         */
        inline void DecodeTrueColor(uint8_t* stream, const TGA::TgaHeader* header, ForRange& xLoop, ForRange& yLoop, SRL::Types::HighColor transparentColor)
        {
            // Allocated space for image data
            uint32_t size = this->width * this->height;
            this->imageData = (uint8_t*)autonew SRL::Types::HighColor[size];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            uint8_t depth = header->Image.PixelColorDepth >> 3;
            int32_t xLocation = xLoop.Start;
            int32_t yLocation = yLoop.Start;

            // Read image data
            for (uint32_t index = 0; index < size; index++)
            {
                uint8_t* pixelData = buffer + (depth * index);
                SRL::Types::HighColor color;

                switch (depth)
                {
                case 2:
                    color = SRL::Types::HighColor::FromARGB15(SRL::Endian::DeserializeUint16(pixelData));
                    break;

                case 3:
                    color = SRL::Types::HighColor::FromRGB24(SRL::Endian::DeserializeUint24(pixelData));
                    break;

                default:
                case 4:
                    color = TGA::ParseArgb(SRL::Endian::DeserializeUint32(pixelData));
                    break;
                }

                uint32_t location = (yLocation * this->width) + xLocation;
                ((SRL::Types::HighColor*)this->imageData)[location] = color != transparentColor ? color : SRL::Types::HighColor();

                xLocation += xLoop.Step;

                if (xLocation == xLoop.End)
                {
                    xLocation = xLoop.Start;
                    yLocation += yLoop.Step;
                }
            }
        }

        /** @brief Decode true color image with RLE compression
         * @param data Image data
         * @param header File header
         * @param xLoop Range of width loop
         * @param yLoop Range of height loop
         * @param transparentColor defines a color that should be changed to be transparent
         */
        inline void DecodeTrueColorRle(uint8_t* stream, const TGA::TgaHeader* header, ForRange& xLoop, ForRange& yLoop, SRL::Types::HighColor transparentColor)
        {
            // Allocated space for image data
            uint32_t size = this->width * this->height;
            this->imageData = (uint8_t*)autonew SRL::Types::HighColor[size];
            uint8_t* buffer = (stream + TGA::ImageDataOffset(header));
            uint8_t depth = header->Image.PixelColorDepth >> 3;
            SRL::Types::HighColor fill;
            int32_t xLocation = xLoop.Start;
            int32_t yLocation = yLoop.Start;

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
                            color = SRL::Types::HighColor::FromARGB15(SRL::Endian::DeserializeUint16(buffer));
                            break;

                        case 3:
                            color = SRL::Types::HighColor::FromRGB24(SRL::Endian::DeserializeUint24(buffer));
                            break;

                        default:
                        case 4:
                            color = TGA::ParseArgb(SRL::Endian::DeserializeUint32(buffer));
                            break;
                        }

                        uint32_t location = (yLocation * this->width) + xLocation;
                        ((SRL::Types::HighColor*)this->imageData)[location] = color != transparentColor ? color : SRL::Types::HighColor();

                        // Move to next pixel data
                        buffer += depth;

                        xLocation += xLoop.Step;

                        if (xLocation == xLoop.End)
                        {
                            xLocation = xLoop.Start;
                            yLocation += yLoop.Step;
                        }

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
                        fill = SRL::Types::HighColor::FromARGB15(SRL::Endian::DeserializeUint16(buffer));
                        break;

                    case 3:
                        fill = SRL::Types::HighColor::FromRGB24(SRL::Endian::DeserializeUint24(buffer));
                        break;

                    default:
                    case 4:
                        fill= TGA::ParseArgb(SRL::Endian::DeserializeUint32(buffer));
                        break;
                    }

                    fill = fill != transparentColor ? fill : SRL::Types::HighColor();

                    // Repeat the pixel color
                    for (int repeater = 0; repeater <= packet->Count; repeater++)
                    {
                        uint32_t location = (yLocation * this->width) + xLocation;
                        ((SRL::Types::HighColor*)this->imageData)[location] = fill;

                        xLocation += xLoop.Step;

                        if (xLocation == xLoop.End)
                        {
                            xLocation = xLoop.Start;
                            yLocation += yLoop.Step;
                        }

                        pixel++;
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
            uint8_t* stream = autonew uint8_t[file->Size.Bytes + 1];
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
                header.Palette.PaletteStart = SRL::Endian::DeserializeUint16(data + 3);
                header.Palette.PaletteLength = SRL::Endian::DeserializeUint16(data + 5);
                header.Palette.PaletteColorDepth = *(data + 7);
                header.Image.Origin.X = SRL::Endian::DeserializeUint16(data + 8);
                header.Image.Origin.Y = SRL::Endian::DeserializeUint16(data + 10);
                header.Image.Size.X = SRL::Endian::DeserializeUint16(data + 12);
                header.Image.Size.Y = SRL::Endian::DeserializeUint16(data + 14);
                header.Image.PixelColorDepth = *(data + 16);
                header.Image.Descriptor.Value = *(data + 17);

                // Lets check whether the header makes sense
                if (header.Image.Size.X == 0 || header.Image.Size.Y == 0)
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

                // Pixel read order
                uint8_t origin = header.Image.Descriptor.Data.Origin;
                ForRange xLoop = { 0, 0, 0 };
                ForRange yLoop = { 0, 0, 0 };

                if (origin == TgaOrigin::TopLeft || origin == TgaOrigin::TopRight) {
                    yLoop.Start = 0; // start bottom, step upward
                    yLoop.Step = 1;
                    yLoop.End = this->height;
                } else {
                    yLoop.Start = this->height - 1; // start at top, step downward
                    yLoop.Step = -1;
                    yLoop.End = -1;
                }

                if (origin == TgaOrigin::TopLeft || origin == TgaOrigin::BottomLeft) {
                    xLoop.Start = 0;
                    xLoop.Step = 1;
                    xLoop.End = this->width;
                } else {
                    xLoop.Start = this->width - 1;
                    xLoop.Step = -1;
                    xLoop.End = -1;
                }

                // Data stream should now be pointing to after the header
                switch (static_cast<TgaTypes>(header.ImageType))
                {
                case TGA::TgaTypes::TgaPaletted:
                    this->palette = this->DecodePalette(stream, &header, settings->TransparentColorIndex);
                    this->DecodePaletted(stream, &header, xLoop, yLoop);
                    break;

                case TGA::TgaTypes::TgaRlePaletted:
                    this->palette = this->DecodePalette(stream, &header, settings->TransparentColorIndex);
                    this->DecodeRlePaletted(stream, &header, xLoop, yLoop);
                    break;

                case TGA::TgaTypes::TgaTrueColor:
                    this->DecodeTrueColor(stream, &header, xLoop, yLoop, settings->TransparentColor);
                    break;

                case TGA::TgaTypes::TgaRleTrueColor:
                    this->DecodeTrueColorRle(stream, &header, xLoop, yLoop, settings->TransparentColor);
                    break;

                default:
                    SRL::Debug::Assert("Image is of unsupported type '%d'!\nCould not decode the image.", header.ImageType);
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
         * @param data TGA file
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

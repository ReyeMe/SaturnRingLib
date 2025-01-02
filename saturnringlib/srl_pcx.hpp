#pragma once

#include "srl_debug.hpp"
#include "srl_bitmap.hpp"
#include "srl_cd.hpp"
#include "srl_log.hpp"

/*
 * This TGA loader is loosely based on TGA loader from yaul by:
 * Israel Jacquez <mrkotfw@gmail.com>
 * David Oberhollenzer
 */

namespace SRL::Bitmap
{
    /** @brief PCX image handling
     */
    struct PCX : IBitmap
    {
    private:
        /** @brief Default palette
         */
        constexpr inline static const uint8_t DefaultPalette[48] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00,
            0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x80,
            0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0,
            0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0xff,
            0xff, 0x00, 0x00, 0xff, 0x00, 0xff, 0xff, 0xff, 0x00,
            0xff, 0xff, 0xff};

        /** @brief Size of the header
         */
        constexpr inline static const uint8_t ManufacturerId = 0x0A;

        /** @brief Size of the header
         */
        constexpr inline static const size_t HeaderSize = 128;

        /** @brief Size of the palette
         */
        constexpr inline static const size_t PaletteSize = 256;

        /** @brief ...
         */
        constexpr inline static const size_t ManufacturerOffset = 0;

        /** @brief ...
         */
        constexpr inline static const size_t VersionOffset = 1;

        /** @brief ...
         */
        constexpr inline static const size_t EncodingOffset = 2;

        /** @brief ...
         */
        constexpr inline static const size_t BitsPerPixelOffset = 3;

        /** @brief ...
         */
        constexpr inline static const size_t XMinOffset = 4;

        /** @brief ...
         */
        constexpr inline static const size_t YMinOffset = 6;

        /** @brief ...
         */
        constexpr inline static const size_t XMaxOffset = 8;

        /** @brief ...
         */
        constexpr inline static const size_t YMaxOffset = 10;

        /** @brief ...
         */
        constexpr inline static const size_t HDpiOffset = 12;

        /** @brief ...
         */
        constexpr inline static const size_t VDpiOffset = 14;

        /** @brief ...
         */
        constexpr inline static const size_t ColormapOffset = 16;

        /** @brief ...
         */
        constexpr inline static const size_t ReservedOffset = 64;

        /** @brief ...
         */
        constexpr inline static const size_t NPlanesOffset = 65;

        /** @brief ...
         */
        constexpr inline static const size_t BytesPerLineOffset = 66;

        /** @brief ...
         */
        constexpr inline static const size_t PaletteInfoOffset = 68;

        /** @brief ...
         */
        constexpr inline static const size_t HScreenSizeOffset = 70;

        /** @brief ...
         */
        constexpr inline static const size_t VScreenSizeSizeOffset = 72;

        /** @brief ...
         */
        constexpr inline static const size_t FillerOffset = 74;

        /** @brief ...
         */
        constexpr inline static const size_t PaletteBeginOffset = 1;

        enum class Version : uint8_t
        {
            // with palette
            V2_5 = 0x00,

            // with palette
            V2_8WithPalette = 0x02,

            // without palette
            V2_8WithoutPalette = 0x03,

            PaintbrushForWindows = 0x04,

            // 24 bit pcx or paletted
            V3_0 = 0x05,
        };

        enum class BitsPerPixel : uint8_t
        {
            BitsPerPixel1 = 1,

            BitsPerPixel2 = 2,

            BitsPerPixel4 = 4,

            BitsPerPixel8 = 8,
        };

        enum class Encoding : uint8_t
        {
            RLE = 0x01,
        };

        /* for the 24 bit demuxing */
        enum class PlaneColor : uint8_t
        {
            PlaneRed = 0x00,

            PlaneGreen = 0x01,

            PlaneBlue = 0x02,
        };

#pragma pack(push, 1)
        /** @brief TGA image header
         */
        struct PCXHeader
        {
            uint8_t manufacturer; /* 0x0A: ZSoft */
            Version version;
            /* 0x02 with palette
               0x03 without palette
               0x05 24 bit pcx or paletted
             */

            Encoding encoding;         /* 0x01: PCX RLE */
            BitsPerPixel bitsPerPixel; /* number of bits per pixel 1, 2, 4, 8 */
            uint16_t xMin;             /* window */
            uint16_t yMin;
            uint16_t xMax;
            uint16_t yMax;
            uint16_t hDpi;
            uint16_t vDpi;
            uint8_t colormap[48];  /* first 16 colors, R, G, B as 8 bit values */
            uint8_t reserved;      /* 0x00 */
            uint8_t nplanes;       /* number of planes */
            uint16_t bytesPerLine; /* number of bytes that represent a scanline plane
                                 == must be an even number! */
            uint16_t paletteInfo;  /* 0x01 = color/bw
                                 0x02 = grayscale
                               */
            uint16_t hScreenSize;  /* horizontal screen size */
            uint16_t vScreenSize;  /* vertical screen size */
        };
#pragma pack(pop)

        PCXHeader hdr;

        /** @brief Color palette
         */
        SRL::Bitmap::Palette *palette;

        /** @brief Width of the bitmap
         */
        size_t width;

        /** @brief Height of the bitmap
         */
        size_t height;

        /** @brief Bytes required to hold one complete uncompressed scan line
         */
        uint32_t totalBytes;

        uint32_t bufrSize;

        /** @brief Image data
         */
        uint8_t *imageData;

        uint8_t *bufr;

        bool LoadPalette(Cd::File *file)
        {
            int c;
            int checkbyte;

            if (!file)
            {
                SRL::Logger::LogFatal("%s(l%d) : Invalid file pointer", __FUNCTION__, __LINE__);
                return false;
            }

            if (this->hdr.bitsPerPixel == BitsPerPixel::BitsPerPixel1 &&
                this->hdr.nplanes == 4)
            {
                SRL::Logger::LogInfo("PCX without a palette detected");
                /* copy over the default palette to the header */
                for (c = 0; c < 48; c++)
                {
                    this->hdr.colormap[c] = DefaultPalette[c];
                }

                /* copy over the default palette to the palette structure */
                for (c = 0; c < 16; c++)
                {
                    this->palette->Colors[c].Opaque = 1;
                    this->palette->Colors[c].Red = DefaultPalette[c * 3 + 0];
                    this->palette->Colors[c].Green = DefaultPalette[c * 3 + 1];
                    this->palette->Colors[c].Blue = DefaultPalette[c * 3 + 2];
                }
                return true;
            }
            else if (this->hdr.bitsPerPixel == BitsPerPixel::BitsPerPixel8 &&
                     this->hdr.nplanes == 1)
            {
                /* first seek to the end of the file -769 */
                file->Seek(PaletteSize * 3, Cd::SeekMode::EndOfFile);
                uint8_t stream[PaletteSize * 3];
                int32_t read = file->Read(PaletteSize * 3, stream);

                uint8_t *data = stream;
                checkbyte = SRL::ENDIAN::DeserializeUint8(data);

                // if (checkbyte != 0x0c) /* magic value */
                // {
                //     SRL::Logger::LogFatal("Expected a 256 color palette, didn't find it, but : %d", checkbyte);
                //     return false;
                // }

                /* okay. so we're at the right part of the file now, we just need
                  to populate our palette! */
                for (c = 0; c < 256; c++)
                {
                    this->palette->Colors[c].Opaque = 1;
                    this->palette->Colors[c].Red = SRL::ENDIAN::DeserializeUint8(data + c + PaletteBeginOffset);
                    this->palette->Colors[c].Green = SRL::ENDIAN::DeserializeUint8(data + c + PaletteBeginOffset);
                    this->palette->Colors[c].Blue = SRL::ENDIAN::DeserializeUint8(data + c + PaletteBeginOffset);
                }

                /* now copy over the first 16 colors into the header */
                for (c = 0; c < 16; c++)
                {
                    this->hdr.colormap[c * 3 + 0] = this->palette->Colors[c].Red;
                    this->hdr.colormap[c * 3 + 1] = this->palette->Colors[c].Green;
                    this->hdr.colormap[c * 3 + 2] = this->palette->Colors[c].Blue;
                }

                return true;
            }
            else
            {
                return false;
            }

            return true;
        }

        bool LoadPix(Cd::File *file)
        {
            int i;
            uint32_t l;
            int chr, cnt;
            unsigned char *bpos;

            if (!file)
            {
                SRL::Logger::LogFatal("%s(l%d) : Invalid File pointer", __FUNCTION__, __LINE__);
                return false;
            }

            /* Here's a program fragment using PCX_encget.  This reads an
            entire file and stores it in a (large) buffer, pointed
            to by the variable "bufr". "fp" is the file pointer for
            the image */

            this->bufrSize = this->hdr.bytesPerLine * this->hdr.nplanes * (1 + this->hdr.yMax - this->hdr.yMin);

            SRL::Logger::LogDebug("%s(l%d) : bufrSize = %d", __FUNCTION__, __LINE__, this->bufrSize);

            file->Seek(HeaderSize);

            //   uint8_t* stream = new uint8_t[file->Size.Bytes - - HeaderSize + 1];

            /* first, we need to go to the right point in the file */
            // fseek( fp, 128, SEEK_SET ); /* oh yeah. this is important */

            //   int32_t read = file->LoadBytes(HeaderSize, file->Size.Bytes - HeaderSize, stream);

            // if( pi->bufr )  free( pi->bufr );

            this->bufr = new uint8_t[this->bufrSize];

            bpos = this->bufr;

            for (l = 0; l < this->bufrSize;) /* increment by cnt below */
            {
                int32_t ret = PCX_encget(&chr, &cnt, file);

                if (ret < 0)
                {
                    break;
                }

                for (i = 0; i < cnt; i++)
                    *bpos++ = chr;

                l += cnt;
            }

            return true;
        }

        ////////////////////////////////////////////////////////////////////////////////

        /*
         * PCX_encget
         *
         *  This procedure reads one encoded block from the image
         *  file and stores a count and data byte.
         */
        int32_t PCX_encget(
            int *pbyt,     /* where to place data */
            int *pcnt,     /* where to place count */
            Cd::File *file /* image file handle */
        )
        {
            uint8_t i;
            int32_t read;
            *pcnt = 1; /* assume a "run" length of one */

            /* check for EOF */
            read = file->Read(1, &i);

            if (read < 0)
            {
                return read;
            }

            if (0xC0 == (0xC0 & i)) /* is it a RLE repeater */
            {
                /* YES.  set the repeat count */
                *pcnt = 0x3F & i;

                read = file->Read(1, &i);

                if (read < 0)
                {
                    return read;
                }
            }
            /* set the byte */
            *pbyt = i;

            /* return an 'OK' */
            return (SRL::Cd::ErrorCode::ErrorOk);
        }

        int32_t PCX_toImage()
        {
            int pcx_pos, image_pos, set_aside;
            size_t x, y, p;
            // IMAGE * i = NULL;

            // if( !pi )  return( NULL );

            // i = Image_Create( pi->width, pi->height, pi->hdr.bitsPerPixel );

            if (this->hdr.nplanes == 1)
            {
                /* paletted image! */
                pcx_pos = image_pos = 0;
                for (y = 0; y < this->height; y++)
                {
                    for (x = 0; x < this->hdr.bytesPerLine; x++)
                    {
                        /* the width might be different than 'bytesPerLine */
                        if (x < this->width)
                        {

                            // SRL::Types::HighColor color;
                            //
                            // switch (this->hdr.bitsPerPixel)
                            // {
                            // case BitsPerPixel::BitsPerPixel2:
                            //     color = SRL::Types::HighColor::FromARGB15(SRL::ENDIAN::DeserializeUint16(pixelData));
                            //     break;

                            // case BitsPerPixel3:
                            //     color = SRL::Types::HighColor::FromRGB24(SRL::ENDIAN::DeserializeUint24(pixelData));
                            //     break;

                            // default:
                            // case BitsPerPixel::BitsPerPixel4:
                            //     color = TGA::ParseArgb(SRL::ENDIAN::DeserializeUint32(pixelData));
                            //     break;
                            // }

                            // i->data[image_pos].r = this->pal[ this->bufr[pcx_pos] ].r;
                            // i->data[image_pos].g = this->pal[ this->bufr[pcx_pos] ].g;
                            // i->data[image_pos].b = this->pal[ this->bufr[pcx_pos] ].b;
                            // i->data[image_pos].a = this->bufr[pcx_pos];
                            image_pos++;
                        }
                        pcx_pos++;
                    }
                }
            }
            else
            {

                /* 24 bit image */
                pcx_pos = image_pos = 0;
                for (y = 0; y < this->height; y++)
                {
                    set_aside = image_pos; /* since they're muxed weird */
                    for (p = 0; p < this->hdr.nplanes; p++)
                    {
                        image_pos = set_aside;
                        for (x = 0; x < this->hdr.bytesPerLine; x++)
                        {
                            /* the width might be different than 'bytesPerLine */
                            if (x < this->width)
                            {
                                // switch (PlaneColor(p))
                                // {
                                // case PlaneColor::PlaneRed:
                                //     i->data[image_pos].r = this->bufr[pcx_pos];
                                //     break;
                                //
                                // case PlaneColor::PlaneGreen:
                                //     i->data[image_pos].g = this->bufr[pcx_pos];
                                //     break;
                                //
                                // default:
                                // case PlaneColor::PlaneBlue:
                                //     i->data[image_pos].b = this->bufr[pcx_pos];
                                //     break;
                                // }
                                image_pos++;
                            }
                            pcx_pos++;
                        }
                    }
                }
            }
            return 1;
        }

        bool LoadData(Cd::File *file)
        {
            int c;

            if (!file)
            {
                SRL::Logger::LogFatal("%s(l%d) : Invalid File pointer", __FUNCTION__, __LINE__);
                return false;
            }

            uint8_t stream[HeaderSize + 1];
            int32_t read = file->LoadBytes(0, HeaderSize, stream);

            // Open file
            if (read == HeaderSize)
            {
                // Load file
                uint8_t *data = stream;

                /* read in the header blocks. */
                this->hdr.manufacturer = SRL::ENDIAN::DeserializeUint8(data + ManufacturerOffset);
                this->hdr.version = Version(SRL::ENDIAN::DeserializeUint8(data + VersionOffset));
                this->hdr.encoding = Encoding(SRL::ENDIAN::DeserializeUint8(data + EncodingOffset));
                this->hdr.bitsPerPixel = BitsPerPixel(SRL::ENDIAN::DeserializeUint8(data + BitsPerPixelOffset));

                this->hdr.xMin = SRL::ENDIAN::DeserializeUint16(data + XMinOffset);
                this->hdr.yMin = SRL::ENDIAN::DeserializeUint16(data + YMinOffset);
                this->hdr.xMax = SRL::ENDIAN::DeserializeUint16(data + XMaxOffset);
                this->hdr.yMax = SRL::ENDIAN::DeserializeUint16(data + YMaxOffset);

                /* do a little precomputing... */
                this->width = this->hdr.xMax - this->hdr.xMin + 1;
                this->height = this->hdr.yMax - this->hdr.yMin + 1;

                this->hdr.hDpi = SRL::ENDIAN::DeserializeUint16(data + HDpiOffset);
                this->hdr.vDpi = SRL::ENDIAN::DeserializeUint16(data + VDpiOffset);

                /* read in 16 color colormap */
                for (c = 0; c < 48; c++)
                {
                    this->hdr.colormap[c] = SRL::ENDIAN::DeserializeUint8(data + ColormapOffset + c);
                }

                this->hdr.reserved = SRL::ENDIAN::DeserializeUint8(data + ReservedOffset);
                this->hdr.nplanes = SRL::ENDIAN::DeserializeUint8(data + NPlanesOffset);
                this->hdr.bytesPerLine = SRL::ENDIAN::DeserializeUint16(data + BytesPerLineOffset);
                this->hdr.paletteInfo = SRL::ENDIAN::DeserializeUint16(data + PaletteInfoOffset);
                this->hdr.hScreenSize = SRL::ENDIAN::DeserializeUint16(data + HScreenSizeOffset);
                this->hdr.vScreenSize = SRL::ENDIAN::DeserializeUint16(data + VScreenSizeSizeOffset);

                this->totalBytes = this->hdr.bytesPerLine * this->hdr.nplanes;

                /* ignore the filler */
            }
            else
            {
                SRL::Logger::LogFatal("%s(l%d) : Cannot read .PCX", __FUNCTION__, __LINE__);
                return false;
            }

            if (!IsFormatValid())
            {
                SRL::Logger::LogFatal("%s(l%d) : Invalid File", __FUNCTION__, __LINE__);
                return false;
            }

            palette = new SRL::Bitmap::Palette(PaletteSize);

            // Load the palette
            if (!LoadPalette(file))
            {
                SRL::Logger::LogFatal("%s(l%d) : Cannot load palette", __FUNCTION__, __LINE__);
                return false;
            }

            uint32_t pixels = (this->width * this->height) >> 1;
            this->imageData = new uint8_t[pixels];

            LoadPix(file);

            return true;
        }

    public:
        /** @brief Construct RGB555 TGA image from file
         * @param data TGA file
         * @param settings TGA loader settings
         */
        PCX(Cd::File *data) : imageData(nullptr), palette(nullptr)
        {
            this->LoadData(data);
        }

        /** @brief Construct RGB555 TGA image from file
         * @param filename TGA file name
         * @param settings TGA loader settings
         */
        PCX(const char *filename) : imageData(nullptr), palette(nullptr)
        {
            Cd::File file = Cd::File(filename);

            if (file.Exists())
            {
                this->LoadData(&file);
            }
            else
            {
                SRL::Debug::Assert("File '%s' is missing!", filename);
            }
        }

        /** @brief Destroy the TGA image
         */
        ~PCX()
        {
            if (this->imageData != nullptr)
            {
                delete this->imageData;
            }

            if (this->palette != nullptr)
            {
                delete this->palette;
            }
        }

        /** @brief Get image data
         * @return Pointer to image data
         */
        uint8_t *GetData() override
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

        /** @brief Check if format is wrong or not
         * @return True if format is supported
         */
        bool IsFormatValid() const
        {
            bool isValid = true;

            if (this->width <= 0 || this->height <= 0)
            {
                SRL::Logger::LogDebug("Image has invalid width and/or height");
                isValid = false;
            }

            if (ManufacturerId != this->hdr.manufacturer)
            {
                SRL::Logger::LogDebug("Invalid Manufacturer");
                isValid = false;
            }

            if (Version::V3_0 != this->hdr.version)
            {
                SRL::Logger::LogDebug("Unsupported version");
                isValid = false;
            }

            if (Encoding::RLE != this->hdr.encoding)
            {
                SRL::Logger::LogDebug("Invalid encoding");
                isValid = false;
            }

            if (BitsPerPixel::BitsPerPixel8 == this->hdr.bitsPerPixel)
            {
                if (1 != this->hdr.nplanes)
                {
                    SRL::Logger::LogDebug("Invalid 8bit format");
                    isValid = false;
                }
            }

            return isValid;
        }

        /** @brief ...
         * @return ...
         */
        void Dump() const
        {
            int c, d, e;

            SRL::Logger::LogInfo("PCX Image:");
            SRL::Logger::LogInfo("Manufacturer: 0x%02x (0x0a)", this->hdr.manufacturer);
            SRL::Logger::LogInfo("Version: %d", this->hdr.version);
            SRL::Logger::LogInfo("Encoding: %d", this->hdr.encoding);
            SRL::Logger::LogInfo("BitsPerPixel: %d", this->hdr.bitsPerPixel);

            SRL::Logger::LogInfo("Window: (%d %d)-(%d %d) : %d x %d",
                                 this->hdr.xMin, this->hdr.yMin, this->hdr.xMax, this->hdr.yMax,
                                 this->width, this->height);

            SRL::Logger::LogInfo("H DPI: %d", this->hdr.hDpi);
            SRL::Logger::LogInfo("V DPI: %d", this->hdr.vDpi);
            SRL::Logger::LogInfo("reserved: %d", this->hdr.reserved);
            SRL::Logger::LogInfo("n planes: %d", this->hdr.nplanes);
            SRL::Logger::LogInfo("bytesPerLine: %d", this->hdr.bytesPerLine);
            SRL::Logger::LogInfo("paletteInfo: %d", this->hdr.paletteInfo);
            SRL::Logger::LogInfo("hScreenSize: %d", this->hdr.hScreenSize);
            SRL::Logger::LogInfo("vScreenSize: %d", this->hdr.vScreenSize);

            SRL::Logger::LogInfo(" 16 color palette header data: ");
            for (c = 0, d = 3, e = 0; c < 48; c++, d++, e++)
            {
                if (e == 12)
                {
                    d = 0;
                    e = 0;
                    SRL::Logger::LogInfo("[%02x] %02x", c / 3, this->hdr.colormap[c]);
                }
                else if (d == 3)
                {
                    d = 0;
                    SRL::Logger::LogInfo("[%02x] %02x", c / 3, this->hdr.colormap[c]);
                }
            }

            // SRL::Logger::LogInfo(" 256 color palette data:");
            // for (c = 0, d = 0; c < 256; c++, d++)
            // {
            //     if (d == 4)
            //     {
            //         d = 0;
            //         SRL::Logger::LogInfo(" ");
            //     }
            //     SRL::Logger::LogInfo("   [%02x] : %02x %02x %02x", c,
            //                             this->palette->Colors[c].Red,
            //                             this->palette->Colors[c].Green,
            //                             this->palette->Colors[c].Blue);
            // }

            SRL::Logger::LogInfo("%6ld bytes", this->bufrSize);
            SRL::Logger::LogInfo("%6d pixels", this->width * this->height);
        }
    };
}

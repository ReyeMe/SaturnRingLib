#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"
#include "srl_bitmap.hpp" // for IBit
#include "srl_string.hpp" // for memset

namespace SRL
{
    /** @brief Interface for displaying ASCII text. Currently a direct replacement for slPrint.
     *  It removes any possible dependency on NBG0 system variables and displays 4bpp fonts
     *  to reduce required memory. Allows Storing and displaying up to 6 fonts and 8 color pallets.
     */
    class ASCII
    {
    private:
        /** @brief Pointer to the ASCII map in Vdp2 VRAM 64x64
         */
        inline static uint16_t* tileMap = (uint16_t*)(VDP2_VRAM_B1 + 0x1E000);

        /** @brief index of the current 16 color pallet that font will use, formatted to quickly modify map data
         */
        inline static uint16_t  colorBank = 1 << 12;

        /** @brief offset to the tile data of the current font that will print, formatted to quickly modify map data
         */
        inline static uint16_t  fontBank = 640;

        /** @brief the current number of fonts that have been loaded to Vdp2 VRAM
         */
        inline static uint8_t   numFonts = 1;

        /** @brief ...
         */
        inline static uint8_t   maxXPosition = 63;

        /** @brief ...
         */
        inline static uint8_t   maxYPosition = 63;

        /** @brief ...
         */
        inline static uint8_t   maxFont = 5;

        /** @brief ...
         */
        inline static uint8_t   maxColorIndex = 15;

        /** @brief ...
         */
        inline static uint8_t   maxPaletteIndex = 7;

    public:
        /** @brief Copies 4bpp Bitmap ASCII table to VRAM as 4bpp tileset
         *  @param bmp pointer to an IBitmap interface to load
         *  @note image layout must be grid of 8x8 pixel characters arranged in ASCII order from left to right,
         *  top to bottom, with 1 empty 8x8 tile preceding the characters (see example in VDP2 Samples)
         *  @param fontId Index in the font table to load this font to (range 0-5)
         */
        inline static void LoadFont(SRL::Bitmap::IBitmap* bmp, uint8_t fontId = 0)
        {
            if (fontId > 5) fontId = 5;

            uint8_t* src = bmp->GetData();

            // Font table starts at top and builds down to stay out of the way of VDP2 allocator
            uint8_t* dest = (uint8_t*)(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (fontId * 0x1000));
            uint16_t X = bmp->GetInfo().Width >> 1;
            uint16_t Y = bmp->GetInfo().Height >> 3;
            uint8_t* st = src;

            for (int y = 0; y < Y; ++y)
            {
                for (int x = 0; x < (X >> 2); ++x)
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        for (int j = 0; j < 4; ++j) *dest++ = *st++;
                        st += (X - 4);
                    }

                    st = (src += 4);
                }

                src += (7 * X);
                st = src;
            }
        }

        /** @brief Use to load a 4bpp version of SGLs internal font
         * @param source Address storing SGLs 8bpp font
         * @param fontId Font index to load to (range 0-5)
         */
        inline static void LoadFontSG(uint8_t* source, uint8_t fontId = 0)
        {
            if (fontId > 5) fontId = 5;

            // Font table starts at top and builds down to stay out of the way of VDP2 allocator
            uint8_t* dest = reinterpret_cast<uint8_t *>(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (fontId * 0x1000));

            for (int i = 0; i < 0xC00; ++i)
            {
                *(dest++) = (*source << 4) | *(source + 1);
                source += 2;
            }
        }

        /** @brief Set current color pallet to print with (range 0-7)
         *  @param paletteId index of the 16 color pallet in CRAM (limited to the first 8 pallets)
         *  @returns false if paletteId are out-of-range, true otherwise
         */
        inline static bool SetPalette(uint8_t paletteId)
        {
            bool status = true;

            if (paletteId > maxPaletteIndex) status = false;

            paletteId =  std::min(paletteId, maxPaletteIndex);

            ASCII::colorBank = paletteId << 12;

            return status;
        }

        /** @brief Set color in the specified palette index of the current font pallet
         *  @param color RGB555 color to set in current pallet
         *  @param colorIndex index to write the color to in the currently active font pallet (Clamped to 16 color palette)
         *  @returns false if colorIndex are out-of-range, true otherwise
         */
        inline static bool SetColor(uint16_t color, uint8_t colorIndex)
        {
            bool status = true;

            if (colorIndex > maxColorIndex) status = false;

            colorIndex =  std::min(colorIndex, maxColorIndex);

            uint16_t* colorAdr = reinterpret_cast<uint16_t *>(VDP2_COLRAM + (ASCII::colorBank >> 6));
            colorAdr[colorIndex] = color;

            return status;
        }

        /** @brief Set current font to print with (range 0-5)
         *  @param fontId Index of the desired font in font table
         *  @returns false if fontId are out-of-range, true otherwise
         */
        inline static bool SetFont(uint8_t fontId)
        {
            bool status = true;

            if (fontId > maxFont) status = false;

            fontId =  std::min(fontId, maxFont);

            ASCII::fontBank = 128 * (maxFont - fontId);

            return status;
        }

        /** @brief Display ASCII string on single line. Does not clamp to screen bounds or handle overflow
         *  @param myString The string to print
         *  @param x Starting tile X coordinate on screen (0-63)
         *  @param y Starting tile Y coordinate on screen (0-63)
         *  @returns false if positions are out-of-range, true otherwise
         *  @note Tile (0,0) is aligned to the top left corner of the screen
         */
        inline static bool Print(const char* myString, uint8_t x, uint8_t y)
        {
            return Print(const_cast<char*>(myString), x, y);
        }

        /** @brief Display ASCII string on single line. Does not clamp to screen bounds or handle overflow
         *  @param myString The string to print
         *  @param x Starting tile X coordinate on screen (0-63)
         *  @param y Starting tile Y coordinate on screen (0-63)
         *  @returns false if positions are out-of-range, true otherwise
         *  @note Tile (0,0) is aligned to the top left corner of the screen
         */
        inline static bool Print(char* myString, uint8_t x, uint8_t y)
        {
            bool status = true;
            int mapIndex;
            int charOffset = ASCII::fontBank; // 128*(5-font);

            if(x > maxXPosition || y > maxXPosition)
                status = false;

            x =  std::min(x, maxXPosition);
            y =  std::min(y, maxYPosition);

            mapIndex = x + (y << 6);

            while(*myString != '\0')
            {
                ASCII::tileMap[mapIndex++] = ((uint8_t)(*myString++) + charOffset) | ASCII::colorBank;
            }

            return status;
        }

        /** @brief Clears the ASCII tile map.
         *  @returns false if tileMap is null or memset fails, true otherwise
         */
        inline static bool Clear()
        {
            bool status = true;

            if (ASCII::tileMap == nullptr)
            {
                status = false;
            }
            else
            {
                void *result = memset(ASCII::tileMap, 0, 64 * 64 * sizeof(uint16_t)); // Clear the tile map
                if (result == nullptr)
                {
                    status = false;
                }
            }

            return status;
        }
    };
}

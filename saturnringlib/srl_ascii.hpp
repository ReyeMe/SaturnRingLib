#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"

namespace SRL
{
    /** @brief Interface for displaying ASCII text. Currently a direct replacement for slPrint.
     *  It removes any possible dependecy on NBG0 system variables and displays 4bpp fonts
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

        /** @brief offset to the tile data of the current font that will print, formatted to quckly modify map data
         */
        inline static uint16_t  fontBank = 640;

        /** @brief the current number of fonts that have been loaded to Vdp2 VRAM
         */
        inline static uint8_t   numFonts = 1;
    public:
        /** @brief Copies 4bpp Bitmap ASCII table to VRAM as 4bpp tileset
         *  @param bmp pointer to an Ibitmap interface to load
         *  @note image layout must be grid of 8x8 pixel characters arranged in ASCII order from left to right,
         *  top to bottom, with 1 empty 8x8 tile preceeding the characters (see example in VDP2 Samples)
         *  @param fontId Index in the font table to load this font to (range 0-5)
         */
        inline static void LoadFont(SRL::Bitmap::IBitmap* bmp, uint16_t fontId = 0)
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
        inline static void LoadFontSG(uint8_t* source, uint16_t fontId = 0)
        {
            if (fontId > 5) fontId = 5;
            
            // Font table starts at top and builds down to stay out of the way of VDP2 allocator
            uint8_t* dest = (uint8_t*)(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (fontId * 0x1000));

            for (int i = 0; i < 0xC00; ++i)
            {
                *(dest++) = (*source << 4) | *(source + 1);
                source += 2;
            }
        }
        
        /** @brief Set current color pallet to print with (range 0-7)
         *  @param paletteId index of the 16 color pallet in CRAM (limited to the first 8 pallets) 
         */
        inline static void SetPalette(uint16_t paletteId)
        {
            if (paletteId > 7)paletteId = 7;
            
            ASCII::colorBank = paletteId << 12;
        }

        /** @brief Set color in the specified pallet index of the current font pallet
         *  @param color RGB555 color to set in current pallet
         *  @param colorIndex index to write the color to in the currently active font pallet (Clamped to 16 color palette) 
         */
        inline static void SetColor(uint16_t color, uint8_t colorIndex)
        {
            if (colorIndex > 15)colorIndex = 15;
            
            uint16_t* colorAdr = (uint16_t*)(VDP2_COLRAM + (ASCII::colorBank >> 6));
            colorAdr[colorIndex] = color;
        }
        
        /** @brief Set current font to print with (range 0-5)
         *  @param fontId Index of the desired font in font table
         */
        inline static void SetFont(uint16_t fontId)
        {
            if (fontId > 5) fontId = 5;

            ASCII::fontBank = 128 * (5 - fontId);
        }

        /** @brief Display ASCII string on single line. Does not clamp to screen bounds or handle overflow
         *  @param myString The string to print
         *  @param x Starting tile X coordinate on screen (0-64)
         *  @param y Starting tile Y coordinate on screen (0-64)
         *  @note Tile (0,0) is alligned to the top left corner of the screen
         */ 
        inline static void Print(char* myString, uint8_t x, uint8_t y)
        {
            int mapIndex;
            int charOffset = ASCII::fontBank; // 128*(5-font);
            if(x > 63) x = 63;
            if(y > 63) y = 63;
            mapIndex = x + (y << 6);

            while(*myString != '\0')
            {
                ASCII::tileMap[mapIndex++] = ((uint8_t)(*myString++) + charOffset) | ASCII::colorBank;
            }
        }
    }; 
}
    

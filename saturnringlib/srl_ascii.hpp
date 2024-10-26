#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"


namespace SRL
{
    
     /** @brief Interface for displaying ASCII text. Currently a direct replacement for slPrint,
         removes any possible dependecy on NBG0 system variables and displays 4bpp fonts
         to reduce required memory. Allows Storing and displaying up to 6 fonts and 8 color pallets.
     */
    class ASCII
    {
    private:
        /*Pointer to the ASCII map in Vdp2 VRAM 64x64*/
        inline static uint16_t* TileMap = (uint16_t*)(VDP2_VRAM_B1 + 0x1E000);
        /*@brief index of the current 16 color pallet that font will use, formatted to quickly modify map data*/
        inline static uint16_t  ColorBank = 1 << 12;
        /*@brief offset to the tile data of the current font that will print, formatted to quckly modify map data*/
        inline static uint16_t  FontBank = 640;
        /*@brief the current number of fonts that have been loaded to Vdp2 VRAM*/
        inline static uint8_t   NumFonts = 1;
    public:
        /** @brief Copies 4bpp Bitmap ASCII table to VRAM as 4bpp tileset
        *  @param bmp pointer to an Ibitmap interface to load
        *  @note image layout must be grid of 8x8 pixel characters arranged in ASCII order from left to right,
        *  top to bottom, with 1 empty 8x8 tile proceeding the characters (see example in VDP2 Samples)
        *  @param FontID Index in the font table to load this font to (range 0-5)
        */
        inline static void LoadFont(SRL::Bitmap::IBitmap* bmp, uint16_t FontID = 0)
        {
            if (FontID > 5) FontID = 5;
            uint8_t* src = bmp->GetData();
            //Font table starts at top and builds down to stay out of the way of VDP2 allocator
            uint8_t* dest = (uint8_t*)(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (FontID * 0x1000));
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
        *   @param Source Address storing SGLs 8bpp font
        *   @param FontID Font index to load to (range 0-5)
        *   
        */
        inline static void LoadFontSG(uint8_t* Source, uint16_t FontID = 0)
        {
            if (FontID > 5) FontID = 5;
            //Font table starts at top and builds down to stay out of the way of VDP2 allocator
            uint8_t* dest = (uint8_t*)(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (FontID * 0x1000));
            for (int i = 0; i < 0xC00; ++i)
            {
                *(dest++) = (*Source << 4) | *(Source + 1);
                Source += 2;
            }
        }
        
        /** @brief Set current color pallet to print with (range 0-7)
        *   @param index to 16 color pallet in CRAM (clamped to the first 8 pallets) 
        */
        inline static void SetPalette(uint16_t ID)
        {
            if (ID > 7)ID = 7;
            ColorBank = ID << 12;
        }
        /** @brief Set color in the specified pallet index of the current font pallet
            @param Color RBG555 color to set in current pallet
            @param Index Palette Index to write the color to in the currently active font pallet 
        */
        inline static void SetColor(uint16_t Color, uint16_t Index)
        {
            uint16_t* ColorAdr = (uint16_t*)(VDP2_COLRAM + (ColorBank >> 6));
            *ColorAdr = Color;
        }
        /** @brief Set current font to print with (range 0-5)
        *   @param ID Index of the desired font in font table
        */
        inline static void SetFont(uint16_t ID)
        {
            if (ID > 5) ID = 5;
            FontBank = 128 * (5 - ID);
        }
         /** @brief Display ASCII string on single line. Does not clamp to screen bounds or handle overflow
         *   @param MyString The string to print
         *   @param X Starting  tile X coordinate on screen
         *   @param Y Starting  tile Y coordinate on screen
         */ 
        inline static void Print(char* MyString, uint8_t X, uint8_t Y)
        {
            int MapIndex;
            int CharOffset = ASCII::FontBank;//128*(5-font);
            if(X>63) X = 63;
            if(Y>63) Y = 63;
            MapIndex = X+(Y<<6);
            while(*MyString !='\0')
            {
                ASCII::TileMap[MapIndex++] = ((uint8_t)(*MyString++) + CharOffset)|ColorBank;
            }
        }
        
    }; 
}
    

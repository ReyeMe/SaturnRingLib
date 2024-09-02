#pragma once

#include "srl_tv.hpp"
#include "srl_cram.hpp"

//hey there- are these changes showing up correctly?
//testing again
namespace SRL
{
    
     /** @brief Interface for displaying ASCII text. Currently a direct replacement for slPrint,
          removes any possible dependecy on NBG0 system variables and displays 4bpp fonts
         to reduce required memory
     */
    class ASCII
    {
    private:
        inline static uint16_t* TileMap = (uint16_t*)(VDP2_VRAM_B1 + 0x1E000);//[4096];// 64x64 grid of characters to be displayed
        inline static uint16_t  ColorBank = 1 << 12;
        inline static uint16_t  FontBank = 640;
        inline static uint8_t   NumFonts = 1;
    public:
        /** @brief Copies 4bpp Bitmap ASCII table to VRAM as 4bpp tileset
          */
        inline static void LoadFont(SRL::Bitmap::IBitmap* bmp, uint16_t FontID = 0)
        {
            uint8_t* src = bmp->GetData();
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
          */
        inline static void LoadFontSG(uint8_t* src, uint16_t FontID = 0)
        {
            uint8_t* dest = (uint8_t*)(VDP2_VRAM_B1 + 0x1D000 + 0x400 - (FontID * 0x1000));
            for (int i = 0; i < 0xC00; ++i)
            {
                *(dest++) = (*src << 4) | *(src + 1);
                src += 2;
            }
        }
       
        /** @brief Set current color pallet to print with (range 0-7)
           */
        inline static void SetPalette(uint16_t ID)
        {
            ColorBank = ID << 12;
        }
        /** @brief Set current font to print with (range 0-5)
            */
        inline static void SetFont(uint16_t ID)
        {
            if (ID > 5) ID = 5;
            FontBank = 128 * (5 - ID);
        }
         /** @brief Display ASCII string on single line. Does not clamp to screen bounds or handle overflow
            */ 
        inline static void Print(char* mystring, uint8_t X, uint8_t Y)
        {
            int MapIndex;
            int CharOffset = ASCII::FontBank;//128*(5-font);
            if(X>63) X = 63;
            if(Y>63) Y = 63;
            MapIndex = X+(Y<<6);
            while(*mystring !='\0')
            {
                ASCII::TileMap[MapIndex++] = ((uint8_t)(*mystring++) + CharOffset)|ColorBank;
            }
        }
        
    }; 
}
    

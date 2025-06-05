#pragma once

#include "srl_memory.hpp"
#include "srl_cram.hpp"

/** @brief Bitmap loading
 */
namespace SRL::Bitmap
{
    /** @brief Color palette loaded from bitmap
     */
    struct Palette
    {
        /** @brief Number of colors in the palette
         */
        size_t Count;

        /** @brief Palette colors
         */
        SRL::Types::HighColor* Colors;

        /** @brief Construct a new Palette object
         * @param count Number of colors in the palette
         */
        Palette(size_t count) : Colors(nullptr), Count(count)
        {
            this->Colors = autonew SRL::Types::HighColor[count];
        }

        /** @brief Construct a new Palette object
         * @param colors Palette colors
         * @param count Number of colors in the palette
         */
        Palette(SRL::Types::HighColor* colors, size_t count) : Colors(colors), Count(count)
        {
            // Do nothing
        }

        /** @brief Destroy the Palette object
         */
        ~Palette()
        {
            delete this->Colors;
        }
    };

    /** @brief Bitmap information
     */
    struct BitmapInfo
    {
        /** @brief Used palette (not in use for RGB555 images)
         */
        Bitmap::Palette* Palette;

        /** @brief Image width
         */
        uint16_t Width;

        /** @brief Image height
         */
        uint16_t Height;

        /** @brief Image color mode
         */
        CRAM::TextureColorMode ColorMode;

        /** @brief Construct a new Bitmap info
         * @param width Image width
         * @param height Image height
         */
        BitmapInfo(uint16_t width, uint16_t height) : Width(width), Height(height), ColorMode(CRAM::TextureColorMode::RGB555), Palette(nullptr)
        {
            // Do nothing
        }

        /** @brief Construct a new Bitmap info
         * @param width Image width
         * @param height Image height
         * @param palette Color palette
         */
        BitmapInfo(uint16_t width, uint16_t height, Bitmap::Palette* palette) : Width(width), Height(height), Palette(palette)
        {
            if (palette != nullptr)
            {
                if (palette->Count <= 16)
                {
                    this->ColorMode = CRAM::TextureColorMode::Paletted16;
                }
                else if (palette->Count <= 64)
                {
                    this->ColorMode = CRAM::TextureColorMode::Paletted64;
                }
                else if (palette->Count <= 128)
                {
                    this->ColorMode = CRAM::TextureColorMode::Paletted128;
                }
                else
                {
                    this->ColorMode = CRAM::TextureColorMode::Paletted256;
                }
            }
            else
            {
                this->ColorMode = CRAM::TextureColorMode::RGB555;
            }
        }
    };

    /** @brief Basic bitmap interface
     */
    struct IBitmap
    {
        /** @brief Get image data
         * @return Pointer to image data
         */
        virtual uint8_t* GetData()
        {
            return nullptr;
        }
        
        /** @brief Get bitmap info
         * @return Bitmap info
         */
        virtual BitmapInfo GetInfo()
        {
            return BitmapInfo(0, 0);
        }
    };
}

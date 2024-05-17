#pragma once

#include "srl_base.hpp"
#include "srl_color.hpp"

namespace SRL
{
    /** @brief Color RAM management
     */
    class CRAM
    {
    public:

        /** @brief Base RAM address
         */
        inline static const constexpr void* BaseAddress = (void*)VDP2_COLRAM;
    
        /** @brief Texture color mode
         */
        enum class TextureColorMode : Uint16
        {
            /** @brief 256 color image
             */
            Paletted256 = 5,

            /** @brief 128 color image
             */
            Paletted128 = 4,

            /** @brief 128 color image
             */
            Paletted64 = 3,

            /** @brief 16 color image
             */
            Paletted16 = 2,

            /** @brief 32k color image
             */
            RGB555 = 1
        };

        /** @brief Color palette in color RAM
         */
        struct Palette
        {
        private:

            /** @brief Color data
             */
            void* data;

            /** @brief Color mode
             */
            TextureColorMode paletteMode;

            /** @brief Palette identifier
             */
            Uint16 id;

            /** @brief Construct a new Palette entry
             * @param data Color data pointer
             * @param mode Color mode
             * @param id Palette identifier
             */
            Palette(void* data, TextureColorMode mode, Uint16 id) : data(data), paletteMode(mode), id(id) { }

        public:

            /** @brief Get palette identifier
             * @return Palette identifier
             */
            Uint16 GetId()
            {
                return this->id;
            }

            /** @brief Get color mode
             * @return Color mode 
             */
            TextureColorMode GetMode()
            {
                return this->paletteMode;
            }

            /** @brief Get color data
             * @return Color data 
             */
            void* GetData()
            {
                return this->data;
            }
        };
    };
}

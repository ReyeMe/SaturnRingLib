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
        inline static const void* BaseAddress = (const void*)(VDP2_COLRAM);
    
        /** @brief Texture color mode
         */
        enum class TextureColorMode : uint16_t
        {
            /** @brief 256 color image
             */
            Paletted256 = 6,

            /** @brief 128 color image
             */
            Paletted128 = 5,

            /** @brief 64 color image
             */
            Paletted64 = 4,

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
            SRL::Types::HighColor* data;

            /** @brief Color mode
             */
            CRAM::TextureColorMode paletteMode;

            /** @brief Palette identifier
             */
            uint16_t id;

        public:

            /** @brief Construct a new empty palette
             */
            Palette() : paletteMode(CRAM::TextureColorMode::RGB555), id(0) { }

            /** @brief Construct a new Palette entry
             * @param mode Color mode
             * @param id Palette identifier
             */
            Palette(const CRAM::TextureColorMode mode, const uint16_t id) : paletteMode(mode), id(id)
            {
                if (mode == CRAM::TextureColorMode::RGB555)
                {
                    this->data == nullptr;
                }
                else
                {
                    this->data = ((SRL::Types::HighColor*)CRAM::BaseAddress) + (id * (16 << (((uint16_t)mode) - 2)));
                }
            }

            /** @brief Get palette identifier
             * @return Palette identifier
             */
            uint16_t GetId()
            {
                return this->id;
            }

            /** @brief Get color mode
             * @return Color mode 
             */
            CRAM::TextureColorMode GetMode()
            {
                return this->paletteMode;
            }

            /** @brief Get color data
             * @return Color data 
             */
            SRL::Types::HighColor* GetData()
            {
                return this->data;
            }

            /** @brief Load color data to palette
             * @param data Color data
             * @param count Number of color to load (-1 means full palette)
             * @return Number of colors that have been loaded, -1 on error
             */
            int16_t Load(Types::HighColor* data, const int16_t count = -1)
            {
                int16_t colorCount = count;

                // Not valid in RGB555 color mode configuration
                if (this->paletteMode != CRAM::TextureColorMode::RGB555)
                {
                    if (count < 0)
                    {
                        colorCount = (16 << (((uint16_t)this->paletteMode) - 2));
                    }

                    // Copy colors to CRAM
                    slDMACopy(
                        data,
                        this->GetData(),
                        colorCount * sizeof(Types::HighColor));
                    slDMAWait();

                    return colorCount;
                }

                return -1;
            }
        };

    private:

        /** @brief Allocation mask
         * @note Each entry is one 256 color bank
         */
        inline static uint16_t AllocationMask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

    public:

        /** @brief Gets a value indicating whether said color bank is being used or not
         * @param bank Color bank index
         * @param size Color bank size
         * @return true if bank is being used
         */
        static bool GetBankUsedState(uint16_t bank, CRAM::TextureColorMode size)
        {
            switch (size)
            {
            case CRAM::TextureColorMode::Paletted256:
                return CRAM::AllocationMask[bank] != 0;

            case CRAM::TextureColorMode::Paletted128:
                return (CRAM::AllocationMask[bank >> 1] & (0xff << ((bank & 0x1) << 1))) != 0;
            
            case CRAM::TextureColorMode::Paletted64:
                return (CRAM::AllocationMask[bank >> 2] & (0xf << ((bank & 0x2) << 2))) != 0;
            
            case CRAM::TextureColorMode::Paletted16:
                return (CRAM::AllocationMask[bank >> 4] & (1 << (bank & 0xf))) != 0;

            default:
                return false;
            }
        }

        /** @brief Sets a value in bank indicating whether said color bank is being used or not
         * @param bank Color bank index
         * @param size Color bank size
         * @param used true if bank is being used, otherwise false
         */
        static void SetBankUsedState(uint16_t bank, CRAM::TextureColorMode size, bool used)
        {
            switch (size)
            {
            case CRAM::TextureColorMode::Paletted256:
                CRAM::AllocationMask[bank] = used ? 0xffff : 0x0000;
                break;

            case CRAM::TextureColorMode::Paletted128:
                if (used)
                {
                    CRAM::AllocationMask[bank >> 1] |= (0xff << ((bank & 0x1) << 1));
                }
                else
                {
                    CRAM::AllocationMask[bank >> 1] &= ~(0xff << ((bank & 0x1) << 1));
                }

                break;
            
            case CRAM::TextureColorMode::Paletted64:
                if (used)
                {
                    (CRAM::AllocationMask)[bank >> 2] |= (0xf << ((bank & 0x2) << 2));
                }
                else
                {
                    (CRAM::AllocationMask)[bank >> 2] &= ~(0xf << ((bank & 0x2) << 2));
                }

                break;
            
            case CRAM::TextureColorMode::Paletted16:

                if (used)
                {
                    CRAM::AllocationMask[bank >> 4] |= (1 << (bank & 0xf));
                }
                else
                {
                    CRAM::AllocationMask[bank >> 4] &= ~(1 << (bank & 0xf));
                }

                break;

            default:
                break;
            }
        }

        /** @brief Get the Free color RAM bank
         * @param size Color palette size
         * @return -1 if no free bank was found, or Color bank index
         */
        static int32_t GetFreeBank(CRAM::TextureColorMode size)
        {
            if (size != CRAM::TextureColorMode::RGB555)
            {
                int32_t freeBank = -1;

                // Here we will search the allocation mask for free space
                // Palettes cannot overlap and must be aligned to their respective bank size
                switch (size)
                {
                case CRAM::TextureColorMode::Paletted256:
                    for (int32_t id = 0; id < 8 && freeBank < 0; CRAM::AllocationMask[id] == 0 ? freeBank = id : id++);
                    break;

                case CRAM::TextureColorMode::Paletted128:
                    for (int32_t id = 0; id < 16 && freeBank < 0; (CRAM::AllocationMask[id >> 1] & (0xff << ((id & 0x1) << 1))) == 0 ? freeBank = id : id++);
                    break;
                
                case CRAM::TextureColorMode::Paletted64:
                    for (int32_t id = 0; id < 32 && freeBank < 0; (CRAM::AllocationMask[id >> 2] & (0xf << ((id & 0x2) << 2))) == 0 ? freeBank = id : id++);
                    break;
                
                case CRAM::TextureColorMode::Paletted16:
                    for (int32_t id = 0; id < 128 && freeBank < 0; (CRAM::AllocationMask[id >> 4] & (1 << (id & 0xf))) == 0 ? freeBank = id : id++);
                    break;
                default:
                    break;
                }

                return freeBank;
            }

            return -1;
        }
    };
}

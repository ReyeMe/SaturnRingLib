#pragma once

#include "srl_devcart.hpp"
#include "srl_string.hpp"

#include <cstdint>

namespace SRL
{
    namespace Cartridge
    {

        static constexpr uint32_t InterruptStatusRegister = 0x25fe00a4UL;  /* Interrupt status register */

        /** @brief Cartridge ID values
         */
        enum CartridgeId
        {
            /** @brief No cartridge detected
             */
            None = 0,

            /** @brief 1 MiB cartridge
             */
            Cart1MiB = 0x5A,

            /** @brief 4 MiB cartridge
             */
            Cart4MiB = 0x5C,

            /** @brief USB dev cartridge
             */
            USBDevCart, 

            /** @brief Data cartridge
             */
            DataCart, 

        };


        namespace Cartridge1MiB
        {
            static constexpr CartridgeId Id = Cart1MiB;

            /** @brief 1 MiB cartridge addres  access
             */
            static constexpr uintptr_t Address = 0x02400000UL;

            static constexpr size_t Size = 1024 * 1024; // 1 MiB

            static constexpr const char *Name = "1 MiB Cartridge";
        };

        namespace Cartridge4MiB
        {
            static constexpr CartridgeId Id = Cart4MiB;

            /** @brief 4 MiB cartridge address for contiguous access
             */
            static constexpr uintptr_t Address = 0x24000000UL;

            static constexpr size_t Size = 4 * 1024 * 1024; // 4 MiB

            static constexpr const char *Name = "4 MiB Cartridge";
        };

        namespace CartridgeUSBDev
        {
            static constexpr CartridgeId Id = USBDevCart;

            static constexpr const char *Name = "USB Dev Cartridge";
        };

        namespace CartridgeData
        {
            static constexpr CartridgeId Id = DataCart;

             /** @brief Data cartridge address for contiguous access
             */
            static constexpr uintptr_t Address = 0x22000000UL;

            static constexpr const char *Name = "Data Cartridge";

            static constexpr char HWId[] = "SEGA SEGASA";

            static constexpr size_t Size = 0; // TBD
        };

        /** @brief Convert a raw cartridge ID byte to a CartridgeId
         * @param rawId Raw cartridge ID byte
         * @return Corresponding CartridgeId
         */
        inline static CartridgeId RawIdToCartridgeId(uint8_t rawId)
        {
            switch (rawId)
            {
            case Cart1MiB:
                return CartridgeId::Cart1MiB;
            case Cart4MiB:
                return CartridgeId::Cart4MiB;
            default:
                return CartridgeId::None;
            }
        }

        /** @brief Cartridge ID register address
         */
        static constexpr uintptr_t CartridgeIdRegister = 0x24FFFFFFUL;

        
        /** @brief Detect cartridge type by reading the cartridge ID register.
         *
         *  This function reads the cartridge ID register to identify the type of memory cartridge inserted.
         *  It temporarily configures the SCU (System Control Unit) for cartridge access, reads the ID,
         *  and then restores the SCU configuration.
         *
         * @return uint8_t The raw cartridge ID read from the CartridgeIdRegister.
         */
        inline static uint8_t DetectMemoryCartridge()
        {
            // Save SCU configuration
            uint32_t scuMask = *((volatile uint32_t *)InterruptStatusRegister);

            // Configure SCU for cartridge access
            *((volatile uint32_t *)InterruptStatusRegister) = 0x00000000;

            // Read cartridge ID
            uint8_t cartId = *((volatile uint8_t *)CartridgeIdRegister);

            // Restore SCU configuration
            *((volatile uint32_t *)InterruptStatusRegister) = scuMask;

            // Determine cartridge type based on ID
            return cartId;
        }


        inline static bool DetectDataCartridge()
        {
            bool bReturn = false;

            // Save SCU configuration
            uint32_t scuMask = *((volatile uint32_t *)InterruptStatusRegister);

            // Configure SCU for cartridge access
            *((volatile uint32_t *)InterruptStatusRegister) = 0x00000000;

                        // Read cartridge ID
            bReturn = (0 == strncmp((char *)CartridgeData::HWId, (char *)CartridgeData::Address, sizeof(CartridgeData::HWId)));


            // Restore SCU configuration
            *((volatile uint32_t *)InterruptStatusRegister) = scuMask;

            // Determine cartridge type based on ID
            return bReturn;
        }

        /** @brief Detect cartridge type by checking RAM and USB cartridges.
         *
         *  This function attempts to identify the cartridge type by first checking for a RAM cartridge
         *  using DetectMemoryCartridge(). If no RAM cartridge is detected, it then checks for a USB
         *  development cartridge using SRL::DevCart::CS0::isAvailable().
         *
         * @return CartridgeId The detected cartridge type.
         */
        inline static CartridgeId DetectCartridgeType()
        {
            // 1- Try to identificate a RAM cartridge
            CartridgeId ramCartId = RawIdToCartridgeId(DetectMemoryCartridge());

            if (ramCartId != CartridgeId::None)
            {
                return ramCartId;
            }

            // 2- Try to identify a USB dev cartridge
            if (SRL::DevCart::CS0::isConnected())
            {
                return CartridgeId::USBDevCart;
            }

            // 3- Try to identify a data cartridge
            if (DetectDataCartridge())
            {
                return CartridgeId::DataCart;
            }

            // Determine cartridge type based on ID
            return ramCartId;
        }

        /** @brief Get base address for cartridge type
         * @param type Cartridge type
         * @return Base address for t   he cartridge
         */
        inline static void *GetBaseAddressForType(SRL::Cartridge::CartridgeId type)
        {
            switch (type)
            {
            case CartridgeId::Cart1MiB:
                return (void *)Cartridge1MiB::Address; // Use special address for 1MiB for contiguous access
            case CartridgeId::Cart4MiB:
                return (void *)Cartridge4MiB::Address;
            case CartridgeId::DataCart:
                return (void *)CartridgeData::Address;
            case CartridgeId::USBDevCart:
            default:
                return nullptr;
            }
        }

        /** @brief Get size for cartridge type
         * @param type Cartridge type
         * @return Size in bytes
         */
        inline static size_t GetSizeForType(CartridgeId type)
        {
            switch (type)
            {
            case CartridgeId::Cart1MiB:
                return Cartridge1MiB::Size;
            case CartridgeId::Cart4MiB:
                return Cartridge4MiB::Size;
            case CartridgeId::DataCart:
                return CartridgeData::Size;
            default:
                return 0;
            }
        }

        /** @brief Get size for cartridge type
         * @param type Cartridge type
         * @return Size in bytes
         */
        inline static const char * GetStringFromType(CartridgeId type)
        {
            switch (type)
            {
            case CartridgeId::Cart1MiB:
                return Cartridge1MiB::Name;
            case CartridgeId::Cart4MiB:
                return Cartridge4MiB::Name;
            case CartridgeId::USBDevCart:
                return CartridgeUSBDev::Name;
            default:
                return "Unknown Cartridge";
            }
        }

        /**
         * @brief Check if a specific cartridge type is present.
         * 
         * This function detects the currently inserted cartridge type and compares it against the provided type.
         * 
         * @param type The CartridgeId to check for.
         * @return true If the specified cartridge type is detected, false otherwise.
         */
        inline static bool IsCartridgeTypePresent(CartridgeId type)
        {
            return DetectCartridgeType() == type;
        }


    } // namespace Cartridge
} // namespace SRL

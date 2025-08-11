
#pragma once

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
            Cart4MiB = 0x5C
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
        static constexpr uint32_t CartridgeIdRegister = 0x24FFFFFFUL;

        /** @brief 1 MiB cartridge address for contiguous access
         */
        static constexpr uint32_t Address1MiB = 0x02400000UL;

        /** @brief 4 MiB cartridge address
         */
        static constexpr uint32_t Address4MiB = 0x24000000UL;

        /** @brief Detect cartridge type
         * @return Detected cartridge type
         */
        inline static CartridgeId DetectCartridgeType()
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
            return RawIdToCartridgeId(cartId);
        }

        /** @brief Get base address for cartridge type
         * @param type Cartridge type
         * @return Base address for the cartridge
         */
        inline static void *GetBaseAddressForType(SRL::Cartridge::CartridgeId type)
        {
            switch (type)
            {
            case CartridgeId::Cart1MiB:
                return (void *)Address1MiB; // Use special address for 1MiB for contiguous access
            case CartridgeId::Cart4MiB:
                return (void *)Address4MiB;
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
                return 1024 * 1024; // 1 MiB
            case CartridgeId::Cart4MiB:
                return 4 * 1024 * 1024; // 4 MiB
            default:
                return 0;
            }
        }

    } // namespace Cartridge
} // namespace SRL

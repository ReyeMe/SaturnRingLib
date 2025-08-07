
#pragma once

#include <cstdint>

namespace SRL
{
    namespace Cartridge
    {

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

            /** @brief Cartridge ID register address
             */
            static constexpr uint32_t CartridgeIdRegister = 0x24FFFFFF;

            /** @brief Detect cartridge type
             * @return Detected cartridge type
             */
            inline static CartridgeId DetectCartridgeType()
            {
                // Save SCU configuration
                uint32_t scuMask = *((volatile uint32_t*)0x25FE00A4);
                
                // Configure SCU for cartridge access
                *((volatile uint32_t*)0x25FE00A4) = 0x00000000;
                
                // Read cartridge ID
                uint8_t cartId = *((volatile uint8_t*)CartridgeIdRegister);
                
                // Restore SCU configuration
                *((volatile uint32_t*)0x25FE00A4) = scuMask;
                
                // Determine cartridge type based on ID
                if (cartId == CartRam::Cart1MiB)
                {
                    return CartRam::CartridgeId::Cart1MiB;
                }
                else if (cartId == CartRam::Cart4MiB)
                {
                    return CartRam::CartridgeId::Cart4MiB;
                }
                
                return CartRam::CartridgeId::None;
            }

        
    } // namespace Cartridge
} // namespace SRL

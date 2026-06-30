#pragma once

#include "srl_base.hpp"

#include <cstdint>

#ifdef __cplusplus

namespace SRL
{
    /** @brief System Control Unit (SCU) helpers
     */
    namespace SCU
    {
        /** @brief SCU DSP (Digital Signal Processor) helpers
         */
        namespace DSP
        {
            /** @brief Result of checking if DSP program has ended
             */
            enum class EndState : uint8_t
            {
                /** @brief DSP program is still running
                 */
                NotEnded = 0,

                /** @brief DSP program has ended
                 */
                Ended = 1,
            };

            /** @brief Default SCU DSP register map
             * @details Register addresses are static for the SCU DSP.
             */
            struct RegisterMap
            {
                /** @brief DSP control register address
                 */
                static constexpr uintptr_t RwCtrl = 0x25FE0080UL;

                /** @brief DSP program data write port address
                 */
                static constexpr uintptr_t ProgramData = 0x25FE0084UL;

                /** @brief DSP data RAM address write port address
                 */
                static constexpr uintptr_t DataAddress = 0x25FE0088UL;

                /** @brief DSP data RAM read/write data port address
                 */
                static constexpr uintptr_t DataData = 0x25FE008CUL;

                /** @brief SCU interrupt status register address
                 */
                static constexpr uintptr_t InterruptStatusRegister = 0x25FE00A4UL;

                /** @brief SH2 cache control register address
                 */
                static constexpr uintptr_t CacheControlRegister = 0xFFFFFE92UL;
            };

            /** @brief DSP interrupt status bit
             */
            static constexpr uint32_t InterruptDspBit = 0x20U;

            /** @brief DSP busy bit in the DSP control register
             */
            static constexpr uint32_t DspBusyBit = 0x00800000U;

            /** @brief Cache purge bit in the cache control register
             */
            static constexpr uint16_t CachePurgeBit = 0x0010U;

            /** @brief Write a 32-bit value to a memory-mapped register
             * @param address Register address
             * @param value Value to write
             */
            inline static void WriteRegister32(uintptr_t address, uint32_t value)
            {
                *reinterpret_cast<volatile uint32_t *>(address) = value;
            }

            /** @brief Read a 32-bit value from a memory-mapped register
             * @param address Register address
             * @return Current register value
             */
            inline static uint32_t ReadRegister32(uintptr_t address)
            {
                return *reinterpret_cast<volatile uint32_t *>(address);
            }

            /** @brief Load DSP program into the DSP program RAM
             * @param destination Address in DSP program RAM
             * @param source Pointer to source program data
             * @param count Number of 32-bit words to write
             */
            inline static void LoadProgram(uint8_t destination, const uint32_t *source, uint16_t count)
            {
#if defined(__GNUC__)
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wvolatile"
#endif
                INT_ChgMsk(INT_MSK_NULL, INT_MSK_DSP);
#if defined(__GNUC__)
                #pragma GCC diagnostic pop
#endif

                WriteRegister32(RegisterMap::RwCtrl, 0U);

                uint32_t controlValue = 0x00008000U | (uint32_t)destination;
                WriteRegister32(RegisterMap::RwCtrl, controlValue);

                for (uint16_t index = 0; index < count; index++)
                {
                    WriteRegister32(RegisterMap::ProgramData, *source++);
                }
            }

            /** @brief Write data into the DSP data RAM
             * @param destination Address in DSP data RAM
             * @param source Pointer to source data
             * @param count Number of 32-bit words to write
             */
            inline static void WriteData(uint8_t destination, const uint32_t *source, uint16_t count)
            {
                WriteRegister32(RegisterMap::RwCtrl, 0U);
                WriteRegister32(RegisterMap::DataAddress, (uint32_t)destination);

                for (uint16_t index = 0; index < count; index++)
                {
                    WriteRegister32(RegisterMap::DataData, *source++);
                }
            }

            /** @brief Read data from the DSP data RAM
             * @param destination Pointer to destination buffer
             * @param source Address in DSP data RAM
             * @param count Number of 32-bit words to read
             */
            inline static void ReadData(uint32_t *destination, uint8_t source, uint16_t count)
            {
                WriteRegister32(RegisterMap::RwCtrl, 0U);
                WriteRegister32(RegisterMap::DataAddress, (uint32_t)source);

                for (uint16_t index = 0; index < count; index++)
                {
                    *destination++ = ReadRegister32(RegisterMap::DataData);
                }
            }

            /** @brief Start executing DSP program
             * @param programCounter Start address/program counter in DSP program RAM
             */
            inline static void Start(uint8_t programCounter)
            {
                WriteRegister32(RegisterMap::RwCtrl, 0U);

                uint32_t controlValue = 0x00018000U | (uint32_t)programCounter;
                WriteRegister32(RegisterMap::RwCtrl, controlValue);
            }

            /** @brief Stop DSP program execution
             */
            inline static void Stop()
            {
                WriteRegister32(RegisterMap::RwCtrl, 0U);
            }

            /** @brief Check if DSP processing has ended
             * @return EndState::Ended if DSP has finished processing, otherwise EndState::NotEnded
             */
            inline static EndState CheckEnd()
            {
                uint32_t interruptStatus = *reinterpret_cast<volatile uint32_t *>(RegisterMap::InterruptStatusRegister);

                if ((interruptStatus & InterruptDspBit) != 0U)
                {
                    while ((ReadRegister32(RegisterMap::RwCtrl) & DspBusyBit) != 0U)
                    {
                    }

                    *reinterpret_cast<volatile uint32_t *>(RegisterMap::InterruptStatusRegister) = ~InterruptDspBit;
                    *reinterpret_cast<volatile uint16_t *>(RegisterMap::CacheControlRegister) |= CachePurgeBit;

                    return EndState::Ended;
                }

                return EndState::NotEnded;
            }

        } // namespace DSP
    } // namespace SCU
} // namespace SRL

#endif // __cplusplus

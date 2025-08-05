// SatCom Library
// by cafe-alpha
// Modernized by OpenAI's ChatGPT
// Original: http://ppcenter.free.fr/satcom/
// License: See LICENSE file for details

#pragma once

#include <cstdint>

namespace SRL
{
    namespace DevCart
    {

        // CS0 area (Flash memory and USB related registers)
        namespace CS0
        {
            constexpr static unsigned CART_BASE_ADR = 0x22000000UL;
            constexpr static uint32_t FLASH_MEMORY_BASE = CART_BASE_ADR + 0x0; // 1MB flash (2 x 512KB chips)
            constexpr static unsigned long USB_FLAGS = CART_BASE_ADR + 0x200001UL;    // RW USB module fifo status register
            constexpr static uint32_t USB_FIFO = CART_BASE_ADR + 0x100001;     // RW USB module fifo data
            // 0x223x to 0x227x unused

            constexpr static uint32_t FIRM_MAXLEN = 1024 * 1024; // Maximum length allowed for firmware (limited by LRAM size, because flashing cart from stream data is not supported)

            class USBFlags
            {
            public:
                // Anonymous enum: constants accessible as USBFlags::TXE, etc.
                enum : uint8_t
                {
                    RXF = 1 << 0,
                    TXE = 1 << 1,
                    PWREN = 1 << 7
                };

            private:
                uint8_t bits_;

            public:
                // Constructors
                USBFlags() : bits_(0) {}
                explicit USBFlags(uint8_t bits) : bits_(bits) {}
                USBFlags(std::initializer_list<uint8_t> flags) : bits_(0)
                {
                    for (auto f : flags)
                        bits_ |= f;
                }

                // Conversion to bool
                explicit operator bool() const { return bits_ != 0; }

                // Bitwise OR
                USBFlags operator|(USBFlags other) const { return USBFlags(bits_ | other.bits_); }
                USBFlags &operator|=(USBFlags other)
                {
                    bits_ |= other.bits_;
                    return *this;
                }

                // Bitwise AND
                USBFlags operator&(USBFlags other) const { return USBFlags(bits_ & other.bits_); }
                USBFlags &operator&=(USBFlags other)
                {
                    bits_ &= other.bits_;
                    return *this;
                }

                // Bitwise NOT
                USBFlags operator~() const { return USBFlags(static_cast<uint8_t>(~bits_)); }

                // Test if a flag is set
                bool has(uint8_t flag) const { return (bits_ & flag) != 0; }

                // Get raw bits
                uint8_t bits() const { return bits_; }
            };

            static inline bool isTXEFull()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)&USBFlags::TXE) == USBFlags::TXE);
            }

            static inline void waitTXE()
            {
                // Bad design, no timeout !
                while (isTXEFull());
            }

            static inline bool isRXFFull()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)&USBFlags::RXF) == USBFlags::RXF);
            }

            static inline void waitRXF()
            {
                // Bad design, no timeout !
                while (isRXFFull());
            }

            static inline void send(const uint8_t * c)
            {
                waitTXE();
                *(volatile uint8_t *)(USB_FLAGS) = *c;
            }

            static inline uint8_t read()
            {
                waitRXF();
                return *(volatile uint8_t *)(USB_FLAGS);
            }

            static inline bool isAvailable()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)&USBFlags::PWREN) == USBFlags::PWREN);
            }

        }

        // CS1 area (CPLD registers)
        namespace CS1
        {
            constexpr static uint32_t CPLD_BASE_ADDR = 0x24000000L;

            enum class Register : uint32_t
            {
                CPLD_FF55 = CPLD_BASE_ADDR + 0x00,
                CPLD_55 = CPLD_BASE_ADDR + 0x01,
                CPLD_FFAA = CPLD_BASE_ADDR + 0x02,
                CPLD_AA = CPLD_BASE_ADDR + 0x03,
                CART_CPLD_VER = CPLD_BASE_ADDR + 0x05,
                CART_BETA_ID = CPLD_BASE_ADDR + 0x07,
                CPLD_IO = CPLD_BASE_ADDR + 0x09,
                SDIN_BITS = CPLD_BASE_ADDR + 0x0B,
                LED_SETTING = CPLD_BASE_ADDR + 0x0D,
                SD_CLK_SET = CPLD_BASE_ADDR + 0x0F,
                REG_STDOUT_BIT = CPLD_BASE_ADDR + 0x11,
                REG_SD_IO_0 = CPLD_BASE_ADDR + 0x11,
                REG_SD_IO_1 = CPLD_BASE_ADDR + 0x13,
                REG_SD_IO_2 = CPLD_BASE_ADDR + 0x15,
                REG_SD_IO_3 = CPLD_BASE_ADDR + 0x17,
                REG_SD_REINSERT = CPLD_BASE_ADDR + 0x19
            };

            enum class SD_LSHFT : uint8_t
            {
                SD_LEDG_LSHFT = 0,
                SD_LEDR_LSHFT = 1,
                SD_SW1_LSHFT = 4,
                SD_EJECT_LSHFT = 7
            };

            enum class SD_CTRL_LSHFT : uint8_t
            {
                SD_CSL_LSHFT = 0,
                SD_DIN_LSHFT = 1,
                SD_CLK_LSHFT = 2
            };

        }
    } // namespace DevCart
} // namespace SRL

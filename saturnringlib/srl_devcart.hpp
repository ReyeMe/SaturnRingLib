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
            constexpr static uintptr_t CART_BASE_ADR = 0x22000000UL;            // Base address of the cartridge in CS0 area
            constexpr static uintptr_t FLASH_MEMORY_BASE = CART_BASE_ADR + 0x0; // Base address of the flash memory (1MB)
            constexpr static uintptr_t USB_FLAGS = CART_BASE_ADR + 0x200001UL;  // Address of the USB flags register (Read/Write)
            constexpr static uintptr_t USB_FIFO = CART_BASE_ADR + 0x100001;     // Address of the USB FIFO data register (Read/Write)
            // 0x223x to 0x227x unused

            constexpr static size_t FIRM_MAXLEN = 1024 * 1024; // Maximum length allowed for firmware (1MB)

            /**
             * @brief Class representing the USB flags register.
             *
             * This class provides a convenient way to access and manipulate the individual bits
             * in the USB flags register.
             */
            class USBFlags
            {
            public:
                // Anonymous enum: constants accessible as USBFlags::TXE, etc.
                enum : uint8_t
                {
                    RXF = 1 << 0,  // RXF: Receive FIFO Full
                    TXE = 1 << 1,  // TXE: Transmit FIFO Empty
                    PWREN = 1 << 7 // PWREN: Power Enable
                };

            private:
                uint8_t bits_; // Internal storage for the flags

            public:
                // Constructors
                USBFlags() : bits_(0) {}                                  // Default constructor: no flags set
                explicit USBFlags(uint8_t bits) : bits_(bits) {}          // Constructor: initialize with raw bits
                USBFlags(std::initializer_list<uint8_t> flags) : bits_(0) // Constructor: initialize with a list of flags
                {
                    for (auto f : flags)
                        bits_ |= f; // Set each flag provided in the initializer list
                }

                // Conversion to bool
                explicit operator bool() const { return bits_ != 0; } // Operator to check if any flag is set

                // Bitwise OR
                USBFlags operator|(USBFlags other) const { return USBFlags(bits_ | other.bits_); } // OR operator: combines flags
                USBFlags &operator|=(USBFlags other)
                {
                    bits_ |= other.bits_; // OR assignment: adds flags
                    return *this;
                }

                // Bitwise AND
                USBFlags operator&(USBFlags other) const { return USBFlags(bits_ & other.bits_); } // AND operator: keeps common flags
                USBFlags &operator&=(USBFlags other)
                {
                    bits_ &= other.bits_; // AND assignment: removes flags that are not common
                    return *this;
                }

                // Bitwise NOT
                USBFlags operator~() const { return USBFlags(static_cast<uint8_t>(~bits_)); } // NOT operator: inverts all flags

                // Test if a flag is set
                bool has(uint8_t flag) const { return (bits_ & flag) != 0; } // Checks if a specific flag is set

                // Get raw bits
                uint8_t bits() const { return bits_; } // Returns the raw bits representing the flags
            };

            /**
             * @brief Checks if the Transmit FIFO Empty (TXE) flag is full.
             *
             * This function reads the USB_FLAGS register and checks if the TXE bit is set.
             *
             * @return true If the TXE flag is set (FIFO is full), false otherwise.
             */
            static inline bool isTXEFull()
            {
                return ((*(uint8_t *)(USB_FLAGS)&USBFlags::TXE) == USBFlags::TXE);
            }

            /**
             * @brief Waits until the Transmit FIFO Empty (TXE) flag is no longer full.
             *
             * This function continuously checks the TXE flag until it is cleared, indicating that
             * the transmit FIFO is no longer full and data can be written.
             *
             * Note: This function has no timeout and will loop indefinitely if the TXE flag is never cleared.
             */
            static inline void waitTXE()
            {
                // Bad design, no timeout !
                while (isTXEFull())
                    ;
            }

            /**
             * @brief Checks if the Receive FIFO Full (RXF) flag is full.
             *
             * This function reads the USB_FLAGS register and checks if the RXF bit is set.
             *
             * @return true If the RXF flag is set (FIFO is full), false otherwise.
             */
            static inline bool isRXFFull()
            {
                return ((*(uint8_t *)(USB_FLAGS)&USBFlags::RXF) == USBFlags::RXF);
            }

            /**
             * @brief Waits until the Receive FIFO Full (RXF) flag is no longer full.
             *
             * This function continuously checks the RXF flag until it is cleared, indicating that
             * the receive FIFO is no longer full and data can be read.
             *
             * Note: This function has no timeout and will loop indefinitely if the RXF flag is never cleared.
             */
            static inline void waitRXF()
            {
                // Bad design, no timeout !
                while (isRXFFull())
                    ;
            }

            /**
             * @brief Writes a buffer to the USB FIFO.
             *
             * This function waits for the TXE flag to be cleared (FIFO not full) and then writes the provided
             * buffer to the USB_FIFO register.
             *
             * @param c Pointer to the buffer to be written.
             * @param size Number of bytes to write.
             */
            static inline void write(const uint8_t *c, size_t size = 1)
            {
                for (size_t i = 0; i < size; ++i)
                {
                    while (isTXEFull())
                        ;                                   // Simplified and corrected waitTXE
                    *(volatile uint8_t *)(USB_FIFO) = c[i]; // Write the byte to the FIFO
                }
            }

            /**
             * @brief Reads a byte from the USB FIFO.
             *
             * This function waits for the RXF flag to be set (FIFO full) and then reads a byte from
             * the USB_FIFO register.
             *
             * @return uint8_t The byte read from the FIFO.
             */
            static inline uint8_t read()
            {
                waitRXF();                     // Wait for the receive FIFO to be full
                return *(uint8_t *)(USB_FIFO); // Read and return the byte from the FIFO
            }

            /**
             * @brief Checks if the USB device is available.
             *
             * This function reads the USB_FLAGS register and checks if certain bits are clear,
             * indicating that the USB device is available and ready for communication.
             *
             * @return true If the USB device is available, false otherwise.
             */
            static inline bool isAvailable()
            {
                return (!(*(uint8_t *)(USB_FLAGS) & 0x7C));
            }

        }

        // CS1 area (CPLD registers)
        namespace CS1
        {
            constexpr static uint32_t CPLD_BASE_ADDR = 0x24000000L; // Base address for CPLD registers

            /**
             * @brief Enumeration of CPLD registers.
             *
             * This enum class defines the addresses of various registers within the CPLD (Complex Programmable Logic Device).
             * These registers control various functionalities of the device, such as LED settings, SD card interface, and more.
             */
            enum class Register : uint32_t
            {
                CPLD_55 = CPLD_BASE_ADDR + 0x01,        // Register CPLD_55
                CPLD_AA = CPLD_BASE_ADDR + 0x03,        // Register CPLD_AA
                CART_CPLD_VER = CPLD_BASE_ADDR + 0x05,  // Register CART_CPLD_VER
                CART_BETA_ID = CPLD_BASE_ADDR + 0x07,   // Register CART_BETA_ID
                CPLD_IO = CPLD_BASE_ADDR + 0x09,        // Register CPLD_IO
                SDIN_BITS = CPLD_BASE_ADDR + 0x0B,      // Register SDIN_BITS
                LED_SETTING = CPLD_BASE_ADDR + 0x0D,    // Register LED_SETTING
                SD_CLK_SET = CPLD_BASE_ADDR + 0x0F,     // Register SD_CLK_SET
                REG_STDOUT_BIT = CPLD_BASE_ADDR + 0x11, // Register REG_STDOUT_BIT
                REG_SD_IO_0 = CPLD_BASE_ADDR + 0x11,    // Register REG_SD_IO_0
                REG_SD_IO_1 = CPLD_BASE_ADDR + 0x13,    // Register REG_SD_IO_1
                REG_SD_IO_2 = CPLD_BASE_ADDR + 0x15,    // Register REG_SD_IO_2
                REG_SD_IO_3 = CPLD_BASE_ADDR + 0x17,    // Register REG_SD_IO_3
                REG_SD_REINSERT = CPLD_BASE_ADDR + 0x19 // Register REG_SD_REINSERT
            };

            /**
             * @brief Enumeration of SD card LED and switch shift values.
             *
             * This enum class defines the bit shift values for accessing specific components related to the SD card,
             * such as LEDs and switches.
             */
            enum class SD_LSHFT : uint8_t
            {
                SD_LEDG_LSHFT = 0, // SD card green LED shift value
                SD_LEDR_LSHFT = 1, // SD card red LED shift value
                SD_SW1_LSHFT = 4,  // SD card switch 1 shift value
                SD_EJECT_LSHFT = 7 // SD card eject shift value
            };

            /**
             * @brief Enumeration of SD card control signal shift values.
             *
             * This enum class defines the bit shift values for controlling the SD card interface,
             * such as chip select (CS), data input (DIN), and clock (CLK) signals.
             */
            enum class SD_CTRL_LSHFT : uint8_t
            {
                SD_CSL_LSHFT = 0, // SD card chip select (CS) shift value
                SD_DIN_LSHFT = 1, // SD card data input (DIN) shift value
                SD_CLK_LSHFT = 2  // SD card clock (CLK) shift value
            };

        }
    } // namespace DevCart
} // namespace SRL

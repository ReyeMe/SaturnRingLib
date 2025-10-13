// Based on SatCom Library by cafe-alpha, Original: http://ppcenter.free.fr/satcom/

#pragma once

#include <cstdint>  // For uintptr_t, size_t, uint8_t, uint32_t
#include <initializer_list>  // For std::initializer_list in USBFlags constructor

namespace SRL
{
    namespace DevCart
    {

        /** @brief CS0 area: Flash memory and USB-related registers.
         * 
         *  This namespace groups constants and functions for accessing the cartridge's CS0 memory space,
         *  which includes flash memory and USB communication registers (likely for a Sega Saturn USB dev cart).
         *  Addresses are memory-mapped I/O; accesses should use volatile pointers to prevent optimization issues.
         */
        namespace CS0
        {
            /** @brief Base address of the cartridge in CS0 area.
             * 
             *  This is the starting point for flash and USB registers (overlaps with DataCart in srl_cartridge.hpp).
             */
            constexpr static uintptr_t CART_BASE_ADR = 0x22000000UL;            // Base address of the cartridge in CS0 area

            /** @brief Base address of the flash memory (1MB region).
             */
            constexpr static uintptr_t FLASH_MEMORY_BASE = CART_BASE_ADR + 0x0; // Base address of the flash memory (1MB)

            /** @brief Address of the USB flags register (8-bit Read/Write).
             * 
             *  This register holds status flags for USB FIFO operations (RXF, TXE, PWREN).
             */
            constexpr static uintptr_t USB_FLAGS = CART_BASE_ADR + 0x200001UL;  // Address of the USB flags register (Read/Write)

            /** @brief Address of the USB FIFO data register (8-bit Read/Write).
             * 
             *  Used for sending/receiving bytes over USB.
             */
            constexpr static uintptr_t USB_FIFO = CART_BASE_ADR + 0x100001;     // Address of the USB FIFO data register (Read/Write)
            // 0x223x to 0x227x unused  // Reserved/unused address range in hardware

            /** @brief Maximum length allowed for firmware uploads (matches flash size).
             */
            constexpr static size_t FIRM_MAXLEN = 1024 * 1024; // Maximum length allowed for firmware (1MB)

            /**
             * @brief Class representing the USB flags register bits.
             *
             * This class provides a type-safe way to manipulate the bits in the USB flags register
             * (RXF, TXE, PWREN). It supports bitwise operations and flag checking.
             * 
             * Note: Only bits 0,1,7 are defined; others are ignored/reserved.
             */
            class USBFlags
            {
            public:
                // Bit position constants (accessible as USBFlags::TXE, etc.)
                enum : uint8_t
                {
                    RXF = 1 << 0,  // RXF: Receive FIFO Full (data available to read)
                    TXE = 1 << 1,  // TXE: Transmit FIFO Empty (ready to accept data)
                    PWREN = 1 << 7 // PWREN: Power Enable (USB power control)
                };

                /** @brief Mask for all defined flags (bits 0,1,7). */
                static constexpr uint8_t ALL_FLAGS = (RXF | TXE | PWREN);                  // Mask for all defined flags

                /** @brief Inverted mask for all defined flags (for clearing/checking undefined bits). */
                static constexpr uint8_t NOT_ALL_FLAGS = static_cast<uint8_t>(~ALL_FLAGS); // Mask for not all defined flags

            private:
                uint8_t bits_; // Raw bit storage (8-bit value read/written to hardware)

            public:
                /** @brief Default constructor: Initializes with no flags set. */
                USBFlags() : bits_(0) {}                                  

                /** @brief Constructor: Initialize with raw bit value. */
                explicit USBFlags(uint8_t bits) : bits_(bits) {}          

                /** @brief Constructor: Initialize by OR-ing a list of flag constants.
                 *  @param flags Initializer list of flag enums (e.g., {USBFlags::RXF, USBFlags::TXE}).
                 */
                USBFlags(std::initializer_list<uint8_t> flags) : bits_(0) 
                {
                    for (auto f : flags)
                        bits_ |= f; // Set each provided flag
                }

                /** @brief Conversion to bool: True if any flag is set. */
                explicit operator bool() const { return bits_ != 0; } 

                /** @brief Bitwise OR: Combine with another USBFlags. */
                USBFlags operator|(USBFlags other) const { return USBFlags(bits_ | other.bits_); } 

                /** @brief Bitwise OR assignment: Add flags from another. */
                USBFlags &operator|=(USBFlags other)
                {
                    bits_ |= other.bits_; 
                    return *this;
                }

                /** @brief Bitwise AND: Keep only common flags. */
                USBFlags operator&(USBFlags other) const { return USBFlags(bits_ & other.bits_); } 

                /** @brief Bitwise AND assignment: Retain common flags. */
                USBFlags &operator&=(USBFlags other)
                {
                    bits_ &= other.bits_; 
                    return *this;
                }

                /** @brief Bitwise NOT: Invert all bits (careful: affects undefined bits too). */
                USBFlags operator~() const { return USBFlags(static_cast<uint8_t>(~bits_)); } 

                /** @brief Check if a specific flag is set.
                 *  @param flag The flag constant to test (e.g., USBFlags::TXE).
                 *  @return True if set.
                 */
                bool has(uint8_t flag) const { return (bits_ & flag) != 0; } 

                /** @brief Get the raw bit value (for writing to hardware). */
                uint8_t bits() const { return bits_; } 
            };

            /**
             * @brief Checks if the Transmit FIFO Empty (TXE) flag is set.
             *
             * Reads the USB_FLAGS register and tests the TXE bit (set means FIFO is empty/ready? 
             * Note: Doc says "full" but TXE typically means empty in USB contexts—verify hardware spec).
             *
             * @return true If TXE is set (FIFO empty?), false otherwise.
             */
            static inline bool isTXEFull()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::TXE) != 0;  // Added volatile for MMIO safety
            }

            /**
             * @brief Waits until the Transmit FIFO is ready (TXE cleared?).
             *
             * Polls isTXEFull() until false. 
             * 
             * Warning: Infinite loop if hardware never clears—consider adding timeout in production code.
             * Note: Naming "waitTXE" but checks "isTXEFull"—clarify if TXE means empty or full per hardware.
             */
            static inline void waitTXE()
            {
                // Bad design, no timeout! TODO: Add optional timeout parameter or counter
                while (isTXEFull())
                    ;  // Busy-wait
            }

            /**
             * @brief Checks if the Receive FIFO (RXF) is empty.
             *
             * @return true If RXF is cleared (FIFO empty), false otherwise.
             */
            static inline bool isRXFEmpty()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::RXF) == 0;  // Added volatile
            }

            /**
             * @brief Waits until data is available in Receive FIFO (RXF set).
             * 
             * Warning: Infinite loop possible—add timeout if needed.
             */
            static inline void waitRXF()
            {
                // Bad design, no timeout !
                while (isRXFEmpty())
                    ;  // Busy-wait
            }

            /**
             * @brief Writes a single byte to the USB FIFO.
             *
             * Waits for TXE condition then writes.
             * 
             * @param c Pointer to the byte to write.
             * @return size_t 1 on success.
             */
            static inline size_t write(const uint8_t *c)
            {
                size_t counter = 0;

                waitTXE();                   
                *(volatile uint8_t *)(USB_FIFO) = *c; // Volatile for MMIO
                ++counter;
                return counter;
            }

            /**
             * @brief Writes a buffer to the USB FIFO.
             *
             * Writes byte-by-byte, waiting for TXE each time. Inefficient for large buffers due to per-byte wait.
             * 
             * @param c Pointer to the buffer.
             * @param size Number of bytes to write.
             * @return size_t Number of bytes written.
             * 
             * Note: Overload ambiguity—single byte vs buffer; const_cast unnecessary if using const uint8_t*.
             * Bug: Increments pointer incorrectly in loop (passes original c each time? No: casted++ but passes casted to write which takes const).
             * Fix: Pass &casted[i] or use loop variable properly.
             */
            static inline size_t write(const uint8_t *c, size_t size)
            {
                size_t counter = 0;
                for (size_t i = 0; i < size; i++)
                {
                    counter += write(c + i);
                }
                return counter;
            }

            /**
             * @brief Reads a single byte from the USB FIFO.
             *
             * Waits for RXF then reads.
             * 
             * @return uint8_t The byte read.
             */
            static inline uint8_t read()
            {
                waitRXF();                     
                return *(volatile uint8_t *)(USB_FIFO); // Volatile for MMIO
            }

            /**
             * @brief Checks if the USB device is connected and ready.
             *
             * Tests if undefined bits in FLAGS are clear (using NOT_ALL_FLAGS mask).
             * Assumes connection when reserved bits == 0.
             * 
             * @return true If connected, false otherwise.
             */
            static inline bool isConnected()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::NOT_ALL_FLAGS) == 0;  // Volatile
            }

        }  // namespace CS0

        /** @brief CS1 area: CPLD registers.
         * 
         *  Groups constants for accessing CPLD (Complex Programmable Logic Device) registers,
         *  which control LEDs, SD card interface, I/O, etc.
         */
        namespace CS1
        {
            /** @brief Base address for CPLD registers in CS1 space. */
            constexpr static uint32_t CPLD_BASE_ADDR = 0x24000000L; // Base address for CPLD registers (note: L suffix for long)

            /**
             * @brief Enumeration of CPLD register addresses.
             *
             * These are offset from CPLD_BASE_ADDR. Values like 0x55/0xAA may be for handshake/init sequences.
             * All are 8-bit or 16-bit accesses—verify per hardware.
             */
            enum class Register : uint32_t
            {
                CPLD_55 = CPLD_BASE_ADDR + 0x01,        // Register CPLD_55 (possibly init/write 0x55)
                CPLD_AA = CPLD_BASE_ADDR + 0x03,        // Register CPLD_AA (possibly init/write 0xAA)
                CART_CPLD_VER = CPLD_BASE_ADDR + 0x05,  // Register: CPLD version (read-only?)
                CART_BETA_ID = CPLD_BASE_ADDR + 0x07,   // Register: Beta/ID identifier
                CPLD_IO = CPLD_BASE_ADDR + 0x09,        // Register: General I/O control
                SDIN_BITS = CPLD_BASE_ADDR + 0x0B,      // Register: SD input bits
                LED_SETTING = CPLD_BASE_ADDR + 0x0D,    // Register: LED settings (bitfield for colors/modes)
                SD_CLK_SET = CPLD_BASE_ADDR + 0x0F,     // Register: SD clock configuration
                REG_STDOUT_BIT = CPLD_BASE_ADDR + 0x11, // Register: Stdout bit (debug/output?)
                REG_SD_IO_0 = CPLD_BASE_ADDR + 0x11,    // Register: SD I/O port 0 (shared address with above?)
                REG_SD_IO_1 = CPLD_BASE_ADDR + 0x13,    // Register: SD I/O port 1
                REG_SD_IO_2 = CPLD_BASE_ADDR + 0x15,    // Register: SD I/O port 2
                REG_SD_IO_3 = CPLD_BASE_ADDR + 0x17,    // Register: SD I/O port 3
                REG_SD_REINSERT = CPLD_BASE_ADDR + 0x19 // Register: SD reinsert/eject command
            };

            /** Note: REG_STDOUT_BIT and REG_SD_IO_0 share the same address—likely bit aliases or mode-dependent. Document usage to avoid conflicts. */

            /**
             * @brief Bit shifts for SD card LED and switch controls in registers (e.g., LED_SETTING).
             */
            enum class SD_LSHFT : uint8_t
            {
                SD_LEDG_LSHFT = 0, // Green LED bit position
                SD_LEDR_LSHFT = 1, // Red LED bit position (typo fix: SD_LEDR_LSHFT?)
                SD_SW1_LSHFT = 4,  // Switch 1 bit
                SD_EJECT_LSHFT = 7 // Eject detect bit
            };

            /**
             * @brief Bit shifts for SD card control signals (CS, DIN, CLK) in control registers.
             */
            enum class SD_CTRL_LSHFT : uint8_t
            {
                SD_CSL_LSHFT = 0, // Chip Select low-active?
                SD_DIN_LSHFT = 1, // Data In
                SD_CLK_LSHFT = 2  // Clock signal
            };

            // TODO: Add access functions (read/write) for these registers, similar to CS0 USB funcs.
            // Example: uint8_t readRegister(Register reg) { return *(volatile uint8_t*)reg; }

        }  // namespace CS1
    } // namespace DevCart
} // namespace SRL
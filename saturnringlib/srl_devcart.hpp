// Based on SatCom Library by cafe-alpha, Original:
// http://ppcenter.free.fr/satcom/

#pragma once
#include <cstddef> // For size_t
#include <cstdint> // For uintptr_t, size_t, uint8_t, uint32_t
#include <initializer_list>

/** @brief Namespace for interacting with a USB development cartridge for the Sega Saturn
 * This provides access to registers for USB communication
 */
namespace SRL
{
    /** @brief Namespace for Sega Saturn USB development cartridge hardware access
     */
    namespace DevCart
    {
        /** @brief CS0 area: Flash memory and USB-related registers
         * This namespace groups constants and functions for accessing the cartridge's
         * CS0 memory space, which includes flash memory and USB communication registers
         * (likely for a Sega Saturn USB dev cart). Addresses are memory-mapped I/O;
         * accesses should use volatile pointers to prevent optimization issues.
         */
        namespace CS0
        {
            /** @brief Base address of the cartridge in CS0 area
             * This is the starting point for flash and USB registers (overlaps with
             * DataCart in srl_cartridge.hpp).
             */
            constexpr static uintptr_t CartBaseAdr = 0x22000000UL;

            /** @brief Address of the Wasca Prepare counter register
             */
            constexpr static uintptr_t CartPcntr = CartBaseAdr + 0x1FFFFF0UL;

            /** @brief Address of the Wasca Status register
             */
            constexpr static uintptr_t CartStatus = CartBaseAdr + 0x1FFFFF2UL;

            /** @brief Address of the Wasca hardware version register major/minor (e.g. 0x050C = v5.12)
             */
            constexpr static uintptr_t CartHwver = CartBaseAdr + 0x1FFFFF6UL;

            /** @brief Address of the Signature register ("wasca " in ASCII: 0x7761 0x7363 0x6120)
             */
            constexpr static uintptr_t CartSignature = CartBaseAdr + 0x1FFFFFAUL;

            /** @brief Base address of the flash memory (1MB region)
             */
            constexpr static uintptr_t FlashMemoryBase = CartBaseAdr + 0x0;

            /** @brief Address of the USB flags register (8-bit Read/Write)
             * This register holds status flags for USB FIFO operations (Rxf, Txe, Pwren).
             */
            constexpr static uintptr_t USBFlagsAdr = CartBaseAdr + 0x200001UL;

            /** @brief Address of the USB FIFO data register (8-bit Read/Write)
             * Used for sending/receiving bytes over USB.
             */
            constexpr static uintptr_t UsbFifo = CartBaseAdr + 0x100001;
            // 0x223x to 0x227x unused  // Reserved/unused address range in hardware

            /** @brief Registers for controlling the SD card on the development cartridge
             * These registers are mapped in the CS0 memory space.
             */
            namespace SDCardRegisters
            {
                /** @brief Address of the Card Identification Number Register
                 */
                constexpr static uintptr_t CartCid = CartBaseAdr + 0x1FF0200UL;

                /** @brief Address of the Card Specific Data Register
                 */
                constexpr static uintptr_t CartCsd = CartBaseAdr + 0x1FF0210UL;

                /** @brief Address of the Operation Condition Register
                 */
                constexpr static uintptr_t CartOcr = CartBaseAdr + 0x1FF0220UL;

                /** @brief Address of the SD Card Status Register
                 */
                constexpr static uintptr_t CartSr = CartBaseAdr + 0x1FF0224UL;

                /** @brief Address of the Relative Card Address Register
                 */
                constexpr static uintptr_t CartRc = CartBaseAdr + 0x1FF0228UL;

                /** @brief Address of the Command Argument Register
                 */
                constexpr static uintptr_t CartCmdArg = CartBaseAdr + 0x1FF022CUL;

                /** @brief Address of the Command Register
                 */
                constexpr static uintptr_t CartCmd = CartBaseAdr + 0x1FF0230UL;

                /** @brief Address of the Auxiliary Status Register
                 */
                constexpr static uintptr_t CartAsr = CartBaseAdr + 0x1FF0234UL;

                /** @brief Address of the Response R1 register
                 */
                constexpr static uintptr_t CartRR1 = CartBaseAdr + 0x1FF0238UL;

                /** @brief Address of the wasca Specific SD Control Register
                 */
                constexpr static uintptr_t CartWsscr = CartBaseAdr + 0x1FF0FFEUL;

            } // namespace SDCardRegisters

            /** @brief Maximum length allowed for firmware uploads (matches flash size)
             */
            constexpr static size_t FirmMaxlen = 1024 * 1024;

            /** @brief Struct containing the USB flags register bits and masks
             */
            struct USBFlags
            {
                /** @brief Bit positions for the USB flags register
                 */
                enum : uint8_t
                {
                    /** @brief Rxf: Receive FIFO Full (data available to read)
                     */
                    Rxf = 1 << 0,

                    /** @brief Txe: Transmit FIFO Empty (ready to accept data)
                     */
                    Txe = 1 << 1,

                    /** @brief Pwren: Power Enable (USB power control)
                     */
                    Pwren = 1 << 7
                };

                /** @brief Mask for all defined flags (bits 0,1,7) */
                static constexpr uint8_t AllFlags = (Rxf | Txe | Pwren);

                /** @brief Inverted mask for all defined flags (for clearing/checking undefined bits) */
                static constexpr uint8_t NotAllFlags = static_cast<uint8_t>(~AllFlags);
            };

            /** @brief Checks if the Transmit FIFO Empty (Txe) flag is set
             * Reads the UsbFlags register and tests the Txe bit. When the Txe bit is set,
             * the transmit FIFO is full and cannot accept new data. The function name
             * `IsTxeFull` is accurate in this context, though `Txe` often means "Transmit
             * Empty" in other hardware.
             * @return true If Txe is set (FIFO is full), false otherwise
             */
            static inline bool IsTxeFull()
            {
                return ((*(volatile uint8_t *)(USBFlagsAdr)) & USBFlags::Txe) != 0;
            }

            /** @brief Reads the raw UsbFlags register value
             */
            static inline uint8_t ReadFlags() { return *(volatile uint8_t *)(USBFlagsAdr); }

            /** @brief Waits until the Transmit FIFO is ready, with timeout
             * Polls `IsTxeFull()` until it returns false. If `maxPolls` is non-zero and
             * reached first, the function returns false to signal timeout.
             * @param maxPolls Maximum number of polling iterations while FIFO is full (0 for infinite wait)
             * @return true if FIFO became ready before timeout, false otherwise
             */
            static inline bool WaitTxe(uint32_t maxPolls = 0)
            {
                while (IsTxeFull())
                {
                    if (maxPolls > 0)
                    {
                        --maxPolls;
                        if (maxPolls == 0)
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            /** @brief Checks if the Receive FIFO (Rxf) is empty
             * Reads the UsbFlags register and checks the Rxf bit.
             * The FIFO is considered empty while Rxf is set.
             * @return true If Rxf is set (FIFO is empty), false otherwise
             */
            static inline bool IsRxfEmpty()
            {
                return ((*(volatile uint8_t *)(USBFlagsAdr)) & USBFlags::Rxf) != 0;
            }

            /** @brief Waits until data is available in the receive FIFO, with timeout
             * Polls `IsRxfEmpty()` until it returns false. If `maxPolls` is non-zero and
             * reached first, the function returns false to signal timeout.
             * @param maxPolls Maximum number of polling iterations while FIFO is empty (0 for infinite wait)
             * @return true if data became available before timeout, false otherwise
             */
            static inline bool WaitRxf(uint32_t maxPolls = 0)
            {
                while (IsRxfEmpty())
                {
                    if (maxPolls > 0)
                    {
                        --maxPolls;
                        if (maxPolls == 0)
                        {
                            return false;
                        }
                    }
                }
                return true;
            }

            /** @brief Writes a single byte to the USB FIFO
             * This function waits until the transmit FIFO is not full (`WaitTxe()`) and
             * then writes a single byte to the FIFO.
             * @param c Pointer to the byte to write
             * @return size_t 1 on success
             */
            static inline size_t Write(const uint8_t *c)
            {
                size_t counter = 0;

                WaitTxe();
                *(volatile uint8_t *)(UsbFifo) = *c; // Volatile for MMIO
                ++counter;
                return counter;
            }

            /** @brief Writes a sequence of bytes to the USB FIFO
             * This function iterates through the buffer, calling `Write()` for each byte.
             * @param c Pointer to the buffer
             * @param size Number of bytes to write
             * @return size_t Number of bytes written
             */
            static inline size_t Write(const uint8_t *c, size_t size)
            {
                size_t counter = 0;
                for (size_t i = 0; i < size; i++)
                {
                    counter += Write(c + i);
                }
                return counter;
            }

            /** @brief Reads a single byte from the USB FIFO
             * This function waits until data is available in the receive FIFO (`WaitRxf()`)
             * and then reads a single byte.
             * @return uint8_t The byte read
             */
            static inline uint8_t Read()
            {
                WaitRxf();
                return *(volatile uint8_t *)(UsbFifo); // Volatile for MMIO
            }

            /** @brief Checks if the USB device is connected and ready
             * This function checks the `UsbFlags` register. It assumes the device is
             * connected if the reserved bits (those not in `AllFlags`) are all zero. This
             * is a common way to detect hardware presence on embedded systems.
             * @param Flags A UsbFlags value already read (one slow bus access); read now if omitted
             * @return true If connected, false otherwise
             */
            static inline bool IsConnected(const uint8_t Flags = ReadFlags())
            {
                // SatCom-compatible test: bits 7..2 must be low when FTDI is USB powered.
                return (Flags & 0xFCU) == 0;
            }

            /** @brief Returns true when USB dev cart flag register pattern looks valid
             * @param Flags A UsbFlags value already read (one slow bus access); read now if omitted
             */
            static inline bool IsPortAvailable(const uint8_t Flags = ReadFlags())
            {
                // SatCom-compatible availability test: reserved bits 6..2 should stay low.
                return (Flags & 0x7CU) == 0;
            }

        } // namespace CS0

        /** @brief CS1 area: CPLD registers
         * This namespace groups constants for accessing the CPLD (Complex Programmable
         * Logic Device) registers, which are used to control features like LEDs, the SD
         * card interface, and general-purpose I/O.
         */
        namespace CS1
        {
            /** @brief Base address for CPLD registers in CS1 space (note: L suffix for long) */
            constexpr static uint32_t CpldBaseAddr = 0x24000000L;

            /** @brief Enumeration of CPLD register addresses
             * These are offsets from `CpldBaseAddr`. The values `0x55` and `0xAA` are
             * likely part of a handshake or initialization sequence. Access to these
             * registers is typically 8-bit or 16-bit; refer to the hardware documentation
             * for specifics.
             */
            enum class Register : uint32_t
            {
                /** @brief Register Cpld55 (possibly handshake init/write 0x55)
                 */
                Cpld55 = CpldBaseAddr + 0x01,

                /** @brief Register CpldAa (possibly handshake init/write 0xAA)
                 */
                CpldAa = CpldBaseAddr + 0x03,

                /** @brief Register: CPLD version (read-only)
                 */
                CartCpldVer = CpldBaseAddr + 0x05,

                /** @brief Register: Beta/ID identifier
                 */
                CartBetaId = CpldBaseAddr + 0x07,

                /** @brief Register: General I/O control
                 */
                CpldIo = CpldBaseAddr + 0x09,

                /** @brief Register: SD input bits
                 */
                SdinBits = CpldBaseAddr + 0x0B,

                /** @brief Register: LED settings (bitfield for colors/modes)
                 */
                LedSetting = CpldBaseAddr + 0x0D,

                /** @brief Register: SD clock configuration
                 */
                SdClkSet = CpldBaseAddr + 0x0F,

                /** @brief Register: Stdout bit (debug/output)
                 */
                RegStdoutBit = CpldBaseAddr + 0x11,

                /** @brief Register: SD I/O port 0 (shared address with stdout bit)
                 */
                RegSdIo0 = CpldBaseAddr + 0x11,

                /** @brief Register: SD I/O port 1
                 */
                RegSdIo1 = CpldBaseAddr + 0x13,

                /** @brief Register: SD I/O port 2
                 */
                RegSdIo2 = CpldBaseAddr + 0x15,

                /** @brief Register: SD I/O port 3
                 */
                RegSdIo3 = CpldBaseAddr + 0x17,

                /** @brief Register: SD reinsert/eject command
                 */
                RegSdReinsert = CpldBaseAddr + 0x19,

                /** @brief Register: SD write-protect / SD present status
                 */
                RegSdWriteProtect = CpldBaseAddr + 0x1B
            };

            /** @brief Reads an 8-bit CS1 register value from the DevCart CPLD space
             */
            static inline uint8_t ReadRegister(const Register Reg)
            {
                return *(volatile uint8_t *)(static_cast<uint32_t>(Reg));
            }

            /** @brief Returns true when the expected CPLD identification bytes are present
             */
            static inline bool HasWascaSignature()
            {
                return ReadRegister(Register::Cpld55) == 0x55 &&
                       ReadRegister(Register::CpldAa) == 0xAA;
            }

            /** @brief Returns true when cartridge reports USB Gamer's CPLD version
             */
            static inline bool IsUsbGamersCartridge()
            {
                return ReadRegister(Register::CartCpldVer) == 0x19;
            }
        } // namespace CS1
    } // namespace DevCart
} // namespace SRL
#pragma once

#include <srl_base.hpp>
#include <srl_debug.hpp>

namespace SRL
{
    /** @brief Netlink/XBAND driver
     */
    class Netlink
    {
    private:

        /** @brief How much to wait before trying to access modem
         */
        inline static constexpr uint32_t TickDelay = 200;

        /** @brief How many time to try to access the modem
         */
        inline static constexpr uint32_t CheckRetry = 5;

        /** @brief Count before UART times out
         */
        inline static constexpr uint32_t UartTimeout = 10000;

        /** @brief Modem registers
         */
        struct Registers
        {
        private:

            /** @brief Register addresses
             */
            inline static const uint32_t registerTable[] =
            {
                0x25895001, // DLAB(0) => RBR, THR; DLAB(1) => DLL
                0x25895005, // DLAB(0) => IER; DLAB(1) => DLH
                0x25895009, // IIR, FCR
                0x2589500D, // LCR
                0x25895011, // MCR
                0x25895015, // LSR
                0x25895019, // MSR
                0x2589501D, // Scratch
                0x2582503D  // Register access latch
            };
            
            /** @brief Confirm read from or write to a register
             */
            static void ConfirmReadWrite()
            {
                *(volatile uint8_t*)(registerTable[8]) = 0x00;
            }

            /** @brief Deleted constructor
             */
            Registers() = delete;

        public:

            /** @brief Modem registers
             */
            enum class Register : uint8_t
            {
                /** @brief Receive buffer register
                 * @note Read-only, Available when DLAB = 0
                 */
                ReceiveBuffer = 0,

                /** @brief Transmit buffer register
                 * @note Write-only, Available when DLAB = 0
                 */
                TransmitHolding = 0,

                /** @brief Least significant bit of a divisor latch
                 * @note Read/Write, Available when DLAB = 1
                 */
                DivisorLatchLeast = 0,
                
                /** @brief Most significant bit of a divisor latch
                 * @note Read/Write, Available when DLAB = 1
                 */
                DivisorLatchMost = 1,

                /** @brief Interrupt enable register
                 * @note Read/Write, Available when DLAB = 0
                 */
                InterruptEnable = 1,

                /** @brief Interrupt identification register
                 * @note Read-only, Available when DLAB = 0 or 1
                 */
                InterruptIdentification = 2,

                /** @brief FIFO control register
                 * @note Write-only, Available when DLAB = 0 or 1
                 */
                FifoControl = 2,

                /** @brief Line control register
                 * @note Read/Write
                 */
                LineControl = 3,

                /** @brief Modem control register
                 * @note Read/Write
                 */
                ModemControl = 4,

                /** @brief Line status register
                 * @note Read-only, Available when DLAB = 0
                 */
                LineStatus = 5,

                /** @brief Modem status register
                 * @note Read-only, Available when DLAB = 0
                 */
                ModemStatus = 6,

                /** @brief Scratchpad register
                 */
                Scratch = 7
            };

            /** @brief Raised modem interrupt type
             */
            enum InterruptType : uint8_t
            {
                InterruptPending = 0x01,

                TransmitHoldEmpty = 0x02,

                ReceiveDataAvailable = 0x05,

                ReceiveLineStatus= 0x07,

                TimeOutInterruptPending = 0x0D
            };

            /** @brief Line status values
             */
            enum LineStatus : uint8_t
            {
                DataReady = 0x01,

                OverrunError = 0x02,

                ParityError = 0x04,

                FramingError = 0x08,

                BreakInterrupt = 0x10,

                EmptyTransmitterHoldingRegister = 0x20,

                EmptyDataHoldingRegister = 0x40,

                ErrorInReceivedFIFO = 0x80
            };

            /** @brief Read register value
             * @param number Register number
             * @return Value stored in the register
             */
            static uint8_t PeekRegister(Register number)
            {
                return *(volatile uint8_t*)(Registers::registerTable[(uint8_t)number]);
            }

            /** @brief Read register value
             * @param number Register number
             * @return Value stored in the register
             */
            static uint8_t ReadRegister(Register number)
            {
                uint8_t result = Registers::PeekRegister(number);
                Registers::ConfirmReadWrite();
                return result;
            }

            /** @brief Sets divisor latch access bit
             * @note DLAB is located at bit 7 of LCR
             * @param value bit value to be set
             */
            static void SetDivisorLatch(bool value)
            {
                // Get current state
                uint8_t currentState = Registers::ReadRegister(Register::LineControl);
                
                // Set new state
                Registers::SetRegister(Register::LineControl, (currentState & ~((uint8_t)1 << 7)) | ((uint8_t)value << 7));
            }

            /** @brief Sets RST bit
             * @param value bit value to be set
             */
            static void SetRSTLatch(bool value)
            {
                // Get current state
                uint8_t currentState = Registers::ReadRegister(Register::ModemControl);
                
                // Set new state
                Registers::SetRegister(Register::ModemControl, (currentState & ~((uint8_t)1 << 1)) | ((uint8_t)value << 1));
            }

            /** @brief Sets DST bit
             * @param value bit value to be set
             */
            static void SetDSTLatch(bool value)
            {
                // Get current state
                uint8_t currentState = Registers::ReadRegister(Register::ModemControl);
                
                // Set new state
                Registers::SetRegister(Register::ModemControl, (currentState & ~((uint8_t)1 << 0)) | ((uint8_t)value << 0));
            }

            /** @brief Set the Register value
             * @param number Register number
             * @param value Value to store
             */
            static void SetRegister(Register number, uint8_t value)
            {
                *(volatile uint8_t*)(Registers::registerTable[(uint8_t)number]) = value;
                Registers::ConfirmReadWrite();
            }
        };

        /** @brief Modem hardware control
         */
        struct ModemHwControl
        {
        private:
            
            /** @brief Modem SMPC commands
             */
            enum class SmpcCommands : uint8_t
            {
                /** @brief Turn on modem
                 */
                TurnOn = 0x0A,

                /** @brief Turn off modem
                 */
                TurnOff = 0x0B
            };

            /** @brief Deleted constructor
             */
            ModemHwControl() = delete;

        public:

            /** @brief Set the Interrupt handler
             * @param function Interrupt handler function reference
             */
            static void SetInterruptHandler(void (*function)())
            {
                volatile uint32_t * const setScuFunc = (uint32_t *)0x06000300;
                volatile uint32_t * const setMask = (uint32_t *)0x06000340;
                volatile uint32_t * const getMask = (uint32_t *)0x06000348;

                //Registers::SetRegister(Registers::Register::InterruptEnable, 0x0f);
                ((void (*)(uint32_t))*setMask)(*getMask | ((uint32_t)0x00100000));
                ((void (*)(uint32_t, void (*)(void)))*setScuFunc)(0x5C, function);
            }

            /** @brief Turn modem on
             */
            static void ModemOn()
            {
                PER_SMPC_NO_IREG(SmpcCommands::TurnOn);
            }

            /** @brief Turn modem off
             */
            static void ModemOff()
            {
                PER_SMPC_NO_IREG(SmpcCommands::TurnOff);
            }
        };

        /** @brief Indicates whether data is currently being read
         */
        inline static bool IsReading = false;

        /** @brief Has received some data
         */
        inline static bool HasData = false;

        /** @brief Can write
         */
        inline static bool CanWrite = false;

        #pragma interrupt(InterruptHandler)
        static void InterruptHandler()
        {
            /*switch ((Registers::InterruptType)Registers::ReadRegister(Registers::Register::InterruptIdentification))
            {
            case Registers::InterruptType::ReceiveDataAvailable:
                Netlink::HasData = true;

                if (!Netlink::IsReading)
                {
                    Netlink::OnReceive.Invoke();
                }
                break;
            
            case Registers::InterruptType::TransmitHoldEmpty

            default:
                break;
            }*/
        }
        
    public:

        /** @brief Deleted constructor
         */
        Netlink() = delete;

        /** @brief Data received
         */
        inline static SRL::Types::Event<> OnReceive;

        /** @brief Initialize modem
         */
        static bool Initialize()
        {
            bool result = false;

            // Ensure that modem is off
            ModemHwControl::ModemOff();

            // Turn modem on
            ModemHwControl::ModemOn();

            // Test if modem is alive
            for (uint32_t retry = Netlink::CheckRetry; retry > 0; retry--)
            {
                // Wait a bit before every test
                for (int nothing = Netlink::TickDelay; nothing > 0; nothing--)
                {
                    nop();
                }

                Registers::SetRegister(Registers::Register::Scratch, 0xA5);
                
                // Check if modem is alive
                if (Registers::ReadRegister(Registers::Register::Scratch) == 0xA5)
                {
                    Registers::SetDivisorLatch(false);
                    ModemHwControl::SetInterruptHandler(Netlink::InterruptHandler);
                    return true;
                }
            }

            return false;
        }

        /** @brief Write buffer to the modem
         * @param source Source buffer
         * @param length Buffer length
         * @return false on time-out
         */
        static bool Write(uint8_t* source, size_t length)
        {
            Registers::SetRSTLatch(true);

            for (size_t byte = 0; byte < length; byte++)
            {
                uint32_t timeout = Netlink::UartTimeout;

                // Wait for data to send
                while (!((Registers::LineStatus)Registers::ReadRegister(Registers::Register::LineStatus) & Registers::LineStatus::EmptyTransmitterHoldingRegister))
                {
                    if (timeout-- <= 0)
                    {
                        return false;
                    }
                }
                
                Registers::SetRegister(Registers::Register::TransmitHolding, source[byte]);
            }

            Registers::SetRSTLatch(false);
            return true;
        }

        /** @brief Reads a specified number of bytes from modem into a target buffer
         * @param target Target buffer
         * @param length Buffer length
         * @return Number of bytes read before timeout
         */
        static size_t Read(uint8_t* target, size_t length)
        {
            Registers::SetDSTLatch(true);

            for (size_t byte = 0; byte < length; byte++)
            {
                uint32_t timeout = Netlink::UartTimeout;

                // Wait for data
                while (!((Registers::LineStatus)Registers::ReadRegister(Registers::Register::LineStatus) & Registers::LineStatus::DataReady))
                {
                    if (timeout-- <= 0)
                    {
                        Registers::SetDSTLatch(false);
                        return byte;
                    }
                }
                
                target[byte] = Registers::ReadRegister(Registers::Register::ReceiveBuffer);
            }

            return length;
        }
    };
}

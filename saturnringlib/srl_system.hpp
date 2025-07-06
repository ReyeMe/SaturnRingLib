#pragma once

#include "srl_base.hpp"

namespace SRL
{

    /** @brief System-level hardware control and BIOS services
     * @details Provides access to Sega Saturn's BIOS service routines and hardware control
     *          through a modern C++ interface.
     */
    class System final
    {
    private:
        /** @brief Type alias for unsigned integer handler function pointer */
        using UintHandler = void(*)(uint32_t, void*);

        /** @brief Type alias for unsigned integer handler getter function pointer */
        using UintHandlerGetter = void* (*)(uint32_t);

        /** @brief Type alias for unsigned integer processor function pointer */
        using UintProcessor = uint32_t(*)(uint32_t);

        /** @brief Type alias for void unsigned integer processor function pointer */
        using VoidUintProcessor = void(*)(uint32_t);

        /** @brief Type alias for unsigned integer setter function pointer */
        using UintSetter = void(*)(uint32_t);

        /** @brief Type alias for unsigned integer pair setter function pointer */
        using UintPairSetter = void(*)(uint32_t, uint32_t);

        /** @brief Type alias for unsigned integer array setter function pointer */
        using UintArraySetter = void(*)(uint32_t*);

        /** @brief Type alias for void function pointer */
        using VoidFunction = void(*)();

        /** @brief Type alias for integer processor function pointer */
        using IntProcessor = int32_t(*)(int32_t);

        /** @brief Get a BIOS function pointer from a memory address
         *  @tparam T Function pointer type
         *  @param address Memory address of the function
         *  @return Function pointer of type T
         */
        template<typename T>
        static auto GetBiosFunction(uint32_t address)
        {
            return *reinterpret_cast<volatile T*>(address);
        }

    public:
        /** @brief Interrupt types for SCU and SH2
         *  @details These values correspond to the hardware interrupt vectors for the SCU.
         *           They are used with SetInterruptHandler and GetInterruptHandler.
         */
        enum class InterruptType : uint32_t
        {
            /** @brief V-Blank In interrupt (0x40) */
            VBlankIn = 0x40,

            /** @brief V-Blank Out interrupt (0x41) */
            VBlankOut = 0x41,

            /** @brief H-Blank In interrupt (0x42) */
            HBlankIn = 0x42,

            /** @brief Timer 0 interrupt (0x43) */
            Timer0 = 0x43,

            /** @brief Timer 1 interrupt (0x44) */
            Timer1 = 0x44,

            /** @brief DSP End interrupt (0x45) */
            DspEnd = 0x45,

            /** @brief Sound Request interrupt (0x46) */
            SoundRequest = 0x46,

            /** @brief System Manager interrupt (0x47) */
            SystemManager = 0x47
        };

        /** @brief System clock mode
         *  @details Controls the master clock frequency which affects pixel clock and display timing.
         */
        enum class ClockMode : uint32_t
        {
            /** @brief 26.0 MHz mode (320/640 pixels per line)
             *  @details Standard resolution mode with 320 pixels per line (non-interlaced)
             *           or 640 pixels per line (interlaced).
             */
            Mode26MHz = 0,

            /** @brief 28.6 MHz mode (352/704 pixels per line)
             *  @details High resolution mode with 352 pixels per line (non-interlaced)
             *           or 704 pixels per line (interlaced).
             */
            Mode28MHz = 1
        };

        /** @brief Set an interrupt handler
         *  @param type Type of interrupt to handle
         *  @param handler Function to call when interrupt occurs
         */
        static void SetInterruptHandler(InterruptType type, void* handler)
        {
            auto func = GetBiosFunction<UintHandler>(0x6000300);
            func(static_cast<uint32_t>(type), handler);
        }

        /** @brief Get the current interrupt handler
         *  @param type Type of interrupt
         * return Current interrupt handler function pointer
         */
        static void* GetInterruptHandler(InterruptType type)
        {
            auto func = GetBiosFunction<UintHandlerGetter>(0x6000304);
            return func(static_cast<uint32_t>(type));
        }

        /** @brief Set an SH2 interrupt vector
         *  @param vector Interrupt vector number
         *  @param handler Function to call when interrupt occurs
         */
        static void SetInterruptVector(uint32_t vector, void* handler)
        {
            auto func = GetBiosFunction<UintHandler>(0x6000310);
            func(vector, handler);
        }

        /** @brief Get the current SH2 interrupt vector
         *  @param vector Interrupt vector number
         *  @return Current interrupt handler function pointer
         */
        static void* GetInterruptVector(uint32_t vector)
        {
            auto func = GetBiosFunction<UintHandlerGetter>(0x6000314);
            return func(vector);
        }

        /** @brief Test and set a system semaphore
         *  @param semaphore Semaphore number to test and set
         *  @return Previous value of the semaphore (0 if not set, non-zero if set)
         */
        static uint32_t TestAndSetSemaphore(uint32_t semaphore)
        {
            auto func = GetBiosFunction<UintProcessor>(0x6000330);
            return func(semaphore);
        }

        /** @brief Clear a system semaphore
         *  @param semaphore Semaphore number to clear
         */
        static void ClearSemaphore(uint32_t semaphore)
        {
            auto func = GetBiosFunction<VoidUintProcessor>(0x6000334);
            func(semaphore);
        }

        /** @brief Set the SCU interrupt mask
         *  @param mask Bitmask of interrupts to enable/disable
         */
        static void SetInterruptMask(uint32_t mask)
        {
            auto func = GetBiosFunction<UintSetter>(0x6000340);
            func(mask);
        }

        /** @brief Modify the SCU interrupt mask
         *  @param andMask Bitmask to AND with current mask
         *  @param orMask Bitmask to OR with current mask
         */
        static void ChangeInterruptMask(uint32_t andMask, uint32_t orMask)
        {
            auto func = GetBiosFunction<UintPairSetter>(0x6000344);
            func(andMask, orMask);
        }

        /** @brief Get the current SCU interrupt mask
         *  @return Current interrupt mask value
         */
        static uint32_t GetInterruptMask()
        {
            return *reinterpret_cast<volatile uint32_t*>(0x6000348);
        }

        /** @brief Set the system clock mode
         *  @param mode Clock mode to set
         */
        static void SetClockMode(ClockMode mode)
        {
            auto func = GetBiosFunction<UintSetter>(0x6000320);
            func(static_cast<uint32_t>(mode));
        }

        /** @brief Get the current system clock mode
         *  @return Current clock mode
         */
        static ClockMode GetClockMode()
        {
            return static_cast<ClockMode>(*reinterpret_cast<volatile uint32_t*>(0x6000324));
        }

        /** @brief Interrupt priority table (32 entries) */
        struct InterruptPriorityTable
        {
            static constexpr size_t COUNT = 32;  ///< Number of interrupt priority entries
            uint32_t priorities[COUNT];          ///< Array of interrupt priorities

            /** @brief Default constructor initializes all priorities to zero */
            constexpr InterruptPriorityTable() : priorities{ 0 } {}

            /** @brief Access interrupt priority with compile-time bounds checking
             *  @tparam I Index (0-31)
             *  @return Reference to the priority value
             */
            template<size_t I>
            constexpr uint32_t& at() noexcept
            {
                static_assert(I < COUNT, "Interrupt priority index out of bounds");
                return priorities[I];
            }

            /** @brief Const access to interrupt priority with compile-time bounds checking
             *  @tparam I Index (0-31)
             *  @return Const reference to the priority value
             */
            template<size_t I>
            constexpr const uint32_t& at() const noexcept
            {
                static_assert(I < COUNT, "Interrupt priority index out of bounds");
                return priorities[I];
            }

            /** @brief Access interrupt priority with runtime bounds checking
             *  @param index Priority index (0-31)
             *  @return Reference to the priority value
             *  @note No bounds checking is performed in release builds for performance
             */
            constexpr uint32_t& operator[](size_t index) noexcept
            {
                return priorities[index];
            }

            /** @brief Const access to interrupt priority with runtime bounds checking
             *  @param index Priority index (0-31)
             *  @return Const reference to the priority value
             *  @note No bounds checking is performed in release builds for performance
             */
            constexpr const uint32_t& operator[](size_t index) const noexcept
            {
                return priorities[index];
            }
        };

        /** @brief Set interrupt priorities
         *  @param priorityTable Reference to interrupt priority table containing 32 priority values
         *
         *  Example:
         *  @code
         *  SRL::System::InterruptPriorityTable priorities;
         *  priorities[0] = 1;  // Set priority for interrupt 0
         *  priorities[1] = 2;  // Set priority for interrupt 1
         *  // ...
         *  System::SetInterruptPriorities(priorities);
         *  @endcode
         */
        static void SetInterruptPriorities(const InterruptPriorityTable& priorityTable)
        {
            auto func = GetBiosFunction<UintArraySetter>(0x6000280);
            // Safe const_cast because the BIOS function won't modify the data
            func(const_cast<uint32_t*>(priorityTable.priorities));
        }

        /** @brief Execute CD multiplayer startup
         *
         *  This function initializes the CD-ROM for multiplayer mode.
         *  Should be called before any multiplayer operations.
         */
        static void ExecuteCdMultiplayer()
        {
            auto func = GetBiosFunction<VoidFunction>(0x600026C);
            func();
        }

        /** @brief Get reference to power-off clear memory
         *  @return Reference to the first byte of power-off clear memory
         *
         *  The power-off clear memory area is preserved during soft resets
         *  but cleared on power cycle.
         */
        static volatile uint8_t& PowerOffClearMemory()
        {
            return *reinterpret_cast<volatile uint8_t*>(0x6000210);
        }

        /** @brief Check MPEG status
         *  @param dummy Parameter required by BIOS (must be 0)
         *  @return MPEG status code (0 on success)
         *  @note This function is part of the preliminary MPEG spec
         */
        static int32_t CheckMpeg(int32_t dummy = 0)
        {
            auto func = GetBiosFunction<IntProcessor>(0x6000274);
            return func(dummy);
        }

        /** @brief Check if a CD track is valid
         *  @param trackNumber The track number to check (1-99)
         *  @note This function uses the SGL SYS_CheckTrack function internally
         *  @note This sets SGL global variables that can be checked later
         */
        static void CheckTrack(int32_t trackNumber)
        {
            SYS_CheckTrack(trackNumber);
        }

        /** @brief Terminate program execution
         *  @param exitCode The exit code to return to the system (default: 0)
         *  @note This function does not return
         *  @note This function uses the SGL SYS_Exit function internally
         */
        [[noreturn]] static void Exit(int32_t exitCode = 0)
        {
            SYS_Exit(exitCode);
            // Ensure we never return, even if SYS_Exit somehow does
            while (true) {}
        }
    };
} // namespace SRL

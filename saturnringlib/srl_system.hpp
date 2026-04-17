#pragma once

#include "srl_base.hpp"

namespace SRL
{

    /** @brief System-level hardware control and BIOS services.
     *  @details Provides a modern C++ interface to the Sega Saturn BIOS service routines.
     *           Each method maps to a function pointer stored in the system work area
     *           (0x6000000-0x6000FFF).
     *
     *  @par Functional Groups:
     *  | Group | Functions |
     *  |-------|-----------|
     *  | SCU Interrupt Routine Access | SetInterruptHandler, GetInterruptHandler |
     *  | SH2 Interrupt Vector Access | SetInterruptVector, GetInterruptVector |
     *  | SCU Interrupt Mask | SetInterruptMask, ChangeInterruptMask, GetInterruptMask |
     *  | Simple Semaphore | TestAndSetSemaphore, ClearSemaphore |
     *  | System Clock | SetClockMode, GetClockMode |
     *  | SCU Interrupt Priority | SetInterruptPriorities |
     *  | CD Multiplayer | ExecuteCdMultiplayer |
     *  | Power-On Clear Memory | PowerOffClearMemory |
     *  | MPEG Check | CheckMpeg |
     *  | CD Track Verification | CheckTrack |
     *  | Exit | Exit |
     *
     *  @warning SCU interrupt-related operations and clock changes must only be performed
     *           from the **master SH2**. Using them from the slave SH2 is undefined.
     *
     *  @par Basic Usage:
     *  @code
     *  // Set up interrupt mask allowing only V-Blank
     *  System::SetInterruptMask(~static_cast<uint32_t>(Interrupt::Mask::VBlankIn));
     *
     *  // Read current clock mode
     *  auto clock = System::GetClockMode();
     *  @endcode
     *
     *  @see Interrupt for the higher-level type-safe interrupt API
     *  @see Timer for hardware timer services
     */
    class System final
    {
    private:
        /// @name BIOS Function Pointer Types
        /// @brief Type aliases matching the calling conventions of each BIOS entry point.
        //@{
        using UintHandler = void(*)(uint32_t, void*);       ///< Handler set/get signature
        using UintHandlerGetter = void* (*)(uint32_t);      ///< Handler query signature
        using UintProcessor = uint32_t(*)(uint32_t);        ///< Semaphore test-and-set signature
        using VoidUintProcessor = void(*)(uint32_t);        ///< Semaphore clear signature
        using UintSetter = void(*)(uint32_t);               ///< Mask set / clock change signature
        using UintPairSetter = void(*)(uint32_t, uint32_t); ///< Mask change (AND/OR) signature
        using UintArraySetter = void(*)(uint32_t*);         ///< Priority table signature
        using VoidFunction = void(*)();                     ///< CD Multiplayer launch signature
        using IntProcessor = int32_t(*)(int32_t);           ///< MPEG check signature
        //@}

        /** @brief Dereference a BIOS function pointer from a system work area address.
         *  @tparam T Function pointer type matching the entry's calling convention
         *  @param address Memory-mapped address in the system work area (0x6000000-0x6000FFF)
         *  @return Function pointer of type T ready for invocation
         */
        template<typename T>
        static auto GetBiosFunction(uint32_t address)
        {
            return *reinterpret_cast<volatile T*>(address);
        }

    public:
        /** @brief SCU interrupt type identifiers.
         *  @details Vector numbers for SCU interrupt sources (0x40-0x4F). Used with
         *           SetInterruptHandler() and GetInterruptHandler().
         *
         *  @see Interrupt::Vector for the full vector enum including CPU exceptions
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

        /** @brief System clock mode selection.
         *  @details Controls the master CPU clock frequency and pixel clock, which in turn
         *           determines horizontal resolution. Changing the clock triggers a partial
         *           hardware reset (VDP1/VDP2/SCSP registers are destroyed).
         *
         *  @warning Changing the clock mode resets VDP1, VDP2, SCSP, and the slave SH2.
         *           DRAM content is destroyed. SDRAM and CD block are preserved.
         *           See SetClockMode() for details.
         *
         *  @see SetClockMode(), GetClockMode()
         */
        enum class ClockMode : uint32_t
        {
            /** @brief 26.0 MHz CPU clock (320/640 pixels per line).
             *  @details Standard resolution. 320px non-interlaced, 640px interlaced.
             */
            Mode26MHz = 0,

            /** @brief 28.6 MHz CPU clock (352/704 pixels per line).
             *  @details High resolution. 352px non-interlaced, 704px interlaced.
             */
            Mode28MHz = 1
        };

        /** @name SCU Interrupt Routine Access
         *  Register and query SCU interrupt handler functions.
         */
        //@{

        /** @brief Register a function as an SCU interrupt handler.
         *  @param type SCU interrupt source to handle (0x40-0x4F)
         *  @param handler Function pointer to register (or nullptr to clear)
         *  @details Calls the BIOS entry at 0x6000300. The handler is invoked by the
         *           interrupt dispatcher when the corresponding SCU interrupt fires.
         *
         *  @warning **SCU handlers only.** The handler function **must** be declared with
         *           `__attribute__((interrupt_handler))` so the compiler emits `rte` instead
         *           of `rts`. This requirement does **not** apply to CPU/TRAP vectors
         *           (0x60-0x8F) which use normal `rts`. Omitting the attribute on an SCU
         *           handler will crash on return from interrupt.
         *
         *  @see Interrupt::SetHandler() for the type-safe wrapper with compile-time checks
         */
        static void SetInterruptHandler(InterruptType type, void* handler)
        {
            auto func = GetBiosFunction<UintHandler>(0x6000300);
            func(static_cast<uint32_t>(type), handler);
        }

        /** @brief Query the currently registered SCU interrupt handler.
         *  @param type SCU interrupt source to query (0x40-0x4F)
         *  @return Function pointer to the current handler, or nullptr if none is set
         *  @details Reads from the BIOS entry at 0x6000304.
         */
        static void* GetInterruptHandler(InterruptType type)
        {
            auto func = GetBiosFunction<UintHandlerGetter>(0x6000304);
            return func(static_cast<uint32_t>(type));
        }
        //@}

        /** @name SH2 Interrupt Vector Access
         *  Register and query SH2 interrupt vector entries.
         */
        //@{

        /** @brief Set an SH2 interrupt vector entry.
         *  @param vector Interrupt vector number (valid ranges: 0x40-0x4F for SCU, 0x60-0x8F for CPU)
         *  @param handler Function pointer to register (or nullptr to restore default)
         *  @details Calls the BIOS entry at 0x6000310. Unlike SetInterruptHandler(), this
         *           operates on raw vector numbers and can target CPU exception vectors (0x60-0x8F)
         *           as well as TRAP vectors (0x80-0x8F). CPU/TRAP handlers use normal `rts`
         *           and do **not** need `__attribute__((interrupt_handler))`.
         *
         *  @note From the master SH2, vectors 0x94 (slave startup) and 0x100-0x17F
         *        (slave vector table mirror) are also accessible.
         *
         *  @see Interrupt::SetHandler() for the type-safe wrapper
         */
        static void SetInterruptVector(uint32_t vector, void* handler)
        {
            auto func = GetBiosFunction<UintHandler>(0x6000310);
            func(vector, handler);
        }

        /** @brief Query the current SH2 interrupt vector entry.
         *  @param vector Interrupt vector number
         *  @return Function pointer registered at the given vector, or nullptr
         *  @details Reads from the BIOS entry at 0x6000314.
         */
        static void* GetInterruptVector(uint32_t vector)
        {
            auto func = GetBiosFunction<UintHandlerGetter>(0x6000314);
            return func(vector);
        }
        //@}

        /** @name Simple Semaphore
         *  Atomic test-and-set semaphore operations.
         */
        //@{

        /** @brief Atomically test and set a system semaphore.
         *  @param semaphore Semaphore number to test and set
         *  @return Previous value: 0 if the semaphore was free (now acquired),
         *          non-zero if it was already held
         *  @details Calls the BIOS entry at 0x6000330. Internally uses the SH-2 TAS.B
         *           (Test And Set) instruction for atomic read-modify-write.
         *
         *  @warning Many emulators (including Mednafen) do not correctly emulate the
         *           TAS.B instruction's bus cycle. This function may return incorrect
         *           results in emulated environments.
         *
         *  @par Example:
         *  @code
         *  if (System::TestAndSetSemaphore(5) == 0) {
         *      // Semaphore acquired - critical section
         *      System::ClearSemaphore(5);
         *  }
         *  @endcode
         *
         *  @see ClearSemaphore()
         */
        static uint32_t TestAndSetSemaphore(uint32_t semaphore)
        {
            auto func = GetBiosFunction<UintProcessor>(0x6000330);
            return func(semaphore);
        }

        /** @brief Release a system semaphore.
         *  @param semaphore Semaphore number to clear
         *  @details Calls the BIOS entry at 0x6000334.
         *
         *  @see TestAndSetSemaphore()
         */
        static void ClearSemaphore(uint32_t semaphore)
        {
            auto func = GetBiosFunction<VoidUintProcessor>(0x6000334);
            func(semaphore);
        }
        //@}

        /** @name SCU Interrupt Mask
         *  Control which SCU interrupt sources are enabled.
         */
        //@{

        /** @brief Replace the entire SCU interrupt mask.
         *  @param mask New bitmask - set bits **disable** the corresponding interrupt source
         *  @details Calls the BIOS entry at 0x6000340.
         *
         *  @par Example:
         *  @code
         *  // Enable only V-Blank In (disable everything else)
         *  System::SetInterruptMask(~static_cast<uint32_t>(Interrupt::Mask::VBlankIn));
         *  @endcode
         *
         *  @see Interrupt::SetMask() for the type-safe enum wrapper
         */
        static void SetInterruptMask(uint32_t mask)
        {
            auto func = GetBiosFunction<UintSetter>(0x6000340);
            func(mask);
        }

        /** @brief Modify the SCU interrupt mask with AND/OR logic.
         *  @param andMask Bits to AND with current mask (use to enable interrupts)
         *  @param orMask  Bits to OR with current mask (use to disable interrupts)
         *  @details Calls the BIOS entry at 0x6000344.
         *           Result = (currentMask & andMask) | orMask.
         *
         *  @par Example:
         *  @code
         *  // Enable V-Blank In and V-Blank Out without affecting other bits
         *  System::ChangeInterruptMask(0xFFFFFFFC, 0);  // Clear bits 0,1
         *  @endcode
         *
         *  @see Interrupt::ChangeMask() for the type-safe enum wrapper
         */
        static void ChangeInterruptMask(uint32_t andMask, uint32_t orMask)
        {
            auto func = GetBiosFunction<UintPairSetter>(0x6000344);
            func(andMask, orMask);
        }

        /** @brief Read the current SCU interrupt mask.
         *  @return Current interrupt mask value (memory-mapped at 0x6000348)
         *  @details Direct memory read from 0x6000348, not a function call.
         */
        static uint32_t GetInterruptMask()
        {
            return *reinterpret_cast<volatile uint32_t*>(0x6000348);
        }
        //@}

        /** @name System Clock
         *  Change and query the master CPU clock frequency.
         */
        //@{

        /** @brief Switch the system clock frequency.
         *  @param mode Target clock mode (26 MHz or 28 MHz)
         *  @details Calls the BIOS entry at 0x6000320. This triggers a partial hardware
         *           reset via SMPC command. Processing time is ~130 ms.
         *
         *  @par Hardware Side Effects:
         *  | Affected | Result |
         *  |----------|--------|
         *  | VDP1, VDP2, SCSP | Registers reset - must be re-initialized |
         *  | DRAM | Content destroyed |
         *  | Slave SH2 | Stopped (OFF) |
         *  | Master SH2 FRT, SCI | Reset (enters standby during switch) |
         *  | SDRAM, CD block | Preserved |
         *
         *  @warning After calling this function you **must** re-initialize VDP1, VDP2 and
         *           SCSP. The TV mode must be set quickly to avoid sync glitches.
         *
         *  @see GetClockMode()
         */
        static void SetClockMode(ClockMode mode)
        {
            auto func = GetBiosFunction<UintSetter>(0x6000320);
            func(static_cast<uint32_t>(mode));
        }

        /** @brief Query the current system clock mode.
         *  @return Last value written by SetClockMode() (memory-mapped at 0x6000324)
         *  @details Direct memory read from 0x6000324. Returns the value of the last
         *           SetClockMode() call, **not** a live hardware reading.
         */
        static ClockMode GetClockMode()
        {
            return static_cast<ClockMode>(*reinterpret_cast<volatile uint32_t*>(0x6000324));
        }
        //@}

        /** @name SCU Interrupt Priority
         *  Reprogram the SCU interrupt priority dispatch table.
         */
        //@{

        /** @brief SCU interrupt priority table (32 longword entries).
         *  @details Each entry is a 32-bit value with two packed fields:
         *           - **Upper 16 bits**: SH2 SR lower-word value set at interrupt entry
         *           - **Lower 16 bits**: SCU interrupt mask OR'd with current mask at entry
         *
         *  @warning Incorrect priority settings **will crash the system**. The BIOS
         *           performs no validation on the table content.
         *
         *  @see SetInterruptPriorities()
         */
        struct InterruptPriorityTable
        {
            static constexpr size_t COUNT = 32;  ///< Number of SCU interrupt priority entries
            uint32_t priorities[COUNT];          ///< Packed SR-value | mask-value entries

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

        /** @brief Program the SCU interrupt routine priority table.
         *  @param priorityTable 32-entry table of packed SR | mask values
         *  @details Calls the BIOS entry at 0x6000280. Requires SCU revision 2.1+
         *           (version register at 0x25FE00C4 >= 3). The table is copied into the
         *           BIOS work area and remains active until overwritten or reset.
         *
         *  @warning **This is a dangerous operation.** If the table contains inconsistent
         *           priority settings the system will crash. No validation is performed
         *           on the table content.
         *
         *  @par Example:
         *  @code
         *  System::InterruptPriorityTable priorities;
         *  priorities[0]  = 0x00f0ffff;  // V-Blank In
         *  priorities[1]  = 0x00e0fffe;  // V-Blank Out
         *  priorities[9]  = 0x0060ffff;  // DMA2 (highest priority)
         *  priorities[10] = 0x0050fdff;  // DMA1
         *  priorities[11] = 0x0050f9ff;  // DMA0
         *  System::SetInterruptPriorities(priorities);
         *  @endcode
         *
         *  @see InterruptPriorityTable for the entry format
         */
        static void SetInterruptPriorities(const InterruptPriorityTable& priorityTable)
        {
            auto func = GetBiosFunction<UintArraySetter>(0x6000280);
            // Safe const_cast because the BIOS function won't modify the data
            func(const_cast<uint32_t*>(priorityTable.priorities));
        }

        //@}

        /** @name CD Multiplayer
         *  Launch the CD Multiplayer application.
         */
        //@{

        /** @brief Launch the CD Multiplayer and relinquish control.
         *  @details Calls the BIOS entry at 0x600026C. Reverts the system to its
         *           post-power-on state and launches the CD Multiplayer.
         *           **This function does not return.**
         */
        static void ExecuteCdMultiplayer()
        {
            auto func = GetBiosFunction<VoidFunction>(0x600026C);
            func();
        }

        //@}

        /** @name Power-On Clear Memory
         *  8-byte BIOS-managed memory area preserved across soft resets.
         */
        //@{

        /** @brief Access the power-on clear memory area.
         *  @return Reference to the first byte at 0x6000210 (8 bytes total)
         *  @details This 8-byte area is initialized to zero only at power-on. Content
         *           survives NMI / soft reset (Reset button), making it useful for
         *           passing small amounts of state between reset cycles.
         *
         *  @warning Do not access beyond 8 bytes from the returned address.
         */
        static volatile uint8_t& PowerOffClearMemory()
        {
            return *reinterpret_cast<volatile uint8_t*>(0x6000210);
        }

        //@}

        /** @name MPEG Cartridge Check
         *  Check for the presence and readiness of the MPEG cartridge.
         */
        //@{

        /** @brief Check MPEG cartridge status (blocking).
         *  @param dummy Reserved parameter - must always be 0
         *  @return Positive value if MPEG cartridge is present and operational,
         *          negative value on error or if no MPEG cartridge is installed
         *  @details Calls the BIOS entry at 0x6000274. This function blocks until
         *           the check completes. Must be called after every disc swap
         *           (CD door open) and after a CD block soft reset.
         *
         *  @note Do not call during SCU-DMA transfers (A-bus conflict).
         *        If the check fails, retry once before treating as a permanent failure.
         */
        static int32_t CheckMpeg(int32_t dummy = 0)
        {
            auto func = GetBiosFunction<IntProcessor>(0x6000274);
            return func(dummy);
        }

        //@}

        /** @name CD Track Verification
         *  Verify disc authenticity by comparing track positions.
         */
        //@{

        /** @brief Verify a CD track position against TOC data.
         *  @param trackNumber Track number to verify (must be >= 2; specify a CD-DA track)
         *  @details Compares the specified
         *           track's start position against TOC data. If they match, the function
         *           returns normally. **If they do not match, control is transferred to the
         *           CD Multiplayer and this function does not return.**
         *
         *  @warning This function may **never return** if the disc is considered invalid
         *           (track mismatch, missing track, or CDC error). Use only for disc
         *           authentication checks, typically at startup.
         *
         *  @note Track 1 has no meaning for verification. Specify the last CD-DA track
         *        for best results.
         */
        static void CheckTrack(int32_t trackNumber)
        {
            SYS_CheckTrack(trackNumber);
        }

        //@}

        /** @name Exit
         *  Terminate the application and transfer control to the system.
         */
        //@{

        /** @brief Terminate the application.
         *  @param exitCode Function code controlling post-exit behavior:
         *         | Code | Behavior |
         *         |------|----------|
         *         | 0 | Launch Demo-Demo menu if applicable, else CD Multiplayer |
         *         | 1 | Launch CD Multiplayer unconditionally |
         *         | 2 | Launch Demo-Demo menu unconditionally |
         *         | <0 | Enter infinite loop (halt) |
         *  @details Performs system
         *           re-initialization: disables interrupts, switches to 26 MHz,
         *           resets CD block, and restores the default stack pointer (0x6002000).
         *           **This function does not return.**
         */
        [[noreturn]] static void Exit(int32_t exitCode = 0)
        {
            SYS_Exit(exitCode);
            // Ensure we never return, even if SYS_Exit somehow does
            while (true) {}
        }
        //@}
    };
} // namespace SRL

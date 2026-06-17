#pragma once

#include "srl_system.hpp"

#include <type_traits>
#include <utility>

namespace SRL
{
    /** @brief Type-safe interrupt management for the Sega Saturn.
     *  @details Provides a modern C++ interface over the BIOS interrupt services
     *           and direct SCU register access. Supports both SCU interrupts
     *           (0x40-0x4F) and CPU/TRAP vectors (0x60-0x8F) with compile-time
     *           validation.
     *
     *  @par Architecture:
     *  | Vector Range | Type | Return |
     *  |--------------|------|--------|
     *  | 0x40-0x4F | SCU Interrupts | `rte` (mandatory) |
     *  | 0x60-0x8F | CPU / TRAP | `rts` (default) |
     *
     *  @par Basic Usage:
     *  @code
     *  // Set interrupt mask (enable V-Blank only)
     *  Interrupt::SetMask(Interrupt::Mask::VBlankIn);
     *
     *  // Register an SCU interrupt handler
     *  void __attribute__((interrupt_handler)) myVBlank() { }
     *  Interrupt::SetHandler(Interrupt::Vector::VBlankIn, &myVBlank);
     *
     *  // Check and clear interrupt status
     *  if (static_cast<uint32_t>(Interrupt::GetStatus() & Interrupt::Status::VBlankIn)) {
     *      Interrupt::ResetStatus(Interrupt::Status::VBlankIn);
     *  }
     *  @endcode
     *
     *  @warning SCU interrupt handlers (0x40-0x4F) **must** use
     *           `__attribute__((interrupt_handler))` to emit `rte` instead of `rts`.
     *           Omitting this attribute will crash on return from interrupt.
     *
     *  @see System for the lower-level BIOS wrappers
     *  @see Timer for FRT overflow interrupt usage
     */
    class Interrupt final
    {
    private:
        friend class Timer;
        /** @brief Get a reference to the interrupt status register
         * @return Reference to the memory-mapped status register
         * @note This provides direct access to the hardware register at 0x25fe00a4
         * @internal
         */
        static volatile uint32_t& StatusRegister()
        {
            return *reinterpret_cast<volatile uint32_t*>(0x25fe00a4);
        }

        /** @brief Get a reference to the A-Bus acknowledge register
         * @return Reference to the memory-mapped acknowledge register
         * @note This provides direct access to the hardware register at 0x25fe00bc
         * @internal
         */
        static volatile uint32_t& AcknowledgeRegister()
        {
            return *reinterpret_cast<volatile uint32_t*>(0x25fe00bc);
        }

    public:
        /** @brief Interrupt mask bits for the SCU
         * @details These flags control which interrupts are enabled. Multiple flags can be
         *          combined using the bitwise OR operator (|).
         *
         * @code
         * // Enable V-Blank and H-Blank interrupts
         * auto mask = Interrupt::Mask::VBlankIn | Interrupt::Mask::HBlankIn;
         * Interrupt::SetMask(mask);
         *
         * // Or use the predefined combination
         * Interrupt::SetMask(Interrupt::Mask::Default);
         * @endcode
         */
        enum class Mask : uint32_t
        {
            /** @brief No interrupts enabled (0x0000) */
            None = 0,

            /** @brief Enable V-Blank In interrupt (0x0001) */
            VBlankIn = (1u << 0),

            /** @brief Enable V-Blank Out interrupt (0x0002) */
            VBlankOut = (1u << 1),

            /** @brief Enable H-Blank In interrupt (0x0004) */
            HBlankIn = (1u << 2),

            /** @brief Enable Timer 0 interrupt (0x0008) */
            Timer0 = (1u << 3),

            /** @brief Enable Timer 1 interrupt (0x0010) */
            Timer1 = (1u << 4),

            /** @brief Enable DSP End interrupt (0x0020) */
            DspEnd = (1u << 5),

            /** @brief Enable Sound Request interrupt (0x0040) */
            SoundReq = (1u << 6),

            /** @brief Enable System Manager interrupt (0x0080) */
            SystemMgr = (1u << 7),

            /** @brief Enable Controller interrupt (0x0100) */
            Pad = (1u << 8),

            /** @brief Enable Level 2 DMA interrupt (0x0200) */
            Dma2 = (1u << 9),

            /** @brief Enable Level 1 DMA interrupt (0x0400) */
            Dma1 = (1u << 10),

            /** @brief Enable Level 0 DMA interrupt (0x0800) */
            Dma0 = (1u << 11),

            /** @brief Enable VDP1 Interrupt (0x1000) */
            Vdp1 = (1u << 12),

            /** @brief Enable VDP2 Interrupt (0x2000) */
            Vdp2 = (1u << 13),

            /** @brief Enable CPU Interrupt (0x4000) */
            Cpu = (1u << 14),

            /** @brief Default interrupt mask (0x7FFF)
             *  @details Enables all standard interrupts except User interrupt
             */
            Default = VBlankIn | VBlankOut | HBlankIn | Timer0 | Timer1 | DspEnd |
            SoundReq | SystemMgr | Pad | Dma2 | Dma1 | Dma0 | Vdp1 | Vdp2 | Cpu,

            /** @brief All interrupts mask (0x7FFF) */
            All = 0x7FFF,

            /** @brief Enable User interrupt (0x8000) */
            User = (1u << 15)
        };

        /** @brief Interrupt status bits
         * @details These flags indicate which interrupts have occurred. They can be checked
         *          using the bitwise AND operator (&) with GetStatus().
         *
         * Example:
         * @code
         * // Check if V-Blank occurred
         * if (Interrupt::GetStatus() & Interrupt::Status::VBlankIn) {
         *     // Handle V-Blank
         *     Interrupt::ResetStatus(Interrupt::Status::VBlankIn);
         * }
         * @endcode
         */
        enum class Status : uint32_t
        {
            /** @brief V-Blank In occurred (0x0001) */
            VBlankIn = (1u << 0),

            /** @brief V-Blank Out occurred (0x0002) */
            VBlankOut = (1u << 1),

            /** @brief H-Blank In occurred (0x0004) */
            HBlankIn = (1u << 2),

            /** @brief Timer 0 interrupt occurred (0x0008) */
            Timer0 = (1u << 3),

            /** @brief Timer 1 interrupt occurred (0x0010) */
            Timer1 = (1u << 4),

            /** @brief DSP End interrupt occurred (0x0020) */
            DspEnd = (1u << 5),

            /** @brief Sound Request interrupt occurred (0x0040) */
            SoundReq = (1u << 6),

            /** @brief System Manager interrupt occurred (0x0080) */
            SystemMgr = (1u << 7),

            /** @brief Controller interrupt occurred (0x0100) */
            Pad = (1u << 8),

            /** @brief Level 2 DMA interrupt occurred (0x0200) */
            Dma2 = (1u << 9),

            /** @brief Level 1 DMA interrupt occurred (0x0400) */
            Dma1 = (1u << 10),

            /** @brief Level 0 DMA interrupt occurred (0x0800) */
            Dma0 = (1u << 11),

            /** @brief VDP1 Interrupt occurred (0x1000) */
            Vdp1 = (1u << 12),

            /** @brief VDP2 Interrupt occurred (0x2000) */
            Vdp2 = (1u << 13),

            /** @brief CPU Interrupt occurred (0x4000) */
            Cpu = (1u << 14),

            /** @brief User interrupt occurred (0x8000) */
            User = (1u << 15),

            /** @brief A-Bus interrupt status (0xFFFF0000)
             *  @details Represents all A-Bus interrupt status bits (bits 16-31)
             */
            ABus = 0xFFFF0000,

            // Common combinations
            /** @brief Combined V-Blank status (VBlankIn | VBlankOut) */
            VBlank = VBlankIn | VBlankOut,

            /** @brief All status bits set (0xFFFFFFFF) */
            All = 0xFFFFFFFF
        };

        /** @brief Interrupt acknowledge control values
         * @details These values are used to acknowledge specific interrupts to the hardware.
         *          Acknowledging an interrupt clears its pending status.
         *
         * @note Multiple acknowledgements can be combined using the bitwise OR operator (|).
         */
        enum class Acknowledge : uint32_t
        {
            /** @brief No interrupt acknowledgement (0x00000000) */
            None = 0x00000000,

            /** @brief Acknowledge V-Blank In interrupt (0x00000001) */
            VBlankIn = (1u << 0),

            /** @brief Acknowledge V-Blank Out interrupt (0x00000002) */
            VBlankOut = (1u << 1),

            /** @brief Acknowledge H-Blank In interrupt (0x00000004) */
            HBlankIn = (1u << 2),

            /** @brief Acknowledge all interrupts (0xFFFFFFFF) */
            All = 0xFFFFFFFF
        };

        /** @brief Interrupt vector numbers
         * @details These values correspond to the hardware interrupt vectors
         *          for both SCU and CPU interrupts.
         */
        enum class Vector : uint32_t
        {
            // SCU Vectors (0x40-0x4F)
            /** @brief V-Blank In interrupt vector (0x40) */
            VBlankIn = 0x40,

            /** @brief V-Blank Out interrupt vector (0x41) */
            VBlankOut = 0x41,

            /** @brief H-Blank In interrupt vector (0x42) */
            HBlankIn = 0x42,

            /** @brief Timer 0 interrupt vector (0x43) */
            Timer0 = 0x43,

            /** @brief Timer 1 interrupt vector (0x44) */
            Timer1 = 0x44,

            /** @brief DSP End interrupt vector (0x45) */
            DspEnd = 0x45,

            /** @brief Sound Request interrupt vector (0x46) */
            SoundReq = 0x46,

            /** @brief System Manager interrupt vector (0x47) */
            SystemMgr = 0x47,

            /** @brief Controller interrupt vector (0x48) */
            Pad = 0x48,

            /** @brief Level 2 DMA interrupt vector (0x49) */
            Dma2 = 0x49,

            /** @brief Level 1 DMA interrupt vector (0x4A) */
            Dma1 = 0x4A,

            /** @brief Level 0 DMA interrupt vector (0x4B) */
            Dma0 = 0x4B,

            /** @brief VDP1 Interrupt vector (0x4C) */
            Vdp1 = 0x4C,

            /** @brief VDP2 Interrupt vector (0x4D) */
            Vdp2 = 0x4D,

            /** @brief CPU Interrupt vector (0x4E) */
            Cpu = 0x4E,

            /** @brief User interrupt vector (0x4F) */
            User = 0x4F,

            // CPU Exception Vectors (0x60-0x6F)
            /** @brief CPU Reset vector (0x60) */
            Reset = 0x60,

            /** @brief Bus error exception vector (0x61) */
            BusError = 0x61,

            /** @brief Address error exception vector (0x62) */
            Address = 0x62,

            /** @brief Illegal instruction exception vector (0x63) */
            Illegal = 0x63,

            /** @brief Zero division exception vector (0x64) */
            ZeroDiv = 0x64,

            /** @brief CHK instruction exception vector (0x65) */
            Chk = 0x65,

            /** @brief TRAPV instruction exception vector (0x66) */
            TrapV = 0x66,

            /** @brief Privilege violation exception vector (0x67) */
            Privilege = 0x67,

            /** @brief Trace exception vector (0x68) */
            Trace = 0x68,

            /** @brief Line A emulator exception vector (0x69) */
            LineA = 0x69,

            /** @brief Line F emulator exception vector (0x6A) */
            LineF = 0x6A,

            /** @brief Spurious interrupt vector (0x6B) */
            Spurious = 0x6B,

            /** @brief IRQ1 interrupt vector (0x6C) */
            Irq1 = 0x6C,

            /** @brief IRQ2 interrupt vector (0x6D) */
            Irq2 = 0x6D,

            /** @brief IRQ3 interrupt vector (0x6E) */
            Irq3 = 0x6E,

            /** @brief H-Blank interrupt vector (0x6F) */
            HBlank = 0x6F,

            /** @brief V-Blank interrupt vector (0x70) */
            VBlank = 0x70,

            // TRAP Instruction Vectors (0x80-0x8F)
            /** @brief TRAP #0 instruction vector (0x80) */
            Trap0 = 0x80,

            /** @brief TRAP #1 instruction vector (0x81) */
            Trap1 = 0x81,

            /** @brief TRAP #2 instruction vector (0x82) */
            Trap2 = 0x82,

            /** @brief TRAP #3 instruction vector (0x83) */
            Trap3 = 0x83,

            /** @brief TRAP #4 instruction vector (0x84) */
            Trap4 = 0x84,

            /** @brief TRAP #5 instruction vector (0x85) */
            Trap5 = 0x85,

            /** @brief TRAP #6 instruction vector (0x86) */
            Trap6 = 0x86,

            /** @brief TRAP #7 instruction vector (0x87) */
            Trap7 = 0x87,

            /** @brief TRAP #8 instruction vector (0x88) */
            Trap8 = 0x88,

            /** @brief TRAP #9 instruction vector (0x89) */
            Trap9 = 0x89,

            /** @brief TRAP #10 instruction vector (0x8A) */
            TrapA = 0x8A,

            /** @brief TRAP #11 instruction vector (0x8B) */
            TrapB = 0x8B,

            /** @brief TRAP #12 instruction vector (0x8C) */
            TrapC = 0x8C,

            /** @brief TRAP #13 instruction vector (0x8D) */
            TrapD = 0x8D,

            /** @brief TRAP #14 instruction vector (0x8E) */
            TrapE = 0x8E,

            /** @brief TRAP #15 instruction vector (0x8F) */
            TrapF = 0x8F
        };

        /** @name Mask Control
         *  Enable or disable SCU interrupt sources.
         */
        //@{

        /** @brief Replace the entire SCU interrupt mask.
         *  @param mask New interrupt mask (combined Mask flags)
         *  @details Thin wrapper over System::SetInterruptMask(). Set bits **disable**
         *           the corresponding interrupt source.
         *
         *  @par Example:
         *  @code
         *  // Enable only V-Blank In
         *  Interrupt::SetMask(Interrupt::Mask::VBlankIn);
         *
         *  // Disable all interrupts
         *  Interrupt::SetMask(Interrupt::Mask::All);
         *  @endcode
         *
         *  @see ChangeMask() for selective enable/disable without replacing the full mask
         */
        static void SetMask(Mask mask)
        {
            System::SetInterruptMask(static_cast<uint32_t>(mask));
        }

        /** @brief Selectively modify the SCU interrupt mask.
         *  @param enable  Mask bits to AND with the current mask (preserves selected interrupts)
         *  @param disable Mask bits to OR with the current mask (disables selected interrupts)
         *  @details Thin wrapper over System::ChangeInterruptMask().
         *           Result = (currentMask & enable) | disable.
         *
         *  @par Example:
         *  @code
         *  // Enable V-Blank without changing other mask bits
         *  Interrupt::ChangeMask(~Interrupt::Mask::VBlankIn, Interrupt::Mask::None);
         *  @endcode
         *
         *  @see SetMask() to replace the entire mask at once
         */
        static void ChangeMask(Mask enable, Mask disable)
        {
            System::ChangeInterruptMask(
                static_cast<uint32_t>(enable),
                static_cast<uint32_t>(disable)
            );
        }

        //@}

        /** @name Status and Acknowledge
         *  Query and clear interrupt status / acknowledge registers.
         */
        //@{

        /** @brief Read the SCU interrupt status register.
         *  @return Bitmask of interrupts that have occurred (SCU IST register at 0x25FE00A4)
         *  @details Each set bit indicates the corresponding interrupt has fired since
         *           the last ResetStatus() call. Use bitwise AND with Status flags to test.
         *
         *  @see ResetStatus() to clear specific status bits
         */
        static Status GetStatus()
        {
            return static_cast<Status>(StatusRegister());
        }

        /** @brief Clear interrupt status bits (write-1-to-clear).
         *  @param status Status bits to clear
         *  @details Writes to the SCU IST register at 0x25FE00A4. Set bits in the
         *           parameter clear the corresponding interrupt status flags.
         *
         *  @warning Writing incorrect values may clear status bits you intended to preserve.
         *           Always use specific Status flags rather than raw values.
         */
        static void ResetStatus(Status status)
        {
            StatusRegister() = static_cast<uint32_t>(status);
        }

        /** @brief Set the A-Bus interrupt acknowledge register.
         *  @param ack Acknowledge control value to write (SCU AIACK register at 0x25FE00BC)
         *  @details Acknowledging an interrupt clears its pending status in the A-Bus
         *           interrupt controller.
         *
         *  @see GetAcknowledge()
         */
        static void SetAcknowledge(Acknowledge ack)
        {
            AcknowledgeRegister() = static_cast<uint32_t>(ack);
        }

        /** @brief Read the A-Bus interrupt acknowledge register.
         *  @return Current acknowledge value (SCU AIACK register at 0x25FE00BC)
         *
         *  @see SetAcknowledge()
         */
        static Acknowledge GetAcknowledge()
        {
            return static_cast<Acknowledge>(AcknowledgeRegister());
        }

        //@}

        /** @name Handler Registration
         *  Register interrupt handler functions with compile-time validation.
         */
        //@{

        /** @brief Register an interrupt handler function.
         *  @tparam Func Callable type (function pointer or stateless lambda)
         *  @param vector Interrupt vector to set handler for
         *  @param handler Function to call when the interrupt occurs
         *  @return true if the handler was set successfully, false if the vector is out of range
         *  @details Routes to System::SetInterruptHandler() for SCU vectors (0x40-0x4F) or
         *           System::SetInterruptVector() for CPU/TRAP vectors (0x60-0x8F).
         *           Compile-time checks ensure the callable has the correct signature
         *           (`void()`) and has no captures.
         *
         *  @par Handler Requirements:
         *  | Vector Range | Type | Lambda? | Attribute Required | Return |
         *  |--------------|------|---------|-------------------|--------|
         *  | 0x40-0x4F | SCU Interrupts | **No** | **Mandatory** `__attribute__((interrupt_handler))` | `rte` |
         *  | 0x60-0x8F | CPU / TRAP | Yes (stateless) | Not required | `rts` |
         *
         *  @warning **SCU interrupts (0x40-0x4F):** handlers must be **functions** (not lambdas)
         *           with `__attribute__((interrupt_handler))`. Lambdas are **not allowed** for SCU
         *           because they cannot have the interrupt attribute and will crash on return.
         *           CPU/TRAP handlers (0x60-0x8F) can use lambdas or regular functions.
         *
         *  @par Example:
         *  @code
         *  // SCU interrupt handler - MANDATORY attribute, NO lambdas
         *  void __attribute__((interrupt_handler)) myVBlankHandler() {
         *      // Handle V-Blank
         *  }
         *  Interrupt::SetHandler(Interrupt::Vector::VBlankIn, &myVBlankHandler);
         *
         *  // CPU/TRAP handler - lambdas OK, no attribute needed
         *  Interrupt::SetHandler(Interrupt::Vector::Trap0, []() {
         *      // Handle TRAP
         *  });
         *  @endcode
         *
         *  @see System::SetInterruptHandler(), System::SetInterruptVector()
         */
        template<typename Func>
        static bool SetHandler(Vector vector, Func&& handler) noexcept
        {
            return SetHandlerImpl(vector, std::forward<Func>(handler));
        }

        /** @brief Trigger a TRAP software interrupt by vector.
         *  @param vector TRAP vector to fire (must be Trap0–TrapF, i.e. 0x80–0x8F)
         *  @return true if the vector is a valid TRAP vector and the instruction was issued,
         *          false if the vector is out of the TRAP range.
         *  @details Executes the SH-2 `trapa #N` instruction corresponding to the given
         *           vector. Because `trapa` requires a compile-time immediate, this
         *           dispatches via a switch over all 16 TRAP vectors.
         *           The registered handler (if any) will be invoked synchronously before
         *           this function returns.
         *
         *  @par Example:
         *  @code
         *  // Fire TRAP #3 (used by GDB stub)
         *  Interrupt::Trigger(Interrupt::Vector::Trap3);
         *  @endcode
         *
         *  @note Only TRAP vectors (0x80–0x8F) are supported. SCU and CPU exception
         *        vectors cannot be triggered from software on SH-2.
         */
        static constexpr bool Trigger(Vector vector) noexcept
        {
            switch (vector)
            {
                case Vector::Trap0:  asm volatile("trapa #0"  ::: "memory"); return true;
                case Vector::Trap1:  asm volatile("trapa #1"  ::: "memory"); return true;
                case Vector::Trap2:  asm volatile("trapa #2"  ::: "memory"); return true;
                case Vector::Trap3:  asm volatile("trapa #3"  ::: "memory"); return true;
                case Vector::Trap4:  asm volatile("trapa #4"  ::: "memory"); return true;
                case Vector::Trap5:  asm volatile("trapa #5"  ::: "memory"); return true;
                case Vector::Trap6:  asm volatile("trapa #6"  ::: "memory"); return true;
                case Vector::Trap7:  asm volatile("trapa #7"  ::: "memory"); return true;
                case Vector::Trap8:  asm volatile("trapa #8"  ::: "memory"); return true;
                case Vector::Trap9:  asm volatile("trapa #9"  ::: "memory"); return true;
                case Vector::TrapA:  asm volatile("trapa #10" ::: "memory"); return true;
                case Vector::TrapB:  asm volatile("trapa #11" ::: "memory"); return true;
                case Vector::TrapC:  asm volatile("trapa #12" ::: "memory"); return true;
                case Vector::TrapD:  asm volatile("trapa #13" ::: "memory"); return true;
                case Vector::TrapE:  asm volatile("trapa #14" ::: "memory"); return true;
                case Vector::TrapF:  asm volatile("trapa #15" ::: "memory"); return true;
                default: return false;
            }
        }

        //@}

    private:
        /** @brief Implementation of SetHandler() with compile-time validation.
         *  @tparam Func Callable type
         *  @param vector Target interrupt vector
         *  @param handler Callable to register
         *  @return true on success, false if vector is out of range
         *  @internal
         */
        template<typename Func>
        static bool SetHandlerImpl(Vector vector, Func&& handler) noexcept
        {
            static_assert(std::is_invocable_r_v<void, Func>, 
                "Handler must be callable with no arguments and return void");
            static_assert(std::is_convertible_v<decltype(+handler), void (*)()>, 
                "Handler must be convertible to void(*)() (no captures)");

            using HandlerPtr = void (*)();
            HandlerPtr handler_ptr = +handler;
            const auto vector_num = static_cast<uint32_t>(vector);

            // For SCU vectors (0x40-0x4F)
            if (vector_num >= 0x40 && vector_num <= 0x4F)
            {
                // Note: Lambdas cannot be used for SCU interrupts because they cannot
                // have __attribute__((interrupt_handler)). Use regular functions only.
                System::SetInterruptHandler(
                    static_cast<System::InterruptType>(vector_num),
                    reinterpret_cast<void*>(handler_ptr)
                );
                return true;
            }

            // For CPU vectors (0x60-0x8F)
            if (vector_num >= 0x60 && vector_num <= 0x8F)
            {
                System::SetInterruptVector(
                    vector_num,
                    reinterpret_cast<void*>(handler_ptr)
                );
                return true;
            }

            return false;
        }

        /** @brief Query the currently registered interrupt handler.
         *  @tparam Func Function pointer type to cast the result to
         *  @param vector Interrupt vector to query
         *  @return Function pointer to the current handler, or nullptr if none is set
         *  @details Routes to System::GetInterruptHandler() for SCU vectors (<= 0x4F)
         *           or System::GetInterruptVector() for CPU/TRAP vectors (>0x4F).
         *
         *  @par Example:
         *  @code
         *  using VBlankHandler = void(*)();
         *  auto handler = Interrupt::GetHandler<VBlankHandler>(Interrupt::Vector::VBlankIn);
         *  if (handler) {
         *      handler();  // Invoke manually
         *  }
         *  @endcode
         *  @internal
         */
        template<typename Func>
        static Func GetHandler(Vector vector)
        {
            if (static_cast<uint32_t>(vector) <= 0x4F)
            {
                return reinterpret_cast<Func>(
                    System::GetInterruptHandler(
                        static_cast<System::InterruptType>(static_cast<uint32_t>(vector))
                    )
                    );
            }
            else
            {
                return reinterpret_cast<Func>(
                    System::GetInterruptVector(static_cast<uint32_t>(vector))
                    );
            }
        }

    };

    /** @brief Bitwise OR operator for Interrupt::Mask
     *  @param a First mask value
     *  @param b Second mask value
     *  @return New mask with bits set from either operand
     *
     *  Example:
     *  @code
     *  auto combined = Interrupt::Mask::VBlank | Interrupt::Mask::HBlankIn;
     *  @endcode
     */
    constexpr Interrupt::Mask operator|(Interrupt::Mask a, Interrupt::Mask b)
    {
        return static_cast<Interrupt::Mask>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
    }

    /** @brief Bitwise OR operator for Interrupt::Status
     *  @param a First status value
     *  @param b Second status value
     *  @return New status with bits set from either operand
     *
     *  Example:
     *  @code
     *  auto status = Interrupt::Status::VBlankIn | Interrupt::Status::HBlankIn;
     *  @endcode
     */
    constexpr Interrupt::Status operator|(Interrupt::Status a, Interrupt::Status b)
    {
        return static_cast<Interrupt::Status>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
    }

    /** @brief Bitwise AND operator for Interrupt::Mask
     *  @param a First mask value
     *  @param b Second mask value (used as a bitmask)
     *  @return New mask with bits set where both operands have them set
     *
     *  Example:
     *  @code
     *  auto active = currentMask & Interrupt::Mask::VBlank;
     *  @endcode
     */
    constexpr Interrupt::Mask operator&(Interrupt::Mask a, Interrupt::Mask b)
    {
        return static_cast<Interrupt::Mask>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
            );
    }

    /** @brief Bitwise AND operator for Interrupt::Status
     *  @param a Status value to check
     *  @param b Bitmask to apply
     *  @return New status with bits set where both operands have them set
     *
     *  Example:
     *  @code
     *  if (status & Interrupt::Status::VBlankIn) {
     *      // V-Blank is active
     *  }
     *  @endcode
     */
    constexpr Interrupt::Status operator&(Interrupt::Status a, Interrupt::Status b)
    {
        return static_cast<Interrupt::Status>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b)
            );
    }

    /** @brief Bitwise NOT operator for Interrupt::Mask
     *  @param a Mask to invert
     *  @return Inverted mask with all bits flipped
     *
     *  Example:
     *  @code
     *  // Enable all interrupts except V-Blank
     *  Interrupt::SetMask(~Interrupt::Mask::VBlank);
     *  @endcode
     */
    constexpr Interrupt::Mask operator~(Interrupt::Mask a)
    {
        return static_cast<Interrupt::Mask>(~static_cast<uint32_t>(a));
    }

    /** @brief Bitwise NOT operator for Interrupt::Status
     *  @param a Status to invert
     *  @return Inverted status with all bits flipped
     *
     *  Example:
     *  @code
     *  // Check for any status except V-Blank
     *  if (status & ~Interrupt::Status::VBlankIn) {
     *      // Some interrupt other than V-Blank is active
     *  }
     *  @endcode
     *
     */
    constexpr Interrupt::Status operator~(Interrupt::Status a)
    {
        return static_cast<Interrupt::Status>(~static_cast<uint32_t>(a));
    }
}


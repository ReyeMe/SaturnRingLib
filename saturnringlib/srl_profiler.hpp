#pragma once

#include "srl_timer.hpp"
#include <cstdint>

/** @file srl_profiler.hpp
 *  @brief Function-level CPU profiler for the master SH-2 (milestone 1 of the profiler design).
 *
 *  @details Built when the makefile sets `SRL_PROFILER = 1`. Files listed in
 *  `SRL_PROFILE_SOURCES` (default: all of `SOURCES`) are compiled with
 *  `-finstrument-functions`, so GCC calls the hooks below on entry and exit of
 *  each of their functions. The hooks keep a shadow call stack and add each
 *  call's time into two tables that live in the global `g_srl_prof`:
 *  - one row per function: calls, total ticks, self ticks;
 *  - one row per caller -> callee pair: call count.
 *
 *  Times are FRT ticks (PHI/128, about 4.5 us). Nothing leaves the Saturn by itself:
 *  pull the block with GDB (`source tools/scripts/srl_prof.gdb`, then
 *  `srl-prof-dump prof.bin`) and read it with `tools/scripts/srl_prof.py`.
 *
 *  @note Only the master CPU is profiled; calls made on the slave are ignored.
 *  @note Time spent halted in GDB counts too: a breakpoint inside a profiled
 *  function inflates it.
 */

#ifndef SRL_PROFILER_MAX_FUNCS
    /** @brief Function table rows (power of two). */
    #define SRL_PROFILER_MAX_FUNCS 512
#endif

#ifndef SRL_PROFILER_MAX_ARCS
    /** @brief Caller -> callee table rows (power of two). */
    #define SRL_PROFILER_MAX_ARCS 1024
#endif

namespace SRL
{
    /** @brief Everything the profiler records, laid out for the PC-side reader.
     *  @warning tools/scripts/srl_prof.py parses this byte for byte (big-endian):
     *  bump Version on any layout change.
     */
    struct ProfilerBlock
    {
        static constexpr uint32_t FuncCapacity = SRL_PROFILER_MAX_FUNCS;
        static constexpr uint32_t ArcCapacity = SRL_PROFILER_MAX_ARCS;
        static constexpr uint32_t StackCapacity = 64;

        static_assert((FuncCapacity & (FuncCapacity - 1)) == 0, "SRL_PROFILER_MAX_FUNCS must be a power of two");
        static_assert((ArcCapacity & (ArcCapacity - 1)) == 0, "SRL_PROFILER_MAX_ARCS must be a power of two");

        /** @brief One profiled function; Address 0 = free row. */
        struct Func
        {
            uint32_t Address;
            uint32_t Calls;
            uint32_t TotalTicks;
            uint32_t SelfTicks;
        };

        /** @brief One caller -> callee pair; From 0 = free row.
         *  @details From is the caller's start address, or the return address inside
         *  the caller when the caller was entered before profiling started.
         */
        struct Arc
        {
            uint32_t From;
            uint32_t To;
            uint32_t Count;
        };

        /** @brief 64-byte header. */
        struct Header
        {
            char Magic[4];            ///< "SRLP"
            uint32_t Version;         ///< Layout version (1)
            uint32_t SelfAddress;     ///< &g_srl_prof, so the PC can check the ELF matches the dump
            uint32_t TickHz;          ///< FRT ticks per second
            uint32_t HookCostQ16;     ///< Cost of one enter + exit hook pair, in ticks, 16.16 fixed point
            uint32_t Enabled;         ///< 1 while recording
            uint32_t Frames;          ///< SRL::Core::Synchronize() calls since the last Reset()
            uint32_t ElapsedTicks;    ///< Time since the last Reset(), updated every frame
            uint32_t FuncCapacity;
            uint32_t ArcCapacity;
            uint32_t StackCapacity;
            uint32_t DroppedFuncs;    ///< Exits not recorded: function table full
            uint32_t DroppedArcs;     ///< Calls not recorded: arc table full
            uint32_t DroppedDepth;    ///< Calls deeper than StackCapacity, not timed
            uint32_t MismatchedExits; ///< Exits that did not match the shadow stack (stack was reset)
            uint32_t Reserved;
        };

        Header Head;
        Func Funcs[FuncCapacity];
        Arc Arcs[ArcCapacity];
    };

    static_assert(sizeof(ProfilerBlock::Header) == 64, "profiler header must stay 64 bytes");
}

/** @brief The profiler's data, at a fixed unmangled symbol so GDB can dump it by name. */
inline SRL::ProfilerBlock g_srl_prof;

extern "C"
{
    void __cyg_profile_func_enter(void *fn, void *callSite) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit(void *fn, void *callSite) __attribute__((no_instrument_function));
}

namespace SRL
{
    /** @brief Controls the function profiler (see srl_profiler.hpp).
     *  @details SRL::Core::Initialize() calls Init(), which starts recording.
     */
    class Profiler final
    {
        /// @cond Internal
        friend void ::__cyg_profile_func_enter(void *, void *);
        friend void ::__cyg_profile_func_exit(void *, void *);
        friend class Core;
        /// @endcond

    private:
        /** @brief One shadow-stack frame. */
        struct Frame
        {
            uint32_t Fn;
            uint32_t StartTicks;
            uint32_t ChildTicks;
        };

        inline static Frame stack[ProfilerBlock::StackCapacity];
        inline static uint32_t depth = 0;
        inline static uint32_t masterVbr = 0;
        inline static uint32_t resetTicks = 0;

        /** @brief Rows probed before a lookup gives up and counts the event as dropped. */
        static constexpr uint32_t MaxProbes = 8;

        /** @brief FTCSR overflow flag. */
        static constexpr uint8_t FtcsrOvf = 0x02;

        /** @brief Masks all CPU interrupts and returns the previous SR. */
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t MaskInterrupts()
        {
            uint32_t sr;
            asm volatile("stc sr, %0" : "=r"(sr));
            asm volatile("ldc %0, sr" : : "r"(sr | 0xF0U) : "memory");
            return sr;
        }

        /** @brief Restores SR saved by MaskInterrupts(). */
        __attribute__((always_inline, no_instrument_function)) static inline void RestoreInterrupts(uint32_t sr)
        {
            asm volatile("ldc %0, sr" : : "r"(sr) : "memory");
        }

        /** @brief Current time in ticks: (overflow count << 16) | FRC, truncated to 32 bits.
         *  @warning Call with interrupts masked: the overflow interrupt can't run then,
         *  so a pending overflow is detected from FTCSR instead.
         */
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t Now()
        {
            uint32_t high = Timer::overflowCounter;
            const uint32_t frc = (static_cast<uint32_t>(Timer::FrchReg) << 8) | Timer::FrclReg;

            // FRC wrapped but the overflow interrupt hasn't run yet.
            if ((Timer::Ftcsr & FtcsrOvf) != 0 && frc < 0x8000U)
            {
                high++;
            }

            return (high << 16) | frc;
        }

        /** @brief True on the master SH-2 (SGL gives each CPU its own vector table). */
        __attribute__((always_inline, no_instrument_function)) static inline bool IsMaster()
        {
            uint32_t vbr;
            asm volatile("stc vbr, %0" : "=r"(vbr));
            return vbr == Profiler::masterVbr;
        }

        /** @brief Finds or adds the row of a function. @return nullptr when the table is full */
        __attribute__((no_instrument_function)) static ProfilerBlock::Func *FindFunc(uint32_t fn)
        {
            constexpr uint32_t mask = ProfilerBlock::FuncCapacity - 1;
            uint32_t slot = (fn >> 2) & mask;

            for (uint32_t probe = 0; probe < Profiler::MaxProbes; probe++)
            {
                ProfilerBlock::Func &row = g_srl_prof.Funcs[slot];

                if (row.Address == fn)
                {
                    return &row;
                }

                if (row.Address == 0)
                {
                    row.Address = fn;
                    return &row;
                }

                slot = (slot + 1) & mask;
            }

            return nullptr;
        }

        /** @brief Counts one call from -> to. */
        __attribute__((no_instrument_function)) static void AddArc(uint32_t from, uint32_t to)
        {
            constexpr uint32_t mask = ProfilerBlock::ArcCapacity - 1;
            uint32_t slot = ((from ^ (to << 3)) >> 2) & mask;

            for (uint32_t probe = 0; probe < Profiler::MaxProbes; probe++)
            {
                ProfilerBlock::Arc &row = g_srl_prof.Arcs[slot];

                if (row.From == from && row.To == to)
                {
                    row.Count++;
                    return;
                }

                if (row.From == 0)
                {
                    row.From = from;
                    row.To = to;
                    row.Count = 1;
                    return;
                }

                slot = (slot + 1) & mask;
            }

            g_srl_prof.Head.DroppedArcs++;
        }

        /** @brief Body of __cyg_profile_func_enter. */
        __attribute__((always_inline, no_instrument_function)) static inline void Enter(uint32_t fn, uint32_t callSite)
        {
            if (g_srl_prof.Head.Enabled == 0 || !Profiler::IsMaster())
            {
                return;
            }

            const uint32_t sr = Profiler::MaskInterrupts();
            const uint32_t d = Profiler::depth;

            if (d < ProfilerBlock::StackCapacity)
            {
                Profiler::AddArc(d != 0 ? Profiler::stack[d - 1].Fn : callSite, fn);
                Frame &frame = Profiler::stack[d];
                frame.Fn = fn;
                frame.ChildTicks = 0;
                frame.StartTicks = Profiler::Now(); // last, so the callee's time excludes this hook
            }
            else
            {
                g_srl_prof.Head.DroppedDepth++;
            }

            Profiler::depth = d + 1;
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Body of __cyg_profile_func_exit. */
        __attribute__((always_inline, no_instrument_function)) static inline void Exit(uint32_t fn)
        {
            if (g_srl_prof.Head.Enabled == 0 || !Profiler::IsMaster())
            {
                return;
            }

            // Mask before reading the time, so an interrupt can't add child time after it.
            const uint32_t sr = Profiler::MaskInterrupts();
            const uint32_t now = Profiler::Now();
            const uint32_t d = Profiler::depth;

            if (d == 0)
            {
                // Exit of a function entered before Start()/Reset().
                Profiler::RestoreInterrupts(sr);
                return;
            }

            Profiler::depth = d - 1;

            if (d <= ProfilerBlock::StackCapacity)
            {
                const Frame &frame = Profiler::stack[d - 1];

                if (frame.Fn != fn)
                {
                    g_srl_prof.Head.MismatchedExits++;
                    Profiler::depth = 0;
                }
                else
                {
                    const uint32_t total = now - frame.StartTicks;

                    if (d >= 2)
                    {
                        Profiler::stack[d - 2].ChildTicks += total;
                    }

                    ProfilerBlock::Func *row = Profiler::FindFunc(fn);

                    if (row != nullptr)
                    {
                        row->Calls++;
                        row->TotalTicks += total;
                        row->SelfTicks += total - frame.ChildTicks;
                    }
                    else
                    {
                        g_srl_prof.Head.DroppedFuncs++;
                    }
                }
            }

            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Called by SRL::Core::Synchronize() once per frame. */
        __attribute__((no_instrument_function)) static void OnFrame()
        {
            if (g_srl_prof.Head.Enabled == 0)
            {
                return;
            }

            const uint32_t sr = Profiler::MaskInterrupts();
            g_srl_prof.Head.Frames++;
            g_srl_prof.Head.ElapsedTicks = Profiler::Now() - Profiler::resetTicks;
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Times enter + exit hook pairs called the way instrumented code calls them.
         *  @return Cost of one pair in ticks, 16.16 fixed point (fastest of 8 batches, to skip interrupts)
         */
        __attribute__((no_instrument_function)) static uint32_t MeasureHookCost()
        {
            constexpr uint32_t pairsPerBatch = 128;
            void (*volatile enter)(void *, void *) = __cyg_profile_func_enter;
            void (*volatile leave)(void *, void *) = __cyg_profile_func_exit;
            void *dummy = reinterpret_cast<void *>(&Profiler::MeasureHookCost);
            uint32_t best = UINT32_MAX;

            for (uint32_t batch = 0; batch < 8; batch++)
            {
                uint32_t sr = Profiler::MaskInterrupts();
                const uint32_t start = Profiler::Now();
                Profiler::RestoreInterrupts(sr);

                for (uint32_t i = 0; i < pairsPerBatch; i++)
                {
                    enter(dummy, dummy);
                    leave(dummy, dummy);
                }

                sr = Profiler::MaskInterrupts();
                const uint32_t ticks = Profiler::Now() - start;
                Profiler::RestoreInterrupts(sr);

                if (ticks < best)
                {
                    best = ticks;
                }
            }

            return (best << 16) / pairsPerBatch;
        }

        /** @brief Sets up the header, measures the hook cost and starts recording.
         *  @note Called by SRL::Core::Initialize() after SRL::Timer::Init().
         */
        __attribute__((no_instrument_function)) static void Init()
        {
            asm volatile("stc vbr, %0" : "=r"(Profiler::masterVbr));

            ProfilerBlock::Header &head = g_srl_prof.Head;
            head.Magic[0] = 'S';
            head.Magic[1] = 'R';
            head.Magic[2] = 'L';
            head.Magic[3] = 'P';
            head.Version = 1;
            head.SelfAddress = reinterpret_cast<uint32_t>(&g_srl_prof);
            head.TickHz = Tickstamp::GetSecondsDivider();
            head.FuncCapacity = ProfilerBlock::FuncCapacity;
            head.ArcCapacity = ProfilerBlock::ArcCapacity;
            head.StackCapacity = ProfilerBlock::StackCapacity;

            head.Enabled = 1;
            head.HookCostQ16 = Profiler::MeasureHookCost();
            Profiler::Reset();
        }

    public:
        /** @brief Starts recording, keeping what was recorded so far. */
        __attribute__((no_instrument_function)) static void Start()
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            Profiler::depth = 0;
            g_srl_prof.Head.Enabled = 1;
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Stops recording; the data stays readable. */
        __attribute__((no_instrument_function)) static void Stop()
        {
            g_srl_prof.Head.Enabled = 0;
        }

        /** @brief Clears all recorded data and counters. Recording state is unchanged. */
        __attribute__((no_instrument_function)) static void Reset()
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            ProfilerBlock::Header &head = g_srl_prof.Head;

            for (ProfilerBlock::Func &row : g_srl_prof.Funcs)
            {
                row = {};
            }

            for (ProfilerBlock::Arc &row : g_srl_prof.Arcs)
            {
                row = {};
            }

            head.Frames = 0;
            head.ElapsedTicks = 0;
            head.DroppedFuncs = 0;
            head.DroppedArcs = 0;
            head.DroppedDepth = 0;
            head.MismatchedExits = 0;
            Profiler::depth = 0;
            Profiler::resetTicks = Profiler::Now();
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Cost of one enter + exit hook pair measured at Init(), in ticks (16.16). */
        __attribute__((no_instrument_function)) static uint32_t GetHookCostQ16()
        {
            return g_srl_prof.Head.HookCostQ16;
        }
    };
}

extern "C"
{
    /** @brief Called by GCC on entry to every function built with -finstrument-functions.
     *  @note Weak: each translation unit that includes this header emits a copy, the linker keeps one.
     */
    __attribute__((weak, no_instrument_function)) void __cyg_profile_func_enter(void *fn, void *callSite)
    {
        SRL::Profiler::Enter(reinterpret_cast<uint32_t>(fn), reinterpret_cast<uint32_t>(callSite));
    }

    /** @brief Called by GCC on exit from every function built with -finstrument-functions. */
    __attribute__((weak, no_instrument_function)) void __cyg_profile_func_exit(void *fn, void *callSite)
    {
        (void)callSite;
        SRL::Profiler::Exit(reinterpret_cast<uint32_t>(fn));
    }
}

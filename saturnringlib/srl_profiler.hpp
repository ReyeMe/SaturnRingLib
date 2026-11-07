#pragma once

/** @file srl_profiler.hpp
 *  @brief CPU profiler: per-function times on both SH-2s, a PC sampler, and a per-frame event timeline.
 *
 *  @details Built when the makefile sets `SRL_PROFILER = 1`; otherwise only the
 *  SRL_PROF_SCOPE() / SRL_PROF_MARK() macros exist, and they compile to nothing.
 *
 *  Function times: files listed in `SRL_PROFILE_SOURCES` (default: all of `SOURCES`)
 *  are compiled with `-finstrument-functions`, so GCC calls the hooks below on entry
 *  and exit of each of their functions. The hooks keep a shadow call stack per CPU and
 *  add each call's time into that CPU's tables: one row per function (calls, total
 *  ticks, own ticks) and one row per caller -> callee pair (call count).
 *
 *  PC sampler: with `SRL_PROFILER_SAMPLE_HZ = N`, a timer interrupt on the master counts
 *  where the program counter is, N times a second, in a histogram. It sees all code,
 *  including SGL and functions built without hooks.
 *
 *  Timeline: SRL_PROF_SCOPE("name") and SRL_PROF_MARK("name") record timestamped events
 *  in a buffer that keeps the first SRL_PROFILER_MAX_EVENTS since the last report or Reset()
 *  (later ones are counted as dropped), as do VBlank, the wait inside SRL::Core::Synchronize(), the end of
 *  each VDP1 frame (sprite draw end interrupt), VDP1 frames not finished by the next
 *  VBlank, and the end of each SCU DMA transfer (chained in front of SGL's handlers).
 *  With the sampler on, it also counts which VDP1 command is being drawn (COPR).
 *
 *  The header and function tables live in the global `g_srl_prof` (high work RAM), the timeline
 *  and the sampler's counts in `g_srl_prof_buffers` (low work RAM). Two ways to get them out:
 *  - GDB: `source tools/scripts/srl_prof.gdb`, then `srl-prof-dump prof.bin`;
 *  - streaming: with `SRL_PROFILER_REPORT_FRAMES = N`, Report() sends it over the DevCart
 *    every N frames; `tools/scripts/srl_prof_capture.py` records it through `ftx -g`.
 *    Recording starts with the first report that gets through (see Init()).
 *  Then read it with `tools/scripts/srl_prof.py`.
 *
 *  Times are FRT ticks (PHI/128, about 4.8 us). Each SH-2 has its own FRT: Init() sets
 *  the slave's to the same rate.
 *  @note Timeline events and the sampler are master only.
 *  @note Time spent halted in GDB counts too: a breakpoint inside a profiled
 *  function inflates it.
 */

#ifdef SRL_PROFILER

#include "srl_timer.hpp"
#include "srl_devcart.hpp"
#include "srl_slave.hpp"
#include <cstdint>

#ifndef SRL_PROFILER_MAX_FUNCS
    /** @brief Function table rows per CPU (power of two). */
    #define SRL_PROFILER_MAX_FUNCS 512
#endif

#ifndef SRL_PROFILER_MAX_ARCS
    /** @brief Caller -> callee table rows per CPU (power of two). */
    #define SRL_PROFILER_MAX_ARCS 1024
#endif

#ifndef SRL_PROFILER_MAX_EVENTS
    /** @brief Timeline events kept between two reports (8 bytes each). */
    #define SRL_PROFILER_MAX_EVENTS 4096
#endif

#ifndef SRL_PROFILER_REPORT_FRAMES
    /** @brief Frames between two streamed reports; 0 = no streaming (GDB only). */
    #define SRL_PROFILER_REPORT_FRAMES 0
#endif

#ifndef SRL_PROFILER_SAMPLE_HZ
    /** @brief PC samples per second on the master; 0 = no sampler. */
    #define SRL_PROFILER_SAMPLE_HZ 0
#endif

#ifndef SRL_PROFILER_MAX_BINS
    /** @brief PC histogram bins (4 bytes each), spread over the program's code. */
    #define SRL_PROFILER_MAX_BINS 16384
#endif

#ifndef SRL_PROFILER_MAX_VDP1_COMMANDS
    /** @brief VDP1 command slots counted by the sampler (4 bytes each); slot n is at VRAM n * 32. */
    #define SRL_PROFILER_MAX_VDP1_COMMANDS 2048
#endif

namespace SRL
{
    /** @brief Everything the profiler records, laid out for the PC-side reader.
     *  @warning tools/scripts/srl_prof.py parses a GDB dump of this block followed by
     *  ProfilerBuffers (tools/scripts/srl_prof.gdb) byte for byte (big-endian):
     *  bump Version on any layout change of either.
     */
    struct ProfilerBlock
    {
        static constexpr uint32_t FuncCapacity = SRL_PROFILER_MAX_FUNCS;
        static constexpr uint32_t ArcCapacity = SRL_PROFILER_MAX_ARCS;
        static constexpr uint32_t StackCapacity = 64;
        static constexpr uint32_t EventCapacity = SRL_PROFILER_MAX_EVENTS;
        static constexpr uint32_t HistCapacity = SRL_PROFILER_SAMPLE_HZ != 0 ? SRL_PROFILER_MAX_BINS : 1;
        static constexpr uint32_t Vdp1HistCapacity = SRL_PROFILER_SAMPLE_HZ != 0 ? SRL_PROFILER_MAX_VDP1_COMMANDS : 1;

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

        /** @brief One timeline event.
         *  @details Word = type << 28 | (address of the name's string literal & 0x0FFFFFFF):
         *  the PC reads the name from the ELF.
         */
        struct Event
        {
            uint32_t Word;
            uint32_t Ticks;
        };

        /** @brief One CPU's function tables. */
        struct CpuTables
        {
            uint32_t DroppedFuncs;    ///< Exits not recorded: function table full
            uint32_t DroppedArcs;     ///< Calls not recorded: arc table full
            uint32_t DroppedDepth;    ///< Calls deeper than StackCapacity, not timed
            uint32_t MismatchedExits; ///< Exits that did not match the shadow stack (stack was reset)
            Func Funcs[FuncCapacity];
            Arc Arcs[ArcCapacity];
        };

        /** @brief 128-byte header. */
        struct Header
        {
            char Magic[4];            ///< "SRLP"
            uint32_t Version;         ///< Layout version (4)
            uint32_t SelfAddress;     ///< &g_srl_prof, so the PC can check the ELF matches the dump
            uint32_t TickHz;          ///< FRT ticks per second
            uint32_t HookCostQ16;     ///< Cost of one enter + exit hook pair, in ticks, 16.16 fixed point
            uint32_t Enabled;         ///< 1 while recording
            uint32_t Frames;          ///< SRL::Core::Synchronize() calls since the last Reset()
            uint32_t ElapsedTicks;    ///< Time since the last Reset(), updated every frame
            uint32_t FuncCapacity;
            uint32_t ArcCapacity;
            uint32_t StackCapacity;
            uint32_t EventCapacity;
            uint32_t EventCount;      ///< Events in Events[] (since the last report)
            uint32_t DroppedEvents;   ///< Events not recorded: buffer full
            uint32_t ReportFrames;    ///< SRL_PROFILER_REPORT_FRAMES
            uint32_t Reports;         ///< Reports sent completely
            uint32_t DroppedReports;  ///< Reports skipped (no DevCart) or cut short (nobody reading)
            uint32_t ReportBytes;     ///< Size of the last complete report
            uint32_t ReportTicks;     ///< Time the last complete report took to send
            uint32_t SampleHz;        ///< SRL_PROFILER_SAMPLE_HZ (0 = no sampler)
            uint32_t HistCapacity;    ///< Bins in Histogram[]
            uint32_t HistLow;         ///< Address of bin 0
            uint32_t HistShift;       ///< Each bin covers 1 << HistShift bytes
            uint32_t HistBins;        ///< Bins in use
            uint32_t Samples;         ///< PC samples taken since the last Reset()
            uint32_t SamplesOutside;  ///< ... of which outside the program's code (BIOS, data)
            uint32_t SlaveReady;      ///< 1 once the slave's FRT and tables are set up
            uint32_t Vdp1DrawEnds;    ///< VDP1 sprite draw end interrupts since the last Reset()
            uint32_t Vdp1Missed;      ///< VBlanks where VDP1 had not finished the previous frame
            uint32_t Vdp1HistCapacity;///< Slots in Vdp1Histogram[]
            uint32_t Vdp1Samples;     ///< PC samples taken while VDP1 was drawing
            uint32_t Reserved;
        };

        Header Head;
        CpuTables Master;
        CpuTables Slave; ///< Accessed through the cache-through mirror: both CPUs use it
    };

    /** @brief The profiler's large buffers, which no function call touches (see g_srl_prof_buffers).
     *  @details A GDB dump appends them right after ProfilerBlock, in this order.
     */
    struct ProfilerBuffers
    {
        ProfilerBlock::Event Events[ProfilerBlock::EventCapacity];
        uint32_t Histogram[ProfilerBlock::HistCapacity]; ///< Samples per bin since the last report
        uint32_t Vdp1Histogram[ProfilerBlock::Vdp1HistCapacity]; ///< Samples per VDP1 command slot (COPR / 4) since the last report
    };

    static_assert(sizeof(ProfilerBlock::Header) == 128, "profiler header must stay 128 bytes");
}

/** @brief The profiler's tables, at a fixed unmangled symbol so GDB can dump it by name.
 *  @details In high work RAM: every profiled call reads and writes them, and cache misses to low
 *  work RAM would add about 4 us per call that the measured hook cost doesn't include.
 */
inline SRL::ProfilerBlock g_srl_prof;

/** @brief The timeline and the sampler's counts: written by scopes, marks and the sampler only.
 *  @details In low work RAM (LWRAM_BSS in sgl.linker), so they take nothing from the 1 MB of high
 *  work RAM; SRL's LWRAM heap is that much smaller.
 */
inline SRL::ProfilerBuffers g_srl_prof_buffers __attribute__((section("LWRAM_BSS")));

extern "C"
{
    void __cyg_profile_func_enter(void *fn, void *callSite) __attribute__((no_instrument_function));
    void __cyg_profile_func_exit(void *fn, void *callSite) __attribute__((no_instrument_function));

    /** @brief FRT compare-match interrupt entry (asm, below): saves registers, calls srl_prof_sample(). */
    void srl_prof_sample_thunk();

    /** @brief Records one PC sample. @param pc Interrupted program counter */
    void srl_prof_sample(uint32_t pc) __attribute__((no_instrument_function));
}

namespace SRL
{
    /** @brief Controls the profiler (see srl_profiler.hpp).
     *  @details SRL::Core::Initialize() calls Init(), which starts recording.
     */
    class Profiler final
    {
        /// @cond Internal
        friend void ::__cyg_profile_func_enter(void *, void *);
        friend void ::__cyg_profile_func_exit(void *, void *);
        friend void ::srl_prof_sample(uint32_t);
        friend class Core;
        /// @endcond

    public:
        /** @brief Kind of a timeline event, stored in the top 4 bits of Event::Word. */
        enum class EventType : uint32_t
        {
            Begin = 1,  ///< A scope starts
            End = 2,    ///< A scope ends
            Instant = 3 ///< A point in time
        };

    private:
        /** @brief Which SH-2 a hook runs on. */
        enum class Cpu
        {
            Master,
            Slave
        };

        /** @brief One shadow-stack frame. */
        struct Frame
        {
            uint32_t Fn;
            uint32_t StartTicks;
            uint32_t ChildTicks;
        };

        /** @brief A CPU's shadow stack and clock. */
        struct CpuState
        {
            Frame Stack[ProfilerBlock::StackCapacity];
            uint32_t Depth;
            uint32_t High;    ///< Slave only: FRT wraps counted in software
            uint32_t LastFrc; ///< Slave only: last FRT value read
        };

        /** @brief Report packet types. */
        enum class PacketType : uint8_t
        {
            Events = 1,   ///< Event records
            Tables = 2,    ///< Header, then per CPU: drop counters, used function rows, used arc rows
            Histogram = 3, ///< Non-zero PC histogram bins since the last report
            Vdp1 = 4       ///< Non-zero VDP1 command slots since the last report, each with its 32-byte command
        };

        inline static CpuState masterState = {};
        inline static CpuState slaveState = {}; ///< Accessed through the cache-through mirror
        inline static uint32_t masterVbr = 0;
        inline static uint32_t resetTicks = 0;
        inline static uint32_t framesSinceReport = 0;
        inline static bool waitingForReader = false; ///< Init() is done; recording starts when a PC first reads
        inline static uint32_t sampleInterval = 0;
        inline static uint32_t nextCompare = 0;
        inline static uint32_t jitter = 0xACE1;
        inline static void (*sglDmaHandlers[3])() = {}; ///< SGL's SCU DMA end routines, level 0 to 2

        /** @brief Rows probed before a lookup gives up and counts the event as dropped. */
        static constexpr uint32_t MaxProbes = 8;

        /** @brief FTCSR flags. */
        static constexpr uint8_t FtcsrOvf = 0x02;
        static constexpr uint8_t FtcsrOcfa = 0x08;

        /** @brief On-chip register addresses not exposed by SRL::Timer. */
        static constexpr uintptr_t OcraHigh = 0xFFFFFE14;
        static constexpr uintptr_t Tocr = 0xFFFFFE17;
        static constexpr uintptr_t Vcrc = 0xFFFFFE66;

        /** @brief TIER: output compare A interrupt enable. */
        static constexpr uint8_t TierOcia = 0x08;

        /** @brief Vector of the FRT compare-match interrupt. */
        static constexpr uint32_t SampleVector = 0x65;

        /** @brief VDP1 registers (read-only ones) and command table. */
        static constexpr uintptr_t Vdp1Edsr = 0x25D00010; ///< bit 0: previous frame drawn, bit 1: current frame drawn
        static constexpr uintptr_t Vdp1Copr = 0x25D00014; ///< address of the command being drawn / 8
        static constexpr uintptr_t Vdp1Vram = 0x25C00000;

        /** @brief SCU interrupt status register: write 0 to a bit to clear it. */
        static constexpr uintptr_t ScuIst = 0x25FE00A4;

        /** @brief SCU sprite draw end interrupt: vector and status/mask bit. */
        static constexpr uint32_t SpriteEndVector = 0x4D;
        static constexpr uint32_t SpriteEndBit = 1U << 13;

        /** @brief SCU DMA end vectors, level 0 to 2. */
        static constexpr uint32_t DmaEndVectors[3] = {0x4B, 0x4A, 0x49};

        /** @brief Longest wait for room in the DevCart FIFO before a report is abandoned (~10 ms). */
        static constexpr uint16_t SendTimeoutTicks = 2000;

        /** @brief The same object through the cache-through mirror, for data both CPUs touch. */
        template <typename T>
        __attribute__((always_inline, no_instrument_function)) static inline T *Uncached(T *pointer)
        {
            return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(pointer) | 0x20000000U);
        }

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

        /** @brief The 16-bit FRT counter of the CPU running this.
         *  @details Reading the high byte latches the low byte in a register shared by every
         *  reader: an interrupt that reads the FRT between the two reads (the sampler does)
         *  would hand back its own low byte, up to 256 ticks off. Hence the masking.
         */
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t Frc()
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            const uint32_t high = Timer::FrchReg;
            const uint32_t frc = (high << 8) | Timer::FrclReg;
            Profiler::RestoreInterrupts(sr);
            return frc;
        }

        /** @brief Current time on the master: (overflow count << 16) | FRC, truncated to 32 bits.
         *  @warning Call with interrupts masked: the overflow interrupt can't run then,
         *  so a pending overflow is detected from FTCSR instead.
         */
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t Now()
        {
            uint32_t high = Timer::overflowCounter;
            const uint32_t frc = Profiler::Frc();

            // FRC wrapped but the overflow interrupt hasn't run yet.
            if ((Timer::Ftcsr & FtcsrOvf) != 0 && frc < 0x8000U)
            {
                high++;
            }

            return (high << 16) | frc;
        }

        /** @brief This CPU's shadow stack and clock. */
        template <Cpu cpu>
        __attribute__((always_inline, no_instrument_function)) static inline CpuState &State()
        {
            if constexpr (cpu == Cpu::Master)
            {
                return Profiler::masterState;
            }
            else
            {
                return *Profiler::Uncached(&Profiler::slaveState);
            }
        }

        /** @brief This CPU's function tables. */
        template <Cpu cpu>
        __attribute__((always_inline, no_instrument_function)) static inline ProfilerBlock::CpuTables &Tables()
        {
            if constexpr (cpu == Cpu::Master)
            {
                return g_srl_prof.Master;
            }
            else
            {
                return *Profiler::Uncached(&g_srl_prof.Slave);
            }
        }

        /** @brief Current time on this CPU.
         *  @details The slave has no overflow interrupt: it counts FRT wraps when it reads the
         *  timer, so a slave function is timed right only if hooks run at least every 0.3 s.
         *  @warning Call with interrupts masked.
         */
        template <Cpu cpu>
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t NowOn(CpuState &state)
        {
            if constexpr (cpu == Cpu::Master)
            {
                (void)state;
                return Profiler::Now();
            }
            else
            {
                const uint32_t frc = Profiler::Frc();

                if (frc < state.LastFrc)
                {
                    state.High += 0x10000U;
                }

                state.LastFrc = frc;
                return state.High | frc;
            }
        }

        /** @brief True on the master SH-2 (SGL gives each CPU its own vector table). */
        __attribute__((always_inline, no_instrument_function)) static inline bool IsMaster()
        {
            uint32_t vbr;
            asm volatile("stc vbr, %0" : "=r"(vbr));
            return vbr == Profiler::masterVbr;
        }

        /** @brief Finds or adds the row of a function. @return nullptr when the table is full */
        __attribute__((no_instrument_function)) static ProfilerBlock::Func *FindFunc(ProfilerBlock::CpuTables &tables, uint32_t fn)
        {
            constexpr uint32_t mask = ProfilerBlock::FuncCapacity - 1;
            uint32_t slot = (fn >> 2) & mask;

            for (uint32_t probe = 0; probe < Profiler::MaxProbes; probe++)
            {
                ProfilerBlock::Func &row = tables.Funcs[slot];

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
        __attribute__((no_instrument_function)) static void AddArc(ProfilerBlock::CpuTables &tables, uint32_t from, uint32_t to)
        {
            constexpr uint32_t mask = ProfilerBlock::ArcCapacity - 1;
            uint32_t slot = ((from ^ (to << 3)) >> 2) & mask;

            for (uint32_t probe = 0; probe < Profiler::MaxProbes; probe++)
            {
                ProfilerBlock::Arc &row = tables.Arcs[slot];

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

            tables.DroppedArcs++;
        }

        /** @brief Appends an event. @warning Call with interrupts masked. */
        __attribute__((always_inline, no_instrument_function)) static inline void PushEvent(EventType type, const char *name, uint32_t ticks)
        {
            ProfilerBlock::Header &head = g_srl_prof.Head;

            if (head.EventCount < ProfilerBlock::EventCapacity)
            {
                ProfilerBlock::Event &event = g_srl_prof_buffers.Events[head.EventCount++];
                event.Word = (static_cast<uint32_t>(type) << 28) | (reinterpret_cast<uint32_t>(name) & 0x0FFFFFFFU);
                event.Ticks = ticks;
            }
            else
            {
                head.DroppedEvents++;
            }
        }

        /** @brief Hook body on entry to a function, on one CPU. */
        template <Cpu cpu>
        __attribute__((always_inline, no_instrument_function)) static inline void EnterOn(uint32_t fn, uint32_t callSite)
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            CpuState &state = Profiler::State<cpu>();
            ProfilerBlock::CpuTables &tables = Profiler::Tables<cpu>();
            const uint32_t d = state.Depth;

            if (d < ProfilerBlock::StackCapacity)
            {
                Profiler::AddArc(tables, d != 0 ? state.Stack[d - 1].Fn : callSite, fn);
                Frame &frame = state.Stack[d];
                frame.Fn = fn;
                frame.ChildTicks = 0;
                frame.StartTicks = Profiler::NowOn<cpu>(state); // last, so the callee's time excludes this hook
            }
            else
            {
                tables.DroppedDepth++;
            }

            state.Depth = d + 1;
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Hook body on exit from a function, on one CPU. */
        template <Cpu cpu>
        __attribute__((always_inline, no_instrument_function)) static inline void ExitOn(uint32_t fn)
        {
            // Mask before reading the time, so an interrupt can't add child time after it.
            const uint32_t sr = Profiler::MaskInterrupts();
            CpuState &state = Profiler::State<cpu>();
            const uint32_t now = Profiler::NowOn<cpu>(state);
            ProfilerBlock::CpuTables &tables = Profiler::Tables<cpu>();
            const uint32_t d = state.Depth;

            if (d == 0)
            {
                // Exit of a function entered before Start()/Reset().
                Profiler::RestoreInterrupts(sr);
                return;
            }

            state.Depth = d - 1;

            if (d <= ProfilerBlock::StackCapacity)
            {
                const Frame &frame = state.Stack[d - 1];

                if (frame.Fn != fn)
                {
                    tables.MismatchedExits++;
                    state.Depth = 0;
                }
                else
                {
                    const uint32_t total = now - frame.StartTicks;

                    if (d >= 2)
                    {
                        state.Stack[d - 2].ChildTicks += total;
                    }

                    ProfilerBlock::Func *row = Profiler::FindFunc(tables, fn);

                    if (row != nullptr)
                    {
                        row->Calls++;
                        row->TotalTicks += total;
                        row->SelfTicks += total - frame.ChildTicks;
                    }
                    else
                    {
                        tables.DroppedFuncs++;
                    }
                }
            }

            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Body of __cyg_profile_func_enter. */
        __attribute__((always_inline, no_instrument_function)) static inline void Enter(uint32_t fn, uint32_t callSite)
        {
            if (Profiler::IsMaster())
            {
                if (g_srl_prof.Head.Enabled != 0)
                {
                    Profiler::EnterOn<Cpu::Master>(fn, callSite);
                }
            }
            else
            {
                const ProfilerBlock::Header &head = *Profiler::Uncached(&g_srl_prof.Head);

                if (head.Enabled != 0 && head.SlaveReady != 0)
                {
                    Profiler::EnterOn<Cpu::Slave>(fn, callSite);
                }
            }
        }

        /** @brief Body of __cyg_profile_func_exit. */
        __attribute__((always_inline, no_instrument_function)) static inline void Exit(uint32_t fn)
        {
            if (Profiler::IsMaster())
            {
                if (g_srl_prof.Head.Enabled != 0)
                {
                    Profiler::ExitOn<Cpu::Master>(fn);
                }
            }
            else
            {
                const ProfilerBlock::Header &head = *Profiler::Uncached(&g_srl_prof.Head);

                if (head.Enabled != 0 && head.SlaveReady != 0)
                {
                    Profiler::ExitOn<Cpu::Slave>(fn);
                }
            }
        }

        /** @brief Body of the FRT compare-match interrupt: re-arms the timer, counts the PC. */
        __attribute__((always_inline, no_instrument_function)) static inline void Sample(uint32_t pc)
        {
            // Clear the compare flag: write 0 to it after reading it as 1, keep the others.
            const uint8_t status = Timer::Ftcsr;
            Timer::Ftcsr = status & ~FtcsrOcfa;

            // Next compare in about one interval; the jitter keeps samples from locking onto a
            // loop that runs at a fixed period (xorshift, -4..+3 ticks).
            uint32_t x = Profiler::jitter;
            x ^= (x << 7) & 0xFFFFU;
            x ^= x >> 9;
            x ^= (x << 8) & 0xFFFFU;
            Profiler::jitter = x;

            const uint32_t now = Profiler::Frc();
            uint32_t next = (Profiler::nextCompare + Profiler::sampleInterval - 4 + (x & 7)) & 0xFFFFU;

            // Fell behind (interrupts were masked a long time): restart from now, or the next
            // match would only come after a full FRT wrap.
            if (((next - now) & 0xFFFFU) > Profiler::sampleInterval + 4)
            {
                next = (now + Profiler::sampleInterval) & 0xFFFFU;
            }

            Profiler::nextCompare = next;
            *reinterpret_cast<volatile uint8_t *>(OcraHigh) = static_cast<uint8_t>(next >> 8);
            *reinterpret_cast<volatile uint8_t *>(OcraHigh + 1) = static_cast<uint8_t>(next);

            ProfilerBlock::Header &head = g_srl_prof.Head;

            if (head.Enabled == 0)
            {
                return;
            }

            head.Samples++;

            // Same code at its cache-through address counts in the same bin.
            const uint32_t offset = (pc & 0x07FFFFFFU) - head.HistLow;

            if (offset < (head.HistBins << head.HistShift))
            {
                g_srl_prof_buffers.Histogram[offset >> head.HistShift]++;
            }
            else
            {
                head.SamplesOutside++;
            }

            // While VDP1 draws (current frame not done), count the command it is on.
            if ((*reinterpret_cast<volatile uint16_t *>(Vdp1Edsr) & 0x2U) == 0)
            {
                const uint32_t slot = static_cast<uint32_t>(*reinterpret_cast<volatile uint16_t *>(Vdp1Copr)) >> 2;
                head.Vdp1Samples++;

                if (slot < ProfilerBlock::Vdp1HistCapacity)
                {
                    g_srl_prof_buffers.Vdp1Histogram[slot]++;
                }
            }
        }

        /** @brief Called by SRL::Core's VBlank handler: marks the frame, checks VDP1 finished the last one. */
        __attribute__((no_instrument_function)) static void OnVblank()
        {
            ProfilerBlock::Header &head = g_srl_prof.Head;

            if (head.Enabled == 0 || !Profiler::IsMaster())
            {
                return;
            }

            const uint32_t edsr = *reinterpret_cast<volatile uint16_t *>(Vdp1Edsr);
            const uint32_t sr = Profiler::MaskInterrupts();
            const uint32_t now = Profiler::Now();
            Profiler::PushEvent(EventType::Instant, "VBlank", now);

            // After the frame change, bit 0 says whether the frame VDP1 was drawing got finished.
            // Only once VDP1 has finished a frame at least once: before that it may not draw at all.
            if ((edsr & 0x1U) == 0 && head.Vdp1DrawEnds != 0)
            {
                head.Vdp1Missed++;
                Profiler::PushEvent(EventType::Instant, "VDP1 missed frame", now);
            }

            Profiler::RestoreInterrupts(sr);
        }

        /** @brief SCU sprite draw end routine (called by the BIOS dispatcher, like a function). */
        __attribute__((no_instrument_function)) static void OnSpriteEnd()
        {
            // Write 0 to its status bit only: 1s leave the other pending interrupts alone.
            *reinterpret_cast<volatile uint32_t *>(ScuIst) = ~SpriteEndBit;

            if (g_srl_prof.Head.Enabled == 0)
            {
                return;
            }

            const uint32_t sr = Profiler::MaskInterrupts();
            g_srl_prof.Head.Vdp1DrawEnds++;
            Profiler::PushEvent(EventType::Instant, "VDP1 draw end", Profiler::Now());
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief SCU DMA end routines: mark the end, then run SGL's routine. */
        __attribute__((no_instrument_function)) static void OnDma0End()
        {
            Profiler::Event(EventType::Instant, "SCU DMA 0 end");
            Profiler::sglDmaHandlers[0]();
        }

        __attribute__((no_instrument_function)) static void OnDma1End()
        {
            Profiler::Event(EventType::Instant, "SCU DMA 1 end");
            Profiler::sglDmaHandlers[1]();
        }

        __attribute__((no_instrument_function)) static void OnDma2End()
        {
            Profiler::Event(EventType::Instant, "SCU DMA 2 end");
            Profiler::sglDmaHandlers[2]();
        }

        /** @brief Hooks the SCU sprite draw end and DMA end interrupts.
         *  @details SCU vectors jump to a BIOS dispatcher that calls the routine registered with
         *  System::SetInterruptHandler() as a plain function. SGL registers DMA end routines
         *  but none for sprite draw end, so that one is enabled here.
         */
        __attribute__((no_instrument_function)) static void InstallVdp1AndDmaHooks()
        {
            using Type = System::InterruptType;
            void (*const ours[3])() = {&Profiler::OnDma0End, &Profiler::OnDma1End, &Profiler::OnDma2End};

            for (uint32_t level = 0; level < 3; level++)
            {
                const Type type = static_cast<Type>(Profiler::DmaEndVectors[level]);
                Profiler::sglDmaHandlers[level] = reinterpret_cast<void (*)()>(System::GetInterruptHandler(type));
                System::SetInterruptHandler(type, reinterpret_cast<void *>(ours[level]));
            }

            System::SetInterruptHandler(static_cast<Type>(SpriteEndVector), reinterpret_cast<void *>(&Profiler::OnSpriteEnd));
            System::ChangeInterruptMask(~SpriteEndBit, 0);
        }

#if SRL_PROFILER_SAMPLE_HZ != 0
        /** @brief Sets up the histogram over the program's code and starts the compare-match interrupt. */
        __attribute__((no_instrument_function)) static void StartSampler()
        {
            // Code runs from the start of the program to the end of SGL's slave code (sgl.linker).
            extern char hwram_prog_start[] __asm__("__hwram_prog_start");
            extern char slprog_end[] __asm__("__slprog_end");

            ProfilerBlock::Header &head = g_srl_prof.Head;
            const uint32_t low = reinterpret_cast<uint32_t>(hwram_prog_start) & 0x07FFFFFFU;
            const uint32_t size = (reinterpret_cast<uint32_t>(slprog_end) & 0x07FFFFFFU) - low;
            uint32_t shift = 1; // SH-2 instructions are 2 bytes

            while ((size >> shift) >= ProfilerBlock::HistCapacity)
            {
                shift++;
            }

            head.HistLow = low;
            head.HistShift = shift;
            head.HistBins = (size >> shift) + 1;
            head.SampleHz = SRL_PROFILER_SAMPLE_HZ;
            Profiler::sampleInterval = head.TickHz / SRL_PROFILER_SAMPLE_HZ;

            if (Profiler::sampleInterval < 8)
            {
                Profiler::sampleInterval = 8;
            }

            const uint32_t sr = Profiler::MaskInterrupts();

            // Compare-match vector, keeping the input-capture vector SGL uses in the high byte.
            volatile uint16_t &vcrc = *reinterpret_cast<volatile uint16_t *>(Vcrc);
            vcrc = static_cast<uint16_t>((vcrc & 0xFF00U) | SampleVector);
            SRL::Interrupt::SetHandler(static_cast<SRL::Interrupt::Vector>(SampleVector), &srl_prof_sample_thunk);

            // OCRS = 0: OCRA at OcraHigh. Compare A must not clear the counter (CCLRA = 0),
            // which SRL::Timer and SGL rely on.
            volatile uint8_t &tocr = *reinterpret_cast<volatile uint8_t *>(Tocr);
            tocr = tocr & ~0x10U;
            const uint8_t status = Timer::Ftcsr;
            Timer::Ftcsr = status & ~(FtcsrOcfa | 0x01U);

            Profiler::nextCompare = (Profiler::Frc() + Profiler::sampleInterval) & 0xFFFFU;
            *reinterpret_cast<volatile uint8_t *>(OcraHigh) = static_cast<uint8_t>(Profiler::nextCompare >> 8);
            *reinterpret_cast<volatile uint8_t *>(OcraHigh + 1) = static_cast<uint8_t>(Profiler::nextCompare);
            Timer::TierReg = Timer::TierReg | TierOcia;

            Profiler::RestoreInterrupts(sr);
        }
#endif

        /** @brief Runs on the slave: sets its FRT to the master's rate and opens its tables. */
        class SlaveSetupTask : public SRL::Types::ITask
        {
        protected:
            __attribute__((no_instrument_function)) void Do() override
            {
                // Same clock as the master's (PHI/128); input-capture settings stay as SGL set them.
                Timer::ControlReg = (Timer::ControlReg & 0xfc) | 0x2;

                CpuState &state = Profiler::State<Cpu::Slave>();
                state.Depth = 0;
                state.High = 0;
                state.LastFrc = Profiler::Frc();
                Profiler::Uncached(&g_srl_prof.Head)->SlaveReady = 1;
            }
        };

        /** @brief Writes a report to the DevCart FIFO, keeping a Fletcher-16 checksum of the payload. */
        class Sender
        {
        public:
            bool Ok = true;      ///< False once a byte timed out; later bytes are skipped
            uint32_t Bytes = 0;  ///< Bytes written

            /** @brief Writes one byte outside the checksum. */
            __attribute__((no_instrument_function)) void Raw(uint8_t value)
            {
                if (!this->Ok)
                {
                    return;
                }

                const uint32_t start = Profiler::Frc();

                while (SRL::DevCart::CS0::IsTxeFull())
                {
                    if (static_cast<uint16_t>(Profiler::Frc() - start) > Profiler::SendTimeoutTicks)
                    {
                        this->Ok = false;
                        return;
                    }
                }

                *reinterpret_cast<volatile uint8_t *>(SRL::DevCart::CS0::UsbFifo) = value;
                this->Bytes++;
            }

            /** @brief Writes one payload byte. */
            __attribute__((no_instrument_function)) void Byte(uint8_t value)
            {
                this->Raw(value);
                this->sum1 += value;
                this->sum1 = this->sum1 >= 255 ? this->sum1 - 255 : this->sum1;
                this->sum2 += this->sum1;
                this->sum2 = this->sum2 >= 255 ? this->sum2 - 255 : this->sum2;
            }

            /** @brief Writes a big-endian 32-bit payload word. */
            __attribute__((no_instrument_function)) void Word(uint32_t value)
            {
                this->Byte(static_cast<uint8_t>(value >> 24));
                this->Byte(static_cast<uint8_t>(value >> 16));
                this->Byte(static_cast<uint8_t>(value >> 8));
                this->Byte(static_cast<uint8_t>(value));
            }

            /** @brief Writes 32-bit words (the CPU is big-endian, like the wire format). */
            __attribute__((no_instrument_function)) void Words(const void *data, uint32_t count)
            {
                const uint32_t *words = static_cast<const uint32_t *>(data);

                for (uint32_t i = 0; i < count; i++)
                {
                    this->Word(words[i]);
                }
            }

            /** @brief Starts a packet: sync bytes 0xA5 0x5A, type, payload length. */
            __attribute__((no_instrument_function)) void Begin(PacketType type, uint32_t length)
            {
                this->Raw(0xA5);
                this->Raw(0x5A);
                this->Raw(static_cast<uint8_t>(type));
                this->Raw(static_cast<uint8_t>(length >> 24));
                this->Raw(static_cast<uint8_t>(length >> 16));
                this->Raw(static_cast<uint8_t>(length >> 8));
                this->Raw(static_cast<uint8_t>(length));
                this->sum1 = 0;
                this->sum2 = 0;
            }

            /** @brief Ends a packet with the checksum. */
            __attribute__((no_instrument_function)) void End()
            {
                this->Raw(static_cast<uint8_t>(this->sum2));
                this->Raw(static_cast<uint8_t>(this->sum1));
            }

        private:
            uint32_t sum1 = 0;
            uint32_t sum2 = 0;
        };

        /** @brief Empties one CPU's tables. */
        __attribute__((no_instrument_function)) static void ClearTables(ProfilerBlock::CpuTables &tables)
        {
            tables.DroppedFuncs = 0;
            tables.DroppedArcs = 0;
            tables.DroppedDepth = 0;
            tables.MismatchedExits = 0;

            for (ProfilerBlock::Func &row : tables.Funcs)
            {
                row.Address = 0;
                row.Calls = 0;
                row.TotalTicks = 0;
                row.SelfTicks = 0;
            }

            for (ProfilerBlock::Arc &row : tables.Arcs)
            {
                row.From = 0;
                row.To = 0;
                row.Count = 0;
            }
        }

        /** @brief Used rows of one CPU's tables. */
        struct UsedRows
        {
            uint32_t Funcs = 0;
            uint32_t Arcs = 0;
        };

        /** @brief Counts the used rows of one CPU's tables. */
        __attribute__((no_instrument_function)) static UsedRows CountRows(const ProfilerBlock::CpuTables &tables)
        {
            UsedRows used;

            for (const ProfilerBlock::Func &row : tables.Funcs)
            {
                used.Funcs += row.Address != 0 ? 1 : 0;
            }

            for (const ProfilerBlock::Arc &row : tables.Arcs)
            {
                used.Arcs += row.From != 0 ? 1 : 0;
            }

            return used;
        }

        /** @brief Sends one CPU's drop counters and its first `used` rows.
         *  @details Rows are only ever added (never removed) outside Reset(), so at least that many exist.
         */
        __attribute__((no_instrument_function)) static void SendRows(Sender &out, const ProfilerBlock::CpuTables &tables, const UsedRows &used)
        {
            out.Words(&tables, 4);
            out.Word(used.Funcs);
            out.Word(used.Arcs);

            for (uint32_t i = 0, sent = 0; i < ProfilerBlock::FuncCapacity && sent < used.Funcs; i++)
            {
                if (tables.Funcs[i].Address != 0)
                {
                    out.Words(&tables.Funcs[i], 4);
                    sent++;
                }
            }

            for (uint32_t i = 0, sent = 0; i < ProfilerBlock::ArcCapacity && sent < used.Arcs; i++)
            {
                if (tables.Arcs[i].From != 0)
                {
                    out.Words(&tables.Arcs[i], 3);
                    sent++;
                }
            }
        }

        /** @brief Sends the non-zero histogram bins and takes the sent counts off them. */
        __attribute__((no_instrument_function)) static void SendHistogram(Sender &out)
        {
            const ProfilerBlock::Header &head = g_srl_prof.Head;
            uint32_t used = 0;

            for (uint32_t i = 0; i < head.HistBins; i++)
            {
                used += g_srl_prof_buffers.Histogram[i] != 0 ? 1 : 0;
            }

            out.Begin(PacketType::Histogram, 12 + used * 8);
            out.Word(head.HistLow);
            out.Word(head.HistShift);
            out.Word(used);

            // Bins only grow while sending, so at least `used` are non-zero.
            for (uint32_t i = 0, sent = 0; i < head.HistBins && sent < used; i++)
            {
                const uint32_t count = g_srl_prof_buffers.Histogram[i];

                if (count != 0)
                {
                    out.Word(i);
                    out.Word(count);
                    sent++;

                    // The sampler may have added to the bin since: keep what wasn't sent.
                    const uint32_t sr = Profiler::MaskInterrupts();
                    g_srl_prof_buffers.Histogram[i] -= count;
                    Profiler::RestoreInterrupts(sr);
                }
            }

            out.End();
        }

        /** @brief Sends the VDP1 command slots that got samples, each with its command, and takes
         *  the sent counts off them.
         *  @details The command is read now: SGL rebuilds the table every frame, so a slot holds
         *  the same command only while the scene doesn't change its drawing order.
         */
        __attribute__((no_instrument_function)) static void SendVdp1(Sender &out)
        {
            uint32_t used = 0;

            for (const uint32_t count : g_srl_prof_buffers.Vdp1Histogram)
            {
                used += count != 0 ? 1 : 0;
            }

            out.Begin(PacketType::Vdp1, 4 + used * 40);
            out.Word(used);

            for (uint32_t i = 0, sent = 0; i < ProfilerBlock::Vdp1HistCapacity && sent < used; i++)
            {
                const uint32_t count = g_srl_prof_buffers.Vdp1Histogram[i];

                if (count != 0)
                {
                    out.Word(i);
                    out.Word(count);

                    // VDP1 VRAM is read 16 bits at a time.
                    const volatile uint16_t *command = reinterpret_cast<const volatile uint16_t *>(Vdp1Vram + i * 32);

                    for (uint32_t half = 0; half < 16; half += 2)
                    {
                        out.Word((static_cast<uint32_t>(command[half]) << 16) | command[half + 1]);
                    }

                    sent++;

                    const uint32_t sr = Profiler::MaskInterrupts();
                    g_srl_prof_buffers.Vdp1Histogram[i] -= count;
                    Profiler::RestoreInterrupts(sr);
                }
            }

            out.End();
        }

        /** @brief Called by SRL::Core::Synchronize() once per frame. */
        __attribute__((no_instrument_function)) static void OnFrame()
        {
#if SRL_PROFILER_REPORT_FRAMES != 0
            // Also while stopped: what was recorded since the last report still has to go out.
            if (++Profiler::framesSinceReport >= SRL_PROFILER_REPORT_FRAMES)
            {
                Profiler::framesSinceReport = 0;
                Profiler::Report();
            }
#endif

            if (Profiler::waitingForReader && g_srl_prof.Head.Reports != 0)
            {
                const uint32_t sr = Profiler::MaskInterrupts();
                Profiler::resetTicks = Profiler::Now();
                Profiler::RestoreInterrupts(sr);
                Profiler::Start();
            }

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

        /** @brief Sets up the header, measures the hook cost, sets up the slave and the sampler.
         *  @details With streaming, recording starts once the first report gets through, so
         *  loading isn't recorded before the PC listens. Without it, recording starts right away.
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
            head.Version = 4;
            head.SelfAddress = reinterpret_cast<uint32_t>(&g_srl_prof);
            head.TickHz = Tickstamp::GetSecondsDivider();
            head.FuncCapacity = ProfilerBlock::FuncCapacity;
            head.ArcCapacity = ProfilerBlock::ArcCapacity;
            head.StackCapacity = ProfilerBlock::StackCapacity;
            head.EventCapacity = ProfilerBlock::EventCapacity;
            head.HistCapacity = ProfilerBlock::HistCapacity;
            head.Vdp1HistCapacity = ProfilerBlock::Vdp1HistCapacity;
            head.ReportFrames = SRL_PROFILER_REPORT_FRAMES;

            head.Enabled = 1;
            head.HookCostQ16 = Profiler::MeasureHookCost();
            Profiler::Reset();
#if SRL_PROFILER_REPORT_FRAMES != 0
            head.Enabled = 0;
            Profiler::waitingForReader = true;
#endif

            // Bounded wait: if the slave never runs the task, only its calls go unrecorded.
            static SlaveSetupTask slaveSetup;
            SRL::Slave::ExecuteOnSlave(slaveSetup);

            for (uint32_t wait = 0; !slaveSetup.IsDone() && wait < 5000000U; wait++)
            {
            }

            Profiler::InstallVdp1AndDmaHooks();

#if SRL_PROFILER_SAMPLE_HZ != 0
            Profiler::StartSampler();
#endif
        }

    public:
        /** @brief Starts recording, keeping what was recorded so far. Also cancels waiting for a reader. */
        __attribute__((no_instrument_function)) static void Start()
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            Profiler::waitingForReader = false;
            Profiler::masterState.Depth = 0;
            Profiler::Uncached(&Profiler::slaveState)->Depth = 0;
            g_srl_prof.Head.Enabled = 1;
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Stops recording; the data stays readable. Also cancels waiting for a reader. */
        __attribute__((no_instrument_function)) static void Stop()
        {
            Profiler::waitingForReader = false;
            g_srl_prof.Head.Enabled = 0;
        }

        /** @brief Clears all recorded data and counters. Recording state is unchanged.
         *  @warning Clears the slave's tables too: don't call it while the slave runs profiled code.
         */
        __attribute__((no_instrument_function)) static void Reset()
        {
            const uint32_t sr = Profiler::MaskInterrupts();
            ProfilerBlock::Header &head = g_srl_prof.Head;

            // Loops, not "= {}": that may build a 20 KB temporary on the small master stack.
            Profiler::ClearTables(g_srl_prof.Master);
            Profiler::ClearTables(*Profiler::Uncached(&g_srl_prof.Slave));

            for (uint32_t &bin : g_srl_prof_buffers.Histogram)
            {
                bin = 0;
            }

            for (uint32_t &slot : g_srl_prof_buffers.Vdp1Histogram)
            {
                slot = 0;
            }

            head.Frames = 0;
            head.ElapsedTicks = 0;
            head.EventCount = 0;
            head.DroppedEvents = 0;
            head.Samples = 0;
            head.SamplesOutside = 0;
            head.Vdp1DrawEnds = 0;
            head.Vdp1Missed = 0;
            head.Vdp1Samples = 0;
            Profiler::masterState.Depth = 0;
            Profiler::Uncached(&Profiler::slaveState)->Depth = 0;
            Profiler::resetTicks = Profiler::Now();
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Records a timeline event, if recording on the master CPU.
         *  @param type Begin, End or Instant
         *  @param name A string literal: only its address is stored, the PC reads the text from the ELF
         */
        __attribute__((no_instrument_function)) static void Event(EventType type, const char *name)
        {
            if (g_srl_prof.Head.Enabled == 0 || !Profiler::IsMaster())
            {
                return;
            }

            const uint32_t sr = Profiler::MaskInterrupts();
            Profiler::PushEvent(type, name, Profiler::Now());
            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Sends the timeline events, the used table rows of both CPUs and the PC
         *  histogram over the DevCart.
         *  @details SRL::Core::Synchronize() calls it every SRL_PROFILER_REPORT_FRAMES frames.
         *  Sent events and
         *  histogram counts are removed; the function tables keep accumulating. The time the
         *  report took is recorded as a "SRL profiler report" scope. If nobody reads the FIFO
         *  for about 10 ms, the report is abandoned and counted in DroppedReports.
         */
        __attribute__((no_instrument_function)) static void Report()
        {
            ProfilerBlock::Header &head = g_srl_prof.Head;

            if (!SRL::DevCart::CS0::IsConnected())
            {
                head.DroppedReports++;
                return;
            }

            // Snapshot what to send; interrupts keep running (and recording) while it goes out.
            uint32_t sr = Profiler::MaskInterrupts();
            const uint32_t start = Profiler::Now();
            const uint32_t eventCount = head.EventCount;
            const ProfilerBlock::Header snapshot = head;
            Profiler::RestoreInterrupts(sr);

            const ProfilerBlock::CpuTables &slave = *Profiler::Uncached(&g_srl_prof.Slave);
            const UsedRows masterRows = Profiler::CountRows(g_srl_prof.Master);
            const UsedRows slaveRows = Profiler::CountRows(slave);

            Sender out;
            out.Begin(PacketType::Events, eventCount * sizeof(ProfilerBlock::Event));
            out.Words(g_srl_prof_buffers.Events, eventCount * 2);
            out.End();

            out.Begin(PacketType::Tables, sizeof(ProfilerBlock::Header) + 2 * 24 +
                                              (masterRows.Funcs + slaveRows.Funcs) * sizeof(ProfilerBlock::Func) +
                                              (masterRows.Arcs + slaveRows.Arcs) * sizeof(ProfilerBlock::Arc));
            out.Words(&snapshot, sizeof(ProfilerBlock::Header) / 4);
            Profiler::SendRows(out, g_srl_prof.Master, masterRows);
            Profiler::SendRows(out, slave, slaveRows);
            out.End();

            if (head.SampleHz != 0)
            {
                Profiler::SendHistogram(out);
                Profiler::SendVdp1(out);
            }

            sr = Profiler::MaskInterrupts();
            const uint32_t end = Profiler::Now();

            // Keep the events recorded while sending.
            const uint32_t kept = head.EventCount - eventCount;

            for (uint32_t i = 0; i < kept; i++)
            {
                g_srl_prof_buffers.Events[i] = g_srl_prof_buffers.Events[eventCount + i];
            }

            head.EventCount = kept;

            if (out.Ok)
            {
                head.Reports++;
                head.ReportBytes = out.Bytes;
                head.ReportTicks = end - start;
            }
            else
            {
                head.DroppedReports++;
            }

            if (head.Enabled != 0)
            {
                Profiler::PushEvent(EventType::Begin, "SRL profiler report", start);
                Profiler::PushEvent(EventType::End, "SRL profiler report", end);
            }

            Profiler::RestoreInterrupts(sr);
        }

        /** @brief Cost of one enter + exit hook pair measured at Init(), in ticks (16.16). */
        __attribute__((no_instrument_function)) static uint32_t GetHookCostQ16()
        {
            return g_srl_prof.Head.HookCostQ16;
        }
    };

    /** @brief Records a Begin event now and the matching End event when it goes out of scope. */
    class ProfilerScope final
    {
    public:
        /** @param name A string literal */
        __attribute__((always_inline, no_instrument_function)) explicit ProfilerScope(const char *name) : name(name)
        {
            Profiler::Event(Profiler::EventType::Begin, name);
        }

        __attribute__((always_inline, no_instrument_function)) ~ProfilerScope()
        {
            Profiler::Event(Profiler::EventType::End, this->name);
        }

    private:
        const char *name;
    };
}

/// @cond Internal
#define SRL_PROF_CAT_INNER(a, b) a##b
#define SRL_PROF_CAT(a, b) SRL_PROF_CAT_INNER(a, b)
/// @endcond

/** @brief Times the rest of the enclosing block as a timeline slice. @param name A string literal */
#define SRL_PROF_SCOPE(name) SRL::ProfilerScope SRL_PROF_CAT(srlProfScope, __LINE__)(name)

/** @brief Marks a point in time on the timeline. @param name A string literal */
#define SRL_PROF_MARK(name) SRL::Profiler::Event(SRL::Profiler::EventType::Instant, name)

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

    /** @brief Called by srl_prof_sample_thunk with the interrupted PC. */
    __attribute__((weak, no_instrument_function)) void srl_prof_sample(uint32_t pc)
    {
        SRL::Profiler::Sample(pc);
    }
}

// FRT compare-match entry. An interrupt pushes SR then PC, and leaves PR alone: save what a C
// call may clobber (r0-r7, PR, MACH, MACL; rte restores SR), pass the PC, return with rte.
// .weak: every translation unit that includes this header assembles a copy.
__asm__(
    ".weak _srl_prof_sample_thunk\n"
    ".global _srl_prof_sample_thunk\n"
    ".align 2\n"
    "_srl_prof_sample_thunk:\n"
    "sts.l pr, @-r15\n"
    "mov.l r0, @-r15\n"
    "mov.l r1, @-r15\n"
    "mov.l r2, @-r15\n"
    "mov.l r3, @-r15\n"
    "mov.l r4, @-r15\n"
    "mov.l r5, @-r15\n"
    "mov.l r6, @-r15\n"
    "mov.l r7, @-r15\n"
    "sts.l mach, @-r15\n"
    "sts.l macl, @-r15\n"
    "mov.l @(44, r15), r4\n" // r4 = interrupted PC: 11 words pushed above it
    "mov.l 1f, r0\n"
    "jsr @r0\n"
    "nop\n"
    "lds.l @r15+, macl\n"
    "lds.l @r15+, mach\n"
    "mov.l @r15+, r7\n"
    "mov.l @r15+, r6\n"
    "mov.l @r15+, r5\n"
    "mov.l @r15+, r4\n"
    "mov.l @r15+, r3\n"
    "mov.l @r15+, r2\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r0\n"
    "lds.l @r15+, pr\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long _srl_prof_sample\n"
);

#else

/** @brief Times the rest of the enclosing block (no-op: SRL_PROFILER is off). */
#define SRL_PROF_SCOPE(name) ((void)0)

/** @brief Marks a point in time on the timeline (no-op: SRL_PROFILER is off). */
#define SRL_PROF_MARK(name) ((void)0)

#endif

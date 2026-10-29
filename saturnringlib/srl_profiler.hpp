#pragma once

/** @file srl_profiler.hpp
 *  @brief CPU profiler for the master SH-2: per-function times and a per-frame event timeline.
 *
 *  @details Built when the makefile sets `SRL_PROFILER = 1`; otherwise only the
 *  SRL_PROF_SCOPE() / SRL_PROF_MARK() macros exist, and they compile to nothing.
 *
 *  Function times: files listed in `SRL_PROFILE_SOURCES` (default: all of `SOURCES`)
 *  are compiled with `-finstrument-functions`, so GCC calls the hooks below on entry
 *  and exit of each of their functions. The hooks keep a shadow call stack and add each
 *  call's time into two tables: one row per function (calls, total ticks, own ticks)
 *  and one row per caller -> callee pair (call count).
 *
 *  Timeline: SRL_PROF_SCOPE("name") and SRL_PROF_MARK("name") record timestamped events
 *  in a ring buffer, as do VBlank and the wait inside SRL::Core::Synchronize().
 *
 *  Everything lives in the global `g_srl_prof`. Two ways to get it out:
 *  - GDB: `source tools/scripts/srl_prof.gdb`, then `srl-prof-dump prof.bin`;
 *  - streaming: with `SRL_PROFILER_REPORT_FRAMES = N`, Report() sends it over the DevCart
 *    every N frames (never while GDB is connected); `tools/scripts/srl_prof_capture.py`
 *    records it through `ftx -g`.
 *  Then read it with `tools/scripts/srl_prof.py`.
 *
 *  Times are FRT ticks (PHI/128, about 4.8 us).
 *  @note Only the master CPU is profiled; calls and events on the slave are ignored.
 *  @note Time spent halted in GDB counts too: a breakpoint inside a profiled
 *  function inflates it.
 */

#ifdef SRL_PROFILER

#include "srl_timer.hpp"
#include "srl_devcart.hpp"
#ifdef DEBUG
    #include "srl_gdbstub.hpp"
#endif
#include <cstdint>

#ifndef SRL_PROFILER_MAX_FUNCS
    /** @brief Function table rows (power of two). */
    #define SRL_PROFILER_MAX_FUNCS 512
#endif

#ifndef SRL_PROFILER_MAX_ARCS
    /** @brief Caller -> callee table rows (power of two). */
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
        static constexpr uint32_t EventCapacity = SRL_PROFILER_MAX_EVENTS;

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

        /** @brief 96-byte header. */
        struct Header
        {
            char Magic[4];            ///< "SRLP"
            uint32_t Version;         ///< Layout version (2)
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
            uint32_t DroppedFuncs;    ///< Exits not recorded: function table full
            uint32_t DroppedArcs;     ///< Calls not recorded: arc table full
            uint32_t DroppedDepth;    ///< Calls deeper than StackCapacity, not timed
            uint32_t MismatchedExits; ///< Exits that did not match the shadow stack (stack was reset)
            uint32_t EventCount;      ///< Events in Events[] (since the last report)
            uint32_t DroppedEvents;   ///< Events not recorded: buffer full
            uint32_t ReportFrames;    ///< SRL_PROFILER_REPORT_FRAMES
            uint32_t Reports;         ///< Reports sent completely
            uint32_t DroppedReports;  ///< Reports skipped (no DevCart) or cut short (nobody reading)
            uint32_t ReportBytes;     ///< Size of the last complete report
            uint32_t ReportTicks;     ///< Time the last complete report took to send
            uint32_t Reserved;
        };

        Header Head;
        Func Funcs[FuncCapacity];
        Arc Arcs[ArcCapacity];
        Event Events[EventCapacity];
    };

    static_assert(sizeof(ProfilerBlock::Header) == 96, "profiler header must stay 96 bytes");
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
    /** @brief Controls the profiler (see srl_profiler.hpp).
     *  @details SRL::Core::Initialize() calls Init(), which starts recording.
     */
    class Profiler final
    {
        /// @cond Internal
        friend void ::__cyg_profile_func_enter(void *, void *);
        friend void ::__cyg_profile_func_exit(void *, void *);
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
        /** @brief One shadow-stack frame. */
        struct Frame
        {
            uint32_t Fn;
            uint32_t StartTicks;
            uint32_t ChildTicks;
        };

        /** @brief Report packet types. */
        enum class PacketType : uint8_t
        {
            Events = 1, ///< Event records
            Tables = 2  ///< Header, used function rows, used arc rows
        };

        inline static Frame stack[ProfilerBlock::StackCapacity];
        inline static uint32_t depth = 0;
        inline static uint32_t masterVbr = 0;
        inline static uint32_t resetTicks = 0;
        inline static uint32_t framesSinceReport = 0;

        /** @brief Rows probed before a lookup gives up and counts the event as dropped. */
        static constexpr uint32_t MaxProbes = 8;

        /** @brief FTCSR overflow flag. */
        static constexpr uint8_t FtcsrOvf = 0x02;

        /** @brief Longest wait for room in the DevCart FIFO before a report is abandoned (~10 ms). */
        static constexpr uint16_t SendTimeoutTicks = 2000;

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

        /** @brief The 16-bit FRT counter. */
        __attribute__((always_inline, no_instrument_function)) static inline uint32_t Frc()
        {
            // High byte first: reading it latches the low byte.
            const uint32_t high = Timer::FrchReg;
            return (high << 8) | Timer::FrclReg;
        }

        /** @brief Current time in ticks: (overflow count << 16) | FRC, truncated to 32 bits.
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

        /** @brief Appends an event. @warning Call with interrupts masked. */
        __attribute__((always_inline, no_instrument_function)) static inline void PushEvent(EventType type, const char *name, uint32_t ticks)
        {
            ProfilerBlock::Header &head = g_srl_prof.Head;

            if (head.EventCount < ProfilerBlock::EventCapacity)
            {
                ProfilerBlock::Event &event = g_srl_prof.Events[head.EventCount++];
                event.Word = (static_cast<uint32_t>(type) << 28) | (reinterpret_cast<uint32_t>(name) & 0x0FFFFFFFU);
                event.Ticks = ticks;
            }
            else
            {
                head.DroppedEvents++;
            }
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

#if SRL_PROFILER_REPORT_FRAMES != 0
            if (++Profiler::framesSinceReport >= SRL_PROFILER_REPORT_FRAMES)
            {
                Profiler::framesSinceReport = 0;
                Profiler::Report();
            }
#endif
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
            head.Version = 2;
            head.SelfAddress = reinterpret_cast<uint32_t>(&g_srl_prof);
            head.TickHz = Tickstamp::GetSecondsDivider();
            head.FuncCapacity = ProfilerBlock::FuncCapacity;
            head.ArcCapacity = ProfilerBlock::ArcCapacity;
            head.StackCapacity = ProfilerBlock::StackCapacity;
            head.EventCapacity = ProfilerBlock::EventCapacity;
            head.ReportFrames = SRL_PROFILER_REPORT_FRAMES;

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
            head.EventCount = 0;
            head.DroppedEvents = 0;
            Profiler::depth = 0;
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

        /** @brief Sends the timeline events and the used table rows over the DevCart.
         *  @details SRL::Core::Synchronize() calls it every SRL_PROFILER_REPORT_FRAMES frames.
         *  Does nothing while GDB is connected (the bytes would reach GDB). Sent events are
         *  removed from the buffer; the tables keep accumulating. The time the report took
         *  is recorded as a "SRL profiler report" scope. If nobody reads the FIFO for about
         *  10 ms, the report is abandoned and counted in DroppedReports.
         */
        __attribute__((no_instrument_function)) static void Report()
        {
            ProfilerBlock::Header &head = g_srl_prof.Head;

#ifdef DEBUG
            if (SRL::GDBStub::IsConnected())
            {
                return;
            }
#endif

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

            uint32_t funcCount = 0;
            uint32_t arcCount = 0;

            for (const ProfilerBlock::Func &row : g_srl_prof.Funcs)
            {
                funcCount += row.Address != 0 ? 1 : 0;
            }

            for (const ProfilerBlock::Arc &row : g_srl_prof.Arcs)
            {
                arcCount += row.From != 0 ? 1 : 0;
            }

            Sender out;
            out.Begin(PacketType::Events, eventCount * sizeof(ProfilerBlock::Event));
            out.Words(g_srl_prof.Events, eventCount * 2);
            out.End();

            out.Begin(PacketType::Tables, sizeof(ProfilerBlock::Header) + 8 +
                                              funcCount * sizeof(ProfilerBlock::Func) +
                                              arcCount * sizeof(ProfilerBlock::Arc));
            out.Words(&snapshot, sizeof(ProfilerBlock::Header) / 4);
            out.Word(funcCount);
            out.Word(arcCount);

            // Rows are only ever added (never removed) outside Reset(): send the first ones counted.
            for (uint32_t i = 0, sent = 0; i < ProfilerBlock::FuncCapacity && sent < funcCount; i++)
            {
                if (g_srl_prof.Funcs[i].Address != 0)
                {
                    out.Words(&g_srl_prof.Funcs[i], 4);
                    sent++;
                }
            }

            for (uint32_t i = 0, sent = 0; i < ProfilerBlock::ArcCapacity && sent < arcCount; i++)
            {
                if (g_srl_prof.Arcs[i].From != 0)
                {
                    out.Words(&g_srl_prof.Arcs[i], 3);
                    sent++;
                }
            }

            out.End();

            sr = Profiler::MaskInterrupts();
            const uint32_t end = Profiler::Now();

            // Keep the events recorded while sending.
            const uint32_t kept = head.EventCount - eventCount;

            for (uint32_t i = 0; i < kept; i++)
            {
                g_srl_prof.Events[i] = g_srl_prof.Events[eventCount + i];
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
}

#else

/** @brief Times the rest of the enclosing block (no-op: SRL_PROFILER is off). */
#define SRL_PROF_SCOPE(name) ((void)0)

/** @brief Marks a point in time on the timeline (no-op: SRL_PROFILER is off). */
#define SRL_PROF_MARK(name) ((void)0)

#endif

#pragma once

#include <srl_devcart.hpp>
#include <srl_interrupt.hpp>
#include <srl_system.hpp>
#include <srl_log.hpp>
#include <srl_slave.hpp>
#include <cstdint>
#include <cstddef>

extern "C"
{
    // Entry points shared between SRL::GDBStub and the hand-written __asm__
    // thunks at the bottom of this file. They are free extern "C" functions
    // (not class members) because the thunks reference them by fixed,
    // unmangled symbol name; each C++ one is a friend of SRL::GDBStub.

    /**
     * @brief Slave-side breakpoint handler, called by srl_gdbstub_slave_illegal_thunk on the slave SH-2.
     */
    void slave_breakpoint_handler();

    /**
     * @brief Fixed-symbol trampoline into SRL::GDBStub::process_commands(), called by srl_gdbstub_exception_thunk.
     */
    void srl_gdbstub_process_commands();

    /**
     * @brief Shared master-side exception entry: saves the full SH-2 context, runs the RSP loop, restores and returns.
     */
    void srl_gdbstub_exception_thunk();

    /**
     * @brief Illegal-instruction family entry: tags the stop as SIGILL, then falls into srl_gdbstub_exception_thunk.
     * @details Each exception family gets its own tiny entry stub because every
     * SH-2 exception vector lands at a fixed address with no argument-passing
     * convention and no on-chip "cause" register (unlike e.g. SH-3/4's EXPEVT),
     * so this is the only way to tell GDB which exception family fired.
     */
    void srl_gdbstub_illegal_thunk();

    /**
     * @brief Address-error family entry: tags the stop as SIGBUS, then falls into srl_gdbstub_exception_thunk.
     */
    void srl_gdbstub_addrerr_thunk();

    /**
     * @brief User Break Controller entry: flags the stop as a UBC hit, then falls into srl_gdbstub_exception_thunk.
     */
    void srl_gdbstub_ubc_thunk();

    /**
     * @brief Slave-side illegal-instruction entry: saves the slave context and calls slave_breakpoint_handler().
     */
    void srl_gdbstub_slave_illegal_thunk();
}

namespace SRL
{
    /**
     * @brief A basic GDB Remote Serial Protocol stub for the Sega Saturn using the DevCart.
     * @details Software breakpoints are a 0xFFFF (illegal instruction) patch caught
     * on the master's Illegal Instruction vector. Call Init() once at startup and
     * Poll() regularly (SRL::Core does both in DEBUG builds).
     */
    class GDBStub
    {
    public:
        /**
         * @brief Represents the SH-2 CPU register state.
         * @details This exactly matches the register order expected by GDB's SH
         * architecture: R0-R15, PC, PR, GBR, VBR, MACH, MACL, SR. The __asm__
         * thunks also depend on these field offsets (r[N] at N*4, pc at 64).
         */
        struct SH2Context
        {
            /** @brief General purpose registers R0-R15 (R15 is the stack pointer) */
            uint32_t r[16];

            /** @brief Program counter */
            uint32_t pc;

            /** @brief Procedure register (return address) */
            uint32_t pr;

            /** @brief Global base register */
            uint32_t gbr;

            /** @brief Vector base register */
            uint32_t vbr;

            /** @brief Multiply-accumulate register, high word */
            uint32_t mach;

            /** @brief Multiply-accumulate register, low word */
            uint32_t macl;

            /** @brief Status register */
            uint32_t sr;
        };

    private:
        friend void ::slave_breakpoint_handler(void);
        friend void ::srl_gdbstub_process_commands(void);

        /**
         * @brief Master SH-2 register state saved by the exception thunk at the current stop.
         * @details __asm__-named so the thunks can address it by a fixed symbol.
         */
        __attribute__((used)) inline static SH2Context g_ctx __asm__("srl_gdbstub_ctx") = {};

        /**
         * @brief Slave SH-2 register state saved by the slave thunk at its last breakpoint.
         * @details Read by GDB through 'p'/'P' registers 23..45 and `monitor regs slave`.
         */
        __attribute__((used)) inline static SH2Context g_slave_ctx __asm__("srl_gdbstub_slave_ctx") = {};

        // State grouped by topic below. Members referenced by name from the
        // __asm__ thunks at the bottom of this file (g_ctx, g_slave_ctx,
        // g_last_stop_signal, g_ubc_hit) stay standalone: each needs its own
        // fixed symbol, which a struct field cannot have.
        //
        // None of these nested structs use default member initializers: GCC
        // rejects them on a nested struct whose instance is a static member of
        // the still-incomplete enclosing class. Defaults are given in each
        // instance's initializer instead.

        /** @brief Number of times the master exception thunk has been entered (diagnostic) */
        inline static volatile uint32_t g_exception_thunk_count = 0;

        /**
         * @brief GDB Remote Serial Protocol session state.
         */
        struct SessionStatus
        {
            /** @brief Set on any valid RSP packet received */
            volatile bool HasConnection;

            /** @brief Set only after the qSupported exchange */
            volatile bool HandshakeDone;

            /** @brief Number of RSP packets received */
            volatile uint32_t CommandCount;

            /** @brief Most recently received RSP packet (truncated to 63 chars) */
            char LastCommand[64];
        };

        /** @brief Current GDB session state */
        inline static SessionStatus g_session = {false, false, 0, {}};

        /**
         * @brief Last "monitor <text>" command received via qRcmd, and how many
         * have arrived.
         * @details User code polls GetMonitorCommandCount() and dispatches on
         * GetLastMonitorCommand(), e.g. `(gdb) monitor crash illegal`.
         */
        struct MonitorStatus
        {
            /** @brief Text of the most recent non-built-in monitor command */
            char LastCommand[64];

            /** @brief Number of non-built-in monitor commands received */
            volatile uint32_t CommandCount;
        };

        /** @brief Current monitor command state */
        inline static MonitorStatus g_monitor = {{}, 0};

        /** @brief Whether InstallExceptionHandlers() has patched the master's vector table */
        inline static bool g_handlers_installed = false;

        /**
         * @brief DevCart USB data transfer state: RX pushback byte and traffic counters.
         */
        struct TransferStatus
        {
            /** @brief Byte pushed back to be returned by the next __gdb_getc(), or -1 if none */
            int UngetChar;

            /** @brief Incremented each time the stub reads a byte from DevCart RX */
            volatile uint32_t RxDetectCount;

            /** @brief Incremented each time the stub writes a byte to DevCart TX */
            volatile uint32_t TxByteCount;

            /** @brief Incremented when Poll() handles RX data itself, outside of an exception */
            volatile uint32_t PollFallbackCount;
        };

        /** @brief Current data transfer state */
        inline static TransferStatus g_transfer = {-1, 0, 0, 0};

        /**
         * @brief DevCart (USB cartridge) link status, as last observed by Init() / Poll().
         */
        struct DevCartStatus
        {
            /** @brief Cart detected at Init() (CS0 USB_FLAGS readable) */
            bool Ready;

            /** @brief USB port currently available, refreshed on every Poll() */
            bool PortAvailable;
        };

        /** @brief Current DevCart link status */
        inline static DevCartStatus g_devcart = {false, false};
        
        /**
         * @brief POSIX signal of the last stop: 5=SIGTRAP, 2=SIGINT, 4=SIGILL, 10=SIGBUS.
         * @details __asm__-named so the tag thunks can write it by a fixed symbol.
         */
        __attribute__((used)) inline static volatile uint8_t g_last_stop_signal __asm__("srl_gdbstub_last_stop_signal") = 5;

        /** @brief Set to 1 by srl_gdbstub_ubc_thunk when the current stop is a UBC (hbreak/watchpoint) hit */
        __attribute__((used)) inline static volatile uint8_t g_ubc_hit __asm__("srl_gdbstub_ubc_hit") = 0;

        /**
         * @brief How the current debug stop came about (beyond g_last_stop_signal,
         * which the thunks write by symbol).
         */
        struct StopStatus
        {
            /**
             * @brief g_ctx is a synthetic snapshot, not a real exception frame.
             * @details Set by snapshot_polling_context() when Poll() handles a packet while
             * the target runs: r0-r14 are zero and pc/pr are not a resumable instruction, so
             * handle_gdb_step() must not place a step trap from it. Cleared on resume.
             */
            volatile bool CtxIsFake;

            /** @brief Set during PC adjustment if we hit a GDB swbreak */
            bool WasSwBreak;

            /**
             * @brief Signal to report for this stop instead of g_last_stop_signal, or 0 for none.
             * @details Every stop that goes through Break() executes 0xFFFF, so the
             * illegal-instruction thunk always tags it SIGILL. Callers that know
             * better set this first: Poll() sets SIGINT (2) for Ctrl-C and slave
             * breakpoint stops, and Break() itself sets SIGTRAP (5) if nothing set
             * it already. A genuine illegal instruction leaves it at 0 and is still
             * reported as SIGILL. Cleared on resume.
             */
            volatile uint8_t SignalOverride;

            /** @brief The current stop is a hit of GDB's UBC breakpoint/watchpoint (reported as hwbreak/watch) */
            bool WasUbcHit;
        };

        /** @brief Current stop state */
        inline static StopStatus g_stop = {false, false, 0, false};

        /**
         * @brief Signal to report to GDB for the current stop.
         * @return g_stop.SignalOverride if set, otherwise g_last_stop_signal
         */
        inline static uint8_t current_stop_signal()
        {
            return g_stop.SignalOverride != 0 ? g_stop.SignalOverride : g_last_stop_signal;
        }

        /**
         * @brief Slave SH-2 halt state, shared between both CPUs through Work RAM.
         */
        struct SlaveStatus
        {
            /**
             * @brief Slave halted at a breakpoint.
             * @details Set by slave_breakpoint_handler() (see InstallSlaveExceptionHandler()).
             * Poll() turns it into a master stop via Break(), like Ctrl-C; the slave spins
             * on Resume meanwhile.
             */
            volatile bool Stopped;

            /**
             * @brief Master sets this (via handle_gdb_continue()/handle_gdb_step(),
             * same as any other resume) to release a slave halted in Stopped.
             */
            volatile bool Resume;

            /** @brief Whether InstallSlaveExceptionHandler() has run on the slave */
            volatile bool HandlersInstalled;

            /**
             * @brief Master-only: set by Poll() when it reports a slave stop to GDB.
             * SlaveReleaseGuard only releases a reported stop (or any, once the session ends);
             * an unreported one stays parked for the next Poll() to report.
             */
            bool Reported;
        };

        /** @brief Current slave halt state */
        inline static SlaveStatus g_slave = {false, false, false, false};

        /**
         * @brief Cache-through view of a g_slave flag, for reads of a value the other CPU writes.
         * @details The two SH-2 caches don't snoop each other: a CPU polling a flag through
         * its cache keeps seeing its own stale copy (a tight spin never evicts the line),
         * so the slave would never see Resume.
         * @param flag A g_slave member
         * @return The same flag through the 0x20000000 cache-through address
         */
        inline static volatile bool &Uncached(volatile bool &flag)
        {
            return *reinterpret_cast<volatile bool *>(reinterpret_cast<uint32_t>(&flag) | 0x20000000U);
        }

        /** @brief Number of times the slave's illegal-instruction thunk has fired (diagnostic) */
        inline static volatile uint32_t g_slave_bp_count = 0;

        /**
         * @brief Opcode patched in for software breakpoints and step traps (Illegal Instruction).
         * @details Avoids TRAPA, whose vectors (32-63) SGL frequently overwrites
         * for its own CD-ROM and BIOS system calls.
         */
        static constexpr uint16_t SoftwareBreakInstruction = 0xFFFFU;

        /** @brief Number of software breakpoint slots */
        static constexpr size_t MaxSoftwareBreakpoints = 32;

        /** @brief Maximum RSP packet payload length; advertised to GDB as PacketSize (this + 1) */
        static constexpr size_t kPacketDataMax = 399U;

        /**
         * @brief Tracks the state of a single software breakpoint.
         */
        struct SoftwareBreakpoint
        {
            /** @brief Address of the patched instruction */
            uint32_t address;

            /** @brief Instruction that was at address before it was replaced by SoftwareBreakInstruction */
            uint16_t original_instruction;

            /** @brief Whether this slot is in use */
            bool active;
        };

        /**
         * @brief Software breakpoint slots and UBC ownership
         * (everything behind the Z0-Z4 / z0-z4 packets).
         */
        struct BreakpointStatus
        {
            /** @brief Software breakpoint slots (Z0) */
            SoftwareBreakpoint Software[MaxSoftwareBreakpoints];

            /**
             * @brief Whether a GDB watchpoint/hwbreak currently owns UBC channel A (Z1-Z4).
             * @warning The only record of who owns the one UBC channel the stub uses: code that
             * writes BARA/BAMRA/BBRA/BRCR itself silently steals a GDB watchpoint. Go through
             * install_hardware_watchpoint()/remove_hardware_watchpoint() instead.
             */
            bool UbcChannelAActive;

            /** @brief Z-packet type channel A was armed with (1 = hbreak, 2/3/4 = write/read/access watch) */
            uint32_t UbcType;

            /** @brief Address channel A was armed with */
            uint32_t UbcAddress;
        };

        /** @brief Current breakpoint state */
        inline static BreakpointStatus g_breakpoints = {{}, false, 0, 0};

        // --- Utility Functions ---

        /**
         * @brief Converts a hex character to its integer value.
         * @param ch Character to convert ('0'-'9', 'a'-'f' or 'A'-'F')
         * @return Value 0-15, or -1 if ch is not a hex digit
         */
        inline static int hex(char ch)
        {
            if (ch >= 'a' && ch <= 'f')
                return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F')
                return ch - 'A' + 10;
            if (ch >= '0' && ch <= '9')
                return ch - '0';
            return -1;
        }

        /**
         * @brief Converts a 4-bit integer to its hex character equivalent.
         * @param v Value to convert (only the low 4 bits are used)
         * @return Lowercase hex digit character
         */
        inline static char hexchar(int v)
        {
            v &= 0xf;
            return v < 10 ? '0' + v : 'a' + v - 10;
        }

        /** @brief qSupported PacketSize feature string, must match kPacketDataMax + 1
         * @note GDB parses PacketSize as hex: "400" would mean 1024, making GDB request 512-byte 'm' reads the handler rejects.
         */
        static constexpr char kPacketSizeStr[] = "PacketSize=190";
        static_assert(kPacketDataMax + 1U == 0x190, "Update kPacketSizeStr (hex) if kPacketDataMax changes");

        /**
         * @brief Decodes a hex string into memory, one byte store at a time.
         * @param buf Hex string to decode (2 characters per byte)
         * @param mem Destination buffer
         * @param count Number of bytes to decode
         * @return Pointer to the character following the decoded hex string, or nullptr on failure.
         */
        inline static const char *hex2mem(const char *buf, uint8_t *mem, int count)
        {
            // Validate all characters first to prevent partial memory corruption
            for (int i = 0; i < count * 2; i++)
            {
                if (hex(buf[i]) < 0)
                    return nullptr;
            }

            for (int i = 0; i < count; i++)
            {
                int h1 = hex(*buf++);
                int h2 = hex(*buf++);
                *mem++ = (h1 << 4) | h2;
            }
            return buf;
        }

        /**
         * @brief Decodes a hex string into memory, using 32-/16-bit stores where
         * address and remaining length allow, falling back to byte stores only
         * for the unaligned remainder.
         * @details VDP RAM (e.g. VDP2 CRAM) drops single-byte writes: a byte-wise 'M' of
         * 0xEC63 reads back as 0x0063. Whole aligned words/halfwords match its bus width.
         * @param buf Hex string to decode (2 characters per byte)
         * @param addr Destination address
         * @param count Number of bytes to decode
         * @return Pointer to the character following the decoded hex string, or nullptr on failure.
         */
        inline static const char *hex2mem_aligned(const char *buf, uint32_t addr, int count)
        {
            // Validate all characters first to prevent partial memory corruption
            for (int i = 0; i < count * 2; i++)
            {
                if (hex(buf[i]) < 0)
                    return nullptr;
            }

            int i = 0;
            while (i < count)
            {
                const int remaining = count - i;
                const uint32_t cur = addr + static_cast<uint32_t>(i);
                if (remaining >= 4 && (cur & 3U) == 0U)
                {
                    uint32_t v = 0;
                    for (int b = 0; b < 4; b++)
                    {
                        const int h1 = hex(*buf++);
                        const int h2 = hex(*buf++);
                        v = (v << 8) | static_cast<uint32_t>((h1 << 4) | h2);
                    }
                    *reinterpret_cast<volatile uint32_t *>(cur) = v;
                    i += 4;
                }
                else if (remaining >= 2 && (cur & 1U) == 0U)
                {
                    uint16_t v = 0;
                    for (int b = 0; b < 2; b++)
                    {
                        const int h1 = hex(*buf++);
                        const int h2 = hex(*buf++);
                        v = static_cast<uint16_t>((v << 8) | static_cast<uint16_t>((h1 << 4) | h2));
                    }
                    *reinterpret_cast<volatile uint16_t *>(cur) = v;
                    i += 2;
                }
                else
                {
                    const int h1 = hex(*buf++);
                    const int h2 = hex(*buf++);
                    *reinterpret_cast<volatile uint8_t *>(cur) = static_cast<uint8_t>((h1 << 4) | h2);
                    i += 1;
                }
            }
            return buf;
        }

        /**
         * @brief Encodes memory into a hex string.
         * @param mem Source buffer
         * @param buf Destination string, must hold count * 2 + 1 characters
         * @param count Number of bytes to encode
         * @return Pointer to the null terminator of the resulting string.
         */
        inline static char *mem2hex(const uint8_t *mem, char *buf, int count)
        {
            for (int i = 0; i < count; i++)
            {
                *buf++ = hexchar(*mem >> 4);
                *buf++ = hexchar(*mem & 0xF);
                mem++;
            }
            *buf = 0;
            return buf;
        }

        /**
         * @brief Records a received RSP packet in g_session and marks the session as connected.
         * @param cmd Packet payload (truncated to 63 characters)
         */
        inline static void record_command(const char *cmd)
        {
            size_t i = 0;
            while (i < 63 && cmd[i] != '\0')
            {
                g_session.LastCommand[i] = cmd[i];
                ++i;
            }
            g_session.LastCommand[i] = '\0';
            g_session.CommandCount = g_session.CommandCount + 1;
            g_session.HasConnection = true;
        }

        /**
         * @brief Checks whether a string starts with a prefix.
         * @param s String to test
         * @param prefix Expected prefix
         * @return true if s starts with prefix
         */
        inline static bool starts_with(const char *s, const char *prefix)
        {
            size_t i = 0;
            while (prefix[i] != '\0')
            {
                if (s[i] != prefix[i])
                {
                    return false;
                }
                ++i;
            }
            return true;
        }

        /**
         * @brief Compares two null-terminated strings.
         * @param a First string
         * @param b Second string
         * @return true if both strings are identical
         */
        inline static bool str_equals(const char *a, const char *b)
        {
            size_t i = 0;
            while (a[i] != '\0' && b[i] != '\0')
            {
                if (a[i] != b[i])
                    return false;
                ++i;
            }
            return a[i] == b[i];
        }

        /**
         * @brief Checks if a memory range is valid for access, preventing bus errors.
         * @param addr Start address
         * @param length Length of the range in bytes
         * @return true if the range does not wrap around and stays below 0xF0000000
         */
        inline static bool is_valid_memory_range(uint32_t addr, uint32_t length)
        {
            if (length == 0)
            {
                return true;
            }

            // Detect wrap-around in address arithmetic.
            const uint32_t end = addr + length - 1;
            if (end < addr)
            {
                return false;
            }

            // Prevent accesses to the invalid high address space and peripheral space.
            // Reading peripheral space (0xFFFF8000 - 0xFFFFFFFF) via byte-wise access (mov.b)
            // causes bus errors on many SH-2 registers, which crashes the stub.
            if (end >= 0xF0000000U)
            {
                return false;
            }

            return true;
        }

        /**
         * @brief Parses a hex number up to a delimiter or the end of the string.
         * @param p String to parse
         * @param delimiter Character that ends the number
         * @param out_value Parsed value (only written on success)
         * @param out_end Pointer to the delimiter or terminator (only written on success)
         * @return true if at least one digit was parsed and no invalid character was found
         */
        inline static bool parse_hex_u32_until(const char *p, char delimiter, uint32_t &out_value, const char *&out_end)
        {
            uint32_t value = 0;
            bool saw_digit = false;

            while (*p != '\0' && *p != delimiter)
            {
                const int d = hex(*p);
                if (d < 0)
                {
                    return false;
                }
                value = (value << 4) | static_cast<uint32_t>(d);
                saw_digit = true;
                ++p;
            }

            if (!saw_digit)
            {
                return false;
            }

            out_value = value;
            out_end = p;
            return true;
        }

        /**
         * @brief Finds the software breakpoint slot for a given address.
         * @param address Breakpoint address
         * @return The slot index, or -1 if not found.
         */
        inline static int find_breakpoint_slot(uint32_t address)
        {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i)
            {
                if (g_breakpoints.Software[i].active && g_breakpoints.Software[i].address == address)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        /**
         * @brief Finds an unused software breakpoint slot.
         * @return The slot index, or -1 if all slots are in use
         */
        inline static int find_free_breakpoint_slot()
        {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i)
            {
                if (!g_breakpoints.Software[i].active)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        /** @brief Set by PurgeCache() when memory was patched; consumed by FlushCacheIfDirty() */
        inline static bool g_cache_dirty = false;

        /**
         * @brief Purges this CPU's cache immediately (CCR.CP), then waits for it to take effect.
         */
        inline static void ForcePurgeCache()
        {
            *reinterpret_cast<volatile uint8_t *>(0xFFFFFE92) |= 0x10;
            // The cache must not be accessed for two instructions after a purge; 8 to be safe.
            asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop" ::: "memory"); // 8 nops -- pure delay, no register/memory effect
        }

        /**
         * @brief Purges the cache if PurgeCache() was requested since the last purge.
         */
        inline static void FlushCacheIfDirty()
        {
            if (g_cache_dirty)
            {
                ForcePurgeCache();
                g_cache_dirty = false;
            }
        }

        /**
         * @brief RAII guard that runs FlushCacheIfDirty() when it goes out of scope.
         */
        struct CacheFlusher
        {
            /**
             * @brief Purges the cache if any memory was patched.
             */
            ~CacheFlusher() { FlushCacheIfDirty(); }
        };

        /**
         * @brief RAII guard that releases a slave halted at a breakpoint (see
         * InstallSlaveExceptionHandler()) on EVERY exit path from
         * process_commands(), not just the 'c'/'s' ones -- once GDB has been
         * told about that stop (g_slave.Reported) or the session is over.
         * @details 'D', 'k' and a disconnect also resume the master; without this the
         * slave stays parked and no slave job is ever dispatched again.
         */
        struct SlaveReleaseGuard
        {
            /**
             * @brief Releases the slave if it is halted at a breakpoint.
             */
            ~SlaveReleaseGuard()
            {
                // A slave that stopped while the master was already halted was never
                // reported to GDB: keep it parked so the next Poll() reports it.
                if (Uncached(g_slave.Stopped) && (g_slave.Reported || !g_session.HasConnection))
                {
                    g_slave.Reported = false;
                    g_slave.Stopped = false;
                    g_slave.Resume = true;
                }
            }
        };


        /**
         * @brief RAII guard marking process_commands() as running, so a re-entrant Poll() skips its GDB work.
         */
        struct ReentrancyGuard
        {
            /**
             * @brief Set for the entire duration of process_commands().
             * @details Poll() skips its GDB work while this is set (see Poll()).
             */
            inline static volatile bool g_in_process_commands = false;

            /**
             * @brief Marks process_commands() as running.
             */
            ReentrancyGuard() { g_in_process_commands = true; }

            /**
             * @brief Marks process_commands() as no longer running.
             */
            ~ReentrancyGuard() { g_in_process_commands = false; }

            /**
             * @brief Whether process_commands() is currently running.
             * @return true while a ReentrancyGuard is alive
             */
            inline static bool IsInProcessCommands() { return g_in_process_commands; }
        };

        /**
         * @brief Requests a cache purge before execution resumes (deferred to FlushCacheIfDirty()).
         */
        inline static void PurgeCache()
        {
            g_cache_dirty = true;
        }

        /**
         * @brief Clears all active software breakpoints.
         * @param restore_memory Whether to write the original instructions back before freeing the slots
         */
        inline static void clear_breakpoints(bool restore_memory)
        {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i)
            {
                if (!g_breakpoints.Software[i].active)
                {
                    continue;
                }

                if (restore_memory)
                {
                    volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(g_breakpoints.Software[i].address | 0x20000000U);
                    *code = g_breakpoints.Software[i].original_instruction;
                }

                g_breakpoints.Software[i].active = false;
                g_breakpoints.Software[i].address = 0;
                g_breakpoints.Software[i].original_instruction = 0;
            }
            if (restore_memory)
            {
                PurgeCache();
            }
        }

        /**
         * @brief Installs a software breakpoint (0xFFFF) at the specified address.
         * @param address Address of the instruction to break on (must be 2-byte aligned)
         * @return true if the breakpoint is installed (or already was), false if the
         * address is invalid or all slots are in use
         */
        inline static bool install_software_breakpoint(uint32_t address)
        {
            if ((address & 1U) != 0U || !is_valid_memory_range(address, 2U))
            {
                return false;
            }

            if (find_breakpoint_slot(address) >= 0)
            {
                return true;
            }

            const int slot = find_free_breakpoint_slot();
            if (slot < 0)
            {
                return false;
            }

            volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(address | 0x20000000U);
            const uint16_t original = *code;
            *code = SoftwareBreakInstruction;
            if (*code != SoftwareBreakInstruction)
            {
                return false; // read-only memory (BIOS ROM): GDB reports "Cannot insert breakpoint"
            }
            g_breakpoints.Software[slot].address = address;
            g_breakpoints.Software[slot].original_instruction = original;
            g_breakpoints.Software[slot].active = true;
            PurgeCache();
            return true;
        }

        /**
         * @brief Removes a software breakpoint and restores the original instruction.
         * @param address Address of the breakpoint (must be 2-byte aligned)
         * @return true if no breakpoint remains at that address, false if the address is invalid
         */
        inline static bool remove_software_breakpoint(uint32_t address)
        {
            if ((address & 1U) != 0U || !is_valid_memory_range(address, 2U))
            {
                return false;
            }

            const int slot = find_breakpoint_slot(address);
            if (slot < 0)
            {
                return true;
            }

            volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(address | 0x20000000U);
            *code = g_breakpoints.Software[slot].original_instruction;
            g_breakpoints.Software[slot].active = false;
            g_breakpoints.Software[slot].address = 0;
            g_breakpoints.Software[slot].original_instruction = 0;
            PurgeCache();
            return true;
        }

        /**
         * @brief Configures the User Break Controller (UBC) for a hardware watchpoint.
         * @param address Address to watch (exact match)
         * @param type RSP Z-packet type: 1 = hardware breakpoint, 2 = write, 3 = read, 4 = access watchpoint
         * @return true if UBC channel A was programmed, false if it is already in use or type is unsupported
         */
        inline static bool install_hardware_watchpoint(uint32_t address, uint32_t type)
        {
            if (g_breakpoints.UbcChannelAActive)
            {
                return false; // Only one channel supported currently
            }

            volatile uint32_t *BARA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF40U);
            volatile uint16_t *BAMRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF44U);
            volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
            volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF78U); // not 0xFFFFFF60: that's BARBH

            *BARA = address;
            *BAMRA = 0x0000U; // exact match

            // BBRA bits 7-6 select the bus cycle to compare: 00 = never break,
            // 01 = CPU cycles. Leaving them at 00 is why no hbreak/watchpoint ever fired.
            constexpr uint16_t kBreakOnCpuCycles = 0x0040U;
            uint16_t bbra_val = 0;
            switch (type)
            {
            case 1:
                bbra_val = 0x0014U;
                break; // HW breakpoint (instruction fetch, read: a 00 read/write field never breaks)
            case 2:
                bbra_val = 0x0028U;
                break; // Write watchpoint
            case 3:
                bbra_val = 0x0024U;
                break; // Read watchpoint
            case 4:
                bbra_val = 0x002CU;
                break; // Access watchpoint (Read/Write)
            default:
                return false;
            }

            *BBRA = kBreakOnCpuCycles | bbra_val;
            *BRCR = 0x0000U;                  // independent channels, break before the instruction runs
            asm volatile("nop" ::: "memory"); // ensure BRCR write is committed

            g_breakpoints.UbcChannelAActive = true;
            g_breakpoints.UbcType = type;
            g_breakpoints.UbcAddress = address;
            return true;
        }

        /**
         * @brief Disables the User Break Controller (UBC) hardware watchpoint.
         * @param address Unused: there is only one UBC channel to disable
         * @param type Unused: there is only one UBC channel to disable
         * @return true if a watchpoint was active and is now disabled, false if none was active
         */
        inline static bool remove_hardware_watchpoint(uint32_t address, uint32_t type)
        {
            (void)address;
            (void)type;
            if (!g_breakpoints.UbcChannelAActive)
            {
                return false;
            }
            volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
            volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF78U); // not 0xFFFFFF60: that's BARBH
            *BBRA = 0x0000U; // no break condition: this, not BRCR, is what disarms channel A
            *BRCR = 0x0000U;
            g_breakpoints.UbcChannelAActive = false;
            return true;
        }

        /**
         * @brief Tracks the state of a single-step operation.
         */
        struct StepData
        {
            /** @brief Address where the step trap is placed */
            uint32_t address;

            /** @brief Instruction replaced by the step trap */
            uint16_t original_instruction;

            /** @brief Whether a step trap is currently in memory */
            bool active;
        };

        /** @brief Current single-step state */
        inline static StepData g_step_data = {0, 0, false};

        /**
         * @brief Removes the temporary software step trap and restores the original instruction.
         */
        inline static void undo_software_step()
        {
            if (g_step_data.active)
            {
                if (is_valid_memory_range(g_step_data.address, 2U))
                {
                    volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(g_step_data.address | 0x20000000U);
                    *code = g_step_data.original_instruction;
                    PurgeCache();
                }
                g_step_data.active = false;
            }
        }

        /**
         * @brief Places a temporary software step trap to catch execution after one instruction.
         */
        inline static void do_software_step()
        {
            undo_software_step();

            uint32_t pc = GDBStub::MasterSH2().pc;
            if ((pc & 1U) != 0U || !is_valid_memory_range(pc, 2U))
            {
                return;
            }

            volatile uint16_t *code_ptr = reinterpret_cast<volatile uint16_t *>(pc | 0x20000000U);
            uint16_t opcode = *code_ptr;
            uint32_t target_pc = pc + 2U;

            bool is_branch = false;
            bool has_delay_slot = false;
            uint32_t branch_target = pc + 2U;

            if ((opcode & 0xfb00U) == 0x8900U)
            { // BT label, BT/S label
                is_branch = true;
                has_delay_slot = ((opcode & 0xff00U) == 0x8d00U);
                if ((GDBStub::MasterSH2().sr & 1U) != 0U)
                {
                    int8_t disp8 = static_cast<int8_t>(opcode & 0xffU);
                    branch_target = pc + (static_cast<int>(disp8) << 1) + 4U;
                }
                else
                {
                    branch_target = pc + (has_delay_slot ? 4U : 2U);
                }
            }
            else if ((opcode & 0xfb00U) == 0x8b00U)
            { // BF label, BF/S label
                is_branch = true;
                has_delay_slot = ((opcode & 0xff00U) == 0x8f00U);
                if ((GDBStub::MasterSH2().sr & 1U) == 0U)
                {
                    int8_t disp8 = static_cast<int8_t>(opcode & 0xffU);
                    branch_target = pc + (static_cast<int>(disp8) << 1) + 4U;
                }
                else
                {
                    branch_target = pc + (has_delay_slot ? 4U : 2U);
                }
            }
            else if ((opcode & 0xe000U) == 0xa000U)
            { // BRA label, BSR label
                is_branch = true;
                int16_t disp12 = static_cast<int16_t>((opcode & 0x0fffU) << 4) >> 4;
                branch_target = pc + (static_cast<int>(disp12) << 1) + 4U;
            }
            else if ((opcode & 0xf0dfU) == 0x400bU)
            { // JMP @Rm, JSR @Rm
                is_branch = true;
                uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                branch_target = GDBStub::MasterSH2().r[reg_idx];
            }
            else if ((opcode & 0xf0ffU) == 0x0023U)
            { // BRAF Rm
                is_branch = true;
                uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                branch_target = pc + 4U + GDBStub::MasterSH2().r[reg_idx];
            }
            else if ((opcode & 0xf0ffU) == 0x0003U)
            { // BSRF Rm
                is_branch = true;
                uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                branch_target = pc + 4U + GDBStub::MasterSH2().r[reg_idx];
            }
            else if (opcode == 0x000bU)
            { // RTS
                is_branch = true;
                branch_target = GDBStub::MasterSH2().pr;
            }
            else if (opcode == 0x002bU)
            { // RTE
                is_branch = true;
                uint32_t sp = GDBStub::MasterSH2().r[15];
                if (is_valid_memory_range(sp, 8U))
                {
                    branch_target = *reinterpret_cast<volatile uint32_t *>(sp | 0x20000000U);
                }
            }
            else if ((opcode & 0xff00U) == 0xc300U)
            { // TRAPA #imm
                // TRAPA #imm jumps through VBR + imm*4 (imm IS the vector number).
                const uint32_t vec_addr = GDBStub::MasterSH2().vbr + ((opcode & 0xffU) * 4U);
                if (is_valid_memory_range(vec_addr, 4U))
                {
                    const uint32_t handler = *reinterpret_cast<volatile uint32_t *>(vec_addr | 0x20000000U);
                    // A trap the stub handles stops in the stub anyway, reported at pc+2:
                    // never patch the stub's own entry (the trap would re-fault inside it).
                    target_pc = handler == reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk) ? pc + 2U : handler;
                }
            }
            else if (opcode == 0xFFFFU)
            { // Illegal Instruction (our breakpoint)
                if (find_breakpoint_slot(pc) >= 0)
                {
                    target_pc = pc + 2U;
                }
                else
                {
                    target_pc = pc;
                }
            }

            // A delayed branch is stepped together with its delay slot: the trap goes on
            // the target, so the CPU runs the slot and applies PR/SR/SP itself. A trap IN
            // the slot is a slot-illegal exception, which reports the branch destination
            // (hardware-confirmed) and would leave the slot's own instruction unexecuted.
            if (is_branch)
            {
                target_pc = branch_target;
            }

            const bool is_call = (opcode & 0xf000U) == 0xb000U    // BSR
                                 || (opcode & 0xf0ffU) == 0x400bU // JSR
                                 || (opcode & 0xf0ffU) == 0x0003U; // BSRF
            if (!place_step_trap(target_pc) && is_call)
            {
                // The callee can't be patched (BIOS ROM): step over the call instead of running free.
                place_step_trap(pc + 4U);
            }
        }

        /**
         * @brief Patches the single-step trap (0xFFFF) at an address and records it in g_step_data.
         * @param address Instruction address to trap
         * @return false if the address is invalid or read-only (the write did not stick)
         */
        inline static bool place_step_trap(uint32_t address)
        {
            if ((address & 1U) != 0U || !is_valid_memory_range(address, 2U))
            {
                return false;
            }
            volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(address | 0x20000000U);
            // Recorded before the trap is written: a slave that hits it must already
            // see it as the master's step trap (see slave_breakpoint_handler()).
            g_step_data.address = address;
            g_step_data.original_instruction = *code;
            g_step_data.active = true;
            *code = SoftwareBreakInstruction;
            if (*code != SoftwareBreakInstruction)
            {
                g_step_data.active = false;
                return false; // read-only memory (BIOS ROM)
            }
            PurgeCache();
            return true;
        }

        /**
         * @brief Adjusts the program counter (PC) following a breakpoint or step trap.
         */
        inline static void adjust_pc_for_software_breakpoint()
        {
            g_stop.WasSwBreak = false;
            g_stop.WasUbcHit = false;
            if (g_ubc_hit != 0U)
            {
                // UBC break (vector 12): the hardware PC is already right for GDB.
                g_ubc_hit = 0U;
                g_stop.WasUbcHit = true;
                g_last_stop_signal = 5U; // SIGTRAP
                return;
            }
            // The illegal-instruction thunk tags every 0xFFFF as SIGILL. One of our own
            // breakpoints or step traps must be reported as SIGTRAP instead, or GDB treats
            // a normal breakpoint hit as a crash. The pushed PC is the 0xFFFF word itself.
            // A GDB breakpoint placed on a Break() call stops twice there (BP, then Break()).
            if (find_breakpoint_slot(GDBStub::MasterSH2().pc) >= 0 || (g_step_data.active && GDBStub::MasterSH2().pc == g_step_data.address))
            {
                g_stop.WasSwBreak = true;
                g_last_stop_signal = 5U; // SIGTRAP — GDB breakpoint or step trap, not a real crash
                return;                  // PC is already exactly at the breakpoint.
            }

            // A Break() (0xFFFF) GDB did not insert: step past it so continue/step can resume. Not
            // WasSwBreak, or GDB would silently continue over a breakpoint it doesn't know.
            if (is_valid_memory_range(GDBStub::MasterSH2().pc, 2U))
            {
                volatile uint16_t *code = reinterpret_cast<volatile uint16_t *>(GDBStub::MasterSH2().pc | 0x20000000U);
                if (*code == SoftwareBreakInstruction)
                {
                    GDBStub::MasterSH2().pc += 2U;
                    return;
                }
            }

            if (GDBStub::MasterSH2().pc < 2U)
            {
                return;
            }

            // Fallback: If we ever used TRAPA, the PC pushed is PC + 2.
            const uint32_t trap_address = GDBStub::MasterSH2().pc - 2U;
            if (find_breakpoint_slot(trap_address) >= 0 || (g_step_data.active && trap_address == g_step_data.address))
            {
                g_stop.WasSwBreak = true;
                g_last_stop_signal = 5U; // SIGTRAP — GDB breakpoint or step trap, not a real crash
                GDBStub::MasterSH2().pc = trap_address;
            }
        }

        /**
         * @brief Captures a dummy context for asynchronous packet handling outside of exceptions.
         */
        inline static void snapshot_polling_context()
        {
            // Context for packets serviced outside an exception, so GDB never sees a null frame.
            // r0-r14 are zeroed: inside Poll() they only hold compiler temporaries.
            for (int i = 0; i < 16; ++i)
            {
                GDBStub::MasterSH2().r[i] = 0;
            }

            uint32_t sp = 0;
            uint32_t pc = 0;
            uint32_t pr = 0;
            uint32_t gbr = 0;
            uint32_t vbr = 0;
            uint32_t mach = 0;
            uint32_t macl = 0;
            uint32_t sr = 0;

            asm volatile("mov r15, %0" : "=r"(sp));     // sp = current r15 (this function's own stack pointer, not the caller's)
            asm volatile("mova 1f, r0\n\t"              // r0 = address of local label "1" (a fixed point a few instructions below, inside this same function)
                         "mov r0, %0\n\t"               // pc = r0 -- captures that fixed address as the "current PC", since there's no direct "read PC" instruction on SH-2
                         ".align 2\n\t"                 // realign after the mova/mov pair so label 1 lands on a valid instruction boundary
                         "1:\n\t" : "=r"(pc) : : "r0"); // label 1 itself -- purely a marker for mova to compute the address of, emits no code
            asm volatile("sts pr, %0" : "=r"(pr));      // pr = current PR (procedure register / return address)
            asm volatile("stc gbr, %0" : "=r"(gbr));    // gbr = current GBR
            asm volatile("stc vbr, %0" : "=r"(vbr));    // vbr = current VBR
            asm volatile("sts mach, %0" : "=r"(mach));  // mach = current MACH
            asm volatile("sts macl, %0" : "=r"(macl));  // macl = current MACL
            asm volatile("stc sr, %0" : "=r"(sr));      // sr = current SR

            GDBStub::MasterSH2().r[15] = sp;
            GDBStub::MasterSH2().pc = pc;
            GDBStub::MasterSH2().pr = pr;
            GDBStub::MasterSH2().gbr = gbr;
            GDBStub::MasterSH2().vbr = vbr;
            GDBStub::MasterSH2().mach = mach;
            GDBStub::MasterSH2().macl = macl;
            GDBStub::MasterSH2().sr = sr;

            // Not a crash: drop the signal left by any earlier crash stop, or a fresh
            // attach reports it (e.g. SIGILL) and GDB resumes with an unsupported "C04".
            g_last_stop_signal = 5U; // SIGTRAP

            // pc is a label inside this function, not a resume point: continue/step must ignore it.
            g_stop.CtxIsFake = true;
        }

        /**
         * @brief Idle timeout (wait-loop iterations) applied after handshake when no packet arrives.
         * @details Each iteration is ~1-2 us (USB_FLAGS has ~10 wait states), so this is several
         * minutes. The loop also runs while halted, so it must outlast a developer thinking
         * between commands: a few seconds resumed the target out from under GDB.
         */
        static constexpr uint32_t GDB_RX_IDLE_TIMEOUT = 450000000U;

        /** @brief Idle timeout (wait-loop iterations) for TX space, same rationale as GDB_RX_IDLE_TIMEOUT */
        static constexpr uint32_t GDB_TX_IDLE_TIMEOUT = 450000000U;

        /**
         * @brief Waits for USB RX data.
         * @details Before GDB connects: waits forever (so Break() before attach works), but
         * gives up at once if the cable is absent -- a floating USB_FLAGS reads as "RX pending".
         * After the handshake: also gives up after GDB_RX_IDLE_TIMEOUT (GDB killed without D).
         * @warning Unplugging the cable mid-bus-cycle can still hang the console; no software
         * check can catch that (see the sample readme).
         * @return true if data is available, false if the session should be abandoned
         */
        inline static bool __gdb_wait_rx()
        {
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::IsRxfEmpty())
            {
                // Abort immediately on cable unplug, connected or not.
                if (!SRL::DevCart::CS0::IsConnected())
                {
                    g_session.HasConnection = false;
                    g_session.HandshakeDone = false;
                    return false;
                }
                // After handshake, apply idle timeout for dead GDB processes.
                if (g_session.HasConnection && g_session.HandshakeDone)
                {
                    if (++idle > GDB_RX_IDLE_TIMEOUT)
                    {
                        g_session.HasConnection = false;
                        g_session.HandshakeDone = false;
                        return false;
                    }
                }
            }
            return true;
        }

        /**
         * @brief Waits for USB TX space.
         * @details Same exits as __gdb_wait_rx(): cable unplugged, or idle timeout after the handshake.
         * @return true if a byte can be written, false if the session should be abandoned
         */
        inline static bool __gdb_wait_tx()
        {
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::IsTxeFull())
            {
                if (!SRL::DevCart::CS0::IsConnected())
                {
                    g_session.HasConnection = false;
                    g_session.HandshakeDone = false;
                    return false;
                }
                if (g_session.HasConnection && g_session.HandshakeDone)
                {
                    if (++idle > GDB_TX_IDLE_TIMEOUT)
                    {
                        g_session.HasConnection = false;
                        g_session.HandshakeDone = false;
                        return false;
                    }
                }
            }
            return true;
        }

        /**
         * @brief Reads one byte from the DevCart, returning a pushed-back byte first if there is one.
         * @return Byte read (0-255), or -1 if the connection was lost
         */
        inline static int __gdb_getc()
        {
            if (g_transfer.UngetChar != -1)
            {
                int c = g_transfer.UngetChar;
                g_transfer.UngetChar = -1;
                return c;
            }
            if (!__gdb_wait_rx())
            {
                return -1; // disconnected
            }
            const uint8_t c = *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo);
            g_transfer.RxDetectCount = g_transfer.RxDetectCount + 1;
            return static_cast<int>(c);
        }

        /**
         * @brief Writes one byte to the DevCart.
         * @param value Byte to send
         * @return true if sent, false if the connection was lost
         */
        inline static bool __gdb_putc(uint8_t value)
        {
            if (!__gdb_wait_tx())
            {
                return false; // disconnected
            }
            *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo) = value;
            g_transfer.TxByteCount = g_transfer.TxByteCount + 1;
            return true;
        }

        // --- Packet I/O (minimal, libyaul-style) ---

        /**
         * @brief Sends raw packet payload bytes and computes their checksum.
         * @param buffer Payload to send
         * @param len Number of bytes to send
         * @return Modulo-256 sum of the bytes sent (stops early on disconnect)
         */
        inline static uint8_t packet_put_data(const char *buffer, size_t len)
        {
            uint8_t sum = 0;
            for (size_t i = 0; i < len; i++)
            {
                uint8_t ch = static_cast<uint8_t>(buffer[i]);
                sum += ch;
                if (!__gdb_putc(ch))
                    return sum; // disconnect
            }
            return sum;
        }

        /**
         * @brief Sends a GDB RSP packet ($type data#cc) and waits for GDB's acknowledgment.
         * @details Retransmits on '-'. If GDB starts a new packet ('$') instead of
         * acknowledging, the '$' is pushed back for the next packet_get().
         * @param type Optional first payload character (e.g. 'T' or 'O'), or '\0' for none
         * @param data Rest of the payload, may be nullptr when len is 0
         * @param len Length of data
         */
        inline static void packet_put(char type, const char *data, size_t len)
        {
            do
            {
                uint8_t csum = 0;
                uint8_t ch = '$';
                if (!__gdb_putc(ch))
                    return;

                if (type != '\0')
                {
                    ch = static_cast<uint8_t>(type);
                    if (!__gdb_putc(ch))
                        return;
                    csum += ch;
                }

                if (data != nullptr && len > 0)
                {
                    csum += packet_put_data(data, len);
                    if (!g_session.HasConnection)
                        return; // disconnect mid-send
                }

                ch = '#';
                if (!__gdb_putc(ch))
                    return;
                ch = static_cast<uint8_t>(hexchar(csum >> 4));
                if (!__gdb_putc(ch))
                    return;
                ch = static_cast<uint8_t>(hexchar(csum));
                if (!__gdb_putc(ch))
                    return;

                while (true)
                {
                    int raw = __gdb_getc();
                    if (raw < 0)
                        return; // disconnect
                    ch = static_cast<uint8_t>(raw & 0x7F);
                    if (ch == '+')
                    {
                        return;
                    }
                    else if (ch == '-')
                    {
                        break; // retransmit
                    }
                    else if (ch == '$')
                    {
                        g_transfer.UngetChar = '$';
                        return;
                    }
                }
            } while (true);
        }

        /**
         * @brief Receives one GDB RSP packet, acknowledges it and strips any sequence id.
         * @details NACKs ('-') and waits for a retransmission on a bad checksum or
         * an oversized packet. The packet is also recorded via record_command().
         * @param buffer Destination for the null-terminated payload
         * @param max_len Size of buffer
         * @return true if a packet was received, false if disconnected (buffer will contain empty/partial data)
         */
        inline static bool packet_get(char *buffer, size_t max_len)
        {
            buffer[0] = '\0';
            while (true)
            {
                int raw;

                // Wait for '$' packet start, abort on disconnect.
                do
                {
                    raw = __gdb_getc();
                    if (raw < 0)
                        return false; // disconnect
                } while ((raw & 0x7F) != '$');

                uint8_t csum = 0;
                size_t len = 0;
                bool overflow = false;

                while (true)
                {
                    raw = __gdb_getc();
                    if (raw < 0)
                        return false;
                    uint8_t ch = static_cast<uint8_t>(raw & 0x7F);
                    if (ch == '#')
                        break;
                    csum += ch;
                    if (len + 1 < max_len)
                    {
                        buffer[len++] = static_cast<char>(ch);
                    }
                    else
                    {
                        overflow = true;
                    }
                }
                buffer[len] = '\0';

                const int hi_raw = __gdb_getc();
                const int lo_raw = __gdb_getc();
                if (hi_raw < 0 || lo_raw < 0)
                    return false;
                const int hi = hex(static_cast<char>(hi_raw & 0x7F));
                const int lo = hex(static_cast<char>(lo_raw & 0x7F));
                if (hi < 0 || lo < 0)
                {
                    uint8_t nack = '-';
                    __gdb_putc(nack);
                    continue;
                }
                const uint8_t xmit_csum = static_cast<uint8_t>((hi << 4) | lo);

                if (csum != xmit_csum || overflow)
                {
                    uint8_t nack = '-';
                    __gdb_putc(nack);
                    continue;
                }

                uint8_t ack = '+';
                __gdb_putc(ack);

                // Strip sequence id prefix (XX:payload → payload).
                // Ensure XX are valid hex digits so we don't accidentally truncate valid packets (e.g. M<addr>:).
                if (len >= 3 && buffer[2] == ':' && hex(buffer[0]) >= 0 && hex(buffer[1]) >= 0)
                {
                    size_t i = 0;
                    while (buffer[3 + i] != '\0')
                    {
                        buffer[i] = buffer[3 + i];
                        ++i;
                    }
                    buffer[i] = '\0';
                }

                record_command(buffer);
                return true;
            }
        }

        /**
         * @brief Sends a 'T' stop-reply packet to GDB.
         * @details Adds "swbreak:;" for GDB-managed software breakpoints and step
         * traps, "reason:signal;" for other SIGTRAP stops, and "thread:1;".
         * @param signal POSIX signal number to report
         */
        inline static void send_stop_signal(uint8_t signal)
        {
            char buf[64];
            buf[0] = hexchar(signal >> 4);
            buf[1] = hexchar(signal & 0xF);
            int len = 2;

            if (signal == 5)
            {
                // swbreak only for GDB's own breakpoints/step traps: on a Break() or a crash GDB
                // finds no breakpoint there and silently continues.
                if (g_stop.WasSwBreak)
                {
                    const char *swb = "swbreak:;";
                    for (int i = 0; i < 9; ++i)
                        buf[len++] = swb[i];
                }
                else if (g_stop.WasUbcHit && g_breakpoints.UbcChannelAActive)
                {
                    // GDB only reports a watchpoint hit when the stop reply names it.
                    static const char *const kUbcReason[] = {"hwbreak", "watch", "rwatch", "awatch"};
                    const uint32_t type = g_breakpoints.UbcType;
                    len = static_cast<int>(append_str(buf, static_cast<size_t>(len), kUbcReason[(type >= 1U && type <= 4U) ? type - 1U : 0U]));
                    buf[len++] = ':';
                    if (type >= 2U)
                    {
                        len = static_cast<int>(append_hex(buf, static_cast<size_t>(len), g_breakpoints.UbcAddress, 8));
                    }
                    buf[len++] = ';';
                }
                else
                {
                    const char *rsn = "reason:signal;";
                    for (int i = 0; i < 14; ++i)
                        buf[len++] = rsn[i];
                }
            }

            const char *thread = "thread:1;";
            for (int i = 0; i < 9; ++i)
                buf[len++] = thread[i];

            g_last_stop_signal = signal;
            packet_put('T', buf, len);
        }

        /**
         * @brief Appends a string to a buffer (no terminator written).
         * @param buf Destination buffer
         * @param pos Current write position in buf
         * @param s String to append
         * @return New write position
         */
        inline static size_t append_str(char *buf, size_t pos, const char *s)
        {
            while (*s != '\0')
                buf[pos++] = *s++;
            return pos;
        }

        /**
         * @brief Appends a value as fixed-width lowercase hex to a buffer (no terminator written).
         * @param buf Destination buffer
         * @param pos Current write position in buf
         * @param value Value to format
         * @param digits Number of hex digits to write
         * @return New write position
         */
        inline static size_t append_hex(char *buf, size_t pos, uint32_t value, int digits)
        {
            for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4)
            {
                buf[pos++] = hexchar(static_cast<int>(value >> shift));
            }
            return pos;
        }

        /**
         * @brief Sends `text` to the GDB console as $O packets, chunked to the packet size.
         * @details Only while answering a qRcmd: GDB prints $O packets until the final $OK.
         * @param text Text to print on the GDB console
         * @param len Length of text
         */
        inline static void send_monitor_text(const char *text, size_t len)
        {
            constexpr size_t kChunkRaw = kPacketDataMax / 2U;
            char hexbuf[kChunkRaw * 2U + 1U];
            size_t off = 0;
            while (off < len)
            {
                size_t chunk = len - off;
                if (chunk > kChunkRaw)
                    chunk = kChunkRaw;
                mem2hex(reinterpret_cast<const uint8_t *>(text + off), hexbuf, static_cast<int>(chunk));
                packet_put('O', hexbuf, chunk * 2U);
                off += chunk;
            }
        }

        // --- Core Handler ---

        /**
         * @brief Maps a GDB register number to its slot: 0-22 in g_ctx, 23-45 in g_slave_ctx.
         * @details SH2Context is laid out in GDB's order (r0-r15, pc, pr, gbr, vbr, mach, macl, sr),
         * which is also why the 'g' packet can dump it whole.
         * @param reg_idx GDB register number
         * @return The register, or nullptr when reg_idx is out of range
         */
        inline static uint32_t *register_slot(uint32_t reg_idx)
        {
            constexpr uint32_t kRegsPerCpu = sizeof(SH2Context) / sizeof(uint32_t);
            static_assert(kRegsPerCpu == 23U, "SH2Context must stay in GDB's 23-register order");
            if (reg_idx < kRegsPerCpu)
            {
                return reinterpret_cast<uint32_t *>(&g_ctx) + reg_idx;
            }
            if (reg_idx < 2U * kRegsPerCpu)
            {
                return reinterpret_cast<uint32_t *>(&g_slave_ctx) + (reg_idx - kRegsPerCpu);
            }
            return nullptr;
        }

        /**
         * @brief Fixed 'g' packet register count GDB's SH backend requires.
         * @details gdb-multiarch 15.1 and sh-elf-gdb 14.2 both hard-code a 268-byte 'g' packet
         * for sh/sh2: 23 real registers plus 44 blank slots, and reject any other length. So
         * 'g'/'G' pad to this size; the slave registers are only reachable via 'p'/'P' (23..45).
         */
        static constexpr size_t GdbFixedShRegisterCount = 67U;

        /** @brief Number of blank registers padded after the 23 real ones in a 'g' reply */
        static constexpr size_t GdbFixedShPaddingRegisters = GdbFixedShRegisterCount - 23U;

        /**
         * @brief Built-in `monitor regs slave` command: prints g_slave_ctx to the GDB console.
         * @details GDB's SH port has no names for the slave registers (see GdbFixedShRegisterCount).
         * @note Reads as all zero unless InstallSlaveExceptionHandler() has been
         * installed on the slave and the slave has hit at least one breakpoint.
         */
        inline static void send_slave_regs_dump()
        {
            char text[320];
            size_t pos = 0;

            pos = append_str(text, pos, "slave r0-r7 : ");
            for (int i = 0; i < 8; ++i)
            {
                pos = append_hex(text, pos, GDBStub::SlaveSH2().r[i], 8);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            pos = append_str(text, pos, "slave r8-r15: ");
            for (int i = 8; i < 16; ++i)
            {
                pos = append_hex(text, pos, GDBStub::SlaveSH2().r[i], 8);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            pos = append_str(text, pos, "pc=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().pc, 8);
            pos = append_str(text, pos, " pr=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().pr, 8);
            pos = append_str(text, pos, " sr=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().sr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "gbr=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().gbr, 8);
            pos = append_str(text, pos, " vbr=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().vbr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "mach=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().mach, 8);
            pos = append_str(text, pos, " macl=");
            pos = append_hex(text, pos, GDBStub::SlaveSH2().macl, 8);
            text[pos++] = '\n';

            send_monitor_text(text, pos);
        }

        /**
         * @brief Built-in `monitor trace` command: prints where and why the target stopped.
         * @details Read-only (no memory patches, no g_step_data changes), so it can run before a
         * continue to capture a stop that is hard to reproduce.
         */
        inline static void send_halt_trace_dump()
        {
            char text[400];
            size_t pos = 0;

            pos = append_str(text, pos, "pc=");
            pos = append_hex(text, pos, GDBStub::MasterSH2().pc, 8);
            pos = append_str(text, pos, " pr=");
            pos = append_hex(text, pos, GDBStub::MasterSH2().pr, 8);
            pos = append_str(text, pos, " sr=");
            pos = append_hex(text, pos, GDBStub::MasterSH2().sr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "was_swbreak=");
            text[pos++] = g_stop.WasSwBreak ? '1' : '0';
            pos = append_str(text, pos, " signal_override=");
            pos = append_hex(text, pos, g_stop.SignalOverride, 2);
            pos = append_str(text, pos, " last_stop_signal=");
            pos = append_hex(text, pos, g_last_stop_signal, 2);
            text[pos++] = '\n';

            const int bp_slot = find_breakpoint_slot(g_ctx.pc);
            pos = append_str(text, pos, "bp_slot=");
            pos = append_hex(text, pos, static_cast<uint32_t>(bp_slot), 8);
            pos = append_str(text, pos, " step_active=");
            text[pos++] = g_step_data.active ? '1' : '0';
            text[pos++] = '\n';

            if (is_valid_memory_range(GDBStub::MasterSH2().pc, 2U))
            {
                uint16_t opcode = *reinterpret_cast<volatile uint16_t *>(GDBStub::MasterSH2().pc | 0x20000000U);
                pos = append_str(text, pos, "opcode_at_pc=");
                pos = append_hex(text, pos, opcode, 4);
            }
            else
            {
                pos = append_str(text, pos, "opcode_at_pc=invalid_range");
            }
            text[pos++] = '\n';

            send_monitor_text(text, pos);
        }

        /**
         * @brief Applies the optional resume address of a 'c'/'C'/'s'/'S' packet to the master PC.
         * @details "caddr"/"saddr" have no separator; "Csig[;addr]"/"Ssig[;addr]" carry a
         * two-digit signal number (ignored -- we don't support signal delivery) before the
         * optional ';addr'. No address (the common case) or a parse failure leaves PC untouched.
         * @param packet Null-terminated packet payload, starting with the command letter
         */
        inline static void apply_resume_address(const char *packet)
        {
            const char *p = &packet[1];
            if (packet[0] == 'C' || packet[0] == 'S')
            {
                if (hex(p[0]) >= 0 && hex(p[1]) >= 0)
                    p += 2;
                if (*p == ';')
                    ++p;
            }
            uint32_t addr = 0;
            const char *end = p;
            if (*p != '\0' && parse_hex_u32_until(p, '\0', addr, end))
            {
                GDBStub::MasterSH2().pc = addr;
            }
        }

        /**
         * @brief Prepares the CPU state for a GDB single-step command ('s'/'S').
         * @details Places a step trap after the current instruction (see do_software_step()).
         * A parked slave is just released (SlaveReleaseGuard): slave code can't be stepped.
         */
        inline static void handle_gdb_step()
        {
            g_stop.SignalOverride = 0;

            // No real instruction to step from a Poll() snapshot (see g_stop.CtxIsFake).
            if (g_stop.CtxIsFake)
            {
                g_stop.CtxIsFake = false;
                return;
            }

            do_software_step();
        }

        /**
         * @brief Prepares the CPU state for a GDB continue command ('c'/'C').
         * @details No breakpoint step-over here: GDB removes the breakpoint at PC (z0)
         * and steps past it itself before it continues. A slave parked at a breakpoint
         * is released by SlaveReleaseGuard when process_commands() returns.
         */
        inline static void handle_gdb_continue()
        {
            g_stop.SignalOverride = 0;
            g_stop.CtxIsFake = false;
        }

        /**
         * @brief The RSP command loop for one halt: reads packets and answers them until
         * GDB resumes (c/s), detaches, kills or disconnects. Runs in exception context.
         * @note Reached through the extern "C" srl_gdbstub_process_commands() trampoline:
         * GCC ignores an __asm__ label on an in-class member function.
         */
        inline static void process_commands()
        {
            // First: VBlank keeps running while halted (SGL needs it), so a Poll() it calls
            // must see this flag and skip its GDB work. Don't mask interrupts instead.
            ReentrancyGuard reentrancy_guard;

            // Purges the cache on every exit path, so patched code (breakpoints, 'M',
            // step traps) is never run stale. Keep it before the first PurgeCache().
            CacheFlusher flusher;

            // Releases a parked slave on every exit path (see SlaveReleaseGuard).
            SlaveReleaseGuard slave_release_guard;

            // The caches don't snoop each other: drop any line the slave has written since
            // (its saved registers, g_slave_ctx), so this halt never shows a stale copy.
            ForcePurgeCache();

            constexpr size_t max_g_packet_hex_chars = (sizeof(SH2Context) + (GdbFixedShPaddingRegisters * 4U)) * 2;
            static_assert(max_g_packet_hex_chars < 1024, "out_buf is too small for GDB 'g' packet");
            char in_buf[1024];
            char out_buf[1024];

            SRL::Logger::Log::LogPrint("[GDBStub] process_commands() entered. PC: 0x%08lX",
                                       static_cast<unsigned long>(GDBStub::MasterSH2().pc));

            adjust_pc_for_software_breakpoint();

            // Clear the UBC channel A match flags (CMFCA/CMFPA, bits 15-14) in BRCR.
            volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF78U); // not 0xFFFFFF60: that's BARBH
            *BRCR &= ~0xC000U;

            undo_software_step();

            // Before a session, drop stale bytes queued while the Saturn booted (a queued
            // continue would resume the first Break() at once). Stop on unplug: a floating
            // bus never reads empty. Not from Poll() (CtxIsFake): those bytes are GDB's
            // first qSupported.
            if (!g_session.HasConnection && !g_stop.CtxIsFake)
            {
                while (!SRL::DevCart::CS0::IsRxfEmpty())
                {
                    if (!SRL::DevCart::CS0::IsConnected())
                    {
                        break;
                    }
                    (void)*(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo);
                }
            }
            // Not on Poll()'s path either: there a new GDB's first packet brought us here, and
            // HandshakeDone may be stale from a GDB that died -- an unrequested stop reply
            // would arrive before its qSupported answer and shift every reply after it.
            else if (g_session.HandshakeDone && !g_stop.CtxIsFake)
            {
                // GDB is waiting on a c/s: report this stop.
                send_stop_signal(current_stop_signal());
            }

            while (true)
            {
                out_buf[0] = 0;
                if (!packet_get(in_buf, sizeof(in_buf)))
                {
                    // USB disconnected while waiting for a packet -- stop processing.
                    return;
                }

                switch (in_buf[0])
                {
                case '!':
                    // Enable extended-remote mode. We already tolerate vRun without this,
                    // but acknowledging it properly avoids relying on that leniency.
                    packet_put('\0', "OK", 2);
                    break;
                case 'k': // Kill: no defined reply per the RSP spec -- just clean up.
                    g_stop.SignalOverride = 0;
                    clear_breakpoints(true);
                    g_session.HandshakeDone = false;
                    g_session.HasConnection = false;
                    return;
                case '?':
                    // First '?' marks the connection as active and sends the stop reason.
                    g_session.HasConnection = true;
                    send_stop_signal(current_stop_signal());
                    break;
                case 'q':
                    if (starts_with(in_buf, "qSupported"))
                    {
                        // A new GDB session starts with no breakpoints: drop any that a GDB which
                        // died while the target ran (no D/k) left patched in -- they would keep
                        // firing, with GDB silently resuming from stops it never asked for.
                        clear_breakpoints(true);
                        remove_hardware_watchpoint(0U, 0U);

                        // Advertise swbreak and hwbreak (Z1-Z4 are implemented via the UBC,
                        // see install_hardware_watchpoint). No qXfer:features target description:
                        // GDB's SH port ignores it. PacketSize stays in sync with kPacketDataMax.
                        constexpr const char kFeaturesStr[] = ";swbreak+;hwbreak+";

                        static constexpr size_t max_qsupported_len = (sizeof(kPacketSizeStr) - 1) + (sizeof(kFeaturesStr) - 1);
                        static_assert(max_qsupported_len < sizeof(out_buf), "qSupported payload exceeds buffer");

                        size_t out_len = 0;
                        for (size_t i = 0; i < sizeof(kPacketSizeStr) - 1; ++i)
                        {
                            if (out_len < sizeof(out_buf) - 1)
                                out_buf[out_len++] = kPacketSizeStr[i];
                        }
                        for (const char *s = kFeaturesStr; *s; ++s)
                        {
                            if (out_len < sizeof(out_buf) - 1)
                                out_buf[out_len++] = *s;
                        }

                        out_buf[out_len] = '\0';
                        packet_put('\0', out_buf, out_len);
                        g_session.HandshakeDone = true;
                    }
                    else if (starts_with(in_buf, "qfThreadInfo"))
                    {
                        // Single-thread target.
                        packet_put('\0', "m1", 2);
                    }
                    else if (starts_with(in_buf, "qsThreadInfo"))
                    {
                        packet_put('\0', "l", 1);
                    }
                    else if (starts_with(in_buf, "qAttached"))
                    {
                        packet_put('\0', "1", 1);
                    }
                    else if (starts_with(in_buf, "qOffsets"))
                    {
                        constexpr const char offsets[] = "Text=0;Data=0;Bss=0";
                        packet_put('\0', offsets, sizeof(offsets) - 1);
                    }
                    else if (starts_with(in_buf, "qC"))
                    {
                        packet_put('\0', "QC1", 3);
                    }
                    else if (starts_with(in_buf, "qRcmd,"))
                    {
                        // `monitor <text>` (hex-encoded). "regs slave" and "trace" are answered
                        // here; anything else is stored for user code (GetLastMonitorCommand()).
                        const char *hex_payload = in_buf + 6;
                        size_t hex_len = 0;
                        while (hex_payload[hex_len] != '\0')
                            hex_len++;
                        const size_t cmd_len = hex_len / 2;
                        if (hex_len == 0 || (hex_len & 1U) != 0U || cmd_len >= sizeof(g_monitor.LastCommand))
                        {
                            packet_put('\0', "E01", 3);
                        }
                        else if (hex2mem(hex_payload, reinterpret_cast<uint8_t *>(g_monitor.LastCommand), static_cast<int>(cmd_len)))
                        {
                            g_monitor.LastCommand[cmd_len] = '\0';
                            if (str_equals(g_monitor.LastCommand, "regs slave"))
                            {
                                send_slave_regs_dump();
                                packet_put('\0', "OK", 2);
                            }
                            else if (str_equals(g_monitor.LastCommand, "trace"))
                            {
                                send_halt_trace_dump();
                                packet_put('\0', "OK", 2);
                            }
                            else
                            {
                                g_monitor.CommandCount = g_monitor.CommandCount + 1;
                                packet_put('\0', "OK", 2);
                            }
                        }
                        else
                        {
                            packet_put('\0', "E01", 3);
                        }
                    }
                    else
                    {
                        packet_put('\0', nullptr, 0);
                    }
                    break;
                case 'H':
                    packet_put('\0', "OK", 2);
                    break;

                case 'v':
                    // vCont? (and any other v packet but vRun) gets the empty "not supported"
                    // reply: GDB only uses vCont when the stub also offers C/S, and falls back
                    // to plain c/s/C either way.
                    if (starts_with(in_buf, "vRun"))
                    {
                        // Extended-remote run compatibility: treat like continue.
                        handle_gdb_continue();
                        return;
                    }
                    else
                    {
                        packet_put('\0', nullptr, 0);
                    }
                    break;
                case 'g':
                {
                    if (GDBStub::MasterSH2().pc == 0)
                    {
                        snapshot_polling_context();
                    }

                    // Real PR/R14 even inside SGL code without debug info: GDB's `bt` copes,
                    // but a zeroed PR broke every later command of the session.
                    SH2Context ctx_copy = GDBStub::MasterSH2();

                    char *p_out = out_buf;
                    p_out = mem2hex((uint8_t *)&ctx_copy, p_out, sizeof(SH2Context));

                    // Pad to the fixed size GDB's SH port requires (see GdbFixedShRegisterCount).
                    for (size_t i = 0; i < GdbFixedShPaddingRegisters * 4U; ++i)
                    {
                        *p_out++ = '0';
                        *p_out++ = '0';
                    }
                    *p_out = '\0';

                    const int tx_len = static_cast<int>(p_out - out_buf);
                    packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                }
                break;
                case 'G':
                {
                    // GDB echoes the padded 'g' reply: write the core registers, ignore the rest.
                    constexpr size_t core_len = sizeof(SH2Context) * 2;
                    size_t len = 0;
                    while (in_buf[1 + len] != '\0')
                        len++;
                    if (len < core_len)
                    {
                        packet_put('\0', "E01", 3);
                    }
                    else if (hex2mem(&in_buf[1], (uint8_t *)&g_ctx, sizeof(SH2Context)))
                    {
                        packet_put('\0', "OK", 2);
                    }
                    else
                    {
                        packet_put('\0', "E01", 3);
                    }
                }
                break;
                case 'p': // Read a single register
                {
                    uint32_t reg_idx = 0;
                    const char *ptr = &in_buf[1];
                    uint32_t *reg = parse_hex_u32_until(ptr, '\0', reg_idx, ptr) ? register_slot(reg_idx) : nullptr;
                    if (reg == nullptr)
                    {
                        packet_put('\0', "E01", 3);
                        break;
                    }
                    const int tx_len = static_cast<int>(mem2hex(reinterpret_cast<uint8_t *>(reg), out_buf, 4) - out_buf);
                    packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                }
                break;
                case 'P': // Write a single register
                {
                    uint32_t reg_idx = 0;
                    const char *ptr = &in_buf[1];
                    uint32_t *reg = nullptr;
                    if (parse_hex_u32_until(ptr, '=', reg_idx, ptr) && *ptr == '=')
                    {
                        reg = register_slot(reg_idx);
                    }
                    if (reg != nullptr && hex2mem(ptr + 1, reinterpret_cast<uint8_t *>(reg), 4))
                    {
                        packet_put('\0', "OK", 2);
                    }
                    else
                    {
                        packet_put('\0', "E01", 3);
                    }
                }
                break;
                case 'm':
                {
                    uint32_t addr = 0, length = 0;
                    const char *ptr = &in_buf[1];
                    if (!parse_hex_u32_until(ptr, ',', addr, ptr) || *ptr != ',')
                    {
                        packet_put('\0', "E01", 3);
                        break;
                    }
                    ++ptr; // skip ','
                    if (!parse_hex_u32_until(ptr, '\0', length, ptr))
                    {
                        packet_put('\0', "E01", 3);
                        break;
                    }
                    // Keep response within local buffer limits (hex encoding = 2x bytes + NUL).
                    if (length > 511U || !is_valid_memory_range(addr, length))
                    {
                        packet_put('\0', "E01", 3);
                        break;
                    }

                    // Cache-through, like every patch: a breakpoint written this halt must not
                    // be hidden by a stale cache line before CacheFlusher purges.
                    const int tx_len = static_cast<int>(mem2hex((uint8_t *)(addr | 0x20000000U), out_buf, static_cast<int>(length)) - out_buf);
                    packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                }
                break;
                case 'M':
                {
                    uint32_t addr = 0, length = 0;
                    const char *p = &in_buf[1];
                    if (!parse_hex_u32_until(p, ',', addr, p) || *p != ',')
                    {
                        packet_put('\0', "E02", 3);
                        break;
                    }
                    ++p; // skip ','
                    if (!parse_hex_u32_until(p, ':', length, p) || *p != ':')
                    {
                        packet_put('\0', "E02", 3);
                        break;
                    }
                    ++p; // skip ':'

                    if (length > 511U || !is_valid_memory_range(addr, length))
                    {
                        packet_put('\0', "E02", 3);
                        break;
                    }

                    // Cache-through, and word-sized stores for VDP RAM (see hex2mem_aligned()).
                    if (hex2mem_aligned(p, addr | 0x20000000U, length))
                    {
                        PurgeCache();
                        packet_put('\0', "OK", 2);
                    }
                    else
                    {
                        packet_put('\0', "E01", 3);
                    }
                }
                break;
                case 'Z':
                case 'z':
                {
                    // RSP breakpoints/watchpoints: Z[type],addr,kind / z[type],addr,kind
                    const char type_char = in_buf[1];
                    if ((type_char < '0' || type_char > '4') || in_buf[2] != ',')
                    {
                        packet_put('\0', nullptr, 0); // Not supported type or bad format
                        break;
                    }

                    const uint32_t wp_type = static_cast<uint32_t>(type_char - '0');

                    uint32_t addr = 0;
                    const char *p = &in_buf[3];
                    if (!parse_hex_u32_until(p, ',', addr, p) || *p != ',')
                    {
                        packet_put('\0', "E03", 3);
                        break;
                    }

                    ++p; // skip ','
                    uint32_t kind = 0;
                    const char *end = p;
                    if (!parse_hex_u32_until(p, '\0', kind, end))
                    {
                        packet_put('\0', "E03", 3);
                        break;
                    }

                    bool ok = false;
                    if (in_buf[0] == 'Z')
                    {
                        if (wp_type == 0)
                        {
                            // SH-2 instructions are 16-bit (kind normally 2).
                            if (kind == 0U || kind == 2U)
                            {
                                ok = install_software_breakpoint(addr);
                            }
                        }
                        else
                        {
                            ok = install_hardware_watchpoint(addr, wp_type);
                        }
                    }
                    else
                    {
                        if (wp_type == 0)
                        {
                            if (kind == 0U || kind == 2U)
                            {
                                ok = remove_software_breakpoint(addr);
                            }
                        }
                        else
                        {
                            ok = remove_hardware_watchpoint(addr, wp_type);
                        }
                    }

                    packet_put('\0', ok ? "OK" : "E03", ok ? 2 : 3);
                }
                break;
                case 'D': // Detach
                    packet_put('\0', "OK", 2);
                    g_stop.SignalOverride = 0;
                    clear_breakpoints(true);
                    g_session.HandshakeDone = false;
                    g_session.HasConnection = false;
                    return;
                case 'T': // Is thread alive?
                    // Report thread as alive for single-thread target.
                    packet_put('\0', "OK", 2);
                    break;
                case 'c':
                case 'C':
                    // GDB sends "C sig" to resume after a stop it believes was a
                    // signal (e.g. SIGILL); without this it waits forever on the empty reply.
                    apply_resume_address(in_buf);
                    handle_gdb_continue();
                    return;
                case 's':
                case 'S':
                    apply_resume_address(in_buf);
                    handle_gdb_step();
                    return;
                default:
                    packet_put('\0', nullptr, 0);
                    break;
                }
            }
        }
        // --- Exception Handler ---
        // (thunk entry points are declared extern "C" at the top of this file)

        /**
         * @brief Hooks the master's illegal-instruction, address-error, UBC and TRAPA #35
         * vectors, first moving the vector table to RAM if VBR still points at the boot ROM.
         * @details NMI (vector 11, the Reset button) is deliberately left to the BIOS/SGL.
         */
        inline static void InstallExceptionHandlers()
        {
            SRL::Logger::Log::LogPrint("[GDBStub] InstallExceptionHandlers() start");
            if (!g_handlers_installed)
            {
                // Read the current VBR
                uint32_t current_vbr = 0;
                asm volatile("stc vbr, %0" : "=r"(current_vbr)); // current_vbr = this CPU's (the master's) current VBR

                // VBR 0 is the read-only boot ROM: copy its table to RAM and point VBR there.
                if (current_vbr == 0)
                {
                    current_vbr = 0x06000000;
                    volatile uint32_t *src_table = reinterpret_cast<volatile uint32_t *>(0x20000000U);
                    volatile uint32_t *dst_table = reinterpret_cast<volatile uint32_t *>(current_vbr | 0x20000000U);
                    for (int i = 0; i < 64; i++)
                    {
                        dst_table[i] = src_table[i];
                    }
                    asm volatile("ldc %0, vbr" ::"r"(current_vbr)); // VBR = current_vbr (0x06000000) -- point the CPU at the freshly-copied RAM table
                }

                // Write directly to the Cache-Through mirror of the VBR table
                volatile uint32_t *vbr_table = reinterpret_cast<volatile uint32_t *>(current_vbr | 0x20000000U);

                // Vectors 0-31 are CPU exceptions, 32-63 TRAPA #N. Each exception family goes
                // through its tag thunk so GDB sees SIGILL/SIGBUS/hwbreak; TRAPA #35 reports SIGTRAP.
                vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);    // Illegal Instruction
                vbr_table[5] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);    // Reserved Instruction
                vbr_table[6] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);    // Slot Illegal Instruction
                vbr_table[7] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);    // General Illegal Instruction
                vbr_table[8] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);    // Slot Reserved Instruction
                vbr_table[9] = reinterpret_cast<uint32_t>(&srl_gdbstub_addrerr_thunk);    // CPU Address Error
                vbr_table[10] = reinterpret_cast<uint32_t>(&srl_gdbstub_addrerr_thunk);   // DMA Address Error
                vbr_table[12] = reinterpret_cast<uint32_t>(&srl_gdbstub_ubc_thunk);       // User Break Controller
                vbr_table[35] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // TRAPA #35 (Legacy/Fallback)

                ForcePurgeCache();
                g_handlers_installed = true;
            }
            SRL::Logger::Log::LogPrint("[GDBStub] InstallExceptionHandlers() end");
        }

    public:

        /**
         * @brief Master SH-2 register state captured at the current debug stop.
         * @return Reference to the master's saved context
         */
        inline static SH2Context &MasterSH2()
        {
            return g_ctx;
        }

        /**
         * @brief Slave SH-2 register state captured by the slave-side thunks.
         * @return Reference to the slave's saved context
         */
        inline static SH2Context &SlaveSH2()
        {
            return g_slave_ctx;
        }

        /**
         * @brief Returns how many RSP packets have been received.
         * @return Number of RSP packets received since Init()
         */
        inline static uint32_t GetCommandCount()
        {
            return g_session.CommandCount;
        }

        /**
         * @brief Returns the most recently received RSP packet (truncated to 63 chars).
         * @return Null-terminated packet payload, empty if none
         */
        inline static const char *GetLastCommand()
        {
            return g_session.LastCommand;
        }

        /**
         * @brief Initialize the GDB stub and hook exception vectors.
         */
        inline static void Init()
        {
            SRL::Logger::Log::LogPrint("[GDBStub] Init() start");
            g_session.HasConnection = false;
            g_session.HandshakeDone = false;
            g_session.CommandCount = 0;
            g_exception_thunk_count = 0;
            g_transfer.RxDetectCount = 0;
            g_transfer.TxByteCount = 0;
            g_transfer.PollFallbackCount = 0;
            g_session.LastCommand[0] = '\0';
            g_transfer.UngetChar = -1;
            // Detect the cart through CS0 only: reading CS1 can hang the bus on USBGamers carts.
            g_devcart.Ready = SRL::DevCart::CS0::IsPortAvailable(); // CS0-only check
            g_devcart.PortAvailable = SRL::DevCart::CS0::IsPortAvailable();

            g_stop.SignalOverride = 0;
            g_last_stop_signal = 5;
            g_breakpoints.UbcChannelAActive = false;
            clear_breakpoints(false);

            SRL::Logger::Log::LogPrint("[GDBStub] DevCart ready: %d, Port: %d",
                                       g_devcart.Ready ? 1 : 0, g_devcart.PortAvailable ? 1 : 0);

            InstallExceptionHandlers(); // no-op if already installed
            SRL::Logger::Log::LogPrint("[GDBStub] Init() end");
        }

        /**
         * @brief Returns true only after the GDB handshake (qSupported exchange) has completed.
         * @return true once GDB has completed the qSupported handshake
         */
        inline static bool IsConnected()
        {
            return g_session.HandshakeDone;
        }

        /**
         * @brief Returns true once the GDB exception handlers have been installed.
         * @return true once Init() has patched the master's exception vectors
         */
        inline static bool IsHandlersInstalled()
        {
            return g_handlers_installed;
        }

        /**
         * @brief Returns how many times the master exception thunk has executed.
         * @return Number of entries into srl_gdbstub_exception_thunk
         */
        inline static uint32_t GetExceptionThunkCount()
        {
            return g_exception_thunk_count;
        }

        /**
         * @brief Returns how many RX bytes were consumed by the stub from DevCart.
         * @return Number of bytes read from DevCart RX
         */
        inline static uint32_t GetRxDetectCount()
        {
            return g_transfer.RxDetectCount;
        }

        /**
         * @brief Returns how many TX bytes were written by the stub to DevCart.
         * @return Number of bytes written to DevCart TX
         */
        inline static uint32_t GetTxByteCount()
        {
            return g_transfer.TxByteCount;
        }

        /**
         * @brief Returns how many times Poll() processed RX data itself, outside of an exception.
         * @return Number of times Poll() handled RX data itself
         */
        inline static uint32_t GetPollFallbackCount()
        {
            return g_transfer.PollFallbackCount;
        }

        /**
         * @brief Returns true when the DevCart was detected at Init() (CS0 USB_FLAGS readable; CS1 is deliberately not probed).
         * @return true if the DevCart was detected at Init()
         */
        inline static bool IsDevCartReady()
        {
            return g_devcart.Ready;
        }

        /**
         * @brief Returns true when the DevCart USB port reported available at the last Poll().
         * @return true if the DevCart USB port was available at the last Poll()
         */
        inline static bool IsDevCartPortAvailable()
        {
            return g_devcart.PortAvailable;
        }

        /**
         * @brief Returns how many `monitor <text>` commands have been received via qRcmd.
         * @details Compare against a cached value each frame to detect a new command, then
         * dispatch on GetLastMonitorCommand().
         * @warning Commands arrive while the target is stopped, so several sent before one
         * `continue` are seen as one: only the last takes effect. They are not queued.
         * @return Number of non-built-in monitor commands received
         */
        inline static uint32_t GetMonitorCommandCount()
        {
            return g_monitor.CommandCount;
        }

        /**
         * @brief Returns the text of the most recently received `monitor` command.
         * @return Null-terminated command text, empty if none
         */
        inline static const char *GetLastMonitorCommand()
        {
            return g_monitor.LastCommand;
        }

        /**
         * @brief Returns how many times the slave's illegal-instruction
         * (breakpoint) handler has fired.
         * @details Zero unless InstallSlaveExceptionHandler() ran on the slave.
         * @return Number of times the slave breakpoint handler has fired
         */
        inline static uint32_t GetSlaveBreakpointCount()
        {
            return g_slave_bp_count;
        }

        /**
         * @brief Installs GDBStub's breakpoint handler on the Illegal Instruction
         * vector (4) of whichever CPU executes this function.
         *
         * @warning MUST be called from code running ON THE SLAVE SH-2 (e.g. via
         * SRL::Slave::ExecuteOnSlave / InstallSlaveExceptionTask below).
         *
         * @details A breakpoint (0xFFFF) in slave code otherwise lands on the slave's
         * unconfigured boot-ROM vector and never returns. Under SGL the slave's VBR is
         * already in RAM (~0x06000400), so vector 4 is patched in SGL's live table.
         * @warning Slave breakpoints are one-shot (removed when hit) and slave code can't be
         * stepped. GDB sees one CPU: a slave stop halts the session like Ctrl-C, and the
         * slave's state is read with `monitor regs slave`.
         */
        inline static void InstallSlaveExceptionHandler()
        {
            uint32_t vbr = 0;
            asm volatile("stc vbr, %0" : "=r"(vbr)); // vbr = this CPU's (the slave's) current VBR

            if (vbr == 0)
            {
                // Boot ROM VBR: relocate to a table distinct from the master's 0x06000000.
                vbr = 0x06010000U;
                volatile uint32_t *src_table = reinterpret_cast<volatile uint32_t *>(0x20000000U);
                volatile uint32_t *dst_table = reinterpret_cast<volatile uint32_t *>(vbr | 0x20000000U);
                for (int i = 0; i < 64; i++)
                {
                    dst_table[i] = src_table[i];
                }
                asm volatile("ldc %0, vbr" ::"r"(vbr)); // VBR = vbr (0x06010000) -- point the slave at its freshly-copied (or already-relocated) RAM table
            }

            volatile uint32_t *vbr_table = reinterpret_cast<volatile uint32_t *>(vbr | 0x20000000U);
            vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_slave_illegal_thunk);

            ForcePurgeCache();
            g_slave.HandlersInstalled = true;
        }

        /**
         * @brief Convenience task that installs GDBStub's slave-side breakpoint handler.
         * @details Must be dispatched via SRL::Slave::ExecuteOnSlave so that
         * InstallSlaveExceptionHandler() actually executes on the slave CPU:
         * @code
         * SRL::GDBStub::InstallSlaveExceptionTask installTask;
         * SRL::Slave::ExecuteOnSlave(installTask);
         * while (installTask.IsRunning()) { }
         * @endcode
         * @see InstallSlaveExceptionHandler
         */
        class InstallSlaveExceptionTask : public SRL::Types::ITask
        {
        protected:
            /**
             * @brief Runs InstallSlaveExceptionHandler() on the slave CPU.
             */
            void Do() override
            {
                InstallSlaveExceptionHandler();
            }
        };

        /**
         * @brief Enter the GDB stub via software trap (Illegal Instruction).
         * @warning noinline is load-bearing: the stub resumes 2 bytes past the 0xFFFF, which
         * must land on this function's own rts, not on the caller's next instruction.
         */
        __attribute__((noinline)) inline static void Break()
        {
            // 0xFFFF is tagged SIGILL by its thunk: report SIGTRAP unless Poll() already chose
            // a signal (see StopStatus::SignalOverride). Not TRAPA: SGL overwrites those vectors.
            if (g_stop.SignalOverride == 0)
            {
                g_stop.SignalOverride = 5U; // SIGTRAP
            }

            asm volatile(".word 0xFFFF" ::: "memory"); // emit the raw 0xFFFF opcode -- not a real instruction, deliberately traps as Illegal Instruction
        }

        /**
         * @brief Check for incoming GDB interrupt request
         */
        __attribute__((noinline)) inline static void Poll()
        {
            // Called from VBlank while halted in process_commands(): do nothing, so the
            // halt's own packet loop keeps the FIFO (see ReentrancyGuard).
            if (ReentrancyGuard::IsInProcessCommands())
            {
                return;
            }

            // Turn a slave breakpoint into a master stop, reported as SIGINT like Ctrl-C:
            // the master's own stack says nothing about it (use `monitor regs slave`).
            if (Uncached(g_slave.Stopped))
            {
                g_slave.Reported = true; // this halt tells GDB, so SlaveReleaseGuard may release it
                g_stop.SignalOverride = 2U; // SIGINT
                Break();
            }

            const uint8_t usbFlags = SRL::DevCart::CS0::ReadFlags();
            g_devcart.PortAvailable = SRL::DevCart::CS0::IsPortAvailable();

            const bool rxPending = (usbFlags & SRL::DevCart::CS0::USBFlags::Rxf) == 0;
            if (rxPending)
            {
                if (!g_session.HasConnection || !g_session.HandshakeDone)
                {
                    // During initial attach/handshake, do not consume RX bytes here.
                    // Let process_commands() read the full '$...#xx' packet intact.
                    snapshot_polling_context();
                    g_transfer.PollFallbackCount = g_transfer.PollFallbackCount + 1;
                    process_commands();
                }
                else
                {
                    // Active session while target runs: only Ctrl-C should interrupt.
                    // Direct FIFO read, not CS0::Read(): its WaitRxf() spins forever on unplug.
                    const uint8_t ch = *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo);
                    g_transfer.RxDetectCount = g_transfer.RxDetectCount + 1;

                    if (ch == 0x03U)
                    {
                        record_command("<Ctrl-C>");
                        g_stop.SignalOverride = 2U; // SIGINT
                        g_transfer.PollFallbackCount = g_transfer.PollFallbackCount + 1;
                        Break();
                    }
                    else if (ch == '$')
                    {
                        // Preserve packet start byte and process packet without forcing a trap.
                        g_transfer.UngetChar = '$';
                        snapshot_polling_context();
                        g_transfer.PollFallbackCount = g_transfer.PollFallbackCount + 1;
                        process_commands();
                    }
                }
            }

            // Hardware-level disconnect: clear session state.
            if (!SRL::DevCart::CS0::IsConnected())
            {
                g_session.HasConnection = false;
                g_session.HandshakeDone = false;
            }
        }
    };
}

/**
 * @details Fixed-symbol entry point the master exception thunk jsr's into (see
 * the literal pool of _srl_gdbstub_exception_thunk below). Needed because an
 * __asm__ label on an in-class member function definition is ignored by GCC.
 */
extern "C" __attribute__((used)) inline void srl_gdbstub_process_commands(void)
{
    SRL::GDBStub::g_exception_thunk_count = SRL::GDBStub::g_exception_thunk_count + 1; // diagnostic: every real halt bumps this
    // A real exception: drop a fake-context flag left by a Poll() session that ended without c/s.
    SRL::GDBStub::g_stop.CtxIsFake = false;
    SRL::GDBStub::process_commands();
}

/**
 * @details Called by srl_gdbstub_slave_illegal_thunk on the slave, after it saved the
 * slave's registers into g_slave_ctx. Removes the hit breakpoint (one-shot), raises
 * g_slave.Stopped for Poll() on the master, then spins until g_slave.Resume.
 * @note g_slave flags are read through Uncached(): the caches don't snoop each other.
 */
extern "C" __attribute__((used)) inline void slave_breakpoint_handler(void)
{
    SRL::GDBStub::g_slave_bp_count = SRL::GDBStub::g_slave_bp_count + 1; // diagnostic: every slave thunk entry bumps this
    // The master wrote the breakpoint table; drop any stale copy in the slave's cache first.
    SRL::GDBStub::ForcePurgeCache();

    // Illegal Instruction pushes the address of the faulting instruction
    // itself (unlike TRAPA, which pushes PC+2).
    const uint32_t bp_addr = SRL::GDBStub::SlaveSH2().pc;
    if (SRL::GDBStub::find_breakpoint_slot(bp_addr) < 0)
    {
        // Not a GDB breakpoint, but maybe the master's single-step trap (kept out of the
        // breakpoint table): wait for the master to finish its step and restore the
        // instruction. Read through the cache-through mirror -- the master writes these.
        volatile auto &step = *reinterpret_cast<volatile decltype(SRL::GDBStub::g_step_data) *>(
            reinterpret_cast<uint32_t>(&SRL::GDBStub::g_step_data) | 0x20000000U);
        while (step.active && step.address == bp_addr)
        {
            asm volatile("nop"); // spin -- the master removes its trap once its own step stops
        }
        // Not 0xFFFF any more (that trap, or a breakpoint removed after our cache fetched it):
        // run the real instruction -- no stop, PC unchanged.
        if (*reinterpret_cast<volatile uint16_t *>(bp_addr | 0x20000000U) != SRL::GDBStub::SoftwareBreakInstruction)
        {
            SRL::GDBStub::ForcePurgeCache();
            return;
        }
    }

    if (SRL::GDBStub::find_breakpoint_slot(bp_addr) >= 0)
    {
        // One-shot: remove it so the real instruction runs on resume (no slave step-over).
        SRL::GDBStub::remove_software_breakpoint(bp_addr);
    }
    else
    {
        // A 0xFFFF compiled into the code (e.g. Break()): step past it, as on the master.
        SRL::GDBStub::SlaveSH2().pc += 2U;
    }

    // Clear Resume before raising Stopped, so the master can't release us before we wait.
    SRL::GDBStub::g_slave.Resume = false;
    SRL::GDBStub::g_slave.Stopped = true;

    while (!SRL::GDBStub::Uncached(SRL::GDBStub::g_slave.Resume))
    {
        asm volatile("nop"); // spin -- waiting for the master to release this slave breakpoint stop, no state to touch
    }

    SRL::GDBStub::g_slave.Stopped = false;

    // The master may have patched more breakpoints into memory while we were
    // halted; purge the slave's own cache before resuming.
    SRL::GDBStub::ForcePurgeCache();
    // Returning here lets srl_gdbstub_slave_illegal_thunk restore registers and rte.
}

// Exception thunks. Every instruction is commented: a wrong line here silently shows
// GDB wrong registers or corrupts the resume.
//
// SRL_GDBSTUB_CONTEXT_THUNK name, ctx, handler: saves every register into the
// SH2Context at `ctx`, calls `handler`, restores the (possibly GDB-edited) context
// and returns from the exception. Used for the master (g_ctx, process_commands) and
// the slave (g_slave_ctx, slave_breakpoint_handler). SH2Context offsets: r[N] at N*4,
// then pc=64, pr=68, gbr=72, vbr=76, mach=80, macl=84, sr=88.
//
// SRL_GDBSTUB_TAG_THUNK name, var, value: the SH-2 has no "cause" register, so each
// exception family gets a stub that stores `value` into the byte `var`, then jumps to
// the master context thunk with every register (r0/r1 saved and restored) untouched.
//
// One __asm__ statement, since GCC may reorder separate top-level asm; .purgem frees the
// names for other translation units' asm in the same assembler file.
__asm__(
    ".macro SRL_GDBSTUB_CONTEXT_THUNK name, ctx, handler\n"
    ".weak \\name\n"
    ".global \\name\n"
    ".align 2\n"
    "\\name:\n"
    "mov.l r0, @-r15\n"        // push r0 (predecrement r15, store) -- stash the CPU's true pre-exception r0 so it isn't lost to the scratch use below
    "stc.l gbr, @-r15\n"       // push gbr the same way -- stash it too, restored into ctx a few lines down
    "mov.l 1f, r0\n"           // r0 = &ctx (address loaded from the literal pool at label 1, below)
    "mov.l r14, @(14*4, r0)\n" // ctx.r[14] = r14
    "mov.l r13, @(13*4, r0)\n" // ctx.r[13] = r13
    "mov.l r12, @(12*4, r0)\n" // ctx.r[12] = r12
    "mov.l r11, @(11*4, r0)\n" // ctx.r[11] = r11
    "mov.l r10, @(10*4, r0)\n" // ctx.r[10] = r10
    "mov.l r9,  @(9*4,  r0)\n" // ctx.r[9] = r9
    "mov.l r8,  @(8*4,  r0)\n" // ctx.r[8] = r8
    "mov.l r7,  @(7*4,  r0)\n" // ctx.r[7] = r7
    "mov.l r6,  @(6*4,  r0)\n" // ctx.r[6] = r6
    "mov.l r5,  @(5*4,  r0)\n" // ctx.r[5] = r5
    "mov.l r4,  @(4*4,  r0)\n" // ctx.r[4] = r4
    "mov.l r3,  @(3*4,  r0)\n" // ctx.r[3] = r3
    "mov.l r2,  @(2*4,  r0)\n" // ctx.r[2] = r2
    "mov.l r1,  @(1*4,  r0)\n" // ctx.r[1] = r1 -- r0 itself is saved later, once it's no longer needed as the ctx pointer
    "mov r15, r1\n"            // r1 = current r15 (post both of our pushes above, i.e. pre-exception SP minus 16: 8 for the hardware's own PC/SR push, 8 for ours)
    "add #16, r1\n"            // r1 = pre-exception SP exactly -- undoes both pushes, giving GDB the SP the interrupted code actually had, not our exception-frame SP
    "mov.l r1, @(15*4, r0)\n"  // ctx.r[15] = r1 (the reconstructed, user-visible stack pointer)
    "mov.l @r15+, r1\n"        // pop our earlier gbr push back into r1, r15 += 4 (now pointing just past our two pushes)
    "mov.l @r15+, r2\n"        // pop our earlier r0 push into r2, r15 += 4 -- r15 is now exactly where the CPU's own exception entry left it, pointing at its PC/SR frame
    "mov.l r2, @r0\n"          // ctx.r[0] = r2 (the true pre-exception r0, recovered from the stack)
    "mov r0, r2\n"             // r2 = r0 (= &ctx) -- r0 keeps pointing at ctx's base for the rest of this thunk
    "add #64, r2\n"            // r2 = &ctx.pc -- base of the pc/pr/gbr/vbr/mach/macl/sr block, everything below is offset from here
    "mov.l r1, @(2*4, r2)\n"   // ctx.gbr = r1 (the gbr value popped a few lines up; offset 8 from &ctx.pc lands on .gbr)
    "mov.l @r15, r1\n"         // r1 = *r15, i.e. the CPU's own hardware-pushed PC (peek, not pop -- we overwrite it in place later so `rte` can pick up a possibly-modified value); for Illegal Instruction this is the 0xFFFF opcode's own address
    "mov.l r1, @r2\n"          // ctx.pc = r1
    "mov.l @(4, r15), r1\n"    // r1 = *(r15+4), the CPU's own hardware-pushed SR (same peek-not-pop reasoning)
    "mov.l r1, @(24, r2)\n"    // ctx.sr = r1 (offset 24 from &ctx.pc lands on .sr)
    "sts pr, r1\n"             // r1 = current PR -- exceptions don't auto-save PR the way they do PC/SR, so this captures whatever the interrupted code's own return address was
    "mov.l r1, @(1*4, r2)\n"   // ctx.pr = r1
    "stc vbr, r1\n"            // r1 = current VBR
    "mov.l r1, @(3*4, r2)\n"   // ctx.vbr = r1
    "sts mach, r1\n"           // r1 = MACH (multiply/accumulate high word)
    "mov.l r1, @(4*4, r2)\n"   // ctx.mach = r1
    "sts macl, r1\n"           // r1 = MACL (multiply/accumulate low word)
    "mov.l r1, @(5*4, r2)\n"   // ctx.macl = r1 -- full context snapshot is now complete
    "mov.l 2f, r1\n"           // r1 = &handler (literal pool label 2)
    "jsr @r1\n"                // call handler -- PR is set to the address right after this delay slot, i.e. the next instruction
    "nop\n"                    // jsr's mandatory delay slot (executes before the call target starts; nothing needed here). The handler owns the CPU until it decides to return -- for the master, the entire GDB session for this halt happens inside that call
    "mov.l 1f, r0\n"           // r0 = &ctx again -- reload since r0 isn't guaranteed preserved across the call above
    "mov r0, r2\n"             // r2 = r0 (= &ctx)
    "add #64, r2\n"            // r2 = &ctx.pc again, for the restore half
    "mov.l @(1*4, r2), r1\n"   // r1 = ctx.pr (possibly rewritten by the handler, e.g. by single-step bookkeeping)
    "lds r1, pr\n"             // PR = r1
    "mov.l @(3*4, r2), r1\n"   // r1 = ctx.vbr
    "ldc r1, vbr\n"            // VBR = r1
    "mov.l @(4*4, r2), r1\n"   // r1 = ctx.mach
    "lds r1, mach\n"           // MACH = r1
    "mov.l @(5*4, r2), r1\n"   // r1 = ctx.macl
    "lds r1, macl\n"           // MACL = r1
    "mov.l @(2*4, r2), r1\n"   // r1 = ctx.gbr
    "ldc r1, gbr\n"            // GBR = r1
    "mov.l @(24, r2), r1\n"    // r1 = ctx.sr (possibly rewritten by GDB/the handler -- e.g. to change the interrupt mask across a step)
    "mov.l r1, @(4, r15)\n"    // overwrite the hardware's own pushed SR slot on the stack with r1, so the `rte` below resumes with this (possibly new) SR
    "mov.l @r2, r1\n"          // r1 = ctx.pc (possibly rewritten -- this is how GDB redirects execution, e.g. a step-trap target or a `jump`)
    "mov.l r1, @r15\n"         // overwrite the hardware's own pushed PC slot with r1, so `rte` resumes at this (possibly new) address
    "mov.l @(14*4, r0), r14\n" // restore r14 from ctx.r[14] (possibly rewritten by GDB)
    "mov.l @(13*4, r0), r13\n" // restore r13
    "mov.l @(12*4, r0), r12\n" // restore r12
    "mov.l @(11*4, r0), r11\n" // restore r11
    "mov.l @(10*4, r0), r10\n" // restore r10
    "mov.l @(9*4,  r0), r9\n"  // restore r9
    "mov.l @(8*4,  r0), r8\n"  // restore r8
    "mov.l @(7*4,  r0), r7\n"  // restore r7
    "mov.l @(6*4,  r0), r6\n"  // restore r6
    "mov.l @(5*4,  r0), r5\n"  // restore r5
    "mov.l @(4*4,  r0), r4\n"  // restore r4
    "mov.l @(3*4,  r0), r3\n"  // restore r3
    "mov.l @(2*4,  r0), r2\n"  // restore r2
    "mov.l @(1*4,  r0), r1\n"  // restore r1
    // CRITICAL: r0 must be restored LAST, and this instruction assumes r0
    // STILL holds the base pointer to ctx (loaded before the epilogue).
    // Do NOT reorder this or use r0 as a scratch register above!
    "mov.l @r0, r0\n" // restore r0 itself, last, from ctx.r[0] (offset 0) -- r0 is both the value being restored and the pointer used to fetch it, hence "last"
    "rte\n"           // return from exception: on the SH-2 this pops SR then PC off the stack -- the very slots we overwrote above, so this resumes at the (possibly GDB-redirected) address with the (possibly GDB-redirected) SR
    "nop\n"           // rte's mandatory delay slot -- still executes in the pre-return context before control actually transfers
    ".align 4\n"
    "1: .long \\ctx\n"         // literal pool: address of the context struct
    "2: .long \\handler\n"     // literal pool: address of the handler
    ".endm\n"

    ".macro SRL_GDBSTUB_TAG_THUNK name, var, value\n"
    ".weak \\name\n"
    ".global \\name\n"
    ".align 2\n"
    "\\name:\n"
    "mov.l r0, @-r15\n"                  // push r0 -- only r0/r1 are used as scratch here, and both are restored before falling into the context thunk, so it sees them untouched
    "mov.l r1, @-r15\n"                  // push r1
    "mov.l 1f, r0\n"                     // r0 = &var (literal pool label 1)
    "mov #\\value, r1\n"                 // r1 = value
    "mov.b r1, @r0\n"                    // var = value -- tags this halt before the context thunk (which doesn't know which vector fired) takes over
    "mov.l @r15+, r1\n"                  // pop r1 back
    "mov.l @r15+, r0\n"                  // pop r0 back -- r15 is now exactly where the CPU's own exception entry left it, as the context thunk expects
    "bra _srl_gdbstub_exception_thunk\n" // branch into the master context thunk
    "nop\n"                              // bra's mandatory delay slot
    ".align 4\n"
    "1: .long \\var\n"                   // literal pool: address of var
    ".endm\n"

    "SRL_GDBSTUB_CONTEXT_THUNK _srl_gdbstub_exception_thunk, srl_gdbstub_ctx, _srl_gdbstub_process_commands\n"
    "SRL_GDBSTUB_TAG_THUNK _srl_gdbstub_illegal_thunk, srl_gdbstub_last_stop_signal, 4\n"  // SIGILL
    "SRL_GDBSTUB_TAG_THUNK _srl_gdbstub_addrerr_thunk, srl_gdbstub_last_stop_signal, 10\n" // SIGBUS: CPU/DMA address errors
    "SRL_GDBSTUB_TAG_THUNK _srl_gdbstub_ubc_thunk, srl_gdbstub_ubc_hit, 1\n"               // lets process_commands() report a hwbreak/watch hit
    "SRL_GDBSTUB_CONTEXT_THUNK _srl_gdbstub_slave_illegal_thunk, srl_gdbstub_slave_ctx, _slave_breakpoint_handler\n"
    ".purgem SRL_GDBSTUB_CONTEXT_THUNK\n"
    ".purgem SRL_GDBSTUB_TAG_THUNK\n"
);

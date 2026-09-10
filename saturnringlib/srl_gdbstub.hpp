#pragma once

#include <srl_devcart.hpp>
#include <srl_interrupt.hpp>
#include <srl_system.hpp>
#include <srl_log.hpp>
#include <srl_slave.hpp>
#include <cstdint>
#include <cstddef>

extern "C" {
    void slave_ipi_handler(void);
}

namespace SRL
{
    /**
     * @brief A basic GDB Remote Serial Protocol stub for the Sega Saturn using the DevCart.
     * 
     * Uses Interrupt::Vector::Trap3 (trapa #3) for software breakpoints.
     */
    namespace GDBStub
    {
        // GDB Remote protocol expects registers for SH in this exact order:
        // R0-R15, PC, PR, GBR, VBR, MACH, MACL, SR
        /**
         * @brief Represents the SH-2 CPU register state.
         * 
         * This exactly matches the register layout expected by GDB's SH architecture.
         */
        struct SH2Context {
            uint32_t r[16];
            uint32_t pc;
            uint32_t pr;
            uint32_t gbr;
            uint32_t vbr;
            uint32_t mach;
            uint32_t macl;
            uint32_t sr;
        };

        // SRL break vector — using TRAPA #3 (Vector 35, safe from SCU interrupts)
        static constexpr uint32_t BreakTrapNumber = 3;

        // Upstream libyaul-gdbstub compatibility surface.
        static constexpr uint32_t GDBSTUB_LOAD_ADDRESS = 0x202FE000;
        // NOTE: libyaul uses TRAP #32 for their standalone binary; SRL uses TRAP #3 (BreakTrapNumber).
        static constexpr uint32_t GDBSTUB_TRAPA_VECTOR_NUMBER = 32;

        using gdb_device_init_t = void (*)(void);
        using gdb_device_byte_read_t = uint8_t (*)(void);
        using gdb_device_byte_write_t = void (*)(uint8_t value);

        struct __attribute__((aligned(16))) gdb_device_t {
            gdb_device_init_t init;
            gdb_device_byte_read_t byte_read;
            gdb_device_byte_write_t byte_write;
        };

        struct __attribute__((packed)) gdb_version_t {
            unsigned int :8;
            unsigned int major:8;
            unsigned int minor:8;
            unsigned int patch:8;
        };

        struct __attribute__((aligned(16))) gdbstub_t {
            gdb_version_t version;
            void (*init)(void);
            gdb_device_t *device;
        };

        // Globals — inline so they are defined exactly once across all TUs.
        __attribute__((used)) inline SH2Context g_ctx __asm__("srl_gdbstub_ctx") = {};
        // Second SH-2 context for the slave CPU. Exported in the GDB register map
        // (see the 'g'/'G'/'p'/'P' handlers) and made live by the FRT Input Capture
        // Interrupt mechanism below: once InstallSlaveFreezeHandler() has been run on
        // the slave, its own exception thunk snapshots full register state here before
        // spinning, and restores it from here on resume.
        __attribute__((used)) inline SH2Context g_slave_ctx __asm__("srl_gdbstub_slave_ctx") = {};

        // Set true by snapshot_polling_context() immediately before it calls
        // process_commands() from Poll()'s out-of-band packet path (initial
        // handshake, or a GDB packet arriving while the target is nominally
        // "running" outside any real hardware exception). g_ctx in that case has
        // r0-r14 zeroed and a pc/pr that do not correspond to a real, resumable
        // instruction (see snapshot_polling_context()'s doc comment) -- the real
        // CPU registers were never touched. handle_gdb_continue()/handle_gdb_step()
        // must treat this as a no-op rather than using that data for breakpoint
        // restore or step-trap placement, which would corrupt state using
        // garbage addresses/register values. Read-and-cleared by
        // handle_gdb_continue()/handle_gdb_step() so it never leaks into a
        // later, real exception-driven invocation.
        inline volatile bool g_ctx_is_fake = false;

        inline volatile bool g_has_connection = false;    // set on any valid RSP packet received
        inline volatile bool g_handshake_done = false;    // set only after qSupported exchange
        inline volatile bool g_is_ctrl_c_stop __asm__("srl_gdbstub_is_ctrl_c_stop") = false;    // set when stopped via Ctrl-C (or the Saturn's physical Reset button, via NMI), cleared on continue
        inline volatile uint32_t g_command_count = 0;
        __attribute__((used)) inline volatile uint32_t g_exception_thunk_count __asm__("srl_gdbstub_thunk_count") = 0;
        inline char g_last_command[64] = {};
        // Last "monitor <text>" command received via qRcmd, and how many have
        // arrived. User code can poll GetMonitorCommandCount() to detect a new
        // one and dispatch on GetLastMonitorCommand() -- this gives host-side
        // tooling (or a script) a way to trigger sample behavior without a
        // physical gamepad, e.g. `(gdb) monitor crash illegal`.
        inline char g_last_monitor_command[64] = {};
        inline volatile uint32_t g_monitor_command_count = 0;
        inline int g_unget_char = -1;
        inline bool g_handlers_installed = false;
        inline volatile uint32_t g_rx_detect_count = 0;  // incremented each time the stub reads a byte from DevCart RX
        inline volatile uint32_t g_rx_ready_count = 0;   // incremented each time Poll() sees RX data pending
        inline volatile uint32_t g_poll_fallback_count = 0; // incremented when Poll() handles RX without Trap3
        inline bool g_devcart_ready = false;
        inline bool g_devcart_port_available = false;
        inline bool g_devcart_usb_datapath_enabled = true;
        inline uint8_t g_last_usb_flags = 0xFF;
        // __asm__-named (like g_ctx above) so the per-exception-type trampolines
        // below (srl_gdbstub_illegal_thunk / srl_gdbstub_addrerr_thunk) can write
        // to it directly by a fixed symbol, without needing a C++-mangled name.
        __attribute__((used)) inline volatile uint8_t g_last_stop_signal __asm__("srl_gdbstub_last_stop_signal") = 5; // 5=SIGTRAP, 2=SIGINT, 4=SIGILL, 10=SIGBUS
        inline bool g_was_swbreak = false; // Set during PC adjustment if we hit a GDB swbreak
        // Set when $c stepped over a software breakpoint; cleared after re-insertion.
        // When set, the next process_commands() entry is silent (re-inserts BP, continues).
        inline bool g_resuming_from_breakpoint = false;
        // Constraint: If Poll() is called from user code and handles a 'z0' packet that deallocates
        // this specific breakpoint slot before the step-over trap fires, the slot's active flag is cleared.
        // However, the re-insertion handler will silently re-patch the freed address and set active=true
        // on what should be a dead slot. Since process_commands is single-threaded, this race only
        // occurs if user code calls Poll() between the continue and the trap.
        inline int  g_resume_bp_slot = -1; // slot index of the BP that was stepped over
        // Global pause flag used to freeze the slave SH-2 while the master is in GDB.
        inline volatile uint32_t g_debug_pause = 0;

        // Debounce generation counter for the Reset-button/NMI path (see
        // srl_gdbstub_nmi_thunk's doc comment). Incremented by every NMI edge;
        // read back by that same edge's thunk after its debounce wait to detect
        // whether a newer edge (mechanical switch bounce) arrived in the
        // meantime.
        inline volatile uint32_t g_nmi_generation __asm__("srl_gdbstub_nmi_generation") = 0;

        // TEMP DIAGNOSTIC: NMI/Reset-button instrumentation. fire_count increments
        // on every single NMI edge (bounce or genuine). report_count increments
        // only when an edge actually notifies GDB. swallow_count increments when
        // an edge is debounced away. In a working debounce, fire_count may be > 1
        // per press but report_count should always land on exactly 1.
        inline volatile uint32_t g_nmi_fire_count __asm__("srl_gdbstub_nmi_fire_count") = 0;
        inline volatile uint32_t g_nmi_report_count __asm__("srl_gdbstub_nmi_report_count") = 0;
        inline volatile uint32_t g_nmi_swallow_count __asm__("srl_gdbstub_nmi_swallow_count") = 0;

        // --- Slave freeze via SH-2 on-chip FRT Input Capture Interrupt (ICI) ---
        //
        // The slave SH-2 has no path to the SCU interrupt bus, so it cannot be
        // signalled through SRL::Interrupt. The documented cross-CPU mechanism is
        // each SH-2's own on-chip Free-Running Timer (FRT): a word write to a
        // special SCU-mapped address pulses the *other* CPU's FRT input-capture
        // pin, setting that CPU's own FTCSR.ICF flag. If that CPU has enabled the
        // Input Capture Interrupt (TIER.ICIE) and given it a non-zero priority
        // (IPRB), the pulse fires a genuine hardware interrupt — vector 0x64
        // (FRT-ICI) — in that CPU's own, independent VBR table.
        //
        // @warning Hardware-confirmed conflict (see Samples/Debug - GDB Stub/readme.md
        // for the full writeup): SGL's own SRL::Slave::ExecuteOnSlave (slSlaveFunc)
        // uses this exact FRT-ICI mechanism to dispatch jobs to the slave CPU, and
        // InstallSlaveFreezeHandler() below does not coexist with it. On real
        // hardware, g_slave_ici_count (below) tracks past SRL::Slave dispatch
        // activity, not live freeze pulses — it stops incrementing for good once
        // SRL::Slave::ExecuteOnSlave activity ceases, in either call order, and does
        // not respond to subsequent debug stops. Working theory: SGL leaves the
        // slave's own on-chip TIER.ICIE disabled once it has no queued work, and
        // that bit lives in the slave's private peripheral space — the master
        // cannot re-arm it directly, and the only sanctioned way to run code on the
        // slave that could is SRL::Slave::ExecuteOnSlave() itself, which reopens the
        // same conflict. Do not rely on slave-freeze in any project that also uses
        // SRL::Slave; it has not been tested in a project that avoids SRL::Slave
        // entirely.
        static constexpr uint32_t FRT_TIER  = 0xFFFFFE10U; // Timer Interrupt Enable Register
        static constexpr uint32_t FRT_FTCSR = 0xFFFFFE11U; // FRT Control/Status Register
        static constexpr uint32_t FRT_IPRB  = 0xFFFFFE60U; // Interrupt Priority Register B (FRT: bits 11-8)
        static constexpr uint8_t  FRT_ICF   = 0x80U;       // FTCSR.ICF / TIER.ICIE share this bit position
        static constexpr uint32_t FRT_ICI_VECTOR = 0x64U;  // FRT Input Capture Interrupt vector, own VBR

        // Cross-CPU "doorbell" addresses (SCU A-bus mapped). A 16-bit write to one
        // of these pulses the *other* CPU's FRT input-capture pin. Safe to write
        // even if the target CPU never installed a handler for it — it just sets
        // an unused status flag in that case.
        static constexpr uint32_t MasterNotifiesSlave = 0x21000000U;
        static constexpr uint32_t SlaveNotifiesMaster = 0x21800000U;

        // Diagnostic: incremented by the slave-side ICI thunk every time it fires,
        // so the master can confirm (via g_slave_ctx / this counter, both in
        // shared Work RAM) whether the interrupt is actually reaching the slave.
        __attribute__((used)) inline volatile uint32_t g_slave_ici_count __asm__("srl_gdbstub_slave_ici_count") = 0;

        // --- Slave-side breakpoint support (illegal-instruction vector, NOT
        // FRT-ICI -- see InstallSlaveExceptionHandler()'s doc comment for why
        // this is a separate, independent mechanism from the freeze handler
        // above and its documented SRL::Slave conflict) ---
        //
        // Set true by the slave's own illegal-instruction thunk when it hits a
        // software breakpoint (or any illegal instruction) in slave-executed
        // code. Poll() (which runs every VBlank on the master, independent of
        // whatever the master's own C++ code is doing) bridges this into a
        // normal master-side debug stop by calling Break() -- the same way a
        // Ctrl-C byte already does from that exact call site. The slave itself
        // just spins on g_slave_resume in the meantime.
        inline volatile bool g_slave_stopped = false;
        // Master sets this (via handle_gdb_continue()/handle_gdb_step(), same
        // as any other resume) to release a slave halted in g_slave_stopped.
        inline volatile bool g_slave_resume = false;
        inline volatile bool g_slave_handlers_installed = false;
        // Diagnostic: incremented by the slave's illegal-instruction thunk every
        // time it fires, mirroring g_slave_ici_count's role for the freeze handler.
        __attribute__((used)) inline volatile uint32_t g_slave_bp_count __asm__("srl_gdbstub_slave_bp_count") = 0;

        /**
         * @brief Requests that the slave SH-2 freeze (spin) for the duration of a debug stop.
         * @details Sets the shared pause flag and pulses the slave's FRT input-capture
         * pin. If InstallSlaveFreezeHandler() was never run on the slave, this is a
         * harmless no-op from the slave's point of view.
         */
        static inline void SlaveIPISet() {
            g_debug_pause = 1;
            *reinterpret_cast<volatile uint16_t*>(MasterNotifiesSlave) = 0xFFFFU;
        }

        /**
         * @brief Releases a slave previously frozen via SlaveIPISet().
         */
        static inline void SlaveIPIClear() {
            g_debug_pause = 0;
        }

        // We use Illegal Instruction (0xFFFF) by default for software breakpoints.
        // This avoids collisions with SGL which frequently overwrites TRAPA vectors (32-63)
        // for its own CD-ROM and BIOS system calls.
        static constexpr uint16_t SoftwareBreakInstruction = 0xFFFFU;
        static constexpr size_t MaxSoftwareBreakpoints = 32;
        static constexpr size_t kPacketDataMax = 399U;

        /**
         * @brief Tracks the state of a single software breakpoint.
         */
        struct SoftwareBreakpoint {
            uint32_t address;
            uint16_t original_instruction;
            bool active;
        };

        inline SoftwareBreakpoint g_software_breakpoints[MaxSoftwareBreakpoints] = {};


        // --- Utility Functions ---

        /**
         * @brief Converts a hex character to its integer value.
         */
        static inline int hex(char ch) {
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            if (ch >= '0' && ch <= '9') return ch - '0';
            return -1;
        }

        /**
         * @brief Converts a 4-bit integer to its hex character equivalent.
         */
        static inline char hexchar(int v) {
            v &= 0xf;
            return v < 10 ? '0' + v : 'a' + v - 10;
        }

        static constexpr char kPacketSizeStr[] = "PacketSize=400";
        static_assert(kPacketDataMax + 1U == 400, "Update kPacketSizeStr if kPacketDataMax changes");

        /**
         * @brief Decodes a hex string into memory.
         * @return Pointer to the character following the decoded hex string, or nullptr on failure.
         */
        static inline const char* hex2mem(const char* buf, uint8_t* mem, int count) {
            // Validate all characters first to prevent partial memory corruption
            for (int i = 0; i < count * 2; i++) {
                if (hex(buf[i]) < 0) return nullptr;
            }
            
            for (int i = 0; i < count; i++) {
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
         *
         * Hardware-confirmed bug this fixes: plain hex2mem() above stores one
         * byte at a time. That's fine for RAM, but VDP2 CRAM (and VDP RAM in
         * general) does not reliably latch single-byte bus writes -- writing
         * a 16-bit color value via GDB's 'M' packet (or `set *(unsigned
         * short*)addr = val`, which GDB also sends as an 'M' packet) silently
         * dropped the first byte and kept only the second, e.g. writing
         * 0xEC63 read back as 0x0063. Confirmed on real hardware: the same
         * corruption occurred whether targeting CRAM through a 16-bit or a
         * 32-bit `set` expression, since both went through the byte-wise
         * path. Composing and storing whole aligned words/halfwords at once
         * matches the bus cycle width VDP RAM actually requires.
         * @return Pointer to the character following the decoded hex string, or nullptr on failure.
         */
        static inline const char* hex2mem_aligned(const char* buf, uint32_t addr, int count) {
            // Validate all characters first to prevent partial memory corruption
            for (int i = 0; i < count * 2; i++) {
                if (hex(buf[i]) < 0) return nullptr;
            }

            int i = 0;
            while (i < count) {
                const int remaining = count - i;
                const uint32_t cur = addr + static_cast<uint32_t>(i);
                if (remaining >= 4 && (cur & 3U) == 0U) {
                    uint32_t v = 0;
                    for (int b = 0; b < 4; b++) {
                        const int h1 = hex(*buf++);
                        const int h2 = hex(*buf++);
                        v = (v << 8) | static_cast<uint32_t>((h1 << 4) | h2);
                    }
                    *reinterpret_cast<volatile uint32_t*>(cur) = v;
                    i += 4;
                } else if (remaining >= 2 && (cur & 1U) == 0U) {
                    uint16_t v = 0;
                    for (int b = 0; b < 2; b++) {
                        const int h1 = hex(*buf++);
                        const int h2 = hex(*buf++);
                        v = static_cast<uint16_t>((v << 8) | static_cast<uint16_t>((h1 << 4) | h2));
                    }
                    *reinterpret_cast<volatile uint16_t*>(cur) = v;
                    i += 2;
                } else {
                    const int h1 = hex(*buf++);
                    const int h2 = hex(*buf++);
                    *reinterpret_cast<volatile uint8_t*>(cur) = static_cast<uint8_t>((h1 << 4) | h2);
                    i += 1;
                }
            }
            return buf;
        }

        /**
         * @brief Encodes memory into a hex string.
         * @return Pointer to the null terminator of the resulting string.
         */
        static inline char* mem2hex(const uint8_t* mem, char* buf, int count) {
            for (int i = 0; i < count; i++) {
                *buf++ = hexchar(*mem >> 4);
                *buf++ = hexchar(*mem & 0xF);
                mem++;
            }
            *buf = 0;
            return buf;
        }

        static inline void record_command(const char* cmd) {
            size_t i = 0;
            while (i < 63 && cmd[i] != '\0') {
                g_last_command[i] = cmd[i];
                ++i;
            }
            g_last_command[i] = '\0';
            g_command_count = g_command_count + 1;
            g_has_connection = true;
        }

        static inline bool starts_with(const char* s, const char* prefix) {
            size_t i = 0;
            while (prefix[i] != '\0') {
                if (s[i] != prefix[i]) {
                    return false;
                }
                ++i;
            }
            return true;
        }

        static inline bool str_equals(const char* a, const char* b) {
            size_t i = 0;
            while (a[i] != '\0' && b[i] != '\0') {
                if (a[i] != b[i]) return false;
                ++i;
            }
            return a[i] == b[i];
        }

        /**
         * @brief Checks if a memory range is valid for access, preventing bus errors.
         */
        static inline bool is_valid_memory_range(uint32_t addr, uint32_t length) {
            if (length == 0) {
                return true;
            }

            // Detect wrap-around in address arithmetic.
            const uint32_t end = addr + length - 1;
            if (end < addr) {
                return false;
            }

            // Prevent accesses to the invalid high address space and peripheral space.
            // Reading peripheral space (0xFFFF8000 - 0xFFFFFFFF) via byte-wise access (mov.b)
            // causes bus errors on many SH-2 registers, which crashes the stub.
            if (end >= 0xF0000000U) {
                return false;
            }

            return true;
        }

        static inline bool parse_hex_u32_until(const char* p, char delimiter, uint32_t& out_value, const char*& out_end) {
            uint32_t value = 0;
            bool saw_digit = false;

            while (*p != '\0' && *p != delimiter) {
                const int d = hex(*p);
                if (d < 0) {
                    return false;
                }
                value = (value << 4) | static_cast<uint32_t>(d);
                saw_digit = true;
                ++p;
            }

            if (!saw_digit) {
                return false;
            }

            out_value = value;
            out_end = p;
            return true;
        }

        /**
         * @brief Finds the software breakpoint slot for a given address.
         * @return The slot index, or -1 if not found.
         */
        static inline int find_breakpoint_slot(uint32_t address) {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i) {
                if (g_software_breakpoints[i].active && g_software_breakpoints[i].address == address) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        static inline int find_free_breakpoint_slot() {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i) {
                if (!g_software_breakpoints[i].active) {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        inline static bool g_cache_dirty = false;

        static inline void ForcePurgeCache() {
            *reinterpret_cast<volatile uint8_t*>(0xFFFFFE92) |= 0x10;
            // The SH-2 hardware manual requires waiting at least two instructions
            // before accessing the cache after a purge. We add several NOPs to be safe.
            asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop" ::: "memory");
        }

        static inline void FlushCacheIfDirty() {
            if (g_cache_dirty) {
                ForcePurgeCache();
                g_cache_dirty = false;
            }
        }

        struct CacheFlusher {
            ~CacheFlusher() { FlushCacheIfDirty(); }
        };

        // Releases a slave halted at a breakpoint (see
        // InstallSlaveExceptionHandler()) on EVERY exit path from
        // process_commands(), not just the 'c'/'s' ones.
        //
        // Hardware-confirmed bug this fixes: handle_gdb_continue()/
        // handle_gdb_step() used to be the only places that cleared
        // g_slave_stopped/set g_slave_resume. 'D' (detach), 'k' (kill), and
        // the packet_get()-failure/disconnect path all `return` without
        // going through either -- including GDB's own implicit detach at the
        // end of a batch-mode session (e.g. `gdb -batch -ex "target remote
        // ..." -ex "monitor trace"`, used throughout this project's own
        // testing). Any of those left the slave permanently parked in its
        // spin-wait: the master resumes fine (RTE doesn't care about slave
        // state), so Poll()/the main loop look completely healthy, but
        // SlaveCounterTask::Do() never actually returns, so IsRunning()
        // stays true forever and no further slave jobs are ever dispatched
        // again -- confirmed by re-arming the exact same breakpoint address
        // in a fresh session afterward and it never firing again, because
        // the slave was never actually re-entering that code at all.
        struct SlaveReleaseGuard {
            ~SlaveReleaseGuard() {
                if (g_slave_stopped) {
                    g_slave_stopped = false;
                    g_slave_resume = true;
                }
            }
        };

        // Set for the entire duration of process_commands() (see its RAII guard).
        // Poll() checks this at entry and returns immediately if set -- see the
        // comment there for why this exists and why blanket-masking interrupts
        // (an earlier, now-reverted fix) was the wrong approach.
        inline volatile bool g_in_process_commands = false;

        struct ReentrancyGuard {
            ReentrancyGuard() { g_in_process_commands = true; }
            ~ReentrancyGuard() { g_in_process_commands = false; }
        };

        static inline void PurgeCache() {
            g_cache_dirty = true;
        }

        // Hardware-confirmed bug this fixes: a software breakpoint's 0xFFFF
        // patch is written to shared RAM once and PurgeCache() above only
        // purges the MASTER's own instruction cache -- the SH-2 Cache
        // Control Register at 0xFFFFFE92 is private on-chip hardware, with
        // no bus path from one CPU to the other's copy of it. If the
        // breakpoint's address is code that runs on the SLAVE (e.g. inside
        // an SRL::Slave::ExecuteOnSlave() task), and the slave has already
        // cached that line -- which for any repeatedly-dispatched task it
        // almost always has -- the slave keeps executing its own stale,
        // unpatched copy indefinitely. GDB is told "OK", the memory really
        // is patched, but the slave silently never sees it: confirmed by
        // reading srl_gdbstub_slave_bp_count (see GetSlaveBreakpointCount())
        // via raw memory before and after a 15s `continue` with a fresh
        // slave breakpoint installed -- it never incremented, even though
        // the target task is dispatched roughly every 0.25s all session
        // long. This explains the erratic, boot-order-dependent hit rate
        // observed for slave breakpoints before this fix: whether the
        // slave's cache happened to still hold that exact line at the
        // moment of the next dispatch.
        //
        // The master cannot purge the slave's cache directly -- only code
        // running ON the slave can write its own CCR -- so this dispatches
        // a tiny task there via the same SRL::Slave::ExecuteOnSlave()
        // mechanism InstallSlaveExceptionHandler() already uses, with the
        // same bounded (never-indefinite) wait for completion that caller
        // already has to use: see InstallSlaveExceptionHandler()'s own
        // @warning about SRL::Slave::ExecuteOnSlave() dispatches leaving an
        // ITask's IsRunning() flag stuck true. That warning was specifically
        // about VBR-table patching disrupting slSlaveFunc's own
        // dispatch-completion signal; this task does nothing but purge the
        // cache and return, so it's expected NOT to share that failure mode
        // -- but the bounded wait is kept regardless, since process_commands()
        // must never hang the whole debug session waiting on the slave.
        //
        // Called unconditionally by every breakpoint install/remove/restore
        // below rather than only when the target address is "known" to be
        // slave code: the RSP protocol carries no such distinction, and a
        // breakpoint that works only sometimes depending on which CPU
        // happens to execute it is worse than a small, rare dispatch to an
        // idle slave. The one residual risk this doesn't fully close: if
        // the slave is genuinely mid-execution of a DIFFERENT
        // ExecuteOnSlave()-dispatched task at the exact instant a
        // breakpoint is installed, concurrent slSlaveFunc() dispatch
        // behavior is unverified (SGL's implementation is precompiled, not
        // available to inspect) -- in practice this window is tiny, since
        // the master is already halted inside process_commands() (the only
        // thing that ever dispatches NEW user tasks) for the entire
        // duration of any GDB command that could reach this code.
        // Temporary diagnostic: confirms SlaveCachePurgeTask::Do() actually
        // executes ON the slave when dispatched from inside
        // process_commands() -- remove once PurgeSlaveCacheBestEffort() is
        // confirmed reliable on real hardware.
        __attribute__((used)) inline volatile uint32_t g_slave_purge_task_ran_count __asm__("srl_gdbstub_slave_purge_task_ran_count") = 0;

        class SlaveCachePurgeTask : public SRL::Types::ITask {
        protected:
            void Do() override {
                g_slave_purge_task_ran_count = g_slave_purge_task_ran_count + 1;
                *reinterpret_cast<volatile uint8_t*>(0xFFFFFE92) |= 0x10;
                asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop" ::: "memory");
            }
        };
        inline SlaveCachePurgeTask g_slave_cache_purge_task;

        // Temporary diagnostics, same removal plan as g_slave_purge_task_ran_count above.
        __attribute__((used)) inline volatile uint32_t g_slave_purge_called_count __asm__("srl_gdbstub_slave_purge_called_count") = 0;
        __attribute__((used)) inline volatile uint32_t g_slave_purge_wait_iters __asm__("srl_gdbstub_slave_purge_wait_iters") = 0;
        __attribute__((used)) inline volatile uint32_t g_slave_purge_was_running_at_start __asm__("srl_gdbstub_slave_purge_was_running_at_start") = 0;

        static inline void PurgeSlaveCacheBestEffort() {
            g_slave_purge_called_count = g_slave_purge_called_count + 1;
            g_slave_purge_was_running_at_start = g_slave_cache_purge_task.IsRunning() ? 1U : 0U;
            SRL::Slave::ExecuteOnSlave(g_slave_cache_purge_task);
            uint32_t wait = 0;
            while (g_slave_cache_purge_task.IsRunning() && wait < 5000000U) { ++wait; }
            g_slave_purge_wait_iters = wait;
        }

        /**
         * @brief Clears all active software breakpoints.
         */
        static inline void clear_breakpoints(bool restore_memory) {
            for (size_t i = 0; i < MaxSoftwareBreakpoints; ++i) {
                if (!g_software_breakpoints[i].active) {
                    continue;
                }

                if (restore_memory) {
                    volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(g_software_breakpoints[i].address | 0x20000000U);
                    *code = g_software_breakpoints[i].original_instruction;
                }

                g_software_breakpoints[i].active = false;
                g_software_breakpoints[i].address = 0;
                g_software_breakpoints[i].original_instruction = 0;
            }
            if (restore_memory) {
                PurgeCache();
                PurgeSlaveCacheBestEffort();
            }
        }

        /**
         * @brief Installs a software breakpoint (0xFFFF) at the specified address.
         */
        static inline bool install_software_breakpoint(uint32_t address) {
            if ((address & 1U) != 0U || !is_valid_memory_range(address, 2U)) {
                return false;
            }

            if (find_breakpoint_slot(address) >= 0) {
                return true;
            }

            const int slot = find_free_breakpoint_slot();
            if (slot < 0) {
                return false;
            }

            volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(address | 0x20000000U);
            g_software_breakpoints[slot].address = address;
            g_software_breakpoints[slot].original_instruction = *code;
            *code = SoftwareBreakInstruction;
            g_software_breakpoints[slot].active = true;
            PurgeCache();
            // See PurgeSlaveCacheBestEffort()'s doc comment: without this, a
            // breakpoint on slave-executed code silently never fires once
            // the slave's own cache already holds that line.
            PurgeSlaveCacheBestEffort();
            return true;
        }

        /**
         * @brief Removes a software breakpoint and restores the original instruction.
         *
         * @note Deliberately does NOT call PurgeSlaveCacheBestEffort() itself,
         * unlike install_software_breakpoint() -- this function is called
         * from two different execution contexts: the master's own Z/z
         * packet handler (below), and slave_breakpoint_handler()'s one-shot
         * auto-removal, which runs ON THE SLAVE. PurgeSlaveCacheBestEffort()
         * calls SRL::Slave::ExecuteOnSlave(), which assumes it's being
         * called FROM the master TO dispatch work onto the slave -- calling
         * it from code already running on the slave is undefined territory
         * (hardware-confirmed to produce inconsistent results: the purge
         * task's own "did it actually run" counter frequently stayed flat
         * across calls made from that context). slave_breakpoint_handler()
         * doesn't need it anyway: it already purges the slave's OWN cache
         * directly (a plain local ForcePurgeCache(), no cross-CPU dispatch
         * needed since it's already executing there) immediately after
         * this call returns. Master-context callers purge the slave's
         * cache themselves, right after calling this.
         */
        static inline bool remove_software_breakpoint(uint32_t address) {
            if ((address & 1U) != 0U || !is_valid_memory_range(address, 2U)) {
                return false;
            }

            const int slot = find_breakpoint_slot(address);
            if (slot < 0) {
                return true;
            }

            volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(address | 0x20000000U);
            *code = g_software_breakpoints[slot].original_instruction;
            g_software_breakpoints[slot].active = false;
            g_software_breakpoints[slot].address = 0;
            g_software_breakpoints[slot].original_instruction = 0;
            PurgeCache();
            return true;
        }

        // @warning The SH-2 has exactly one usable UBC channel (A), and this flag is the
        // ONLY record of who owns it -- it cannot detect the hardware being reprogrammed
        // by code that writes BARA/BAMRA/BBRA/BRCR directly instead of going through
        // install_hardware_watchpoint()/remove_hardware_watchpoint() (e.g. a project's own
        // UBC-based test trigger). If that happens while a GDB `watch`/`hwbreak` is active,
        // GDB is left believing its watchpoint is still armed after the hardware has
        // silently been repointed elsewhere. Project code that also needs the UBC should
        // route through these two functions rather than programming the registers itself.
        inline bool g_ubc_channel_a_active = false;

        /**
         * @brief Configures the User Break Controller (UBC) for a hardware watchpoint.
         */
        static inline bool install_hardware_watchpoint(uint32_t address, uint32_t type) {
            if (g_ubc_channel_a_active) {
                return false; // Only one channel supported currently
            }

            volatile uint32_t *BARA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF40U);
            volatile uint16_t *BAMRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF44U);
            volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
            volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF60U);

            *BARA = address;
            *BAMRA = 0x0000U; // exact match
            
            uint16_t bbra_val = 0;
            switch(type) {
                case 1: bbra_val = 0x0010U; break; // HW breakpoint (Instruction fetch)
                case 2: bbra_val = 0x0028U; break; // Write watchpoint
                case 3: bbra_val = 0x0024U; break; // Read watchpoint
                case 4: bbra_val = 0x002CU; break; // Access watchpoint (Read/Write)
                default: return false;
            }
            
            *BBRA = bbra_val;
            *BRCR = 0x0001U; // Enable UBC Channel A
            asm volatile("nop" ::: "memory"); // ensure BRCR write is committed
            
            g_ubc_channel_a_active = true;
            return true;
        }

        /**
         * @brief Disables the User Break Controller (UBC) hardware watchpoint.
         */
        static inline bool remove_hardware_watchpoint(uint32_t address, uint32_t type) {
            (void)address;
            (void)type;
            if (!g_ubc_channel_a_active) {
                return false;
            }
            volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF60U);
            *BRCR = 0x0000U; // Disable UBC
            g_ubc_channel_a_active = false;
            return true;
        }

        /**
         * @brief Tracks the state of a single-step operation, including delay slot mechanics.
         */
        struct StepData {
            uint32_t address;
            uint16_t original_instruction;
            bool active;

            bool is_delayed;
            uint32_t delayed_branch_pc;
            uint32_t delayed_target;
            bool delayed_updates_pr;
            uint32_t delayed_pr;
            bool delayed_is_rte;
            uint32_t delayed_sr;
        };
        inline StepData g_step_data = {0, 0, false, false, 0, 0, false, 0, false, 0};

        /**
         * @brief Removes the temporary software step trap and restores the original instruction.
         */
        static inline void undo_software_step() {
            if (g_step_data.active) {
                if (is_valid_memory_range(g_step_data.address, 2U)) {
                    volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(g_step_data.address | 0x20000000U);
                    *code = g_step_data.original_instruction;
                    PurgeCache();
                }
                g_step_data.active = false;
            }
        }

        /**
         * @brief Places a temporary software step trap to catch execution after one instruction.
         */
        static inline void do_software_step() {
            undo_software_step();

            uint32_t pc = g_ctx.pc;
            if ((pc & 1U) != 0U || !is_valid_memory_range(pc, 2U)) {
                return;
            }

            volatile uint16_t* code_ptr = reinterpret_cast<volatile uint16_t*>(pc | 0x20000000U);
            uint16_t opcode = *code_ptr;
            uint32_t target_pc = pc + 2U;

            if (g_step_data.is_delayed) {
                // We are stepping the instruction inside a delay slot.
                // The SH-2 architecture explicitly forbids branch instructions from being
                // placed in delay slots. Thus, we safely ignore the decoded opcode here
                // and assume it's a regular instruction that progresses to PC + 2.
                // If a buggy program violates this rule, the trap will be placed at PC + 2
                // rather than aborting or following the nested branch.
                target_pc = pc + 2U;
            } else {
                bool is_branch = false;
                bool has_delay_slot = false;
                uint32_t branch_target = pc + 2U;

                if ((opcode & 0xfb00U) == 0x8900U) { // BT label, BT/S label
                    is_branch = true;
                    has_delay_slot = ((opcode & 0xff00U) == 0x8d00U);
                    if ((g_ctx.sr & 1U) != 0U) {
                        int8_t disp8 = static_cast<int8_t>(opcode & 0xffU);
                        branch_target = pc + (static_cast<int>(disp8) << 1) + 4U;
                    } else {
                        branch_target = pc + (has_delay_slot ? 4U : 2U);
                    }
                } else if ((opcode & 0xfb00U) == 0x8b00U) { // BF label, BF/S label
                    is_branch = true;
                    has_delay_slot = ((opcode & 0xff00U) == 0x8f00U);
                    if ((g_ctx.sr & 1U) == 0U) {
                        int8_t disp8 = static_cast<int8_t>(opcode & 0xffU);
                        branch_target = pc + (static_cast<int>(disp8) << 1) + 4U;
                    } else {
                        branch_target = pc + (has_delay_slot ? 4U : 2U);
                    }
                } else if ((opcode & 0xe000U) == 0xa000U) { // BRA label, BSR label
                    is_branch = true;
                    has_delay_slot = true;
                    int16_t disp12 = static_cast<int16_t>((opcode & 0x0fffU) << 4) >> 4;
                    branch_target = pc + (static_cast<int>(disp12) << 1) + 4U;
                    if ((opcode & 0xf000U) == 0xb000U) { // BSR label
                        g_step_data.delayed_updates_pr = true;
                        g_step_data.delayed_pr = pc + 4U;
                    }
                } else if ((opcode & 0xf0dfU) == 0x400bU) { // JMP @Rm, JSR @Rm
                    is_branch = true;
                    has_delay_slot = true;
                    uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                    branch_target = g_ctx.r[reg_idx];
                    if ((opcode & 0xf0ffU) == 0x400bU) { // JSR @Rm
                        g_step_data.delayed_updates_pr = true;
                        g_step_data.delayed_pr = pc + 4U;
                    }
                } else if ((opcode & 0xf0ffU) == 0x0023U) { // BRAF Rm
                    is_branch = true;
                    has_delay_slot = true;
                    uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                    branch_target = pc + 4U + g_ctx.r[reg_idx];
                } else if ((opcode & 0xf0ffU) == 0x0003U) { // BSRF Rm
                    is_branch = true;
                    has_delay_slot = true;
                    uint32_t reg_idx = (opcode & 0x0f00U) >> 8;
                    branch_target = pc + 4U + g_ctx.r[reg_idx];
                    g_step_data.delayed_updates_pr = true;
                    g_step_data.delayed_pr = pc + 4U;
                } else if (opcode == 0x000bU) { // RTS
                    is_branch = true;
                    has_delay_slot = true;
                    branch_target = g_ctx.pr;
                } else if (opcode == 0x002bU) { // RTE
                    is_branch = true;
                    has_delay_slot = true;
                    uint32_t sp = g_ctx.r[15];
                    if (is_valid_memory_range(sp, 8U)) {
                        branch_target = *reinterpret_cast<volatile uint32_t*>(sp | 0x20000000U);
                        g_step_data.delayed_is_rte = true;
                        g_step_data.delayed_sr = *reinterpret_cast<volatile uint32_t*>((sp + 4U) | 0x20000000U);
                    }
                } else if ((opcode & 0xff00U) == 0xc300U) { // TRAPA #imm
                    uint32_t vec_num = opcode & 0xffU;
                    if (vec_num <= 31U) {
                        uint32_t vec_addr = g_ctx.vbr + ((32U + vec_num) * 4U);
                        if (is_valid_memory_range(vec_addr, 4U)) {
                            target_pc = *reinterpret_cast<volatile uint32_t*>(vec_addr | 0x20000000U);
                        }
                    }
                } else if (opcode == 0xFFFFU) { // Illegal Instruction (our breakpoint)
                    if (find_breakpoint_slot(pc) >= 0) {
                        target_pc = pc + 2U;
                    } else {
                        target_pc = pc;
                    }
                }

                if (is_branch && has_delay_slot) {
                    g_step_data.is_delayed = true;
                    g_step_data.delayed_branch_pc = pc;
                    g_step_data.delayed_target = branch_target;
                    target_pc = pc + 2U;
                } else if (is_branch) {
                    target_pc = branch_target;
                }
            }

            // Put a single-step trap at the target address.
            if ((target_pc & 1U) == 0U && is_valid_memory_range(target_pc, 2U)) {
                volatile uint16_t* target_code = reinterpret_cast<volatile uint16_t*>(target_pc | 0x20000000U);
                g_step_data.address = target_pc;
                g_step_data.original_instruction = *target_code;
                *target_code = SoftwareBreakInstruction;
                g_step_data.active = true;
                PurgeCache();
            }
        }

        /**
         * @brief Adjusts the program counter (PC) following a breakpoint or step trap.
         */
        static inline void adjust_pc_for_software_breakpoint() {
            g_was_swbreak = false;
            // SoftwareBreakInstruction (0xFFFF) is the SAME opcode CrashProgram() uses to
            // deliberately trigger a real Illegal Instruction crash, so vector 4's thunk
            // (srl_gdbstub_illegal_thunk) cannot tell "GDB's own breakpoint/step-trap fired"
            // apart from "the user's code genuinely executed an illegal instruction" -- it
            // unconditionally pre-sets g_last_stop_signal = SIGILL(4) before this function
            // even runs. Every g_was_swbreak = true branch below IS one of our own traps
            // (a GDB-inserted breakpoint or do_software_step()'s internal step trap), never
            // a real crash, so it must be reported as SIGTRAP(5) -- overriding the thunk's
            // pre-set value -- or GDB won't recognize it as its own breakpoint (no swbreak
            // annotation in the T-packet, see send_stop_signal), and will treat a completely
            // ordinary breakpoint hit as an unrecoverable program crash.
            // SH-2 exception PC semantics:
            //   - Illegal Instruction (0xFFFF): hardware pushes the address of the
            //     faulting instruction itself (the 0xFFFF word), i.e. g_step_data.address.
            //   - TRAPA #imm: hardware pushes PC+2 (past the trap), so we subtract 2.
            //
            // Phase 1 of delay-slot stepping: we placed 0xFFFF at the delay slot
            // (g_step_data.address == branch_pc + 2). The hardware pushes that address.
            // Detect this as g_ctx.pc == g_step_data.address (NOT delayed_branch_pc).
            // Correct PC to the branch target and apply side effects.
            if (g_step_data.active && g_step_data.is_delayed && g_ctx.pc == g_step_data.address) {
                g_was_swbreak = true;
                g_last_stop_signal = 5U; // SIGTRAP — our own step trap, not a real crash
                // Delay slot trap fired — hardware gave us the delay slot's address.
                // Present PC to GDB as the branch target (where execution will resume).
                g_ctx.pc = g_step_data.delayed_target;

                if (g_step_data.delayed_updates_pr) {
                    g_ctx.pr = g_step_data.delayed_pr;
                }
                if (g_step_data.delayed_is_rte) {
                    g_ctx.sr = g_step_data.delayed_sr;
                    g_ctx.r[15] += 8U;
                }
                g_step_data.is_delayed = false;
                g_step_data.delayed_updates_pr = false;
                g_step_data.delayed_is_rte = false;
                return;
            }

            // Normal (non-delayed) step trap or software breakpoint:
            // PC pushed by hardware IS the faulting instruction address.
            // 
            // EDGE CASE: If a user places a GDB software breakpoint exactly on a programmatic
            // Break() call, this check matches first and returns early (PC is NOT advanced).
            // When GDB removes the breakpoint, it restores the original instruction (0xFFFF).
            // Upon resume, the CPU re-executes 0xFFFF and traps a second time. This time,
            // find_breakpoint_slot() will fail, and the block below will correctly advance
            // the PC past the Break(). This is acceptable as the user will just see two
            // stops at the same address (one for their BP, one for the hardcoded Break).
            if (find_breakpoint_slot(g_ctx.pc) >= 0 || (g_step_data.active && g_ctx.pc == g_step_data.address)) {
                g_was_swbreak = true;
                g_last_stop_signal = 5U; // SIGTRAP — GDB breakpoint or step trap, not a real crash
                return; // PC is already exactly at the breakpoint.
            }

            // If it is a programmatic Break() (0xFFFF) not inserted by GDB, we must advance the PC
            // past it so that execution can resume cleanly on continue/step. We do NOT set g_was_swbreak
            // to true, otherwise GDB will auto-continue over it because it isn't in its breakpoint list.
            if (is_valid_memory_range(g_ctx.pc, 2U)) {
                volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(g_ctx.pc | 0x20000000U);
                if (*code == SoftwareBreakInstruction) {
                    g_ctx.pc += 2U;
                    return;
                }
            }

            if (g_ctx.pc < 2U) {
                return;
            }

            // Fallback: If we ever used TRAPA, the PC pushed is PC + 2.
            const uint32_t trap_address = g_ctx.pc - 2U;
            if (find_breakpoint_slot(trap_address) >= 0 || (g_step_data.active && trap_address == g_step_data.address)) {
                g_was_swbreak = true;
                g_last_stop_signal = 5U; // SIGTRAP — GDB breakpoint or step trap, not a real crash
                g_ctx.pc = trap_address;
            }
        }

        /**
         * @brief Captures a dummy context for asynchronous packet handling outside of exceptions.
         */
        static inline void snapshot_polling_context() {
            // Fallback context used when we service GDB packets outside ExceptionThunk.
            // It keeps PC/SP/special registers valid so GDB does not see a null frame.
            // We zero r0-r14 intentionally. The C++ compiler constantly clobbers these
            // during the Poll() execution itself, so capturing them via inline asm
            // would just yield meaningless compiler-generated garbage. Only the
            // structural frame (SP, PC, PR, etc) is accurate.
            for (int i = 0; i < 16; ++i) {
                g_ctx.r[i] = 0;
            }

            uint32_t sp = 0;
            uint32_t pc = 0;
            uint32_t pr = 0;
            uint32_t gbr = 0;
            uint32_t vbr = 0;
            uint32_t mach = 0;
            uint32_t macl = 0;
            uint32_t sr = 0;

            asm volatile("mov r15, %0" : "=r"(sp));
            asm volatile("mova 1f, r0\n\t"
                         "mov r0, %0\n\t"
                         ".align 2\n\t"
                         "1:\n\t" : "=r"(pc) : : "r0");
            asm volatile("sts pr, %0" : "=r"(pr));
            asm volatile("stc gbr, %0" : "=r"(gbr));
            asm volatile("stc vbr, %0" : "=r"(vbr));
            asm volatile("sts mach, %0" : "=r"(mach));
            asm volatile("sts macl, %0" : "=r"(macl));
            asm volatile("stc sr, %0" : "=r"(sr));

            g_ctx.r[15] = sp;
            g_ctx.pc = pc;
            g_ctx.pr = pr;
            g_ctx.gbr = gbr;
            g_ctx.vbr = vbr;
            g_ctx.mach = mach;
            g_ctx.macl = macl;
            g_ctx.sr = sr;

            // g_ctx.pc above is the address of a label inside THIS function
            // (captured via "mova 1f, r0") -- always the same fixed address
            // regardless of where Poll()'s real caller actually is. It is not a
            // valid resume point. Flag this so continue/step treat it as a no-op.
            g_ctx_is_fake = true;
        }

        // --- Transport (libyaul-style device hooks) ---

        // Idle timeout applied after handshake when no packet arrives.
        // MMIO reads (USB_FLAGS) have ~10 wait states on Saturn, so each loop
        // iteration takes ~1-2 us.
        //
        // Hardware-confirmed bug this fixes: this same wait loop runs inside
        // process_commands()'s packet loop -- i.e. while genuinely HALTED,
        // mid-conversation with GDB, not just while polling during normal
        // execution. The original 3,000,000-iteration value (~3-6 seconds) was
        // sized for detecting a GDB process that crashed/was killed without
        // cleanly detaching -- but it can't tell that apart from a developer
        // just reading the stop message and typing a couple of commands.
        // Confirmed directly: pausing as little as 1-3 seconds between a
        // Ctrl-C/breakpoint/NMI stop and the next `continue` was enough to hit
        // this timeout, silently resetting g_has_connection/g_handshake_done
        // and resuming the target out from under GDB, which was still sitting
        // there expecting a reply to whatever it sent next -- exactly the
        // "GDB shows the stop, but Continue does nothing" symptom reported
        // after the Reset-button/NMI work. Bumped ~150x to give a realistic
        // interactive debugging pause (several minutes) before giving up.
        static constexpr uint32_t GDB_RX_IDLE_TIMEOUT = 450000000U;
        static constexpr uint32_t GDB_TX_IDLE_TIMEOUT = 450000000U;

        // @warning These IsConnected() guards (here, __gdb_wait_tx() below, and
        // the drain loop in process_commands()) only cover software polling --
        // hardware-confirmed on real Saturn hardware: physically unplugging the
        // USB cable can still hang the console instantly, with no intervening
        // frame, which points to the SH-2's Bus State Controller stalling on an
        // external WAIT signal from the cart's CPLD/FTDI interface rather than
        // any of these loops. No amount of guarding SRL::DevCart::CS0 call
        // sites in this header can fix a stall that happens inside the bus
        // cycle itself, before any instruction gets to run. See "Unplugging
        // the USB cable mid-session hangs the console" in
        // Samples/Debug - GDB Stub/readme.md for the full writeup.
        //
        // Waits for USB RX data.
        // - Before first connection (g_has_connection=false): waits indefinitely
        //   so Break() before GDB attaches works correctly -- UNLESS the cable
        //   is (or becomes) physically absent, in which case there is clearly
        //   no debugger that could ever attach, so we bail out immediately.
        //   Hardware-confirmed bug this fixes: with the cable unplugged, the
        //   floating USB_FLAGS register can read as "RX pending", pulling
        //   Poll() into process_commands() before any GDB session ever
        //   existed (g_has_connection still false). The disconnect check used
        //   to live inside `if (g_has_connection)`, so it never ran in that
        //   case, and this loop had no other exit condition -- a permanent
        //   hang that merely reconnecting the cable couldn't clear (nothing
        //   was actively sending bytes to satisfy `IsRxfEmpty()`).
        // - After connection established: also aborts on prolonged silence
        //   (GDB process killed without sending D).
        // Returns true if data is available, false if session should be abandoned.
        static inline bool __gdb_wait_rx() {
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::IsRxfEmpty()) {
                // Abort immediately on cable unplug, connected or not.
                if (!SRL::DevCart::CS0::IsConnected()) {
                    g_has_connection = false;
                    g_handshake_done = false;
                    return false;
                }
                // After handshake, apply idle timeout for dead GDB processes.
                if (g_has_connection && g_handshake_done) {
                    if (++idle > GDB_RX_IDLE_TIMEOUT) {
                        g_has_connection = false;
                        g_handshake_done = false;
                        return false;
                    }
                }
            }
            return true;
        }

        // Waits for USB TX space. Same cable-unplug hazard/fix as __gdb_wait_rx()
        // above -- aborts on cable unplug regardless of connection state, and
        // on prolonged silence once a session is established.
        static inline bool __gdb_wait_tx() {
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::IsTxeFull()) {
                if (!SRL::DevCart::CS0::IsConnected()) {
                    g_has_connection = false;
                    g_handshake_done = false;
                    return false;
                }
                if (g_has_connection && g_handshake_done) {
                    if (++idle > GDB_TX_IDLE_TIMEOUT) {
                        g_has_connection = false;
                        g_handshake_done = false;
                        return false;
                    }
                }
            }
            return true;
        }

        static inline int __gdb_getc() {
            if (g_unget_char != -1) {
                int c = g_unget_char;
                g_unget_char = -1;
                return c;
            }
            if (!__gdb_wait_rx()) {
                return -1; // disconnected
            }
            const uint8_t c = *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo);
            g_rx_detect_count = g_rx_detect_count + 1;
            return static_cast<int>(c);
        }

        static inline bool __gdb_putc(uint8_t value) {
            if (!__gdb_wait_tx()) {
                return false; // disconnected
            }
            *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo) = value;
            return true;
        }

        // --- Packet I/O (minimal, libyaul-style) ---

        static inline uint8_t packet_put_data(const char* buffer, size_t len) {
            uint8_t sum = 0;
            for (size_t i = 0; i < len; i++) {
                uint8_t ch = static_cast<uint8_t>(buffer[i]);
                sum += ch;
                if (!__gdb_putc(ch)) return sum; // disconnect
            }
            return sum;
        }

        /**
         * @brief Sends a GDB RSP packet over the communication channel.
         * @param prefix Optional prefix character (e.g. '+' for ack), or '\0' for none.
         * @param payload The packet payload to send.
         * @param payload_len The length of the payload.
         */
        static inline void packet_put(char type, const char* data, size_t len) {
            do {
                uint8_t csum = 0;
                uint8_t ch = '$';
                if (!__gdb_putc(ch)) return;

                if (type != '\0') {
                    ch = static_cast<uint8_t>(type);
                    if (!__gdb_putc(ch)) return;
                    csum += ch;
                }

                if (data != nullptr && len > 0) {
                    csum += packet_put_data(data, len);
                    if (!g_has_connection) return; // disconnect mid-send
                }

                ch = '#';
                if (!__gdb_putc(ch)) return;
                ch = static_cast<uint8_t>(hexchar(csum >> 4));
                if (!__gdb_putc(ch)) return;
                ch = static_cast<uint8_t>(hexchar(csum));
                if (!__gdb_putc(ch)) return;

                while (true) {
                    int raw = __gdb_getc();
                    if (raw < 0) return; // disconnect
                    ch = static_cast<uint8_t>(raw & 0x7F);
                    if (ch == '+') {
                        return;
                    } else if (ch == '-') {
                        break; // retransmit
                    } else if (ch == '$') {
                        g_unget_char = '$';
                        return;
                    }
                }
            } while (true);
        }

        // Returns false if disconnected (buffer will contain empty/partial data).
        static inline bool packet_get(char* buffer, size_t max_len) {
            buffer[0] = '\0';
            while (true) {
                int raw;

                // Wait for '$' packet start, abort on disconnect.
                do {
                    raw = __gdb_getc();
                    if (raw < 0) return false; // disconnect
                } while ((raw & 0x7F) != '$');

                uint8_t csum = 0;
                size_t len = 0;
                bool overflow = false;

                while (true) {
                    raw = __gdb_getc();
                    if (raw < 0) return false;
                    uint8_t ch = static_cast<uint8_t>(raw & 0x7F);
                    if (ch == '#') break;
                    csum += ch;
                    if (len + 1 < max_len) {
                        buffer[len++] = static_cast<char>(ch);
                    } else {
                        overflow = true;
                    }
                }
                buffer[len] = '\0';

                const int hi_raw = __gdb_getc();
                const int lo_raw = __gdb_getc();
                if (hi_raw < 0 || lo_raw < 0) return false;
                const int hi = hex(static_cast<char>(hi_raw & 0x7F));
                const int lo = hex(static_cast<char>(lo_raw & 0x7F));
                if (hi < 0 || lo < 0) {
                    uint8_t nack = '-';
                    __gdb_putc(nack);
                    continue;
                }
                const uint8_t xmit_csum = static_cast<uint8_t>((hi << 4) | lo);

                if (csum != xmit_csum || overflow) {
                    uint8_t nack = '-';
                    __gdb_putc(nack);
                    continue;
                }

                uint8_t ack = '+';
                __gdb_putc(ack);

                // Strip sequence id prefix (XX:payload → payload).
                // Ensure XX are valid hex digits so we don't accidentally truncate valid packets (e.g. M<addr>:).
                if (len >= 3 && buffer[2] == ':' && hex(buffer[0]) >= 0 && hex(buffer[1]) >= 0) {
                    size_t i = 0;
                    while (buffer[3 + i] != '\0') {
                        buffer[i] = buffer[3 + i];
                        ++i;
                    }
                    buffer[i] = '\0';
                }

                record_command(buffer);
                return true;
            }
        }

        static inline void send_stop_signal(uint8_t signal) {
            char buf[64];
            buf[0] = hexchar(signal >> 4);
            buf[1] = hexchar(signal & 0xF);
            int len = 2;
            
            if (signal == 5) {
                // Only append "swbreak:;" if this was actually a GDB-managed software breakpoint
                // or a single-step trap. If we append it for a programmatic Break() or a real crash
                // using 0xFFFF, GDB will fail to find it in its list and auto-continue the target,
                // resulting in an infinite loop.
                if (g_was_swbreak) {
                    const char* swb = "swbreak:;";
                    for (int i = 0; i < 9; ++i) buf[len++] = swb[i];
                } else {
                    const char* rsn = "reason:signal;";
                    for (int i = 0; i < 14; ++i) buf[len++] = rsn[i];
                }
            }
            
            const char* thread = "thread:1;";
            for (int i = 0; i < 9; ++i) buf[len++] = thread[i];
            
            g_last_stop_signal = signal;
            packet_put('T', buf, len);
        }

        static inline size_t append_str(char* buf, size_t pos, const char* s) {
            while (*s != '\0') buf[pos++] = *s++;
            return pos;
        }

        static inline size_t append_hex(char* buf, size_t pos, uint32_t value, int digits) {
            for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4) {
                buf[pos++] = hexchar(static_cast<int>(value >> shift));
            }
            return pos;
        }

        /**
         * @brief Sends `text` to the GDB console as one or more $O (console output)
         * packets, hex-encoding and chunking it as needed to stay within the
         * negotiated packet size. Used to implement built-in `monitor` diagnostic
         * commands (e.g. "regs slave") whose formatted output is too large for a
         * single hex-encoded qRcmd reply. Safe to call only while still inside the
         * RSP command loop responding to a qRcmd request -- GDB keeps reading
         * packets after `monitor` until it sees a non-'O' reply, so any number of
         * $O packets sent here arrive before the final $OK.
         */
        static inline void send_monitor_text(const char* text, size_t len) {
            constexpr size_t kChunkRaw = kPacketDataMax / 2U;
            char hexbuf[kChunkRaw * 2U + 1U];
            size_t off = 0;
            while (off < len) {
                size_t chunk = len - off;
                if (chunk > kChunkRaw) chunk = kChunkRaw;
                mem2hex(reinterpret_cast<const uint8_t*>(text + off), hexbuf, static_cast<int>(chunk));
                packet_put('O', hexbuf, chunk * 2U);
                off += chunk;
            }
        }

        // --- Core Handler ---

        enum class ExtraReg : uint32_t {
            // VDP1 (11 registers, indices 23..33)
            VDP1_TVMR = 0x25D00000,
            VDP1_FBCR = 0x25D00002,
            VDP1_PTMR = 0x25D00004,
            VDP1_EWDR = 0x25D00006,
            VDP1_EWLR = 0x25D00008,
            VDP1_EWRR = 0x25D0000A,
            VDP1_ENDR = 0x25D0000C,
            VDP1_RESERVED_0E = 0x25D0000E,
            VDP1_EDSR = 0x25D00010,
            VDP1_LOPR = 0x25D00012,
            VDP1_COPR = 0x25D00014,

            // VDP2 (142 registers, indices 34..175) -- full register set, TVMD..COBB
            VDP2_TVMD = 0x25F80000,
            VDP2_EXTEN = 0x25F80002,
            VDP2_TVSTAT = 0x25F80004,
            VDP2_VRSIZE = 0x25F80006,
            VDP2_HCNT = 0x25F80008,
            VDP2_VCNT = 0x25F8000A,
            VDP2_RAMCTL = 0x25F8000E,
            VDP2_CYCA0L = 0x25F80010,
            VDP2_CYCA0U = 0x25F80012,
            VDP2_CYCA1L = 0x25F80014,
            VDP2_CYCA1U = 0x25F80016,
            VDP2_CYCB0L = 0x25F80018,
            VDP2_CYCB0U = 0x25F8001A,
            VDP2_CYCB1L = 0x25F8001C,
            VDP2_CYCB1U = 0x25F8001E,
            VDP2_BGON = 0x25F80020,
            VDP2_MZCTL = 0x25F80022,
            VDP2_SFSEL = 0x25F80024,
            VDP2_SFCODE = 0x25F80026,
            VDP2_CHCTLA = 0x25F80028,
            VDP2_CHCTLB = 0x25F8002A,
            VDP2_BMPNA = 0x25F8002C,
            VDP2_BMPNB = 0x25F8002E,
            VDP2_PNCN0 = 0x25F80030,
            VDP2_PNCN1 = 0x25F80032,
            VDP2_PNCN2 = 0x25F80034,
            VDP2_PNCN3 = 0x25F80036,
            VDP2_PNCR = 0x25F80038,
            VDP2_PLSZ = 0x25F8003A,
            VDP2_MPOFN = 0x25F8003C,
            VDP2_MPOFR = 0x25F8003E,
            VDP2_MPABN0 = 0x25F80040,
            VDP2_MPCDN0 = 0x25F80042,
            VDP2_MPABN1 = 0x25F80044,
            VDP2_MPCDN1 = 0x25F80046,
            VDP2_MPABN2 = 0x25F80048,
            VDP2_MPCDN2 = 0x25F8004A,
            VDP2_MPABN3 = 0x25F8004C,
            VDP2_MPCDN3 = 0x25F8004E,
            VDP2_MPABRA = 0x25F80050,
            VDP2_MPCDRA = 0x25F80052,
            VDP2_MPEFRA = 0x25F80054,
            VDP2_MPGHRA = 0x25F80056,
            VDP2_MPIJRA = 0x25F80058,
            VDP2_MPKLRA = 0x25F8005A,
            VDP2_MPMNRA = 0x25F8005C,
            VDP2_MPOPRA = 0x25F8005E,
            VDP2_MPABRB = 0x25F80060,
            VDP2_MPCDRB = 0x25F80062,
            VDP2_MPEFRB = 0x25F80064,
            VDP2_MPGHRB = 0x25F80066,
            VDP2_MPIJRB = 0x25F80068,
            VDP2_MPKLRB = 0x25F8006A,
            VDP2_MPMNRB = 0x25F8006C,
            VDP2_MPOPRB = 0x25F8006E,
            VDP2_SCXIN0 = 0x25F80070,
            VDP2_SCXDN0 = 0x25F80072,
            VDP2_SCYIN0 = 0x25F80074,
            VDP2_SCYDN0 = 0x25F80076,
            VDP2_ZMXIN0 = 0x25F80078,
            VDP2_ZMXDN0 = 0x25F8007A,
            VDP2_ZMYIN0 = 0x25F8007C,
            VDP2_ZMYDN0 = 0x25F8007E,
            VDP2_SCXIN1 = 0x25F80080,
            VDP2_SCXDN1 = 0x25F80082,
            VDP2_SCYIN1 = 0x25F80084,
            VDP2_SCYDN1 = 0x25F80086,
            VDP2_ZMXIN1 = 0x25F80088,
            VDP2_ZMXDN1 = 0x25F8008A,
            VDP2_ZMYIN1 = 0x25F8008C,
            VDP2_ZMYDN1 = 0x25F8008E,
            VDP2_SCXN2 = 0x25F80090,
            VDP2_SCYN2 = 0x25F80092,
            VDP2_SCXN3 = 0x25F80094,
            VDP2_SCYN3 = 0x25F80096,
            VDP2_ZMCTL = 0x25F80098,
            VDP2_SCRCTL = 0x25F8009A,
            VDP2_VCSTAU = 0x25F8009C,
            VDP2_VCSTAL = 0x25F8009E,
            VDP2_LSTA0U = 0x25F800A0,
            VDP2_LSTA0L = 0x25F800A2,
            VDP2_LSTA1U = 0x25F800A4,
            VDP2_LSTA1L = 0x25F800A6,
            VDP2_LCTAU = 0x25F800A8,
            VDP2_LCTAL = 0x25F800AA,
            VDP2_BKTAU = 0x25F800AC,
            VDP2_BKTAL = 0x25F800AE,
            VDP2_RPMD = 0x25F800B0,
            VDP2_RPRCTL = 0x25F800B2,
            VDP2_KTCTL = 0x25F800B4,
            VDP2_KTAOF = 0x25F800B6,
            VDP2_OVPNRA = 0x25F800B8,
            VDP2_OVPNRB = 0x25F800BA,
            VDP2_RPTAU = 0x25F800BC,
            VDP2_RPTAL = 0x25F800BE,
            VDP2_WPSX0 = 0x25F800C0,
            VDP2_WPSY0 = 0x25F800C2,
            VDP2_WPEX0 = 0x25F800C4,
            VDP2_WPEY0 = 0x25F800C6,
            VDP2_WPSX1 = 0x25F800C8,
            VDP2_WPSY1 = 0x25F800CA,
            VDP2_WPEX1 = 0x25F800CC,
            VDP2_WPEY1 = 0x25F800CE,
            VDP2_WCTLA = 0x25F800D0,
            VDP2_WCTLB = 0x25F800D2,
            VDP2_WCTLC = 0x25F800D4,
            VDP2_WCTLD = 0x25F800D6,
            VDP2_LWTA0U = 0x25F800D8,
            VDP2_LWTA0L = 0x25F800DA,
            VDP2_LWTA1U = 0x25F800DC,
            VDP2_LWTA1L = 0x25F800DE,
            VDP2_SPCTL = 0x25F800E0,
            VDP2_SDCTL = 0x25F800E2,
            VDP2_CRAOFA = 0x25F800E4,
            VDP2_CRAOFB = 0x25F800E6,
            VDP2_LNCLEN = 0x25F800E8,
            VDP2_SFPRMD = 0x25F800EA,
            VDP2_CCCTL = 0x25F800EC,
            VDP2_SFCCMD = 0x25F800EE,
            VDP2_PRISA = 0x25F800F0,
            VDP2_PRISB = 0x25F800F2,
            VDP2_PRISC = 0x25F800F4,
            VDP2_PRISD = 0x25F800F6,
            VDP2_PRINA = 0x25F800F8,
            VDP2_PRINB = 0x25F800FA,
            VDP2_PRIR = 0x25F800FC,
            VDP2_CCRSA = 0x25F80100,
            VDP2_CCRSB = 0x25F80102,
            VDP2_CCRSC = 0x25F80104,
            VDP2_CCRSD = 0x25F80106,
            VDP2_CCRNA = 0x25F80108,
            VDP2_CCRNB = 0x25F8010A,
            VDP2_CCRR = 0x25F8010C,
            VDP2_CCRLB = 0x25F8010E,
            VDP2_CLOFEN = 0x25F80110,
            VDP2_CLOFSL = 0x25F80112,
            VDP2_COAR = 0x25F80114,
            VDP2_COAG = 0x25F80116,
            VDP2_COAB = 0x25F80118,
            VDP2_COBR = 0x25F8011A,
            VDP2_COBG = 0x25F8011C,
            VDP2_COBB = 0x25F8011E,
        };

        static constexpr ExtraReg ExtraRegs[] = {
            ExtraReg::VDP1_TVMR, ExtraReg::VDP1_FBCR, ExtraReg::VDP1_PTMR, ExtraReg::VDP1_EWDR,
            ExtraReg::VDP1_EWLR, ExtraReg::VDP1_EWRR, ExtraReg::VDP1_ENDR, ExtraReg::VDP1_RESERVED_0E,
            ExtraReg::VDP1_EDSR, ExtraReg::VDP1_LOPR, ExtraReg::VDP1_COPR,
            ExtraReg::VDP2_TVMD,
            ExtraReg::VDP2_EXTEN,
            ExtraReg::VDP2_TVSTAT,
            ExtraReg::VDP2_VRSIZE,
            ExtraReg::VDP2_HCNT,
            ExtraReg::VDP2_VCNT,
            ExtraReg::VDP2_RAMCTL,
            ExtraReg::VDP2_CYCA0L,
            ExtraReg::VDP2_CYCA0U,
            ExtraReg::VDP2_CYCA1L,
            ExtraReg::VDP2_CYCA1U,
            ExtraReg::VDP2_CYCB0L,
            ExtraReg::VDP2_CYCB0U,
            ExtraReg::VDP2_CYCB1L,
            ExtraReg::VDP2_CYCB1U,
            ExtraReg::VDP2_BGON,
            ExtraReg::VDP2_MZCTL,
            ExtraReg::VDP2_SFSEL,
            ExtraReg::VDP2_SFCODE,
            ExtraReg::VDP2_CHCTLA,
            ExtraReg::VDP2_CHCTLB,
            ExtraReg::VDP2_BMPNA,
            ExtraReg::VDP2_BMPNB,
            ExtraReg::VDP2_PNCN0,
            ExtraReg::VDP2_PNCN1,
            ExtraReg::VDP2_PNCN2,
            ExtraReg::VDP2_PNCN3,
            ExtraReg::VDP2_PNCR,
            ExtraReg::VDP2_PLSZ,
            ExtraReg::VDP2_MPOFN,
            ExtraReg::VDP2_MPOFR,
            ExtraReg::VDP2_MPABN0,
            ExtraReg::VDP2_MPCDN0,
            ExtraReg::VDP2_MPABN1,
            ExtraReg::VDP2_MPCDN1,
            ExtraReg::VDP2_MPABN2,
            ExtraReg::VDP2_MPCDN2,
            ExtraReg::VDP2_MPABN3,
            ExtraReg::VDP2_MPCDN3,
            ExtraReg::VDP2_MPABRA,
            ExtraReg::VDP2_MPCDRA,
            ExtraReg::VDP2_MPEFRA,
            ExtraReg::VDP2_MPGHRA,
            ExtraReg::VDP2_MPIJRA,
            ExtraReg::VDP2_MPKLRA,
            ExtraReg::VDP2_MPMNRA,
            ExtraReg::VDP2_MPOPRA,
            ExtraReg::VDP2_MPABRB,
            ExtraReg::VDP2_MPCDRB,
            ExtraReg::VDP2_MPEFRB,
            ExtraReg::VDP2_MPGHRB,
            ExtraReg::VDP2_MPIJRB,
            ExtraReg::VDP2_MPKLRB,
            ExtraReg::VDP2_MPMNRB,
            ExtraReg::VDP2_MPOPRB,
            ExtraReg::VDP2_SCXIN0,
            ExtraReg::VDP2_SCXDN0,
            ExtraReg::VDP2_SCYIN0,
            ExtraReg::VDP2_SCYDN0,
            ExtraReg::VDP2_ZMXIN0,
            ExtraReg::VDP2_ZMXDN0,
            ExtraReg::VDP2_ZMYIN0,
            ExtraReg::VDP2_ZMYDN0,
            ExtraReg::VDP2_SCXIN1,
            ExtraReg::VDP2_SCXDN1,
            ExtraReg::VDP2_SCYIN1,
            ExtraReg::VDP2_SCYDN1,
            ExtraReg::VDP2_ZMXIN1,
            ExtraReg::VDP2_ZMXDN1,
            ExtraReg::VDP2_ZMYIN1,
            ExtraReg::VDP2_ZMYDN1,
            ExtraReg::VDP2_SCXN2,
            ExtraReg::VDP2_SCYN2,
            ExtraReg::VDP2_SCXN3,
            ExtraReg::VDP2_SCYN3,
            ExtraReg::VDP2_ZMCTL,
            ExtraReg::VDP2_SCRCTL,
            ExtraReg::VDP2_VCSTAU,
            ExtraReg::VDP2_VCSTAL,
            ExtraReg::VDP2_LSTA0U,
            ExtraReg::VDP2_LSTA0L,
            ExtraReg::VDP2_LSTA1U,
            ExtraReg::VDP2_LSTA1L,
            ExtraReg::VDP2_LCTAU,
            ExtraReg::VDP2_LCTAL,
            ExtraReg::VDP2_BKTAU,
            ExtraReg::VDP2_BKTAL,
            ExtraReg::VDP2_RPMD,
            ExtraReg::VDP2_RPRCTL,
            ExtraReg::VDP2_KTCTL,
            ExtraReg::VDP2_KTAOF,
            ExtraReg::VDP2_OVPNRA,
            ExtraReg::VDP2_OVPNRB,
            ExtraReg::VDP2_RPTAU,
            ExtraReg::VDP2_RPTAL,
            ExtraReg::VDP2_WPSX0,
            ExtraReg::VDP2_WPSY0,
            ExtraReg::VDP2_WPEX0,
            ExtraReg::VDP2_WPEY0,
            ExtraReg::VDP2_WPSX1,
            ExtraReg::VDP2_WPSY1,
            ExtraReg::VDP2_WPEX1,
            ExtraReg::VDP2_WPEY1,
            ExtraReg::VDP2_WCTLA,
            ExtraReg::VDP2_WCTLB,
            ExtraReg::VDP2_WCTLC,
            ExtraReg::VDP2_WCTLD,
            ExtraReg::VDP2_LWTA0U,
            ExtraReg::VDP2_LWTA0L,
            ExtraReg::VDP2_LWTA1U,
            ExtraReg::VDP2_LWTA1L,
            ExtraReg::VDP2_SPCTL,
            ExtraReg::VDP2_SDCTL,
            ExtraReg::VDP2_CRAOFA,
            ExtraReg::VDP2_CRAOFB,
            ExtraReg::VDP2_LNCLEN,
            ExtraReg::VDP2_SFPRMD,
            ExtraReg::VDP2_CCCTL,
            ExtraReg::VDP2_SFCCMD,
            ExtraReg::VDP2_PRISA,
            ExtraReg::VDP2_PRISB,
            ExtraReg::VDP2_PRISC,
            ExtraReg::VDP2_PRISD,
            ExtraReg::VDP2_PRINA,
            ExtraReg::VDP2_PRINB,
            ExtraReg::VDP2_PRIR,
            ExtraReg::VDP2_CCRSA,
            ExtraReg::VDP2_CCRSB,
            ExtraReg::VDP2_CCRSC,
            ExtraReg::VDP2_CCRSD,
            ExtraReg::VDP2_CCRNA,
            ExtraReg::VDP2_CCRNB,
            ExtraReg::VDP2_CCRR,
            ExtraReg::VDP2_CCRLB,
            ExtraReg::VDP2_CLOFEN,
            ExtraReg::VDP2_CLOFSL,
            ExtraReg::VDP2_COAR,
            ExtraReg::VDP2_COAG,
            ExtraReg::VDP2_COAB,
            ExtraReg::VDP2_COBR,
            ExtraReg::VDP2_COBG,
            ExtraReg::VDP2_COBB,
        };
        static constexpr size_t NumExtraRegs = sizeof(ExtraRegs) / sizeof(ExtraRegs[0]);
        static constexpr size_t NumSlaveRegs = 24U;
        static constexpr size_t TotalPseudoRegs = NumExtraRegs + NumSlaveRegs;

        // Empirically confirmed on real hardware (gdb-multiarch 15.1 and this repo's
        // bundled sh-elf-gdb 14.2): both have a HARD-CODED, non-negotiable 268-byte
        // 'g' packet size for the "sh"/"sh2" architecture -- 23 real registers plus
        // 44 padding slots that GDB's own static register table already reserves as
        // blank/anonymous (verified via `maintenance print registers` after
        // `set architecture sh2`). Neither client honors qXfer:features:read-declared
        // register counts for this architecture (GDB prints "Target-supplied
        // registers are not supported by the current architecture" and then rejects
        // any 'g' reply whose length does not match its own fixed count exactly --
        // not just longer ones). The default 'g'/'G' packet below therefore pads out
        // to this fixed size instead of appending the VDP1/VDP2/slave pseudo-registers,
        // so basic sessions (breakpoints, stepping, core registers, memory) work with
        // stock GDB. Those pseudo-registers remain reachable via 'p'/'P' with the same
        // indices (23..), and VDP1/VDP2 registers are always readable as ordinary
        // memory via 'm' at their real addresses regardless of this limitation.
        static constexpr size_t GdbFixedShRegisterCount = 67U;
        static constexpr size_t GdbFixedShPaddingRegisters = GdbFixedShRegisterCount - 23U;

        /**
         * @brief Built-in `monitor regs slave` command: dumps the slave SH-2 context
         * (see g_slave_ctx above) to the GDB console via $O packets. Since GDB's SH
         * architecture backend rejects this stub's target-supplied register
         * description outright (see the GdbFixedShRegisterCount comment above), the
         * slave_r0..slave_sr pseudo-registers are never reachable by name through
         * GDB's Registers UI -- this command is the practical way to inspect them.
         * @note Reads as all zero unless InstallSlaveFreezeHandler() has been
         * installed on the slave and at least one debug stop has occurred since --
         * this stub's own samples generally don't call it (see SlaveCounterTask's
         * doc comment for why it's incompatible with SRL::Slave::ExecuteOnSlave()).
         */
        static inline void send_slave_regs_dump() {
            char text[320];
            size_t pos = 0;

            pos = append_str(text, pos, "slave r0-r7 : ");
            for (int i = 0; i < 8; ++i) {
                pos = append_hex(text, pos, g_slave_ctx.r[i], 8);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            pos = append_str(text, pos, "slave r8-r15: ");
            for (int i = 8; i < 16; ++i) {
                pos = append_hex(text, pos, g_slave_ctx.r[i], 8);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            pos = append_str(text, pos, "pc="); pos = append_hex(text, pos, g_slave_ctx.pc, 8);
            pos = append_str(text, pos, " pr="); pos = append_hex(text, pos, g_slave_ctx.pr, 8);
            pos = append_str(text, pos, " sr="); pos = append_hex(text, pos, g_slave_ctx.sr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "gbr="); pos = append_hex(text, pos, g_slave_ctx.gbr, 8);
            pos = append_str(text, pos, " vbr="); pos = append_hex(text, pos, g_slave_ctx.vbr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "mach="); pos = append_hex(text, pos, g_slave_ctx.mach, 8);
            pos = append_str(text, pos, " macl="); pos = append_hex(text, pos, g_slave_ctx.macl, 8);
            text[pos++] = '\n';

            send_monitor_text(text, pos);
        }

        /**
         * @brief Built-in `monitor regs vdp` command: dumps a curated subset of the
         * VDP1/VDP2 registers most relevant to sprite/NBG priority and status work
         * (the same registers this sample's rasterbar/priority debugging touched) to
         * the GDB console via $O packets. Not the full 153-register ExtraRegs set --
         * that's far more than is useful in a console dump. Same rationale as
         * send_slave_regs_dump(): GDB never learns these pseudo-register names, so a
         * `monitor` command is the practical way to see them.
         */
        static inline void send_vdp_regs_dump() {
            static constexpr const char* kVdp1Names[] = { "tvmr", "fbcr", "ptmr", "edsr", "lopr", "copr" };
            static constexpr ExtraReg kVdp1Regs[] = {
                ExtraReg::VDP1_TVMR, ExtraReg::VDP1_FBCR, ExtraReg::VDP1_PTMR,
                ExtraReg::VDP1_EDSR, ExtraReg::VDP1_LOPR, ExtraReg::VDP1_COPR,
            };
            static constexpr const char* kVdp2Names[] = {
                "tvmd", "exten", "tvstat", "spctl",
                "prisa", "prisb", "prisc", "prisd", "prina", "prinb", "prir",
            };
            static constexpr ExtraReg kVdp2Regs[] = {
                ExtraReg::VDP2_TVMD, ExtraReg::VDP2_EXTEN, ExtraReg::VDP2_TVSTAT, ExtraReg::VDP2_SPCTL,
                ExtraReg::VDP2_PRISA, ExtraReg::VDP2_PRISB, ExtraReg::VDP2_PRISC, ExtraReg::VDP2_PRISD,
                ExtraReg::VDP2_PRINA, ExtraReg::VDP2_PRINB, ExtraReg::VDP2_PRIR,
            };
            static constexpr size_t kNumVdp1 = sizeof(kVdp1Regs) / sizeof(kVdp1Regs[0]);
            static constexpr size_t kNumVdp2 = sizeof(kVdp2Regs) / sizeof(kVdp2Regs[0]);

            char text[320];
            size_t pos = 0;

            pos = append_str(text, pos, "vdp1: ");
            for (size_t i = 0; i < kNumVdp1; ++i) {
                pos = append_str(text, pos, kVdp1Names[i]);
                text[pos++] = '=';
                uint16_t val = *reinterpret_cast<volatile uint16_t*>(static_cast<uint32_t>(kVdp1Regs[i]));
                pos = append_hex(text, pos, val, 4);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            pos = append_str(text, pos, "vdp2: ");
            for (size_t i = 0; i < kNumVdp2; ++i) {
                pos = append_str(text, pos, kVdp2Names[i]);
                text[pos++] = '=';
                uint16_t val = *reinterpret_cast<volatile uint16_t*>(static_cast<uint32_t>(kVdp2Regs[i]));
                pos = append_hex(text, pos, val, 4);
                text[pos++] = ' ';
            }
            text[pos++] = '\n';

            send_monitor_text(text, pos);
        }

        /**
         * @brief Built-in `monitor nmi` command: dumps the Reset-button/NMI
         * debounce diagnostic counters (see srl_gdbstub_nmi_thunk's doc comment).
         * fire_count = every NMI edge seen (bounce or genuine); report_count =
         * edges that actually notified GDB; swallow_count = edges debounced
         * away. In a correctly-working debounce, report_count should land on
         * exactly 1 per physical press regardless of how high fire_count goes.
         */
        static inline void send_nmi_diag_dump() {
            char text[160];
            size_t pos = 0;
            pos = append_str(text, pos, "nmi generation="); pos = append_hex(text, pos, g_nmi_generation, 8);
            text[pos++] = '\n';
            pos = append_str(text, pos, "fire_count="); pos = append_hex(text, pos, g_nmi_fire_count, 8);
            text[pos++] = '\n';
            pos = append_str(text, pos, "report_count="); pos = append_hex(text, pos, g_nmi_report_count, 8);
            text[pos++] = '\n';
            pos = append_str(text, pos, "swallow_count="); pos = append_hex(text, pos, g_nmi_swallow_count, 8);
            text[pos++] = '\n';
            send_monitor_text(text, pos);
        }

        /**
         * @brief Built-in `monitor trace` command: dumps a full, read-only snapshot
         * of the current halt state -- where we stopped, why, and what `continue`
         * would decide to do about it -- WITHOUT mutating anything (no memory
         * patches, no g_step_data changes). Meant to be run BEFORE continuing, to
         * capture exactly what the stub sees for a specific stop that's hard to
         * reproduce on demand (e.g. the Reset-button/NMI path, which can land
         * anywhere, including inside precompiled library code with no debug
         * info -- see the "cannot recover" investigation).
         */
        static inline void send_halt_trace_dump() {
            char text[400];
            size_t pos = 0;

            pos = append_str(text, pos, "pc="); pos = append_hex(text, pos, g_ctx.pc, 8);
            pos = append_str(text, pos, " pr="); pos = append_hex(text, pos, g_ctx.pr, 8);
            pos = append_str(text, pos, " sr="); pos = append_hex(text, pos, g_ctx.sr, 8);
            text[pos++] = '\n';

            pos = append_str(text, pos, "was_swbreak="); text[pos++] = g_was_swbreak ? '1' : '0';
            pos = append_str(text, pos, " is_ctrl_c_stop="); text[pos++] = g_is_ctrl_c_stop ? '1' : '0';
            pos = append_str(text, pos, " last_stop_signal="); pos = append_hex(text, pos, g_last_stop_signal, 2);
            text[pos++] = '\n';

            const int bp_slot = find_breakpoint_slot(g_ctx.pc);
            pos = append_str(text, pos, "bp_slot="); pos = append_hex(text, pos, static_cast<uint32_t>(bp_slot), 8);
            pos = append_str(text, pos, " step_active="); text[pos++] = g_step_data.active ? '1' : '0';
            pos = append_str(text, pos, " step_is_delayed="); text[pos++] = g_step_data.is_delayed ? '1' : '0';
            text[pos++] = '\n';

            // What handle_gdb_continue() would do right now, without doing it.
            pos = append_str(text, pos, "would_step_over=");
            text[pos++] = (bp_slot >= 0 || g_step_data.is_delayed) ? '1' : '0';
            text[pos++] = '\n';

            if (is_valid_memory_range(g_ctx.pc, 2U)) {
                uint16_t opcode = *reinterpret_cast<volatile uint16_t*>(g_ctx.pc | 0x20000000U);
                pos = append_str(text, pos, "opcode_at_pc="); pos = append_hex(text, pos, opcode, 4);
            } else {
                pos = append_str(text, pos, "opcode_at_pc=invalid_range");
            }
            text[pos++] = '\n';

            send_monitor_text(text, pos);
        }

        /**
         * @brief Prepares the CPU state for a GDB single-step command ('s' / 'vCont;s').
         * 
         * Places a temporary software breakpoint on the next sequential instruction
         * (or the branch target if the current instruction is a branch), ensuring
         * the stub catches execution immediately after one instruction.
         */
        static inline void handle_gdb_step() {
            g_is_ctrl_c_stop = false;

            // A slave parked at a breakpoint (see InstallSlaveExceptionHandler())
            // is released by SlaveReleaseGuard when process_commands() returns,
            // regardless of which command got us there -- no single-step support
            // for slave code, so 's' just resumes it same as 'c' would.

            // See g_ctx_is_fake's declaration: if the current halt came from
            // Poll()'s out-of-band snapshot path rather than a real exception,
            // g_ctx.pc is not a real instruction address and r0-r14 are zeroed --
            // decoding an opcode there or planting a trap based on it would
            // corrupt state. The real CPU registers were never touched, so the
            // only correct behavior is to do nothing and let execution continue
            // untouched wherever it actually is.
            if (g_ctx_is_fake) {
                g_ctx_is_fake = false;
                return;
            }

            do_software_step();
        }

        /**
         * @brief Prepares the CPU state for a GDB continue command ('c' / 'vCont;c').
         *
         * Restores the original instruction if the CPU is currently halted on a
         * software breakpoint, and sets up a silent step-over trap to re-insert
         * the breakpoint after the instruction executes.
         */
        static inline void handle_gdb_continue() {
            g_is_ctrl_c_stop = false;
            g_debug_pause = false;
            SlaveIPIClear();

            // A slave parked at a breakpoint (see InstallSlaveExceptionHandler())
            // is released by SlaveReleaseGuard when process_commands() returns --
            // see that struct's doc comment for why this must happen on every
            // exit path, not just this one.

            // See handle_gdb_step() / g_ctx_is_fake: same hazard applies to
            // continue's breakpoint-restore-and-step-over logic below.
            if (g_ctx_is_fake) {
                g_ctx_is_fake = false;
                return;
            }

            const int bp_slot = find_breakpoint_slot(g_ctx.pc);
            if (bp_slot >= 0) {
                // Temporarily restore the original instruction so the CPU can execute it.
                // We deliberately leave active = true so the slot remains owned throughout
                // the step-over sequence — no interrupt between here and re-insertion can
                // see a half-released slot with a valid address but active = false.
                volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(g_ctx.pc | 0x20000000U);
                *code = g_software_breakpoints[bp_slot].original_instruction;
                // active intentionally kept true; re-insertion handler will re-patch it.
            }

            if (bp_slot >= 0 || g_step_data.is_delayed) {
                do_software_step();
                g_resuming_from_breakpoint = true;
                // If this is purely a delay-slot step-over (bp_slot == -1), g_resume_bp_slot
                // captures -1. The re-insertion block in process_commands safely skips the -1
                // slot but still honors g_resuming_from_breakpoint, allowing the target to
                // silently resume execution without notifying GDB.
                g_resume_bp_slot = bp_slot;
            }
        }

        __attribute__((used)) inline void process_commands() __asm__("srl_gdbstub_process_commands");

        /**
         * @brief Core GDB Remote Serial Protocol (RSP) packet processor.
         * 
         * This function reads and parses RSP packets ('g', 'G', 'm', 'M', 'z', 'Z', 'vCont', etc.)
         * from the USB FIFO. It reads/writes CPU state via the `g_ctx` global exception frame,
         * manipulates software breakpoints, and responds to the debugger.
         * It executes entirely from the SH-2 exception context.
         */
        __attribute__((used)) inline void process_commands() {
            // Must be the very first thing: marks g_in_process_commands for the
            // entire duration, so Poll() (see its top) knows to skip its
            // GDB-related work if VBlank fires while we're already halted here.
            //
            // History: an earlier fix for the reentrancy bug this guards against
            // used SR.IMASK to block VBlank outright for the whole halt. That
            // caused a NEW, hardware-confirmed regression: VBlank's interrupt
            // handler does other SGL-critical work beyond calling Poll() (frame
            // timing, DMA, etc per "VblankHandling" elsewhere in this file), and
            // starving that for any real inspection pause (confirmed with a
            // 60-second halt) left the target unable to resume correctly at all
            // -- worse than the bug it fixed. This flag is more surgical: VBlank
            // still fires and does its other work every frame even while halted,
            // only the specific re-entrant call into this function is skipped.
            ReentrancyGuard reentrancy_guard;

            // CacheFlusher guarantees a single cache purge on every exit path from this
            // function — continue, step, detach, disconnect, and early error returns alike.
            // This is intentional: any return that follows a memory patch (breakpoint install/
            // remove, M-packet write, or single-step trap placement) must have flushed before
            // the CPU resumes executing the patched region. On disconnect/detach the flush is
            // harmless. DO NOT remove this object or move it past the first PurgeCache() call.
            CacheFlusher flusher;

            // See SlaveReleaseGuard's doc comment: releases a slave parked at a
            // breakpoint on every exit path from this function, same rationale
            // as CacheFlusher above (continue, step, detach, disconnect alike).
            SlaveReleaseGuard slave_release_guard;
            constexpr size_t max_g_packet_hex_chars = (sizeof(SH2Context) + (GdbFixedShPaddingRegisters * 4U)) * 2;
            static_assert(max_g_packet_hex_chars < 1024, "out_buf is too small for GDB 'g' packet");
            char in_buf[1024];
            char out_buf[1024];

            SRL::Logger::Log::LogPrint("[GDBStub] process_commands() entered. PC: 0x%08lX",
                static_cast<unsigned long>(g_ctx.pc));

            adjust_pc_for_software_breakpoint();

            // Clear the UBC Channel A match flag (CMFA) in BRCR to prevent infinite re-entry loops.
            volatile uint16_t* BRCR = reinterpret_cast<volatile uint16_t*>(0xFFFFFF60U);
            *BRCR &= ~0x0080U;

            undo_software_step();

            // --- Silent step-over: re-insert the breakpoint we temporarily removed for $c ---
            if (g_resuming_from_breakpoint) {
                g_resuming_from_breakpoint = false;
                if (g_resume_bp_slot >= 0 && g_resume_bp_slot < static_cast<int>(MaxSoftwareBreakpoints)) {
                    const uint32_t bp_addr = g_software_breakpoints[g_resume_bp_slot].address;
                    if ((bp_addr & 1U) == 0U && is_valid_memory_range(bp_addr, 2U)) {
                        volatile uint16_t* code = reinterpret_cast<volatile uint16_t*>(bp_addr | 0x20000000U);
                        *code = SoftwareBreakInstruction;
                        // active was kept true throughout; confirm the write then leave it true.
                        PurgeCache();
                    } else {
                        // Address became invalid — release the slot cleanly.
                        g_software_breakpoints[g_resume_bp_slot].active = false;
                    }
                }
                g_resume_bp_slot = -1;
                // Resume transparent execution — do NOT report a stop to GDB.
                return;
            }

            // Freeze the slave SH-2 for the duration of this debug stop. Safe
            // no-op if InstallSlaveFreezeHandler() was never run on the slave.
            SlaveIPISet();

            // Drain any stale bytes that GDB sent before this trap fired.
            // Without this, GDB startup packets (including vCont;c) queued in
            // the FIFO while the Saturn was initialising would immediately resume
            // the target upon the very first Break().
            //
            // Same cable-unplug hazard as __gdb_wait_rx()/__gdb_wait_tx() (see
            // their doc comments): a disconnected/floating USB bus can make
            // IsRxfEmpty() read as "never empty", so this drain must bail out
            // on IsConnected()==false rather than spin forever trying to empty
            // a FIFO that will never report empty. This is reached from Poll()'s
            // pre-connection fallback path (snapshot_polling_context() +
            // process_commands(), see Poll() below) BEFORE __gdb_wait_rx() is
            // ever called, so fixing only that function left this loop hanging.
            if (!g_has_connection) {
                while (!SRL::DevCart::CS0::IsRxfEmpty()) {
                    if (!SRL::DevCart::CS0::IsConnected()) {
                        break;
                    }
                    (void)*(volatile uint8_t*)(SRL::DevCart::CS0::UsbFifo);
                }
            } else if (g_handshake_done) {
                // If we are already connected and we just entered the trap handler
                // (e.g. hit a breakpoint or Ctrl-C), we MUST notify GDB proactively.
                send_stop_signal(g_is_ctrl_c_stop ? 2U : g_last_stop_signal);
            }

            // The stop reason (SIGTRAP or SIGINT) is preserved until '?' arrives.
            // g_is_ctrl_c_stop remains active to mask PR/R14, and is cleared upon resume.

            while (true) {
                out_buf[0] = 0;
                if (!packet_get(in_buf, sizeof(in_buf))) {
                    // USB disconnected while waiting for a packet — release the
                    // slave (if frozen) and stop processing.
                    SlaveIPIClear();
                    return;
                }

                switch (in_buf[0]) {
                    case '!':
                        // Enable extended-remote mode. We already tolerate vRun without this,
                        // but acknowledging it properly avoids relying on that leniency.
                        packet_put('\0', "OK", 2);
                        break;
                    case 'k': // Kill: no defined reply per the RSP spec -- just clean up.
                        clear_breakpoints(true);
                        g_handshake_done = false;
                        g_has_connection = false;
                        SlaveIPIClear();
                        return;
                    case '?':
                        // First '?' marks the connection as active and sends the stop reason.
                        g_has_connection = true;
                        send_stop_signal(g_is_ctrl_c_stop ? 2U : g_last_stop_signal);
                        break;
                    case 'q':
                        if (starts_with(in_buf, "qSupported")) {
                            // Advertise swbreak, hwbreak (Z1-Z4 are implemented via the UBC,
                            // see install_hardware_watchpoint), and target description so GDB
                            // knows the arch. Dynamically insert the PacketSize to ensure it
                            // stays in sync with kPacketDataMax.
                            constexpr const char kFeaturesStr[] = ";swbreak+;hwbreak+;qXfer:features:read+";
                            
                            static constexpr size_t max_qsupported_len = (sizeof(kPacketSizeStr) - 1) + (sizeof(kFeaturesStr) - 1);
                            static_assert(max_qsupported_len < sizeof(out_buf), "qSupported payload exceeds buffer");

                            size_t out_len = 0;
                            for (size_t i = 0; i < sizeof(kPacketSizeStr) - 1; ++i) {
                                if (out_len < sizeof(out_buf) - 1) out_buf[out_len++] = kPacketSizeStr[i];
                            }
                            for (const char* s = kFeaturesStr; *s; ++s) {
                                if (out_len < sizeof(out_buf) - 1) out_buf[out_len++] = *s;
                            }
                            
                            out_buf[out_len] = '\0';
                            packet_put('\0', out_buf, out_len);
                            g_handshake_done = true;
                        } else if (starts_with(in_buf, "qXfer:features:read:")) {
                            // qXfer:features:read:target.xml:offset,length
                            // Full SH-2 register description so gdb-multiarch auto-detects
                            // architecture and register layout without needing 'set arch'.
                            static const char target_xml[] =
                                "<?xml version=\"1.0\"?>\n"
                                "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n"
                                "<target version=\"1.0\">\n"
                                "  <architecture>sh</architecture>\n"
                                "  <feature name=\"org.gnu.gdb.sh.core\">\n"
                                "    <reg name=\"r0\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r1\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r2\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r3\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r4\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r5\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r6\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r7\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r8\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r9\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r10\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r11\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r12\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r13\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r14\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"r15\" bitsize=\"32\" type=\"data_ptr\" format=\"hex\"/>\n"
                                "    <reg name=\"pc\"  bitsize=\"32\" type=\"code_ptr\" format=\"hex\" regnum=\"16\"/>\n"
                                "    <reg name=\"pr\"  bitsize=\"32\" type=\"code_ptr\" format=\"hex\"/>\n"
                                "    <reg name=\"gbr\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"vbr\" bitsize=\"32\" type=\"code_ptr\" format=\"hex\"/>\n"
                                "    <reg name=\"mach\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"macl\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"sr\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "  </feature>\n"
                                "  <feature name=\"org.sega.saturn.vdp\">\n"
                                "    <reg name=\"vdp1_tvmr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"23\"/>\n"
                                "    <reg name=\"vdp1_fbcr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"24\"/>\n"
                                "    <reg name=\"vdp1_ptmr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"25\"/>\n"
                                "    <reg name=\"vdp1_ewdr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"26\"/>\n"
                                "    <reg name=\"vdp1_ewlr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"27\"/>\n"
                                "    <reg name=\"vdp1_ewrr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"28\"/>\n"
                                "    <reg name=\"vdp1_endr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"29\"/>\n"
                                "    <reg name=\"vdp1_edsr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"30\"/>\n"
                                "    <reg name=\"vdp1_lopr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"31\"/>\n"
                                "    <reg name=\"vdp1_copr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"32\"/>\n"
                                "    <reg name=\"vdp1_modr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"33\"/>\n"
                                "    <reg name=\"vdp2_tvmd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"34\"/>\n"
                                "    <reg name=\"vdp2_exten\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"35\"/>\n"
                                "    <reg name=\"vdp2_tvstat\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"36\"/>\n"
                                "    <reg name=\"vdp2_vrsize\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"37\"/>\n"
                                "    <reg name=\"vdp2_hcnt\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"38\"/>\n"
                                "    <reg name=\"vdp2_vcnt\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"39\"/>\n"
                                "    <reg name=\"vdp2_ramctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"40\"/>\n"
                                "    <reg name=\"vdp2_cyca0l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"41\"/>\n"
                                "    <reg name=\"vdp2_cyca0u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"42\"/>\n"
                                "    <reg name=\"vdp2_cyca1l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"43\"/>\n"
                                "    <reg name=\"vdp2_cyca1u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"44\"/>\n"
                                "    <reg name=\"vdp2_cycb0l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"45\"/>\n"
                                "    <reg name=\"vdp2_cycb0u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"46\"/>\n"
                                "    <reg name=\"vdp2_cycb1l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"47\"/>\n"
                                "    <reg name=\"vdp2_cycb1u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"48\"/>\n"
                                "    <reg name=\"vdp2_bgon\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"49\"/>\n"
                                "    <reg name=\"vdp2_mzctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"50\"/>\n"
                                "    <reg name=\"vdp2_sfsel\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"51\"/>\n"
                                "    <reg name=\"vdp2_sfcode\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"52\"/>\n"
                                "    <reg name=\"vdp2_chctla\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"53\"/>\n"
                                "    <reg name=\"vdp2_chctlb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"54\"/>\n"
                                "    <reg name=\"vdp2_bmpna\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"55\"/>\n"
                                "    <reg name=\"vdp2_bmpnb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"56\"/>\n"
                                "    <reg name=\"vdp2_pncn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"57\"/>\n"
                                "    <reg name=\"vdp2_pncn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"58\"/>\n"
                                "    <reg name=\"vdp2_pncn2\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"59\"/>\n"
                                "    <reg name=\"vdp2_pncn3\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"60\"/>\n"
                                "    <reg name=\"vdp2_pncr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"61\"/>\n"
                                "    <reg name=\"vdp2_plsz\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"62\"/>\n"
                                "    <reg name=\"vdp2_mpofn\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"63\"/>\n"
                                "    <reg name=\"vdp2_mpofr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"64\"/>\n"
                                "    <reg name=\"vdp2_mpabn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"65\"/>\n"
                                "    <reg name=\"vdp2_mpcdn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"66\"/>\n"
                                "    <reg name=\"vdp2_mpabn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"67\"/>\n"
                                "    <reg name=\"vdp2_mpcdn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"68\"/>\n"
                                "    <reg name=\"vdp2_mpabn2\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"69\"/>\n"
                                "    <reg name=\"vdp2_mpcdn2\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"70\"/>\n"
                                "    <reg name=\"vdp2_mpabn3\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"71\"/>\n"
                                "    <reg name=\"vdp2_mpcdn3\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"72\"/>\n"
                                "    <reg name=\"vdp2_mpabra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"73\"/>\n"
                                "    <reg name=\"vdp2_mpcdra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"74\"/>\n"
                                "    <reg name=\"vdp2_mpefra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"75\"/>\n"
                                "    <reg name=\"vdp2_mpghra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"76\"/>\n"
                                "    <reg name=\"vdp2_mpijra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"77\"/>\n"
                                "    <reg name=\"vdp2_mpklra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"78\"/>\n"
                                "    <reg name=\"vdp2_mpmnra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"79\"/>\n"
                                "    <reg name=\"vdp2_mpopra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"80\"/>\n"
                                "    <reg name=\"vdp2_mpabrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"81\"/>\n"
                                "    <reg name=\"vdp2_mpcdrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"82\"/>\n"
                                "    <reg name=\"vdp2_mpefrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"83\"/>\n"
                                "    <reg name=\"vdp2_mpghrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"84\"/>\n"
                                "    <reg name=\"vdp2_mpijrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"85\"/>\n"
                                "    <reg name=\"vdp2_mpklrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"86\"/>\n"
                                "    <reg name=\"vdp2_mpmnrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"87\"/>\n"
                                "    <reg name=\"vdp2_mpoprb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"88\"/>\n"
                                "    <reg name=\"vdp2_scxin0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"89\"/>\n"
                                "    <reg name=\"vdp2_scxdn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"90\"/>\n"
                                "    <reg name=\"vdp2_scyin0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"91\"/>\n"
                                "    <reg name=\"vdp2_scydn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"92\"/>\n"
                                "    <reg name=\"vdp2_zmxin0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"93\"/>\n"
                                "    <reg name=\"vdp2_zmxdn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"94\"/>\n"
                                "    <reg name=\"vdp2_zmyin0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"95\"/>\n"
                                "    <reg name=\"vdp2_zmydn0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"96\"/>\n"
                                "    <reg name=\"vdp2_scxin1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"97\"/>\n"
                                "    <reg name=\"vdp2_scxdn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"98\"/>\n"
                                "    <reg name=\"vdp2_scyin1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"99\"/>\n"
                                "    <reg name=\"vdp2_scydn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"100\"/>\n"
                                "    <reg name=\"vdp2_zmxin1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"101\"/>\n"
                                "    <reg name=\"vdp2_zmxdn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"102\"/>\n"
                                "    <reg name=\"vdp2_zmyin1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"103\"/>\n"
                                "    <reg name=\"vdp2_zmydn1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"104\"/>\n"
                                "    <reg name=\"vdp2_scxn2\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"105\"/>\n"
                                "    <reg name=\"vdp2_scyn2\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"106\"/>\n"
                                "    <reg name=\"vdp2_scxn3\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"107\"/>\n"
                                "    <reg name=\"vdp2_scyn3\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"108\"/>\n"
                                "    <reg name=\"vdp2_zmctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"109\"/>\n"
                                "    <reg name=\"vdp2_scrctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"110\"/>\n"
                                "    <reg name=\"vdp2_vcstau\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"111\"/>\n"
                                "    <reg name=\"vdp2_vcstal\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"112\"/>\n"
                                "    <reg name=\"vdp2_lsta0u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"113\"/>\n"
                                "    <reg name=\"vdp2_lsta0l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"114\"/>\n"
                                "    <reg name=\"vdp2_lsta1u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"115\"/>\n"
                                "    <reg name=\"vdp2_lsta1l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"116\"/>\n"
                                "    <reg name=\"vdp2_lctau\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"117\"/>\n"
                                "    <reg name=\"vdp2_lctal\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"118\"/>\n"
                                "    <reg name=\"vdp2_bktau\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"119\"/>\n"
                                "    <reg name=\"vdp2_bktal\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"120\"/>\n"
                                "    <reg name=\"vdp2_rpmd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"121\"/>\n"
                                "    <reg name=\"vdp2_rprctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"122\"/>\n"
                                "    <reg name=\"vdp2_ktctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"123\"/>\n"
                                "    <reg name=\"vdp2_ktaof\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"124\"/>\n"
                                "    <reg name=\"vdp2_ovpnra\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"125\"/>\n"
                                "    <reg name=\"vdp2_ovpnrb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"126\"/>\n"
                                "    <reg name=\"vdp2_rptau\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"127\"/>\n"
                                "    <reg name=\"vdp2_rptal\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"128\"/>\n"
                                "    <reg name=\"vdp2_wpsx0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"129\"/>\n"
                                "    <reg name=\"vdp2_wpsy0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"130\"/>\n"
                                "    <reg name=\"vdp2_wpex0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"131\"/>\n"
                                "    <reg name=\"vdp2_wpey0\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"132\"/>\n"
                                "    <reg name=\"vdp2_wpsx1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"133\"/>\n"
                                "    <reg name=\"vdp2_wpsy1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"134\"/>\n"
                                "    <reg name=\"vdp2_wpex1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"135\"/>\n"
                                "    <reg name=\"vdp2_wpey1\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"136\"/>\n"
                                "    <reg name=\"vdp2_wctla\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"137\"/>\n"
                                "    <reg name=\"vdp2_wctlb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"138\"/>\n"
                                "    <reg name=\"vdp2_wctlc\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"139\"/>\n"
                                "    <reg name=\"vdp2_wctld\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"140\"/>\n"
                                "    <reg name=\"vdp2_lwta0u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"141\"/>\n"
                                "    <reg name=\"vdp2_lwta0l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"142\"/>\n"
                                "    <reg name=\"vdp2_lwta1u\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"143\"/>\n"
                                "    <reg name=\"vdp2_lwta1l\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"144\"/>\n"
                                "    <reg name=\"vdp2_spctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"145\"/>\n"
                                "    <reg name=\"vdp2_sdctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"146\"/>\n"
                                "    <reg name=\"vdp2_craofa\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"147\"/>\n"
                                "    <reg name=\"vdp2_craofb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"148\"/>\n"
                                "    <reg name=\"vdp2_lnclen\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"149\"/>\n"
                                "    <reg name=\"vdp2_sfprmd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"150\"/>\n"
                                "    <reg name=\"vdp2_ccctl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"151\"/>\n"
                                "    <reg name=\"vdp2_sfccmd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"152\"/>\n"
                                "    <reg name=\"vdp2_prisa\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"153\"/>\n"
                                "    <reg name=\"vdp2_prisb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"154\"/>\n"
                                "    <reg name=\"vdp2_prisc\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"155\"/>\n"
                                "    <reg name=\"vdp2_prisd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"156\"/>\n"
                                "    <reg name=\"vdp2_prina\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"157\"/>\n"
                                "    <reg name=\"vdp2_prinb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"158\"/>\n"
                                "    <reg name=\"vdp2_prir\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"159\"/>\n"
                                "    <reg name=\"vdp2_ccrsa\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"160\"/>\n"
                                "    <reg name=\"vdp2_ccrsb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"161\"/>\n"
                                "    <reg name=\"vdp2_ccrsc\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"162\"/>\n"
                                "    <reg name=\"vdp2_ccrsd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"163\"/>\n"
                                "    <reg name=\"vdp2_ccrna\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"164\"/>\n"
                                "    <reg name=\"vdp2_ccrnb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"165\"/>\n"
                                "    <reg name=\"vdp2_ccrr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"166\"/>\n"
                                "    <reg name=\"vdp2_ccrlb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"167\"/>\n"
                                "    <reg name=\"vdp2_clofen\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"168\"/>\n"
                                "    <reg name=\"vdp2_clofsl\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"169\"/>\n"
                                "    <reg name=\"vdp2_coar\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"170\"/>\n"
                                "    <reg name=\"vdp2_coag\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"171\"/>\n"
                                "    <reg name=\"vdp2_coab\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"172\"/>\n"
                                "    <reg name=\"vdp2_cobr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"173\"/>\n"
                                "    <reg name=\"vdp2_cobg\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"174\"/>\n"
                                "    <reg name=\"vdp2_cobb\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\" regnum=\"175\"/>\n"
                                "  </feature>\n"
                                "  <feature name=\"org.sega.saturn.slave_sh2\">\n"
                                "    <reg name=\"slave_r0\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"176\"/>\n"
                                "    <reg name=\"slave_r1\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"177\"/>\n"
                                "    <reg name=\"slave_r2\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"178\"/>\n"
                                "    <reg name=\"slave_r3\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"179\"/>\n"
                                "    <reg name=\"slave_r4\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"180\"/>\n"
                                "    <reg name=\"slave_r5\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"181\"/>\n"
                                "    <reg name=\"slave_r6\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"182\"/>\n"
                                "    <reg name=\"slave_r7\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"183\"/>\n"
                                "    <reg name=\"slave_r8\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"184\"/>\n"
                                "    <reg name=\"slave_r9\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"185\"/>\n"
                                "    <reg name=\"slave_r10\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"186\"/>\n"
                                "    <reg name=\"slave_r11\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"187\"/>\n"
                                "    <reg name=\"slave_r12\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"188\"/>\n"
                                "    <reg name=\"slave_r13\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"189\"/>\n"
                                "    <reg name=\"slave_r14\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"190\"/>\n"
                                "    <reg name=\"slave_r15\" bitsize=\"32\" type=\"data_ptr\" format=\"hex\" regnum=\"191\"/>\n"
                                "    <reg name=\"slave_pc\" bitsize=\"32\" type=\"code_ptr\" format=\"hex\" regnum=\"192\"/>\n"
                                "    <reg name=\"slave_pr\" bitsize=\"32\" type=\"code_ptr\" format=\"hex\" regnum=\"193\"/>\n"
                                "    <reg name=\"slave_gbr\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"194\"/>\n"
                                "    <reg name=\"slave_vbr\" bitsize=\"32\" type=\"code_ptr\" format=\"hex\" regnum=\"195\"/>\n"
                                "    <reg name=\"slave_mach\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"196\"/>\n"
                                "    <reg name=\"slave_macl\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"197\"/>\n"
                                "    <reg name=\"slave_sr\" bitsize=\"32\" type=\"uint32\" format=\"hex\" regnum=\"198\"/>\n"
                                "  </feature>\n"
                                "</target>\n";
                            // Send in chunks respecting the requested length from GDB.
                            // Parse offset and length from "qXfer:features:read:target.xml:off,len"
                            static const size_t xml_len = sizeof(target_xml) - 1U;
                            uint32_t xfer_off = 0, xfer_len = 0;
                            bool xfer_ok = false;
                            {
                                const char* colon = in_buf;
                                int colons = 0;
                                while (*colon && colons < 4) { if (*colon++ == ':') ++colons; }
                                // colon now points past the 4th ':', i.e. at "off,len"
                                // If the packet was malformed (fewer than 4 colons), *colon will 
                                // point exactly to '\0'. parse_hex_u32_until handles empty strings
                                // correctly by immediately returning false, safely aborting the parse.
                                if (parse_hex_u32_until(colon, ',', xfer_off, colon) && *colon == ',') {
                                    ++colon;
                                    xfer_ok = parse_hex_u32_until(colon, '\0', xfer_len, colon);
                                }
                            }
                            if (!xfer_ok) {
                                packet_put('\0', "E01", 3);
                                break;
                            }
                            if (xfer_off >= xml_len) {
                                out_buf[0] = 'l'; packet_put('\0', out_buf, 1);
                            } else {
                                size_t avail = xml_len - xfer_off;
                                size_t send  = avail < xfer_len ? avail : xfer_len;
                                if (send > kPacketDataMax) send = kPacketDataMax;
                                
                                // Ensure kPacketDataMax can never be raised so high that out_buf[1 + i] overflows.
                                static_assert(kPacketDataMax < sizeof(out_buf) - 1U, "kPacketDataMax is too large for out_buf");

                                bool last = (xfer_off + send >= xml_len);
                                out_buf[0] = last ? 'l' : 'm';
                                for (size_t i = 0; i < send; ++i) out_buf[1 + i] = target_xml[xfer_off + i];
                                packet_put('\0', out_buf, 1 + send);
                            }
                        } else if (starts_with(in_buf, "qfThreadInfo")) {
                            // Single-thread target.
                            packet_put('\0', "m1", 2);
                        } else if (starts_with(in_buf, "qsThreadInfo")) {
                            packet_put('\0', "l", 1);
                        } else if (starts_with(in_buf, "qAttached")) {
                            packet_put('\0', "1", 1);
                        } else if (starts_with(in_buf, "qOffsets")) {
                            constexpr const char offsets[] = "Text=0;Data=0;Bss=0";
                            packet_put('\0', offsets, sizeof(offsets) - 1);
                        } else if (starts_with(in_buf, "qC")) {
                            packet_put('\0', "QC1", 3);
                        } else if (starts_with(in_buf, "qRcmd,")) {
                            // GDB's `monitor <text>` command: qRcmd,<hex-encoded-ascii-text>.
                            // Two built-in diagnostic commands ("regs slave", "regs vdp") are
                            // handled synchronously right here, replying with $O console-output
                            // packets (see send_monitor_text) before the final OK, since their
                            // data already lives in memory the stub can read itself. Everything
                            // else is decoded into g_last_monitor_command and the counter is
                            // bumped; user code polls GetMonitorCommandCount()/
                            // GetLastMonitorCommand() to react (e.g. "crash illegal").
                            const char* hex_payload = in_buf + 6;
                            size_t hex_len = 0;
                            while (hex_payload[hex_len] != '\0') hex_len++;
                            const size_t cmd_len = hex_len / 2;
                            if (hex_len == 0 || (hex_len & 1U) != 0U || cmd_len >= sizeof(g_last_monitor_command)) {
                                packet_put('\0', "E01", 3);
                            } else if (hex2mem(hex_payload, reinterpret_cast<uint8_t*>(g_last_monitor_command), static_cast<int>(cmd_len))) {
                                g_last_monitor_command[cmd_len] = '\0';
                                if (str_equals(g_last_monitor_command, "regs slave")) {
                                    send_slave_regs_dump();
                                    packet_put('\0', "OK", 2);
                                } else if (str_equals(g_last_monitor_command, "regs vdp")) {
                                    send_vdp_regs_dump();
                                    packet_put('\0', "OK", 2);
                                } else if (str_equals(g_last_monitor_command, "nmi")) {
                                    send_nmi_diag_dump();
                                    packet_put('\0', "OK", 2);
                                } else if (str_equals(g_last_monitor_command, "trace")) {
                                    send_halt_trace_dump();
                                    packet_put('\0', "OK", 2);
                                } else {
                                    g_monitor_command_count = g_monitor_command_count + 1;
                                    packet_put('\0', "OK", 2);
                                }
                            } else {
                                packet_put('\0', "E01", 3);
                            }
                        } else {
                            packet_put('\0', nullptr, 0);
                        }
                        break;
                    case 'H':
                        packet_put('\0', "OK", 2);
                        break;

                    case 'v':
                        // Minimal v packet support for MI/VS Code remote sessions.
                        if (starts_with(in_buf, "vCont?")) {
                            constexpr const char vcont_supported[] = "vCont;c;s";
                            packet_put('\0', vcont_supported, sizeof(vcont_supported) - 1);
                        } else if (starts_with(in_buf, "vCont;")) {
                            // Scan each ;action[:tid] pair.
                            // Only treat as step if s/S applies to thread 1 (:1),
                            // to all threads (:*), or has no thread qualifier at all.
                            // A step directed at an unrecognised thread (e.g. ;s:2) is
                            // ignored — fall through to continue for our single thread.
                            bool has_step = false;
                            const char* p = in_buf + 5; 
                            while (*p != '\0') {
                                if (*p == ';') {
                                    ++p; // Skip ';'
                                    if (*p == '\0') break;
                                    const char action = *p;
                                    
                                    // Scan to the next ';' or the end of the string
                                    const char* next_semi = p;
                                    while (*next_semi != '\0' && *next_semi != ';') {
                                        ++next_semi;
                                    }

                                    if (action == 's' || action == 'S') {
                                        // The token format is 's' or 'S' followed optionally by ':<thread>'
                                        if (p + 1 == next_semi) {
                                            // Just "s" or "S", no qualifier
                                            has_step = true;
                                            break;
                                        } else if (p[1] == ':') {
                                            const char thread_id = p[2];
                                            if (thread_id == '*' || (thread_id == '1' && (p[3] == '\0' || p[3] == ';'))) {
                                                has_step = true;
                                                break;
                                            }
                                        }
                                    }
                                    // Advance pointer to the next ';' (or end of string)
                                    p = next_semi;
                                } else {
                                    ++p;
                                }
                            }
                            if (has_step) {
                                handle_gdb_step();
                            } else {
                                handle_gdb_continue();
                            }
                            return;
                        } else if (starts_with(in_buf, "vRun")) {
                            // Extended-remote run compatibility: treat like continue.
                            handle_gdb_continue();
                            return;
                        } else {
                            packet_put('\0', nullptr, 0);
                        }
                        break;
                    case 'g':
                        {
                            if (g_ctx.pc == 0) {
                                snapshot_polling_context();
                            }
                            
                            // Send a copy of the context. We do NOT zero PR/R14 for Ctrl-C-family
                            // stops here, despite an earlier version of this code doing so to stop
                            // GDB from unwinding into SGL's no-debug-info interrupt wrapper.
                            //
                            // Hardware-confirmed regression that reverted it: reporting PR=0 (a
                            // sentinel GDB reads as "no caller / invalid frame") is what actually
                            // breaks GDB's client, not the reverse. Caught live on a real NMI-frozen
                            // target: `monitor trace` (reading g_ctx.pr directly, unaffected by this
                            // masking) showed a real, valid PR, but GDB's own `print $pr` -- reading
                            // the masked 'g' reply -- showed exactly 0. From that point on, EVERY
                            // subsequent command in that GDB session failed with "Cannot evaluate
                            // expression on the specified stack frame", including plain `monitor`
                            // requests that never touch frame/expression logic at all -- meaning
                            // GDB's client got stuck before it even sent anything further to the
                            // target, blocking Continue along with everything else. Separately
                            // confirmed: a real (unmasked) PR pointing into the same no-debug-info
                            // library code did NOT cause GDB to hang or loop -- `bt` returned
                            // promptly with "Backtrace stopped: frame did not save the PC". So this
                            // masking was solving a problem GDB's current SH-2 backend already
                            // handles fine on its own, while introducing a much worse one.
                            SH2Context ctx_copy = g_ctx;

                            char* p_out = out_buf;
                            p_out = mem2hex((uint8_t*)&ctx_copy, p_out, sizeof(SH2Context));

                            // Pad to the fixed size stock GDB's SH backend requires (see
                            // GdbFixedShPaddingRegisters above) instead of appending the
                            // VDP1/VDP2/slave pseudo-registers -- those remain reachable via
                            // 'p'/'P' with the same register indices, and VDP1/VDP2 are always
                            // readable as ordinary memory via 'm' at their real addresses.
                            for (size_t i = 0; i < GdbFixedShPaddingRegisters * 4U; ++i) {
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
                            // The G packet payload must contain at least the core register set.
                            // GDB reflects back the full g response -- including the trailing
                            // zero-padding block -- so we accept any payload >= core size and
                            // only write the first sizeof(SH2Context)*2 chars.
                            constexpr size_t core_len = sizeof(SH2Context) * 2;
                            size_t len = 0;
                            while (in_buf[1 + len] != '\0') len++;
                            if (len < core_len) {
                                packet_put('\0', "E01", 3);
                            } else if (hex2mem(&in_buf[1], (uint8_t*)&g_ctx, sizeof(SH2Context))) {
                                packet_put('\0', "OK", 2);
                            } else {
                                packet_put('\0', "E01", 3);
                            }
                        }
                        break;
                    case 'p': // Read a single register
                        {
                            uint32_t reg_idx = 0;
                            const char* ptr = &in_buf[1];
                            if (!parse_hex_u32_until(ptr, '\0', reg_idx, ptr)) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx > 22 + TotalPseudoRegs) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx >= 23 && reg_idx < 23 + NumExtraRegs) {
                                uint16_t val = *(volatile uint16_t*)ExtraRegs[reg_idx - 23];
                                const int tx_len = static_cast<int>(mem2hex((uint8_t*)&val, out_buf, 2) - out_buf);
                                packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                                break;
                            }

                            if (reg_idx >= 23 + NumExtraRegs) {
                                const uint32_t slave_reg_index = reg_idx - (23 + NumExtraRegs);
                                uint32_t* reg_ptr = &g_slave_ctx.r[0];
                                if (slave_reg_index < 16U) reg_ptr = &g_slave_ctx.r[slave_reg_index];
                                else if (slave_reg_index == 16U) reg_ptr = &g_slave_ctx.pc;
                                else if (slave_reg_index == 17U) reg_ptr = &g_slave_ctx.pr;
                                else if (slave_reg_index == 18U) reg_ptr = &g_slave_ctx.gbr;
                                else if (slave_reg_index == 19U) reg_ptr = &g_slave_ctx.vbr;
                                else if (slave_reg_index == 20U) reg_ptr = &g_slave_ctx.mach;
                                else if (slave_reg_index == 21U) reg_ptr = &g_slave_ctx.macl;
                                else if (slave_reg_index == 22U) reg_ptr = &g_slave_ctx.sr;
                                else {
                                    packet_put('\0', "E01", 3);
                                    break;
                                }
                                const int tx_len = static_cast<int>(mem2hex(reinterpret_cast<uint8_t*>(reg_ptr), out_buf, 4) - out_buf);
                                packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                                break;
                            }

                            uint32_t* reg_ptr = &g_ctx.r[0];
                            if (reg_idx < 16) reg_ptr = &g_ctx.r[reg_idx];
                            else if (reg_idx == 16) reg_ptr = &g_ctx.pc;
                            else if (reg_idx == 17) reg_ptr = &g_ctx.pr;
                            else if (reg_idx == 18) reg_ptr = &g_ctx.gbr;
                            else if (reg_idx == 19) reg_ptr = &g_ctx.vbr;
                            else if (reg_idx == 20) reg_ptr = &g_ctx.mach;
                            else if (reg_idx == 21) reg_ptr = &g_ctx.macl;
                            else if (reg_idx == 22) reg_ptr = &g_ctx.sr;

                            const int tx_len = static_cast<int>(mem2hex(reinterpret_cast<uint8_t*>(reg_ptr), out_buf, 4) - out_buf);
                            packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                        }
                        break;
                    case 'P': // Write a single register
                        {
                            uint32_t reg_idx = 0;
                            const char* ptr = &in_buf[1];
                            if (!parse_hex_u32_until(ptr, '=', reg_idx, ptr) || *ptr != '=') {
                                packet_put('\0', "E01", 3);
                                break;
                            }
                            ptr++; // skip '='

                            if (reg_idx > 22 + TotalPseudoRegs) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx >= 23 && reg_idx < 23 + NumExtraRegs) {
                                uint16_t val = 0;
                                if (hex2mem(ptr, (uint8_t*)&val, 2)) {
                                    *(volatile uint16_t*)ExtraRegs[reg_idx - 23] = val;
                                    packet_put('\0', "OK", 2);
                                } else {
                                    packet_put('\0', "E01", 3);
                                }
                                break;
                            }

                            if (reg_idx >= 23 + NumExtraRegs) {
                                const uint32_t slave_reg_index = reg_idx - (23 + NumExtraRegs);
                                uint32_t* reg_ptr = &g_slave_ctx.r[0];
                                if (slave_reg_index < 16U) reg_ptr = &g_slave_ctx.r[slave_reg_index];
                                else if (slave_reg_index == 16U) reg_ptr = &g_slave_ctx.pc;
                                else if (slave_reg_index == 17U) reg_ptr = &g_slave_ctx.pr;
                                else if (slave_reg_index == 18U) reg_ptr = &g_slave_ctx.gbr;
                                else if (slave_reg_index == 19U) reg_ptr = &g_slave_ctx.vbr;
                                else if (slave_reg_index == 20U) reg_ptr = &g_slave_ctx.mach;
                                else if (slave_reg_index == 21U) reg_ptr = &g_slave_ctx.macl;
                                else if (slave_reg_index == 22U) reg_ptr = &g_slave_ctx.sr;
                                else {
                                    packet_put('\0', "E01", 3);
                                    break;
                                }
                                if (hex2mem(ptr, reinterpret_cast<uint8_t*>(reg_ptr), 4)) {
                                    packet_put('\0', "OK", 2);
                                } else {
                                    packet_put('\0', "E01", 3);
                                }
                                break;
                            }

                            uint32_t* reg_ptr = &g_ctx.r[0];
                            if (reg_idx < 16) reg_ptr = &g_ctx.r[reg_idx];
                            else if (reg_idx == 16) reg_ptr = &g_ctx.pc;
                            else if (reg_idx == 17) reg_ptr = &g_ctx.pr;
                            else if (reg_idx == 18) reg_ptr = &g_ctx.gbr;
                            else if (reg_idx == 19) reg_ptr = &g_ctx.vbr;
                            else if (reg_idx == 20) reg_ptr = &g_ctx.mach;
                            else if (reg_idx == 21) reg_ptr = &g_ctx.macl;
                            else if (reg_idx == 22) reg_ptr = &g_ctx.sr;

                            if (hex2mem(ptr, reinterpret_cast<uint8_t*>(reg_ptr), 4)) {
                                packet_put('\0', "OK", 2);
                            } else {
                                packet_put('\0', "E01", 3);
                            }
                        }
                        break;
                    case 'm':
                        {
                            uint32_t addr = 0, length = 0;
                            const char* ptr = &in_buf[1];
                            if (!parse_hex_u32_until(ptr, ',', addr, ptr) || *ptr != ',') {
                                packet_put('\0', "E01", 3);
                                break;
                            }
                            ++ptr; // skip ','
                            if (!parse_hex_u32_until(ptr, '\0', length, ptr)) {
                                packet_put('\0', "E01", 3);
                                break;
                            }
                            // Note: reading memory here does not touch the slave, so this is
                            // safe even while g_debug_pause is set (slave frozen) -- the master's
                            // own bus access is independent of slave state.
                            // Keep response within local buffer limits (hex encoding = 2x bytes + NUL).
                            if (length > 511U || !is_valid_memory_range(addr, length)) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            // Read via the cache-through mirror (see install_software_breakpoint
                            // and friends) so a value just patched through that same mirror --
                            // e.g. a breakpoint installed earlier in this same halted session,
                            // before the deferred CacheFlusher purge runs -- isn't masked by a
                            // stale D-cache line still held under the plain (cached) alias.
                            const int tx_len = static_cast<int>(mem2hex((uint8_t*)(addr | 0x20000000U), out_buf, static_cast<int>(length)) - out_buf);
                            packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                        }
                        break;
                    case 'M':
                        {
                            uint32_t addr = 0, length = 0;
                            const char* p = &in_buf[1];
                            if (!parse_hex_u32_until(p, ',', addr, p) || *p != ',') {
                                packet_put('\0', "E02", 3);
                                break;
                            }
                            ++p; // skip ','
                            if (!parse_hex_u32_until(p, ':', length, p) || *p != ':') {
                                packet_put('\0', "E02", 3);
                                break;
                            }
                            ++p; // skip ':'

                            if (length > 511U || !is_valid_memory_range(addr, length)) {
                                packet_put('\0', "E02", 3);
                                break;
                            }

                            // Write via the cache-through mirror, same as every other in-place
                            // patch in this file (breakpoints, step traps) -- see the 'm' handler
                            // just above for why the plain (cached) alias isn't safe here.
                            // Uses hex2mem_aligned() rather than plain hex2mem(): this handler
                            // can target VDP RAM (e.g. CRAM), which doesn't reliably latch
                            // single-byte writes -- see hex2mem_aligned()'s doc comment.
                            if (hex2mem_aligned(p, addr | 0x20000000U, length)) {
                                PurgeCache();
                                packet_put('\0', "OK", 2);
                            } else {
                                packet_put('\0', "E01", 3);
                            }
                        }
                        break;
                    case 'Z':
                    case 'z':
                        {
                            // RSP breakpoints/watchpoints: Z[type],addr,kind / z[type],addr,kind
                            const char type_char = in_buf[1];
                            if ((type_char < '0' || type_char > '4') || in_buf[2] != ',') {
                                packet_put('\0', nullptr, 0); // Not supported type or bad format
                                break;
                            }
                            
                            const uint32_t wp_type = static_cast<uint32_t>(type_char - '0');

                            uint32_t addr = 0;
                            const char* p = &in_buf[3];
                            if (!parse_hex_u32_until(p, ',', addr, p) || *p != ',') {
                                packet_put('\0', "E03", 3);
                                break;
                            }

                            ++p; // skip ','
                            uint32_t kind = 0;
                            const char* end = p;
                            if (!parse_hex_u32_until(p, '\0', kind, end)) {
                                packet_put('\0', "E03", 3);
                                break;
                            }

                            bool ok = false;
                            if (in_buf[0] == 'Z') {
                                if (wp_type == 0) {
                                    // SH-2 instructions are 16-bit (kind normally 2).
                                    if (kind == 0U || kind == 2U) {
                                        ok = install_software_breakpoint(addr);
                                    }
                                } else {
                                    ok = install_hardware_watchpoint(addr, wp_type);
                                }
                            } else {
                                if (wp_type == 0) {
                                    if (kind == 0U || kind == 2U) {
                                        ok = remove_software_breakpoint(addr);
                                        // Master context here (this is GDB's own
                                        // z-packet handler) -- see
                                        // remove_software_breakpoint()'s doc comment
                                        // for why it doesn't do this itself.
                                        if (ok) {
                                            PurgeSlaveCacheBestEffort();
                                        }
                                    }
                                } else {
                                    ok = remove_hardware_watchpoint(addr, wp_type);
                                }
                            }

                            packet_put('\0', ok ? "OK" : "E03", ok ? 2 : 3);
                        }
                        break;
                    case 'D': // Detach
                        packet_put('\0', "OK", 2);
                        clear_breakpoints(true);
                        g_handshake_done = false;
                        g_has_connection = false;
                        SlaveIPIClear();
                        return;
                    case 'T': // Is thread alive?
                        // Report thread as alive for single-thread target.
                        packet_put('\0', "OK", 2);
                        break;
                    case 'c': {
                        // Optional trailing hex address ("caddr"): resume at addr instead
                        // of the current PC. No address is the overwhelmingly common case
                        // (plain "c"), so a parse failure/absence just leaves PC untouched.
                        if (in_buf[1] != '\0') {
                            uint32_t addr = 0;
                            const char* end = &in_buf[1];
                            if (parse_hex_u32_until(&in_buf[1], '\0', addr, end)) {
                                g_ctx.pc = addr;
                            }
                        }
                        handle_gdb_continue();
                        return;
                    }
                    case 's':
                    case 'S':
                        {
                            // "saddr" has no separator; "S sig[;addr]" carries a two-digit
                            // signal number (ignored -- we don't support signal delivery)
                            // before the optional ';addr'.
                            const char* p = &in_buf[1];
                            if (in_buf[0] == 'S') {
                                if (hex(p[0]) >= 0 && hex(p[1]) >= 0) p += 2;
                                if (*p == ';') ++p;
                            }
                            if (*p != '\0') {
                                uint32_t addr = 0;
                                const char* end = p;
                                if (parse_hex_u32_until(p, '\0', addr, end)) {
                                    g_ctx.pc = addr;
                                }
                            }
                        }
                        handle_gdb_step();
                        return;
                    default:
                        packet_put('\0', nullptr, 0);
                        break;
                }
            }
        }
        // --- Exception Handler ---

        extern "C" void srl_gdbstub_exception_thunk();
        // Tiny trampolines that tag g_last_stop_signal with the right POSIX
        // signal for their exception family before falling into the shared
        // thunk above -- see their definition (right after
        // srl_gdbstub_exception_thunk's __asm__ block) for why this needs to
        // be a jump into the SAME shared body rather than a full duplicate:
        // every SH-2 exception vector lands at a fixed address with no
        // argument-passing convention and no on-chip "cause" register (unlike
        // e.g. SH-3/4's EXPEVT), so the only way to tell GDB which exception
        // family fired is to give each family its own tiny entry stub.
        extern "C" void srl_gdbstub_illegal_thunk();
        extern "C" void srl_gdbstub_addrerr_thunk();
        extern "C" void srl_gdbstub_nmi_thunk();


        /**
         * @brief Hook the SH-2 CPU exception vectors for the GDB stub.
         * 
         * Relocates the Vector Base Register (VBR) from ROM to RAM if necessary,
         * and installs `srl_gdbstub_exception_thunk` as the handler for critical
         * CPU traps including Illegal Instruction, Address Errors, and NMI.
         *
         * NMI is how the Saturn's physical Reset button reaches the SH-2 (it is
         * not a hard reset line) -- once this is installed, pressing Reset stops
         * the program at whatever instruction it interrupted and reports SIGINT
         * to GDB, exactly like Ctrl-C, instead of actually rebooting the console.
         */
        static inline void InstallExceptionHandlers() {
            SRL::Logger::Log::LogPrint("[GDBStub] InstallExceptionHandlers() start");
            if (!g_handlers_installed) {
                // Read the current VBR
                uint32_t current_vbr = 0;
                asm volatile("stc vbr, %0" : "=r"(current_vbr));
                
                // If VBR is still 0 (Boot ROM), we cannot write to it. We must relocate to RAM.
                // We use 0x06000000 as a safe fallback and copy the Boot ROM vectors there to preserve the chain.
                if (current_vbr == 0) {
                    current_vbr = 0x06000000;
                    // On the Saturn, physical address 0x00000000 maps to the Boot ROM.
                    // We read from the cache-through mirror at 0x20000000 to ensure we bypass
                    // the cache. Using the 0x20000000 mirror explicitly guarantees we read the
                    // actual ROM vectors even if a dev environment mapped something else to
                    // the cached address 0.
                    volatile uint32_t* src_table = reinterpret_cast<volatile uint32_t*>(0x20000000U);
                    volatile uint32_t* dst_table = reinterpret_cast<volatile uint32_t*>(current_vbr | 0x20000000U);
                    // The SH-2 exception vector table is exactly 256 bytes = 64 × uint32_t entries.
                    // Copying 256 uint32_t would read 1024 bytes — 768 bytes past the table end.
                    for (int i = 0; i < 64; i++) {
                        dst_table[i] = src_table[i];
                    }
                    asm volatile("ldc %0, vbr" :: "r"(current_vbr));
                }
                
                // Write directly to the Cache-Through mirror of the VBR table
                volatile uint32_t* vbr_table = reinterpret_cast<volatile uint32_t*>(current_vbr | 0x20000000U);

                // Patch our exceptions
                // SH-2 exception vector layout from VBR:
                //   Fixed exceptions (reset, NMI, etc): VBR + 0x000..0x07C  (indices 0..31)
                //   External/internal interrupts:         VBR + 0x080..0x0FC  (indices 32..63)
                //   TRAPA #N vectors:                     VBR + 0x080 + N*4   (indices 32+N)
                // The "illegal opcode" family and "address error" family route through
                // their own tiny trampolines so GDB is told SIGILL/SIGBUS instead of a
                // generic SIGTRAP for these -- see srl_gdbstub_illegal_thunk /
                // srl_gdbstub_addrerr_thunk's doc comment. UBC break and TRAPA #3 are
                // genuinely trap/breakpoint-like, so they keep reporting SIGTRAP as
                // before and go straight to the shared thunk. NMI gets its own
                // trampoline too (srl_gdbstub_nmi_thunk) so it's reported as SIGINT --
                // the same signal Ctrl-C uses -- rather than a generic SIGTRAP; this
                // also reuses the existing g_is_ctrl_c_stop-gated PR/R14 masking in the
                // 'g' register-read handler, since NMI (an asynchronous button press)
                // can interrupt SGL/BIOS code with no debug info to unwind through,
                // exactly like a Ctrl-C stop can.
                vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);  // Illegal Instruction
                vbr_table[5] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);  // Reserved Instruction
                vbr_table[6] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);  // Slot Illegal Instruction
                vbr_table[7] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);  // General Illegal Instruction
                vbr_table[8] = reinterpret_cast<uint32_t>(&srl_gdbstub_illegal_thunk);  // Slot Reserved Instruction
                vbr_table[9] = reinterpret_cast<uint32_t>(&srl_gdbstub_addrerr_thunk);  // CPU Address Error
                vbr_table[10] = reinterpret_cast<uint32_t>(&srl_gdbstub_addrerr_thunk); // DMA Address Error
                vbr_table[11] = reinterpret_cast<uint32_t>(&srl_gdbstub_nmi_thunk);      // NMI (Reset button)
                vbr_table[12] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // User Break Controller
                vbr_table[35] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // TRAPA #3 (Legacy/Fallback)

                ForcePurgeCache();
                g_handlers_installed = true;
            }
            SRL::Logger::Log::LogPrint("[GDBStub] InstallExceptionHandlers() end");
        }


        // --- Public API ---

        /**
         * @brief Returns true when cartridge-level USB data path should be enabled.
         */
        inline bool IsUsbDataPathEnabled() {
            return g_devcart_usb_datapath_enabled;
        }

        /**
         * @brief Initialize the GDB stub and hook exception vectors.
         */
        inline void Init() {
            SRL::Logger::Log::LogPrint("[GDBStub] Init() start");
            g_has_connection = false;
            g_handshake_done = false;
            g_command_count = 0;
            g_exception_thunk_count = 0;
            g_rx_detect_count = 0;
            g_rx_ready_count = 0;
            g_poll_fallback_count = 0;
            g_last_command[0] = '\0';
            g_unget_char = -1;
            // Use CS0 USB_FLAGS readability as the proxy for cart detection.
            // Do NOT read CS1 registers here: on USBGamers carts the CS1 space
            // may not be decoded, causing the SH-2 bus to hang (bus error).
            // HasWascaSignature() checks CS1 CPLD magic bytes (0x24000001/03);
            // IsPortAvailable() only reads CS0 USB_FLAGS which is always safe.
            g_devcart_ready = SRL::DevCart::CS0::IsPortAvailable(); // CS0-only check
            g_devcart_port_available = SRL::DevCart::CS0::IsPortAvailable();

            g_last_usb_flags = SRL::DevCart::CS0::ReadFlags();
            g_is_ctrl_c_stop = false;
            g_last_stop_signal = 5;
            g_ubc_channel_a_active = false;
            g_devcart_usb_datapath_enabled = true;
            clear_breakpoints(false);
            // Initialise the pause flag – false by default.
            g_debug_pause = false;

            SRL::Logger::Log::LogPrint("[GDBStub] DevCart ready: %d, Port: %d, USB Datapath: %d",
                g_devcart_ready ? 1 : 0, g_devcart_port_available ? 1 : 0, g_devcart_usb_datapath_enabled ? 1 : 0);

            if (!g_handlers_installed) {
                InstallExceptionHandlers();
            }
            SRL::Logger::Log::LogPrint("[GDBStub] Init() end");
        }

        /**
         * @brief Returns true only after the GDB handshake (qSupported exchange) has completed.
         */
        inline bool IsConnected() {
            return g_handshake_done;
        }

        /**
         * @brief Returns true once the GDB exception handlers have been installed.
         */
        inline bool IsHandlersInstalled() {
            return g_handlers_installed;
        }

        /**
         * @brief Returns how many times ExceptionThunk has executed.
         */
        inline uint32_t GetExceptionThunkCount() {
            return g_exception_thunk_count;
        }

        /**
         * @brief Returns how many RX bytes were consumed by the stub from DevCart.
         */
        inline uint32_t GetRxDetectCount() {
            return g_rx_detect_count;
        }

        /**
         * @brief Returns how many times Poll() observed RX data pending in DevCart FIFO.
         */
        inline uint32_t GetRxReadyCount() {
            return g_rx_ready_count;
        }

        /**
         * @brief Returns how many times Poll() had to process RX without Trap3 entry.
         */
        inline uint32_t GetPollFallbackCount() {
            return g_poll_fallback_count;
        }

        /**
         * @brief Returns true when DevCart CS1 signature registers are readable and valid.
         */
        inline bool IsDevCartReady() {
            return g_devcart_ready;
        }

        /**
         * @brief Returns true when USB_FLAGS reserved bits match expected USB dev cart pattern.
         */
        inline bool IsDevCartPortAvailable() {
            return g_devcart_port_available;
        }


        /**
         * @brief Returns latest raw USB_FLAGS value sampled by the stub.
         */
        inline uint8_t GetLastUsbFlags() {
            return g_last_usb_flags;
        }

        /**
         * @brief Returns how many `monitor <text>` commands have been received via qRcmd.
         * @details Compare against a locally-cached value each frame to detect a new
         * command, then dispatch on GetLastMonitorCommand(). This lets a GDB `monitor`
         * command (or any scripted qRcmd sender) trigger sample/test behavior without
         * needing a physical gamepad.
         *
         * @warning qRcmd is processed entirely on the target while it is stopped
         * (inside process_commands()'s packet loop), so user code only sees this
         * counter change once execution actually resumes via 'c'/'D'. If multiple
         * `monitor` commands are sent before that happens, edge-triggered polling
         * of this counter only observes ONE change and dispatches whatever
         * GetLastMonitorCommand() holds at that point -- earlier commands sent
         * while still stopped are silently coalesced away, not queued. Confirmed
         * on real hardware: two `monitor touch` calls sent back-to-back while
         * stopped only incremented the target variable once after resuming.
         * Send one `monitor` command, `continue`, then repeat if you need each
         * one to take effect individually.
         */
        inline uint32_t GetMonitorCommandCount() {
            return g_monitor_command_count;
        }

        /**
         * @brief Returns the text of the most recently received `monitor` command.
         */
        inline const char* GetLastMonitorCommand() {
            return g_last_monitor_command;
        }

        /**
         * @brief Returns how many times the slave's FRT-ICI freeze handler has fired.
         * @details Reads shared Work RAM, so this is safe to call from the master
         * even though the counter is incremented by code running on the slave.
         */
        inline uint32_t GetSlaveIciCount() {
            return g_slave_ici_count;
        }

        /**
         * @brief Returns how many times the slave's illegal-instruction
         * (breakpoint) handler has fired.
         * @details Reads shared Work RAM, so this is safe to call from the
         * master even though the counter is incremented by code running on
         * the slave. Zero unless InstallSlaveExceptionHandler() has been
         * installed on the slave.
         */
        inline uint32_t GetSlaveBreakpointCount() {
            return g_slave_bp_count;
        }

        extern "C" void srl_gdbstub_slave_ici_thunk();

        /**
         * @brief Installs GDBStub's slave-freeze handler on the FRT Input Capture
         * Interrupt (vector 0x64) of whichever CPU executes this function.
         *
         * @warning MUST be called from code running ON THE SLAVE SH-2 (e.g. via
         * SRL::Slave::ExecuteOnSlave / InstallSlaveFreezeTask below) — VBR, TIER,
         * and IPRB are per-CPU registers, so calling this from the master has no
         * effect on the slave's interrupt controller.
         *
         * @warning Hardware-confirmed: does not coexist with any use of
         * SRL::Slave::ExecuteOnSlave (slSlaveFunc) in the same project — see the
         * @warning above "Slave freeze via SH-2 on-chip FRT Input Capture
         * Interrupt (ICI)" and Samples/Debug - GDB Stub/readme.md for the full
         * hardware writeup. Only use this in a project that never calls
         * SRL::Slave::ExecuteOnSlave.
         */
        static inline void InstallSlaveFreezeHandler() {
            uint32_t vbr = 0;
            asm volatile("stc vbr, %0" : "=r"(vbr));

            if (vbr == 0) {
                // The slave boots with VBR == 0, same as the master. Relocate to a
                // slave-private area distinct from the master's own relocation
                // target (0x06000000, see InstallExceptionHandlers()) so the two
                // CPUs never overwrite each other's copy of the boot ROM vector
                // table.
                vbr = 0x06010000U;
                volatile uint32_t* src_table = reinterpret_cast<volatile uint32_t*>(0x20000000U);
                volatile uint32_t* dst_table = reinterpret_cast<volatile uint32_t*>(vbr | 0x20000000U);
                for (int i = 0; i < 64; i++) {
                    dst_table[i] = src_table[i];
                }
                asm volatile("ldc %0, vbr" :: "r"(vbr));
            }

            volatile uint32_t* vbr_table = reinterpret_cast<volatile uint32_t*>(vbr | 0x20000000U);
            vbr_table[FRT_ICI_VECTOR] = reinterpret_cast<uint32_t>(&srl_gdbstub_slave_ici_thunk);

            // Give the FRT interrupt group (ICI/OCIA/OCIB/OVI) a non-zero priority —
            // priority 0 is always masked regardless of the SR interrupt mask level.
            volatile uint16_t* iprb = reinterpret_cast<volatile uint16_t*>(FRT_IPRB);
            *iprb = static_cast<uint16_t>((*iprb & 0xF0FFU) | (0x0FU << 8));

            // Enable the Input Capture Interrupt itself.
            *reinterpret_cast<volatile uint8_t*>(FRT_TIER) |= FRT_ICF;

            // Lower this CPU's own SR interrupt mask (I3-I0) so priority-15
            // interrupts are actually accepted — out of reset, all interrupts are
            // masked (mask level 15).
            uint32_t sr = 0;
            asm volatile("stc sr, %0" : "=r"(sr));
            sr &= ~0x000000F0U;
            asm volatile("ldc %0, sr" :: "r"(sr) : "memory");

            ForcePurgeCache();
        }

        /**
         * @brief Convenience task that installs GDBStub's slave-freeze handler.
         * @details Must be dispatched via SRL::Slave::ExecuteOnSlave so that
         * InstallSlaveFreezeHandler() actually executes on the slave CPU:
         * @code
         * SRL::GDBStub::InstallSlaveFreezeTask installTask;
         * SRL::Slave::ExecuteOnSlave(installTask);
         * @endcode
         * @see InstallSlaveFreezeHandler
         */
        class InstallSlaveFreezeTask : public SRL::Types::ITask {
        protected:
            void Do() override {
                InstallSlaveFreezeHandler();
            }
        };

        extern "C" void srl_gdbstub_slave_illegal_thunk();

        /**
         * @brief Installs GDBStub's breakpoint handler on the Illegal Instruction
         * vector (4) of whichever CPU executes this function.
         *
         * @warning MUST be called from code running ON THE SLAVE SH-2 (e.g. via
         * SRL::Slave::ExecuteOnSlave / InstallSlaveExceptionTask below).
         *
         * Independent of, and safe alongside, InstallSlaveFreezeHandler() and
         * ongoing SRL::Slave::ExecuteOnSlave() use: that documented conflict is
         * specifically about the FRT Input Capture Interrupt vector (0x64),
         * which SGL's own slSlaveFunc dispatch also uses -- this hooks a
         * completely different vector (illegal instruction) that SGL's
         * dispatch has no reason to touch. Software breakpoints set via GDB's
         * normal 'Z0' packet are just a 0xFFFF memory patch and work
         * regardless of which CPU's code they land in; without this handler,
         * the slave executing one takes an unhandled exception on its
         * unconfigured boot-ROM default vector and never returns (see
         * "don't set breakpoints inside code that runs on the slave" in
         * Samples/Debug - GDB Stub/readme.md for the hardware-confirmed
         * symptom this replaces).
         *
         * @warning No single-step or step-over support for slave code: a
         * breakpoint hit here is one-shot (see slave_breakpoint_handler())
         * -- it's automatically removed the moment it's hit, so resuming
         * doesn't immediately re-fault on the same patched instruction. Set
         * it again with a fresh 'break'/'Z0' if you need it to fire again.
         * There is also no real multi-thread RSP support: a slave stop halts
         * the whole session the same way Ctrl-C does, and the slave's saved
         * state is inspected via the existing `monitor regs slave` / slave
         * pseudo-registers, not by switching GDB threads.
         *
         * @warning Hardware-confirmed: dispatching this function itself via
         * SRL::Slave::ExecuteOnSlave() is unreliable -- the function body
         * completes correctly on the slave (confirmed via
         * g_slave_handlers_installed reading true, and via the installed
         * handler subsequently catching real breakpoints correctly), but the
         * ITask's own `running` flag was observed to never clear on the
         * master side afterward, hanging any `while (task.IsRunning())` wait
         * around this specific dispatch indefinitely. The `if (vbr == 0)`
         * guard below was written assuming the slave's VBR is still at its
         * boot-ROM default the first time any GDBStub code runs there, same
         * as the master -- hardware testing found this false: the slave's
         * VBR reads as already relocated (~0x06000400, not 0) by the time
         * this runs, presumably by SGL's own slInitSystem()/dual-CPU setup,
         * so the guard is skipped and this patches vector 4 directly into
         * that already-live table instead of a fresh copy. That appears to
         * be what disrupts slSlaveFunc's own dispatch-completion signal back
         * to the master, even though it doesn't touch FRT-ICI. Callers must
         * bound their own wait around this dispatch (see main.cxx's
         * installExceptionTask usage) rather than waiting unconditionally.
         */
        static inline void InstallSlaveExceptionHandler() {
            uint32_t vbr = 0;
            asm volatile("stc vbr, %0" : "=r"(vbr));

            if (vbr == 0) {
                // Same relocation target as InstallSlaveFreezeHandler() (see its
                // comment) -- idempotent if both are ever installed on the same
                // slave: whichever runs first does the one-time copy, the second
                // just patches its own vector into the already-relocated table.
                vbr = 0x06010000U;
                volatile uint32_t* src_table = reinterpret_cast<volatile uint32_t*>(0x20000000U);
                volatile uint32_t* dst_table = reinterpret_cast<volatile uint32_t*>(vbr | 0x20000000U);
                for (int i = 0; i < 64; i++) {
                    dst_table[i] = src_table[i];
                }
                asm volatile("ldc %0, vbr" :: "r"(vbr));
            }

            volatile uint32_t* vbr_table = reinterpret_cast<volatile uint32_t*>(vbr | 0x20000000U);
            vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_slave_illegal_thunk);

            ForcePurgeCache();
            g_slave_handlers_installed = true;
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
        class InstallSlaveExceptionTask : public SRL::Types::ITask {
        protected:
            void Do() override {
                InstallSlaveExceptionHandler();
            }
        };

        /**
         * @brief Enter the GDB stub via software trap (Illegal Instruction).
         * 
         * IMPORTANT: The __attribute__((noinline)) is load-bearing. 
         * When this function executes, it triggers an exception via the 0xFFFF instruction.
         * The GDB stub's `adjust_pc_for_software_breakpoint()` handles this exception by 
         * advancing the program counter by 2 bytes (`g_ctx.pc += 2U`).
         * If this function were inlined, `g_ctx.pc += 2U` would incorrectly skip the actual user
         * instruction immediately following the `Break()` call. By forcing this to not be inlined,
         * the instruction following `0xFFFF` is this function's `rts` (Return from Subroutine).
         * The `+= 2U` safely skips the `0xFFFF` and lands on `rts`, returning to the caller.
         */
        __attribute__((noinline)) static void Break() {
            // Force a breakpoint exception.
            // Using Illegal Instruction (0xFFFF) which reliably vectors to VBR[4].
            // SGL frequently overwrites TRAPA vectors (32-63) causing them to be ignored.
            asm volatile(".word 0xFFFF" ::: "memory");
        }

        /**
         * @brief Compatibility alias matching the upstream libyaul GDB stub API.
         */
        inline void gdb_break() {
            Break();
        }

        /**
         * @brief Compatibility initializer matching the upstream gdbstub_t-based startup path.
         */
        inline void Init(gdbstub_t& gdbstub) {
            if (gdbstub.device != nullptr && gdbstub.device->init != nullptr) {
                gdbstub.device->init();
            }

            Init();
        }

        /**
         * @brief Check for incoming GDB interrupt request (Ctrl-C)
         */
        __attribute__((noinline)) inline void Poll() {
            // If we're called reentrantly while the CPU is already halted inside
            // process_commands() (e.g. VBlank firing while mid-conversation with
            // GDB after a breakpoint/Ctrl-C/NMI stop), skip our GDB-related work
            // entirely and let the interrupt handler's OTHER work (whatever
            // called us) proceed normally. See ReentrancyGuard's doc comment.
            // Any pending RX byte is simply left in the FIFO for a later, safe
            // (non-reentrant) Poll() call to pick up -- nothing is lost.
            if (g_in_process_commands) {
                return;
            }

            // Bridge a slave-side breakpoint (see InstallSlaveExceptionHandler())
            // into a normal master-side debug stop -- Poll() runs every VBlank
            // independent of whatever the master's own C++ code is doing (e.g.
            // stuck in a bounded wait for a slave job), so this is reached
            // promptly regardless. Reported as SIGINT, same as Ctrl-C/NMI:
            // there's no meaningful call stack to show on the master side for
            // an async event like this -- the interesting state is the slave's,
            // inspected via `monitor regs slave` / the slave pseudo-registers.
            if (g_slave_stopped) {
                g_is_ctrl_c_stop = true;
                Break();
            }

            const uint8_t usbFlags = SRL::DevCart::CS0::ReadFlags();
            g_last_usb_flags = usbFlags;
            g_devcart_port_available = SRL::DevCart::CS0::IsPortAvailable();


            const bool rxPending = (usbFlags & SRL::DevCart::CS0::USBFlags::Rxf) == 0;
            if (rxPending) {
                g_rx_ready_count = g_rx_ready_count + 1;

                if (!g_has_connection || !g_handshake_done) {
                    // During initial attach/handshake, do not consume RX bytes here.
                    // Let process_commands() read the full '$...#xx' packet intact.
                    snapshot_polling_context();
                    g_poll_fallback_count = g_poll_fallback_count + 1;
                    process_commands();
                } else {
                    // Active session while target runs: only Ctrl-C should interrupt.
                    // Read the FIFO byte directly rather than via
                    // SRL::DevCart::CS0::Read() -- that wrapper calls WaitRxf()
                    // with no poll limit (unconditional, no cable-unplug check),
                    // an entirely separate hang hazard from __gdb_wait_rx()'s
                    // (see its doc comment): if the cable is pulled between the
                    // rxPending check above and here, this would otherwise spin
                    // forever with zero escape. We already confirmed data is
                    // pending via rxPending, matching __gdb_getc()'s own
                    // direct-read pattern below.
                    const uint8_t ch = *(volatile uint8_t *)(SRL::DevCart::CS0::UsbFifo);
                    g_rx_detect_count = g_rx_detect_count + 1;

                    if (ch == 0x03U) {
                        record_command("<Ctrl-C>");
                        g_is_ctrl_c_stop = true;
                        g_poll_fallback_count = g_poll_fallback_count + 1;
                        Break();
                    } else if (ch == '$') {
                        // Preserve packet start byte and process packet without forcing a trap.
                        g_unget_char = '$';
                        snapshot_polling_context();
                        g_poll_fallback_count = g_poll_fallback_count + 1;
                        process_commands();
                    }
                }
            }

            // Hardware-level disconnect: clear session state.
            if (!SRL::DevCart::CS0::IsConnected()) {
                g_has_connection = false;
                g_handshake_done = false;
            }
        }
}
}

extern "C" __attribute__((used)) inline void slave_ipi_handler(void) {
    // Called by srl_gdbstub_slave_ici_thunk (below) on the slave SH-2, AFTER the
    // thunk has already snapshotted the slave's full register state into
    // SRL::GDBStub::g_slave_ctx. Disable further ICI firing while we are already
    // inside one — mirrors the disable/spin/re-enable shape used for master<->slave
    // ICI handlers elsewhere in Saturn homebrew (e.g. libyaul's cpu_dual) — and
    // clear the flag the doorbell write set.
    *reinterpret_cast<volatile uint8_t*>(SRL::GDBStub::FRT_TIER) &= ~SRL::GDBStub::FRT_ICF;
    *reinterpret_cast<volatile uint8_t*>(SRL::GDBStub::FRT_FTCSR) &= ~SRL::GDBStub::FRT_ICF;

    while (SRL::GDBStub::g_debug_pause) {
        asm volatile("nop");
    }

    // The master may have patched breakpoints into memory the slave executes;
    // purge the slave's own cache before resuming.
    SRL::GDBStub::ForcePurgeCache();

    *reinterpret_cast<volatile uint8_t*>(SRL::GDBStub::FRT_TIER) |= SRL::GDBStub::FRT_ICF;
    // Returning here lets srl_gdbstub_slave_ici_thunk restore registers and rte.
}

extern "C" __attribute__((used)) inline void slave_breakpoint_handler(void) {
    // Called by srl_gdbstub_slave_illegal_thunk (below) on the slave SH-2,
    // AFTER the thunk has already snapshotted the slave's full register
    // state into SRL::GDBStub::g_slave_ctx. See
    // SRL::GDBStub::InstallSlaveExceptionHandler()'s doc comment for the
    // overall design (this is the slave-side half of that mechanism).

    // Illegal Instruction pushes the address of the faulting instruction
    // itself (unlike TRAPA, which pushes PC+2) -- advance past it the same
    // way adjust_pc_for_software_breakpoint() does for the master.
    SRL::GDBStub::g_slave_ctx.pc += 2U;

    // One-shot: if this was a GDB-inserted breakpoint, remove it now so
    // resuming doesn't immediately re-fault on the same patched instruction
    // -- there's no step-over support for slave code. Re-add it with a
    // fresh 'break'/'Z0' if you need it to fire again.
    const uint32_t bp_addr = SRL::GDBStub::g_slave_ctx.pc - 2U;
    if (SRL::GDBStub::find_breakpoint_slot(bp_addr) >= 0) {
        SRL::GDBStub::remove_software_breakpoint(bp_addr);
    }

    SRL::GDBStub::g_slave_stopped = true;
    SRL::GDBStub::g_slave_resume = false;

    while (!SRL::GDBStub::g_slave_resume) {
        asm volatile("nop");
    }

    SRL::GDBStub::g_slave_stopped = false;

    // The master may have patched more breakpoints into memory while we were
    // halted; purge the slave's own cache before resuming (mirrors slave_ipi_handler).
    SRL::GDBStub::ForcePurgeCache();
    // Returning here lets srl_gdbstub_slave_illegal_thunk restore registers and rte.
}

__asm__(
    ".weak _srl_gdbstub_exception_thunk\n"
    ".global _srl_gdbstub_exception_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_exception_thunk:\n"
    "mov.l r0, @-r15\n"
    "stc.l gbr, @-r15\n"
    "mov.l 1f, r0\n"
    "mov.l r14, @(14*4, r0)\n"
    "mov.l r13, @(13*4, r0)\n"
    "mov.l r12, @(12*4, r0)\n"
    "mov.l r11, @(11*4, r0)\n"
    "mov.l r10, @(10*4, r0)\n"
    "mov.l r9,  @(9*4,  r0)\n"
    "mov.l r8,  @(8*4,  r0)\n"
    "mov.l r7,  @(7*4,  r0)\n"
    "mov.l r6,  @(6*4,  r0)\n"
    "mov.l r5,  @(5*4,  r0)\n"
    "mov.l r4,  @(4*4,  r0)\n"
    "mov.l r3,  @(3*4,  r0)\n"
    "mov.l r2,  @(2*4,  r0)\n"
    "mov.l r1,  @(1*4,  r0)\n"
    "mov r15, r1\n"
    "add #16, r1\n"
    "mov.l r1, @(15*4, r0)\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r2\n"
    "mov.l r2, @r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l r1, @(2*4, r2)\n"
    "mov.l @r15, r1\n"
    "mov.l r1, @r2\n"
    "mov.l @(4, r15), r1\n"
    "mov.l r1, @(24, r2)\n"
    "sts pr, r1\n"
    "mov.l r1, @(1*4, r2)\n"
    "stc vbr, r1\n"
    "mov.l r1, @(3*4, r2)\n"
    "sts mach, r1\n"
    "mov.l r1, @(4*4, r2)\n"
    "sts macl, r1\n"
    "mov.l r1, @(5*4, r2)\n"
    "mov.l 3f, r1\n"
    "mov.l @r1, r2\n"
    "add #1, r2\n"
    "mov.l r2, @r1\n"
    "mov.l 2f, r1\n"
    "jsr @r1\n"
    "nop\n"
    "mov.l 1f, r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l @(1*4, r2), r1\n"
    "lds r1, pr\n"
    "mov.l @(3*4, r2), r1\n"
    "ldc r1, vbr\n"
    "mov.l @(4*4, r2), r1\n"
    "lds r1, mach\n"
    "mov.l @(5*4, r2), r1\n"
    "lds r1, macl\n"
    "mov.l @(2*4, r2), r1\n"
    "ldc r1, gbr\n"
    "mov.l @(24, r2), r1\n"
    "mov.l r1, @(4, r15)\n"
    "mov.l @r2, r1\n"
    "mov.l r1, @r15\n"
    "mov.l @(14*4, r0), r14\n"
    "mov.l @(13*4, r0), r13\n"
    "mov.l @(12*4, r0), r12\n"
    "mov.l @(11*4, r0), r11\n"
    "mov.l @(10*4, r0), r10\n"
    "mov.l @(9*4,  r0), r9\n"
    "mov.l @(8*4,  r0), r8\n"
    "mov.l @(7*4,  r0), r7\n"
    "mov.l @(6*4,  r0), r6\n"
    "mov.l @(5*4,  r0), r5\n"
    "mov.l @(4*4,  r0), r4\n"
    "mov.l @(3*4,  r0), r3\n"
    "mov.l @(2*4,  r0), r2\n"
    "mov.l @(1*4,  r0), r1\n"
    // CRITICAL: r0 must be restored LAST, and this instruction assumes r0 
    // STILL holds the base pointer to g_ctx (loaded before the epilogue).
    // Do NOT reorder this or use r0 as a scratch register above!
    "mov.l @r0, r0\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_ctx\n"
    "2: .long srl_gdbstub_process_commands\n"
    "3: .long srl_gdbstub_thunk_count\n"
);

// Per-exception-family entry trampolines. The SH-2 has no on-chip "cause"
// register and every exception vector transfers control to a fixed address
// with no argument-passing convention, so the only way to tell GDB which
// exception family actually fired (illegal opcode vs misaligned access,
// rather than always reporting a generic SIGTRAP) is to give each family
// its own tiny entry stub that tags g_last_stop_signal before falling
// through into the real (shared, much larger) context-save thunk above.
//
// Each stub is careful to leave every register exactly as the CPU's own
// exception entry left it before reaching srl_gdbstub_exception_thunk: r0
// and r1 are the only registers touched, and both are saved to the stack
// and restored before the branch, so the shared thunk's own "mov.l r0,
// @-r15" (its first instruction, which captures the TRUE pre-exception r0
// into g_ctx) sees an untouched value. The temporary push/pop is balanced,
// so r15 is also back where the CPU left it by the time of the branch.
__asm__(
    ".weak _srl_gdbstub_illegal_thunk\n"
    ".global _srl_gdbstub_illegal_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_illegal_thunk:\n"
    "mov.l r0, @-r15\n"
    "mov.l r1, @-r15\n"
    "mov.l 1f, r0\n"
    "mov #4, r1\n"        // SIGILL
    "mov.b r1, @r0\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r0\n"
    "bra _srl_gdbstub_exception_thunk\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_last_stop_signal\n"
);

// NMI (the Saturn's physical Reset button) is asynchronous and, unlike the
// illegal-opcode/address-error families above, carries no "this address is
// definitely a fault" semantics -- it can land literally anywhere, including
// inside SGL/BIOS code with no debug info. It should be reported to GDB as
// SIGINT, exactly like Ctrl-C, so tag g_is_ctrl_c_stop (not g_last_stop_signal)
// before falling into the shared thunk -- see send_stop_signal's callers,
// which already prefer g_is_ctrl_c_stop over g_last_stop_signal, and the 'g'
// register-read handler, which already masks PR/R14 when g_is_ctrl_c_stop is
// set for exactly this "interrupted inside code with no unwind info" reason.
//
// Hardware-confirmed bug this debounces: the Reset button is a plain
// mechanical switch, and NMI is genuinely non-maskable -- unlike VBlank (see
// InterruptMaskGuard), nothing can block a second edge from a bouncing
// contact from re-entering this thunk while the first one is still being
// handled, including while already halted and mid-conversation with GDB.
// Symptom before this fix: pressing Reset once stopped cleanly in GDB, but
// Continue immediately re-broke (or hung) instead of resuming -- a queued
// bounce edge firing again before any real instruction could execute.
//
// Fix: a generation-counter debounce. Every edge bumps g_nmi_generation,
// remembers its own value, then busy-waits (long enough for mechanical
// bounce to settle). If a NEWER edge arrives during that wait, it re-enters
// this same thunk from the top as a nested exception -- safe, since nothing
// shared is touched before the wait completes. Only the edge that finds
// g_nmi_generation UNCHANGED after its own wait (i.e. no newer edge arrived)
// actually reports a stop to GDB via the shared thunk. Every earlier
// (bounced) edge, once its nested children finish and it resumes mid-loop,
// finds the generation has moved on and just rte's away transparently --
// never touching g_ctx, so there's no reentrancy corruption risk from an
// unbounded bounce train, only a single clean stop from the last edge.
__asm__(
    ".weak _srl_gdbstub_nmi_thunk\n"
    ".global _srl_gdbstub_nmi_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_nmi_thunk:\n"
    "mov.l r0, @-r15\n"
    "mov.l r1, @-r15\n"
    "mov.l r2, @-r15\n"
    "mov.l 6f, r0\n"
    "mov.l @r0, r1\n"
    "add #1, r1\n"
    "mov.l r1, @r0\n"
    "mov.l 1f, r0\n"
    "mov.l @r0, r1\n"
    "add #1, r1\n"
    "mov.l r1, @r0\n"
    "mov r1, r2\n"
    "mov.l 2f, r1\n"
    "3:\n"
    "dt r1\n"
    "bf 3b\n"
    "mov.l 1f, r0\n"
    "mov.l @r0, r1\n"
    "cmp/eq r1, r2\n"
    "bf 4f\n"
    "mov.l 7f, r0\n"
    "mov.l @r0, r1\n"
    "add #1, r1\n"
    "mov.l r1, @r0\n"
    "mov.l 5f, r0\n"
    "mov #1, r1\n"
    "mov.b r1, @r0\n"
    "mov.l @r15+, r2\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r0\n"
    "bra _srl_gdbstub_exception_thunk\n"
    "nop\n"
    "4:\n"
    "mov.l 8f, r0\n"
    "mov.l @r0, r1\n"
    "add #1, r1\n"
    "mov.l r1, @r0\n"
    "mov.l @r15+, r2\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r0\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_nmi_generation\n"
    "2: .long 300000\n"
    "5: .long srl_gdbstub_is_ctrl_c_stop\n"
    "6: .long srl_gdbstub_nmi_fire_count\n"
    "7: .long srl_gdbstub_nmi_report_count\n"
    "8: .long srl_gdbstub_nmi_swallow_count\n"
);

__asm__(
    ".weak _srl_gdbstub_addrerr_thunk\n"
    ".global _srl_gdbstub_addrerr_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_addrerr_thunk:\n"
    "mov.l r0, @-r15\n"
    "mov.l r1, @-r15\n"
    "mov.l 1f, r0\n"
    "mov #10, r1\n"       // SIGBUS
    "mov.b r1, @r0\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r0\n"
    "bra _srl_gdbstub_exception_thunk\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_last_stop_signal\n"
);

// Slave-side counterpart of the thunk above, installed by
// SRL::GDBStub::InstallSlaveFreezeHandler() onto the slave SH-2's own FRT-ICI
// vector (0x64). Byte-for-byte the same register save/restore sequence as
// _srl_gdbstub_exception_thunk — only the three referenced symbols differ:
// it snapshots into srl_gdbstub_slave_ctx, calls slave_ipi_handler (a plain
// spin-wait, not the RSP command processor), and counts into
// srl_gdbstub_slave_ici_count instead of srl_gdbstub_thunk_count.
__asm__(
    ".weak _srl_gdbstub_slave_ici_thunk\n"
    ".global _srl_gdbstub_slave_ici_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_slave_ici_thunk:\n"
    "mov.l r0, @-r15\n"
    "stc.l gbr, @-r15\n"
    "mov.l 1f, r0\n"
    "mov.l r14, @(14*4, r0)\n"
    "mov.l r13, @(13*4, r0)\n"
    "mov.l r12, @(12*4, r0)\n"
    "mov.l r11, @(11*4, r0)\n"
    "mov.l r10, @(10*4, r0)\n"
    "mov.l r9,  @(9*4,  r0)\n"
    "mov.l r8,  @(8*4,  r0)\n"
    "mov.l r7,  @(7*4,  r0)\n"
    "mov.l r6,  @(6*4,  r0)\n"
    "mov.l r5,  @(5*4,  r0)\n"
    "mov.l r4,  @(4*4,  r0)\n"
    "mov.l r3,  @(3*4,  r0)\n"
    "mov.l r2,  @(2*4,  r0)\n"
    "mov.l r1,  @(1*4,  r0)\n"
    "mov r15, r1\n"
    "add #16, r1\n"
    "mov.l r1, @(15*4, r0)\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r2\n"
    "mov.l r2, @r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l r1, @(2*4, r2)\n"
    "mov.l @r15, r1\n"
    "mov.l r1, @r2\n"
    "mov.l @(4, r15), r1\n"
    "mov.l r1, @(24, r2)\n"
    "sts pr, r1\n"
    "mov.l r1, @(1*4, r2)\n"
    "stc vbr, r1\n"
    "mov.l r1, @(3*4, r2)\n"
    "sts mach, r1\n"
    "mov.l r1, @(4*4, r2)\n"
    "sts macl, r1\n"
    "mov.l r1, @(5*4, r2)\n"
    "mov.l 3f, r1\n"
    "mov.l @r1, r2\n"
    "add #1, r2\n"
    "mov.l r2, @r1\n"
    "mov.l 2f, r1\n"
    "jsr @r1\n"
    "nop\n"
    "mov.l 1f, r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l @(1*4, r2), r1\n"
    "lds r1, pr\n"
    "mov.l @(3*4, r2), r1\n"
    "ldc r1, vbr\n"
    "mov.l @(4*4, r2), r1\n"
    "lds r1, mach\n"
    "mov.l @(5*4, r2), r1\n"
    "lds r1, macl\n"
    "mov.l @(2*4, r2), r1\n"
    "ldc r1, gbr\n"
    "mov.l @(24, r2), r1\n"
    "mov.l r1, @(4, r15)\n"
    "mov.l @r2, r1\n"
    "mov.l r1, @r15\n"
    "mov.l @(14*4, r0), r14\n"
    "mov.l @(13*4, r0), r13\n"
    "mov.l @(12*4, r0), r12\n"
    "mov.l @(11*4, r0), r11\n"
    "mov.l @(10*4, r0), r10\n"
    "mov.l @(9*4,  r0), r9\n"
    "mov.l @(8*4,  r0), r8\n"
    "mov.l @(7*4,  r0), r7\n"
    "mov.l @(6*4,  r0), r6\n"
    "mov.l @(5*4,  r0), r5\n"
    "mov.l @(4*4,  r0), r4\n"
    "mov.l @(3*4,  r0), r3\n"
    "mov.l @(2*4,  r0), r2\n"
    "mov.l @(1*4,  r0), r1\n"
    // CRITICAL: r0 must be restored LAST, and this instruction assumes r0
    // STILL holds the base pointer to g_slave_ctx (loaded before the epilogue).
    // Do NOT reorder this or use r0 as a scratch register above!
    "mov.l @r0, r0\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_slave_ctx\n"
    "2: .long _slave_ipi_handler\n"
    "3: .long srl_gdbstub_slave_ici_count\n"
);

// Slave-side breakpoint thunk, installed by
// SRL::GDBStub::InstallSlaveExceptionHandler() onto the slave SH-2's own
// Illegal Instruction vector (4) -- independent of, and installed alongside,
// the FRT-ICI thunk above if a project uses both. Byte-for-byte the same
// register save/restore sequence as _srl_gdbstub_exception_thunk /
// _srl_gdbstub_slave_ici_thunk -- only the three referenced symbols differ:
// it snapshots into srl_gdbstub_slave_ctx (same struct, same address),
// calls slave_breakpoint_handler (the slave's own halt/spin-wait/resume
// logic, not the master's RSP command processor -- the slave has no direct
// link to the debugger), and counts into srl_gdbstub_slave_bp_count.
__asm__(
    ".weak _srl_gdbstub_slave_illegal_thunk\n"
    ".global _srl_gdbstub_slave_illegal_thunk\n"
    ".align 2\n"
    "_srl_gdbstub_slave_illegal_thunk:\n"
    "mov.l r0, @-r15\n"
    "stc.l gbr, @-r15\n"
    "mov.l 1f, r0\n"
    "mov.l r14, @(14*4, r0)\n"
    "mov.l r13, @(13*4, r0)\n"
    "mov.l r12, @(12*4, r0)\n"
    "mov.l r11, @(11*4, r0)\n"
    "mov.l r10, @(10*4, r0)\n"
    "mov.l r9,  @(9*4,  r0)\n"
    "mov.l r8,  @(8*4,  r0)\n"
    "mov.l r7,  @(7*4,  r0)\n"
    "mov.l r6,  @(6*4,  r0)\n"
    "mov.l r5,  @(5*4,  r0)\n"
    "mov.l r4,  @(4*4,  r0)\n"
    "mov.l r3,  @(3*4,  r0)\n"
    "mov.l r2,  @(2*4,  r0)\n"
    "mov.l r1,  @(1*4,  r0)\n"
    "mov r15, r1\n"
    "add #16, r1\n"
    "mov.l r1, @(15*4, r0)\n"
    "mov.l @r15+, r1\n"
    "mov.l @r15+, r2\n"
    "mov.l r2, @r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l r1, @(2*4, r2)\n"
    "mov.l @r15, r1\n"
    "mov.l r1, @r2\n"
    "mov.l @(4, r15), r1\n"
    "mov.l r1, @(24, r2)\n"
    "sts pr, r1\n"
    "mov.l r1, @(1*4, r2)\n"
    "stc vbr, r1\n"
    "mov.l r1, @(3*4, r2)\n"
    "sts mach, r1\n"
    "mov.l r1, @(4*4, r2)\n"
    "sts macl, r1\n"
    "mov.l r1, @(5*4, r2)\n"
    "mov.l 3f, r1\n"
    "mov.l @r1, r2\n"
    "add #1, r2\n"
    "mov.l r2, @r1\n"
    "mov.l 2f, r1\n"
    "jsr @r1\n"
    "nop\n"
    "mov.l 1f, r0\n"
    "mov r0, r2\n"
    "add #64, r2\n"
    "mov.l @(1*4, r2), r1\n"
    "lds r1, pr\n"
    "mov.l @(3*4, r2), r1\n"
    "ldc r1, vbr\n"
    "mov.l @(4*4, r2), r1\n"
    "lds r1, mach\n"
    "mov.l @(5*4, r2), r1\n"
    "lds r1, macl\n"
    "mov.l @(2*4, r2), r1\n"
    "ldc r1, gbr\n"
    "mov.l @(24, r2), r1\n"
    "mov.l r1, @(4, r15)\n"
    "mov.l @r2, r1\n"
    "mov.l r1, @r15\n"
    "mov.l @(14*4, r0), r14\n"
    "mov.l @(13*4, r0), r13\n"
    "mov.l @(12*4, r0), r12\n"
    "mov.l @(11*4, r0), r11\n"
    "mov.l @(10*4, r0), r10\n"
    "mov.l @(9*4,  r0), r9\n"
    "mov.l @(8*4,  r0), r8\n"
    "mov.l @(7*4,  r0), r7\n"
    "mov.l @(6*4,  r0), r6\n"
    "mov.l @(5*4,  r0), r5\n"
    "mov.l @(4*4,  r0), r4\n"
    "mov.l @(3*4,  r0), r3\n"
    "mov.l @(2*4,  r0), r2\n"
    "mov.l @(1*4,  r0), r1\n"
    // CRITICAL: r0 must be restored LAST, and this instruction assumes r0
    // STILL holds the base pointer to g_slave_ctx (loaded before the epilogue).
    // Do NOT reorder this or use r0 as a scratch register above!
    "mov.l @r0, r0\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_slave_ctx\n"
    "2: .long _slave_breakpoint_handler\n"
    "3: .long srl_gdbstub_slave_bp_count\n"
);

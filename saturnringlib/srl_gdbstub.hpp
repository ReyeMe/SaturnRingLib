#pragma once

#include <srl_devcart.hpp>
#include <srl_interrupt.hpp>
#include <srl_system.hpp>
#include <cstdint>
#include <cstddef>

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

        // SRL break vector — using TRAPA #32 (Standard GDB SH breakpoint)
        static constexpr uint32_t BreakTrapNumber = 32;

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
        inline bool g_is_stepping = false;
        inline volatile bool g_has_connection = false;    // set on any valid RSP packet received
        inline volatile bool g_handshake_done = false;    // set only after qSupported exchange
        inline volatile uint32_t g_command_count = 0;
        __attribute__((used)) inline volatile uint32_t g_exception_thunk_count __asm__("srl_gdbstub_thunk_count") = 0;
        inline char g_last_command[64] = {};
        inline int g_unget_char = -1;
        inline bool g_handlers_installed = false;
        inline volatile uint32_t g_rx_detect_count = 0;  // incremented each time the stub reads a byte from DevCart RX
        inline volatile uint32_t g_rx_ready_count = 0;   // incremented each time Poll() sees RX data pending
        inline volatile uint32_t g_poll_fallback_count = 0; // incremented when Poll() handles RX without Trap3
        inline bool g_devcart_ready = false;
        inline bool g_devcart_port_available = false;
        inline bool g_devcart_usb_datapath_enabled = true;
        inline uint8_t g_last_usb_flags = 0xFF;
        inline volatile bool g_stop_requested_by_ctrl_c = false;
        inline volatile uint8_t g_last_stop_signal = 5; // 5=SIGTRAP, 2=SIGINT
        // Global pause flag used to freeze the slave SH‑2 while the master is in GDB.
        inline volatile uint32_t g_debug_pause = 0;
extern "C" void slave_ipi_handler(void);
        // Simple IPI register – adjust address if your hardware uses a different one.
        #define SLAVE_IPI_REG (*(volatile uint32_t*)0x1F0000U)
        #define SLAVE_IPI_SET()   (SLAVE_IPI_REG = 0x01U)
        #define SLAVE_IPI_CLEAR() (SLAVE_IPI_REG = 0x00U)

        static constexpr uint16_t SoftwareBreakInstruction = static_cast<uint16_t>(0xC300U | (BreakTrapNumber & 0xFFU));
        static constexpr size_t MaxSoftwareBreakpoints = 32;

        struct SoftwareBreakpoint {
            uint32_t address;
            uint16_t original_instruction;
            bool active;
        };

        inline SoftwareBreakpoint g_software_breakpoints[MaxSoftwareBreakpoints] = {};

        static constexpr uint32_t InterruptSetupMask = 0x0F;

        // --- Utility Functions ---

        static inline int hex(char ch) {
            if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            if (ch >= '0' && ch <= '9') return ch - '0';
            return -1;
        }

        static inline char hexchar(int v) {
            v &= 0xf;
            return v < 10 ? '0' + v : 'a' + v - 10;
        }

        static inline const char* hex2mem(const char* buf, uint8_t* mem, int count) {
            for (int i = 0; i < count; i++) {
                int h1 = hex(*buf++);
                int h2 = hex(*buf++);
                *mem++ = (h1 << 4) | h2;
            }
            return buf;
        }

        static inline char* mem2hex(const uint8_t* mem, char* buf, int count) {
            for (int i = 0; i < count; i++) {
                *buf++ = hexchar(*mem >> 4);
                *buf++ = hexchar(*mem % 16);
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

        static inline bool is_valid_memory_range(uint32_t addr, uint32_t length) {
            if (length == 0) {
                return true;
            }

            // Detect wrap-around in address arithmetic.
            const uint32_t end = addr + length - 1;
            if (end < addr) {
                return false;
            }

            // Prevent accesses into clearly invalid high address space that can fault and stall RSP.
            if (addr >= 0xF0000000U || end >= 0xF0000000U) {
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

        static inline void PurgeCache() {
            *reinterpret_cast<volatile uint8_t*>(0xFFFFFE92) |= 0x10;
            // The SH-2 hardware manual requires waiting at least two instructions
            // before accessing the cache after a purge. We add several NOPs to be safe.
            asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop" ::: "memory");
        }

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
            }
        }

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
            return true;
        }

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

        static inline void adjust_pc_for_software_breakpoint() {
            if (g_ctx.pc < 2U) {
                return;
            }

            const uint32_t trap_address = g_ctx.pc - 2U;
            if (find_breakpoint_slot(trap_address) >= 0) {
                g_ctx.pc = trap_address;
            }
        }

        static inline void snapshot_polling_context() {
            // Fallback context used when we service GDB packets outside ExceptionThunk.
            // It keeps PC/SP/special registers valid so GDB does not see a null frame.
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
        }

        // --- Transport (libyaul-style device hooks) ---

        // Idle timeout applied after handshake when no packet arrives.
        // MMIO reads (USB_FLAGS) have ~10 wait states on Saturn, so each loop
        // iteration takes ~1-2 us. 3,000,000 iterations ≈ 3-6 seconds.
        static constexpr uint32_t GDB_RX_IDLE_TIMEOUT = 3000000U;

        // Waits for USB RX data.
        // - Before first connection (g_has_connection=false): waits indefinitely
        //   so Break() before GDB attaches works correctly.
        // - After connection established: aborts on cable unplug (isConnected=false)
        //   or on prolonged silence (GDB process killed without sending D).
        // Returns true if data is available, false if session should be abandoned.
        static inline bool __gdb_wait_rx() {
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::isRXFEmpty()) {
                if (g_has_connection) {
                    // Abort immediately on cable unplug.
                    if (!SRL::DevCart::CS0::isConnected()) {
                        g_has_connection = false;
                        g_handshake_done = false;
                        return false;
                    }
                    // After handshake, apply idle timeout for dead GDB processes.
                    if (g_handshake_done) {
                        if (++idle > GDB_RX_IDLE_TIMEOUT) {
                            g_has_connection = false;
                            g_handshake_done = false;
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        // Waits for USB TX space.
        // Aborts on cable unplug if we had an active session.
        static inline bool __gdb_wait_tx() {
            while (SRL::DevCart::CS0::isTXEFull()) {
                if (g_has_connection && !SRL::DevCart::CS0::isConnected()) {
                    g_has_connection = false;
                    g_handshake_done = false;
                    return false;
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
            const uint8_t c = *(volatile uint8_t *)(SRL::DevCart::CS0::USB_FIFO);
            g_rx_detect_count = g_rx_detect_count + 1;
            return static_cast<int>(c);
        }

        static inline bool __gdb_putc(uint8_t value) {
            if (!__gdb_wait_tx()) {
                return false; // disconnected
            }
            *(volatile uint8_t *)(SRL::DevCart::CS0::USB_FIFO) = value;
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

                while (true) {
                    raw = __gdb_getc();
                    if (raw < 0) return false;
                    uint8_t ch = static_cast<uint8_t>(raw & 0x7F);
                    if (ch == '#') break;
                    csum += ch;
                    if (len + 1 < max_len) buffer[len++] = static_cast<char>(ch);
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

                if (csum != xmit_csum) {
                    uint8_t nack = '-';
                    __gdb_putc(nack);
                    continue;
                }

                uint8_t ack = '+';
                __gdb_putc(ack);

                // Strip sequence id prefix (XX:payload → payload).
                if (len >= 3 && buffer[2] == ':') {
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
            // Use T-packet with thread and swbreak so GDB knows it's a real
            // software-breakpoint stop and does NOT auto-continue.
            // Format: T<sig>swbreak:;thread:1;
            char buf[32];
            buf[0] = hexchar(signal >> 4);
            buf[1] = hexchar(signal & 0xF);
            buf[2] = 's'; buf[3] = 'w'; buf[4] = 'b'; buf[5] = 'r';
            buf[6] = 'e'; buf[7] = 'a'; buf[8] = 'k'; buf[9] = ':';
            buf[10] = ';'; buf[11] = 't'; buf[12] = 'h'; buf[13] = 'r';
            buf[14] = 'e'; buf[15] = 'a'; buf[16] = 'd'; buf[17] = ':';
            buf[18] = '1'; buf[19] = ';';
            g_last_stop_signal = signal;
            packet_put('T', buf, 20);
        }

        // --- Core Handler ---

        __attribute__((used)) inline void process_commands() __asm__("srl_gdbstub_process_commands");
        __attribute__((used)) inline void process_commands() {
            char in_buf[1024];
            char out_buf[1024];

            adjust_pc_for_software_breakpoint();

            // Drain any stale bytes that GDB sent before this trap fired.
            // Without this, GDB startup packets (including vCont;c) queued in
            // the FIFO while the Saturn was initialising would immediately resume
            // the target upon the very first Break().
            if (!g_has_connection) {
                while (!SRL::DevCart::CS0::isRXFEmpty()) {
                    (void)*(volatile uint8_t*)(SRL::DevCart::CS0::USB_FIFO);
                }
            } else if (g_handshake_done) {
                // If we are already connected and we just entered the trap handler
                // (e.g. hit a breakpoint or Ctrl-C), we MUST notify GDB proactively.
                send_stop_signal(g_stop_requested_by_ctrl_c ? 2U : g_last_stop_signal);
                g_stop_requested_by_ctrl_c = false;
            }

            // The stop reason (SIGTRAP or SIGINT) is preserved until '?' arrives.
            // Do not clear g_stop_requested_by_ctrl_c here (if not sent above) — the '?' handler reads it.

            while (true) {
                out_buf[0] = 0;
                if (!packet_get(in_buf, sizeof(in_buf))) {
                    // USB disconnected while waiting for a packet — stop processing.
                    return;
                }

                switch (in_buf[0]) {
                    case '?':
                        // First '?' marks the connection as active and sends the stop reason.
                        g_has_connection = true;
                        send_stop_signal(g_stop_requested_by_ctrl_c ? 2U : g_last_stop_signal);
                        g_stop_requested_by_ctrl_c = false;
                        break;
                    case 'q':
                        if (in_buf[1]=='S' && in_buf[2]=='u' && in_buf[3]=='p' &&
                            in_buf[4]=='p' && in_buf[5]=='o' && in_buf[6]=='r' &&
                            in_buf[7]=='t' && in_buf[8]=='e' && in_buf[9]=='d') {
                            // Advertise swbreak and target description so GDB knows the arch.
                            packet_put('\0', "PacketSize=400;swbreak+;qXfer:features:read+", 44);
                            g_handshake_done = true;
                        } else if (in_buf[1]=='X' && in_buf[2]=='f' && in_buf[3]=='e' &&
                                   in_buf[4]=='r' && in_buf[5]==':' && in_buf[6]=='f') {
                            // qXfer:features:read:target.xml:offset,length
                            // Full SH-2 register description so gdb-multiarch auto-detects
                            // architecture and register layout without needing 'set arch'.
                            static const char target_xml[] =
                                "<?xml version=\"1.0\"?>\n"
                                "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n"
                                "<target version=\"1.0\">\n"
                                "  <architecture>sh2</architecture>\n"
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
                                "    <reg name=\"r15\" bitsize=\"32\" type=\"data_ptr\"/>\n"
                                "    <reg name=\"pc\"  bitsize=\"32\" type=\"code_ptr\" regnum=\"16\"/>\n"
                                "    <reg name=\"pr\"  bitsize=\"32\" type=\"code_ptr\"/>\n"
                                "    <reg name=\"gbr\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"vbr\" bitsize=\"32\" type=\"code_ptr\"/>\n"
                                "    <reg name=\"mach\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"macl\" bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "    <reg name=\"sr\"  bitsize=\"32\" type=\"uint32\" format=\"hex\"/>\n"
                                "  </feature>\n"
                                "</target>\n";
                            // Send in chunks respecting the requested length from GDB.
                            // Parse offset and length from "qXfer:features:read:target.xml:off,len"
                            static const size_t xml_len = sizeof(target_xml) - 1U;
                            uint32_t xfer_off = 0, xfer_len = 0xFFFFU;
                            {
                                const char* colon = in_buf;
                                int colons = 0;
                                while (*colon && colons < 4) { if (*colon++ == ':') ++colons; }
                                // colon now points past the 4th ':', i.e. at "off,len"
                                while (*colon && *colon != ',') xfer_off = (xfer_off << 4) | hex(*colon++);
                                if (*colon == ',') { ++colon; while (*colon) xfer_len = (xfer_len == 0xFFFFU ? 0 : xfer_len) << 4 | hex(*colon++); }
                            }
                            if (xfer_off >= xml_len) {
                                out_buf[0] = 'l'; packet_put('\0', out_buf, 1);
                            } else {
                                size_t avail = xml_len - xfer_off;
                                size_t send  = avail < xfer_len ? avail : xfer_len;
                                if (send > 399U) send = 399U;
                                bool last = (xfer_off + send >= xml_len);
                                out_buf[0] = last ? 'l' : 'm';
                                for (size_t i = 0; i < send; ++i) out_buf[1 + i] = target_xml[xfer_off + i];
                                packet_put('\0', out_buf, 1 + send);
                            }
                        } else if (in_buf[1]=='f' && in_buf[2]=='T' && in_buf[3]=='h' &&
                                   in_buf[4]=='r' && in_buf[5]=='e' && in_buf[6]=='a' &&
                                   in_buf[7]=='d' && in_buf[8]=='I' && in_buf[9]=='n' &&
                                   in_buf[10]=='f' && in_buf[11]=='o' && in_buf[12]=='\0') {
                            // Single-thread target.
                            packet_put('\0', "m1", 2);
                        } else if (in_buf[1]=='s' && in_buf[2]=='T' && in_buf[3]=='h' &&
                                   in_buf[4]=='r' && in_buf[5]=='e' && in_buf[6]=='a' &&
                                   in_buf[7]=='d' && in_buf[8]=='I' && in_buf[9]=='n' &&
                                   in_buf[10]=='f' && in_buf[11]=='o' && in_buf[12]=='\0') {
                            packet_put('\0', "l", 1);
                        } else if (in_buf[1]=='A' && in_buf[2]=='t' && in_buf[3]=='t' &&
                                   in_buf[4]=='a' && in_buf[5]=='c' && in_buf[6]=='h' &&
                                   in_buf[7]=='e' && in_buf[8]=='d') {
                            packet_put('\0', "1", 1);
                        } else if (in_buf[1]=='O' && in_buf[2]=='f' && in_buf[3]=='f' &&
                                   in_buf[4]=='s' && in_buf[5]=='e' && in_buf[6]=='t' &&
                                   in_buf[7]=='s' && in_buf[8]=='\0') {
                            packet_put('\0', "Text=0;Data=0;Bss=0", 19);
                        } else if (in_buf[1]=='C' && in_buf[2]=='\0') {
                            packet_put('\0', "QC1", 3);
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
                            packet_put('\0', "vCont;c", 7);
                        } else if (starts_with(in_buf, "vCont;")) {
                            // Resume target.
                            PurgeCache();
                            return;
                        } else if (starts_with(in_buf, "vRun")) {
                            // Extended-remote run compatibility: treat like continue.
                            PurgeCache();
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
                            const int tx_len = static_cast<int>(mem2hex((uint8_t*)&g_ctx, out_buf, sizeof(SH2Context)) - out_buf);
                            packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                        }
                        break;
                    case 'G':
                        hex2mem(&in_buf[1], (uint8_t*)&g_ctx, sizeof(SH2Context));
                        packet_put('\0', "OK", 2);
                        break;
                    case 'p': // Read a single register
                        {
                            uint32_t reg_idx = 0;
                            const char* ptr = &in_buf[1];
                            if (!parse_hex_u32_until(ptr, '\0', reg_idx, ptr)) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx > 22) {
                                packet_put('\0', "E01", 3);
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

                            if (reg_idx > 22) {
                                packet_put('\0', "E01", 3);
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

                            hex2mem(ptr, reinterpret_cast<uint8_t*>(reg_ptr), 4);
                            packet_put('\0', "OK", 2);
                        }
                        break;
                    case 'm':
                        {
                            uint32_t addr = 0, length = 0;
                            const char* ptr = &in_buf[1];
                            while (*ptr && *ptr != ',') addr = (addr << 4) | hex(*ptr++);
                            if (*ptr == ',') ptr++;
                            while (*ptr) length = (length << 4) | hex(*ptr++);
                            // If the slave is paused, refuse memory reads to avoid USB FIFO overflow.
                            if (g_debug_pause) { packet_put('\0', "E22", 3); break; }
                            // Keep response within local buffer limits (hex encoding = 2x bytes + NUL).
                            if (length > 511U || !is_valid_memory_range(addr, length)) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            const int tx_len = static_cast<int>(mem2hex((uint8_t*)addr, out_buf, static_cast<int>(length)) - out_buf);
                            packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                        }
                        break;
                    case 'M':
                        {
                            uint32_t addr = 0, length = 0;
                            const char* p = &in_buf[1];
                            while (*p && *p != ',') addr = (addr << 4) | hex(*p++);
                            if (*p == ',') p++;
                            while (*p && *p != ':') length = (length << 4) | hex(*p++);
                            if (*p == ':') p++;

                            if (length > 511U || !is_valid_memory_range(addr, length)) {
                                packet_put('\0', "E02", 3);
                                break;
                            }

                            hex2mem(p, (uint8_t*)addr, length);
                            packet_put('\0', "OK", 2);
                        }
                        break;
                    case 'Z':
                    case 'z':
                        {
                            // RSP software breakpoints: Z0,addr,kind / z0,addr,kind
                            if (in_buf[1] != '0' || in_buf[2] != ',') {
                                packet_put('\0', nullptr, 0);
                                break;
                            }

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

                            // SH-2 instructions are 16-bit (kind normally 2).
                            if (kind != 0U && kind != 2U) {
                                packet_put('\0', "E03", 3);
                                break;
                            }

                            const bool ok = (in_buf[0] == 'Z')
                                ? install_software_breakpoint(addr)
                                : remove_software_breakpoint(addr);

                            packet_put('\0', ok ? "OK" : "E03", ok ? 2 : 3);
                        }
                        break;
                    case 'D': // Detach
                        packet_put('\0', "OK", 2);
                        clear_breakpoints(true);
                        g_handshake_done = false;
                        g_has_connection = false;
                        return;
                    case 'T': // Is thread alive?
                        // Report thread as alive for single-thread target.
                        packet_put('\0', "OK", 2);
                        break;
                    case 'c':
                        // Normal continue – first clear the slave‑pause flag so the slave can resume.
                        g_debug_pause = false;
                        // De‑assert the IPI request (write‑clear the register – exact mechanism depends on HW).
                        SLAVE_IPI_CLEAR();
                        PurgeCache();
                        return;
                    case 's':
                    case 'S':
                        // Explicitly block unsupported single-stepping commands with an error
                        // instead of an empty packet, to prevent GDB from hanging or crashing.
                        packet_put('\0', "E01", 3);
                        break;
                    default:
                        packet_put('\0', nullptr, 0);
                        break;
                }
            }
        }
        // --- Exception Handler ---

        extern "C" void srl_gdbstub_exception_thunk();

        inline uint32_t g_vbr_table_buffer[512] = {0};
        inline uint32_t* g_vbr_table_ptr = nullptr;

        static inline void InstallExceptionHandlers() {
            if (!g_handlers_installed) {
                // Force VBR to SGL's standard address to bypass the Boot ROM TRAPA wrapper
                uint32_t current_vbr = 0x06000000;
                asm volatile("ldc %0, vbr" :: "r"(current_vbr));
                
                // Write directly to the Cache-Through mirror of the VBR table
                volatile uint32_t* vbr_table = reinterpret_cast<volatile uint32_t*>(current_vbr | 0x20000000U);
                
                // ---------------------------------------------------------------
                // Install an additional IPI handler for the *slave* CPU.  The slave's
                // VBR lives at the same physical address (0x06000000) but each CPU
                // has its own VBR register, so writing the same vector table entry
                // also installs the handler for the slave.
                // Vector 0x100 (interrupt 0) is repurposed as a software‑IPI.
                // We point it at the function `slave_ipi_handler` (defined in
                // SlaveDebug.hpp) which simply spins while `g_debug_pause` is set.

                // Disabled IPI vector registration – out-of-range write caused crash.
                // vbr_table[0x40] = reinterpret_cast<uint32_t>(&slave_ipi_handler);
                // PurgeCache();

                // ---------------------------------------------------------------

                // Patch our exceptions
                // SH-2 exception vector layout from VBR:
                //   Fixed exceptions (reset, NMI, etc): VBR + 0x000..0x07C  (indices 0..31)
                //   External/internal interrupts:         VBR + 0x080..0x0FC  (indices 32..63)
                //   TRAPA #N vectors:                     VBR + N*4   (indices 32..63)
                vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // Illegal Instruction
                vbr_table[9] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // CPU Address Error
                vbr_table[10] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // DMA Address Error
                vbr_table[12] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // User Break Controller
                vbr_table[BreakTrapNumber] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // TRAPA #32

                PurgeCache();
                g_handlers_installed = true;
            }
        }

extern "C" void slave_ipi_handler(void) {
    // This handler runs on the slave SH‑2 when the master triggers an IPI.
    // It simply spins until the master clears the pause flag.
    while (g_debug_pause) {
        asm volatile("nop");
    }
    // Returning from the interrupt will resume the slave where it left off.
}

        // --- Public API ---

        /**
         * @brief Initialize the GDB stub and hook exception vectors.
         */
        static inline bool IsUsbDataPathEnabled();

        inline void Init() {
            g_has_connection = false;
            g_handshake_done = false;
            g_command_count = 0;
            g_exception_thunk_count = 0;
            g_rx_detect_count = 0;
            g_rx_ready_count = 0;
            g_poll_fallback_count = 0;
            g_last_command[0] = '\0';
            g_unget_char = -1;
            g_devcart_ready = SRL::DevCart::CS1::HasWascaSignature();
            g_devcart_port_available = SRL::DevCart::CS0::isPortAvailable();
            g_devcart_usb_datapath_enabled = IsUsbDataPathEnabled();
            g_last_usb_flags = SRL::DevCart::CS0::readFlags();
            g_stop_requested_by_ctrl_c = false;
            g_last_stop_signal = 5;
            clear_breakpoints(false);
            // Initialise the pause flag – false by default.
            g_debug_pause = false;

            if (!g_handlers_installed) {
                InstallExceptionHandlers();
            }
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
         * @brief Returns true when cartridge-level USB data path should be enabled.
         */
        inline bool IsUsbDataPathEnabled() {
            return g_devcart_usb_datapath_enabled;
        }

        /**
         * @brief Returns latest raw USB_FLAGS value sampled by the stub.
         */
        inline uint8_t GetLastUsbFlags() {
            return g_last_usb_flags;
        }

        /**
         * @brief Enter the GDB stub via software trap (Interrupt::Vector::Trap3).
         */
        static inline void Break() {
            asm volatile("trapa #32" ::: "memory");
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
        inline void Poll() {
            const uint8_t usbFlags = SRL::DevCart::CS0::readFlags();
            g_last_usb_flags = usbFlags;
            g_devcart_port_available = SRL::DevCart::CS0::isPortAvailable();
            g_devcart_usb_datapath_enabled = IsUsbDataPathEnabled();

            const bool rxPending = (usbFlags & SRL::DevCart::CS0::USBFlags::RXF) == 0;
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
                    const uint8_t ch = SRL::DevCart::CS0::read();
                    g_rx_detect_count = g_rx_detect_count + 1;

                    if (ch == 0x03U) {
                        record_command("<Ctrl-C>");
                        g_stop_requested_by_ctrl_c = true;
                        snapshot_polling_context();
                        g_poll_fallback_count = g_poll_fallback_count + 1;
                        process_commands();
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
            if (!SRL::DevCart::CS0::isConnected()) {
                g_has_connection = false;
                g_handshake_done = false;
            }
        }
    }
}

__asm__(
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
    "mov.l @r0, r0\n"
    "rte\n"
    "nop\n"
    ".align 4\n"
    "1: .long srl_gdbstub_ctx\n"
    "2: .long srl_gdbstub_process_commands\n"
    "3: .long srl_gdbstub_thunk_count\n"
);

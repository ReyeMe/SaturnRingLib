#pragma once

#include <srl_devcart.hpp>
#include <srl_interrupt.hpp>
#include <srl_system.hpp>
#include <srl_log.hpp>
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

        static inline void debug_write(char c) {
            if (SRL::DevCart::CS0::waitTXE(500U)) {
                *(volatile uint8_t *)(SRL::DevCart::CS0::USB_FIFO) = static_cast<uint8_t>(c);
            }
        }

        static inline void debug_print(const char* msg) {
            while (*msg) {
                debug_write(*msg++);
            }
        }

        static inline void debug_print_hex(uint32_t val) {
            for (int i = 7; i >= 0; i--) {
                uint32_t nibble = (val >> (i * 4)) & 0xF;
                debug_write(nibble < 10 ? '0' + nibble : 'A' + (nibble - 10));
            }
        }
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
extern "C" void slave_ipi_handler(void);
        // IPI scratch location in Work RAM High (safe for both CPUs, won't bus-error).
        // Using a word near the top of the 1MB Work RAM High region (0x06000000 + 0xFF000).
        // IPI scratch location in Work RAM High, safe for both CPUs.
        static inline volatile uint32_t* SlaveIPIReg() {
            return reinterpret_cast<volatile uint32_t*>(0x060FFF00U);
        }
        static inline void SlaveIPISet()   { *SlaveIPIReg() = 0x01U; }
        static inline void SlaveIPIClear() { *SlaveIPIReg() = 0x00U; }

        // We use Illegal Instruction (0xFFFF) by default for software breakpoints.
        // This avoids collisions with SGL which frequently overwrites TRAPA vectors (32-63)
        // for its own CD-ROM and BIOS system calls.
        static constexpr uint16_t SoftwareBreakInstruction = 0xFFFFU;
        static constexpr size_t MaxSoftwareBreakpoints = 32;
        static constexpr size_t kPacketDataMax = 399U;

        struct SoftwareBreakpoint {
            uint32_t address;
            uint16_t original_instruction;
            bool active;
        };

        inline SoftwareBreakpoint g_software_breakpoints[MaxSoftwareBreakpoints] = {};


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

        template<size_t val>
        struct PacketSizeString {
            char str[32] = "PacketSize=";
            size_t len = 11;
            constexpr PacketSizeString() {
                char temp[10] = {};
                size_t num_len = 0;
                size_t v = val;
                do {
                    temp[num_len++] = '0' + (v % 10);
                    v /= 10;
                } while (v > 0);
                for (size_t i = 0; i < num_len; ++i) {
                    str[len++] = temp[num_len - 1 - i];
                }
                str[len] = '\0';
            }
        };
        static constexpr PacketSizeString<kPacketDataMax + 1U> kPacketSizeStrBuilder{};

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

        static inline bool is_valid_memory_range(uint32_t addr, uint32_t length) {
            if (length == 0) {
                return true;
            }

            // Detect wrap-around in address arithmetic.
            const uint32_t end = addr + length - 1;
            if (end < addr) {
                return false;
            }

            // Prevent accesses that overlap with the invalid high address space that can fault and stall RSP.
            // Note: SH-2 on-chip peripherals reside at 0xFFFF8000 - 0xFFFFFFFF, which we must allow.
            // Two intervals [addr, end] and [0xF0000000, 0xFFFF7FFF] overlap if addr <= 0xFFFF7FFF and end >= 0xF0000000.
            if (addr < 0xFFFF8000U && end >= 0xF0000000U) {
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

        static inline void PurgeCache() {
            g_cache_dirty = true;
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
                    target_pc = pc;
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

        static inline void adjust_pc_for_software_breakpoint() {
            g_was_swbreak = false;
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
            if (find_breakpoint_slot(g_ctx.pc) >= 0 || (g_step_data.active && g_ctx.pc == g_step_data.address)) {
                g_was_swbreak = true;
                return; // PC is already exactly at the breakpoint.
            }

            if (g_ctx.pc < 2U) {
                return;
            }

            // Fallback: If we ever used TRAPA, the PC pushed is PC + 2.
            const uint32_t trap_address = g_ctx.pc - 2U;
            if (find_breakpoint_slot(trap_address) >= 0 || (g_step_data.active && trap_address == g_step_data.address)) {
                g_was_swbreak = true;
                g_ctx.pc = trap_address;
            }
        }

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
        }

        // --- Transport (libyaul-style device hooks) ---

        // Idle timeout applied after handshake when no packet arrives.
        // MMIO reads (USB_FLAGS) have ~10 wait states on Saturn, so each loop
        // iteration takes ~1-2 us. 3,000,000 iterations ≈ 3-6 seconds.
        static constexpr uint32_t GDB_RX_IDLE_TIMEOUT = 3000000U;
        static constexpr uint32_t GDB_TX_IDLE_TIMEOUT = 3000000U;

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
            uint32_t idle = 0;
            while (SRL::DevCart::CS0::isTXEFull()) {
                if (g_has_connection) {
                    if (!SRL::DevCart::CS0::isConnected()) {
                        g_has_connection = false;
                        g_handshake_done = false;
                        return false;
                    }
                    if (g_handshake_done) {
                        if (++idle > GDB_TX_IDLE_TIMEOUT) {
                            g_has_connection = false;
                            g_handshake_done = false;
                            return false;
                        }
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

        // --- Core Handler ---

        static constexpr uint32_t ExtraRegs[] = {
            // VDP1 (11 registers, indices 23..33)
            0x25D00000, 0x25D00002, 0x25D00004, 0x25D00006,
            0x25D00008, 0x25D0000A, 0x25D0000C, 0x25D0000E,
            0x25D00010, 0x25D00012, 0x25D00014,
            // VDP2 (5 registers, indices 34..38)
            0x25F80000, 0x25F80002, 0x25F80004, 0x25F80006,
            0x25F80020
        };
        static constexpr size_t NumExtraRegs = sizeof(ExtraRegs) / sizeof(ExtraRegs[0]);

        /**
         * @brief Prepares the CPU state for a GDB single-step command ('s' / 'vCont;s').
         * 
         * Places a temporary software breakpoint on the next sequential instruction
         * (or the branch target if the current instruction is a branch), ensuring
         * the stub catches execution immediately after one instruction.
         */
        static inline void handle_gdb_step() {
            do_software_step();
            PurgeCache();
        }

        /**
         * @brief Prepares the CPU state for a GDB continue command ('c' / 'vCont;c').
         * 
         * Restores the original instruction if the CPU is currently halted on a
         * software breakpoint, and sets up a silent step-over trap to re-insert
         * the breakpoint after the instruction executes.
         */
        static inline void handle_gdb_continue() {
            g_debug_pause = false;
            SlaveIPIClear();

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
                PurgeCache();
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
            // CacheFlusher guarantees a single cache purge on every exit path from this
            // function — continue, step, detach, disconnect, and early error returns alike.
            // This is intentional: any return that follows a memory patch (breakpoint install/
            // remove, M-packet write, or single-step trap placement) must have flushed before
            // the CPU resumes executing the patched region. On disconnect/detach the flush is
            // harmless. DO NOT remove this object or move it past the first PurgeCache() call.
            CacheFlusher flusher;
            constexpr size_t max_g_packet_size = (sizeof(SH2Context) + NumExtraRegs * 4) * 2;
            static_assert(max_g_packet_size < 1024, "out_buf is too small for GDB 'g' packet");
            char in_buf[1024];
            char out_buf[1024];

            debug_print("[GDBStub] process_commands() entered. PC: 0x");
            debug_print_hex(g_ctx.pc);
            debug_print("\n");

            adjust_pc_for_software_breakpoint();
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
                        if (starts_with(in_buf, "qSupported")) {
                            // Advertise swbreak and target description so GDB knows the arch.
                            // Dynamically insert the PacketSize to ensure it stays in sync with kPacketDataMax.
                            constexpr const char kFeaturesStr[] = ";swbreak+;qXfer:features:read+";
                            
                            static constexpr size_t max_qsupported_len = kPacketSizeStrBuilder.len + (sizeof(kFeaturesStr) - 1);
                            static_assert(max_qsupported_len < sizeof(out_buf), "qSupported payload exceeds buffer");

                            size_t out_len = 0;
                            for (size_t i = 0; i < kPacketSizeStrBuilder.len; ++i) {
                                if (out_len < sizeof(out_buf) - 1) out_buf[out_len++] = kPacketSizeStrBuilder.str[i];
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
                                "    <reg name=\"vdp1_tvmr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_fbcr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_ptmr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_ewdr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_ewlr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_ewrr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_endr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_edsr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_lopr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_copr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp1_modr\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp2_tvmd\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp2_exten\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp2_tvstat\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp2_vrsize\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
                                "    <reg name=\"vdp2_bgon\" bitsize=\"16\" type=\"uint16\" format=\"hex\" group=\"system\"/>\n"
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
                            for (const char* p = in_buf + 5; *p != '\0'; ) {
                                if (*p != ';') { ++p; continue; }
                                ++p; // skip ';'
                                if (*p != 's' && *p != 'S') {
                                    // Not a step action, skip to the next ';'
                                    while (*p != '\0' && *p != ';') { ++p; }
                                    continue;
                                }
                                const char* q = p + 1;
                                // No qualifier or end of string → applies to all
                                if (*q == '\0' || *q == ';') { has_step = true; break; }
                                // Thread qualifier present: :1 or :* are our thread
                                if (*q == ':') {
                                    ++q;
                                    if (*q == '*' || *q == '1') { has_step = true; break; }
                                }
                                // Qualified to another thread — skip this action
                                while (*p != '\0' && *p != ';') { ++p; }
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
                            char* p_out = out_buf;
                            p_out = mem2hex((uint8_t*)&g_ctx, p_out, sizeof(SH2Context));
                            
                            // Append extra pseudo-registers (VDP1/VDP2)
                            for (size_t i = 0; i < NumExtraRegs; ++i) {
                                // These are 16-bit hardware registers
                                uint16_t val = *(volatile uint16_t*)ExtraRegs[i];
                                // We swap manually or rely on memory order if big endian
                                // SH-2 is big endian, so memory order is correct for GDB.
                                p_out = mem2hex((uint8_t*)&val, p_out, 2);
                            }
                            
                            const int tx_len = static_cast<int>(p_out - out_buf);
                            packet_put('\0', out_buf, static_cast<size_t>(tx_len));
                        }
                        break;
                    case 'G':
                        {
                            // The G packet payload must contain at least the core register set.
                            // GDB may reflect back the full g response — which includes trailing
                            // hex chars for NumExtraRegs VDP registers — so we accept any payload
                            // >= core size and only write the first sizeof(SH2Context)*2 chars.
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

                            if (reg_idx > 22 + NumExtraRegs) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx >= 23) {
                                uint16_t val = *(volatile uint16_t*)ExtraRegs[reg_idx - 23];
                                const int tx_len = static_cast<int>(mem2hex((uint8_t*)&val, out_buf, 2) - out_buf);
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

                            if (reg_idx > 22 + NumExtraRegs) {
                                packet_put('\0', "E01", 3);
                                break;
                            }

                            if (reg_idx >= 23) {
                                uint16_t val = 0;
                                if (hex2mem(ptr, (uint8_t*)&val, 2)) {
                                    *(volatile uint16_t*)ExtraRegs[reg_idx - 23] = val;
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

                            if (hex2mem(p, (uint8_t*)addr, length)) {
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
                    case 'c': {
                        handle_gdb_continue();
                        return;
                    }
                    case 's':
                    case 'S':
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


        /**
         * @brief Hook the SH-2 CPU exception vectors for the GDB stub.
         * 
         * Relocates the Vector Base Register (VBR) from ROM to RAM if necessary,
         * and installs `srl_gdbstub_exception_thunk` as the handler for critical
         * CPU traps including Illegal Instruction, Address Errors, and NMI.
         */
        static inline void InstallExceptionHandlers() {
            debug_print("[GDBStub] InstallExceptionHandlers() start\n");
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
                vbr_table[4] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // Illegal Instruction
                vbr_table[5] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // Reserved Instruction
                vbr_table[6] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // Slot Illegal Instruction
                vbr_table[7] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // General Illegal Instruction
                vbr_table[8] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // Slot Reserved Instruction
                vbr_table[9] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk);  // CPU Address Error
                vbr_table[10] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // DMA Address Error
                // vbr_table[11] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // NMI (Reset button)
                vbr_table[12] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // User Break Controller
                vbr_table[35] = reinterpret_cast<uint32_t>(&srl_gdbstub_exception_thunk); // TRAPA #3 (Legacy/Fallback)

                ForcePurgeCache();
                g_handlers_installed = true;
            }
            debug_print("[GDBStub] InstallExceptionHandlers() end\n");
        }


        // --- Public API ---

        /**
         * @brief Initialize the GDB stub and hook exception vectors.
         */
        /**
         * @brief Returns true when cartridge-level USB data path should be enabled.
         */
        inline bool IsUsbDataPathEnabled() {
            return g_devcart_usb_datapath_enabled;
        }

        inline void Init() {
            debug_print("[GDBStub] Init() start\n");
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
            // isPortAvailable() only reads CS0 USB_FLAGS which is always safe.
            g_devcart_ready = SRL::DevCart::CS0::isPortAvailable(); // CS0-only check
            g_devcart_port_available = SRL::DevCart::CS0::isPortAvailable();
            g_devcart_usb_datapath_enabled = IsUsbDataPathEnabled();
            g_last_usb_flags = SRL::DevCart::CS0::readFlags();
            g_stop_requested_by_ctrl_c = false;
            g_last_stop_signal = 5;
            clear_breakpoints(false);
            // Initialise the pause flag – false by default.
            g_debug_pause = false;

            debug_print("[GDBStub] DevCart ready: ");
            debug_print(g_devcart_ready ? "1" : "0");
            debug_print(", Port: ");
            debug_print(g_devcart_port_available ? "1" : "0");
            debug_print(", USB Datapath: ");
            debug_print(g_devcart_usb_datapath_enabled ? "1" : "0");
            debug_print("\n");

            if (!g_handlers_installed) {
                InstallExceptionHandlers();
            }
            debug_print("[GDBStub] Init() end\n");
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
         * @brief Enter the GDB stub via software trap (Illegal Instruction).
         */
        static inline void Break() {
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
            if (!SRL::DevCart::CS0::isConnected()) {
                g_has_connection = false;
                g_handshake_done = false;
            }
        }
}
}

extern "C" void slave_ipi_handler(void) {
    // This handler runs on the slave SH‑2 when the master triggers an IPI.
    // It simply spins until the master clears the pause flag.
    while (SRL::GDBStub::g_debug_pause) {
        asm volatile("nop");
    }
    // Returning from the interrupt will resume the slave where it left off.
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

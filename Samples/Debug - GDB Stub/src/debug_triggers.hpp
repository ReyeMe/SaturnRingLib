#pragma once

#include <srl.hpp>

/**
 * @brief GDB-stub exercise triggers: deliberate SH-2 exceptions (one per
 * vector this sample's readme documents), a clean single-step target, a
 * watchpoint-friendly global, and the `monitor <text>` dispatcher that lets
 * any of these be fired headlessly over GDB's qRcmd channel instead of
 * requiring a human at the gamepad. The Slave-CPU dispatch smoke test lives
 * in its own compilation unit, slave_counter_task.hpp/.cxx.
 *
 * Split into its own compilation unit (separate from main.cxx and the
 * VDP1/VDP2 demo in vdp_demo.hpp/.cxx) specifically to prove the GDB
 * stub -- breakpoints, memory/register access, symbol resolution -- works
 * correctly across a multi-file build, not just when everything lives in
 * one translation unit.
 */

/**
 * @brief Exercises GDB memory writes ('M'/'P') and hardware watchpoints
 * ('Z2'/'Z3'/'Z4', i.e. GDB's `watch`/`rwatch`/`awatch`).
 *
 * From GDB: `watch g_testVariable` then `continue`, then press D-Pad Down
 * (or `monitor touch`) to trigger the watchpoint. Or confirm memory writes
 * land with `set variable g_testVariable = 99` followed by `p g_testVariable`.
 */
extern volatile int32_t g_testVariable;

/**
 * @brief Deliberately triggers an Illegal Instruction exception.
 *
 * Emits the SH-2 Illegal Instruction opcode (0xFFFF). The GDB stub's
 * exception thunk catches this and enters the RSP command loop, allowing
 * post-mortem inspection of the register state and call stack.
 */
[[noreturn]] void CrashProgram();

/**
 * @brief Vector 5 -- Reserved Instruction
 *
 * Opcodes in the reserved space (not the same encoding table as
 * "illegal"). 0xFFFD is one such reserved slot on SH-2.
 * PC pushed = address of the reserved word.
 */
void ReservedInstruction();

/**
 * @brief Vector 6 -- Slot Illegal Instruction
 *
 * An illegal opcode placed in the delay slot of a branch.
 * The branch itself (BRA here) is valid; the word after it is not.
 * PC pushed = address of the delay slot word.
 */
void SlotIllegalInstruction();

/**
 * @brief Vector 7 -- General Illegal Instruction
 *
 * Triggered by executing a privileged instruction (e.g. LDC SR)
 * from user mode, or certain other encoding violations.
 * On the Saturn the CPU is always in privileged mode, so the most
 * reliable way to hit this is a truly undefined secondary opcode.
 * 0xFFFC sits in a general-illegal slot on SH-2.
 */
void GeneralIllegalInstruction();

/**
 * @brief Vector 8 -- Slot Reserved Instruction
 *
 * A reserved opcode in the delay slot of a branch.
 * Same structure as vector 6 but uses a reserved (not illegal) word.
 */
void SlotReservedInstruction();

/**
 * @brief Vector 9 -- CPU Address Error
 *
 * SH-2 requires:  32-bit accesses aligned to 4 bytes
 *                 16-bit accesses aligned to 2 bytes
 * Misaligning either raises this exception.
 * The faulting address is latched in the TEA register (0xFFFFFFE4).
 *
 * @note Hardware-confirmed this does NOT actually fault on this
 * hardware/config -- SRL::GDBStub::GetExceptionThunkCount() stays at 0
 * after calling this, even though the misaligned volatile read is issued
 * exactly as written. Pre-existing, unrelated to the GDB stub packet
 * handling itself; kept here undisturbed pending further investigation.
 */
void CPUAddressError();

/**
 * @brief Vector 10 -- DMA Address Error
 *
 * Triggered when the DMAC is programmed with a source or destination
 * address that violates the transfer-width alignment rules.
 * We configure DMAC channel 0 to transfer a 32-bit word to/from an
 * address that is 1-byte misaligned, then enable it.
 * The DMAC raises the exception before any data moves.
 *
 * DMAC register base: 0xFFFF8000
 *   SAR0  = 0xFFFF8000  (source address)
 *   DAR0  = 0xFFFF8004  (destination address)
 *   TCR0  = 0xFFFF8008  (transfer count)
 *   CHCR0 = 0xFFFF800C  (channel control)
 *   DMAOR = 0xFFFF8040  (DMA operation register)
 */
void DMAAddressError();

/**
 * @brief Vector 12 -- User Break Controller (UBC)
 *
 * The SH-2 UBC is a hardware breakpoint unit with two channels (A/B).
 * We configure channel A to break on the very next instruction fetch
 * by setting the break address to the return address of this function.
 *
 * UBC registers:
 *   BARA  = 0xFFFFFF40  break address A
 *   BAMRA = 0xFFFFFF44  break address mask A (0 = exact match)
 *   BBRA  = 0xFFFFFF48  break bus cycle A
 *   BRCR  = 0xFFFFFF60  break control
 */
void UserBreakController();

/**
 * @brief Vector 35 -- TRAPA #3  (legacy/fallback software breakpoint)
 *
 * Executes the TRAPA instruction with immediate value 3.
 * The SH-2 pushes PC+2 and SR onto the stack and vectors through
 * VBR + 0x080 + (3 * 4) = VBR + 0x08C.
 * Note: PC pushed is the instruction AFTER the trapa, not the trapa
 * itself -- adjust_pc_for_software_breakpoint handles this via the
 * TRAPA fallback path (subtracting 2).
 */
void Trapa3();

/**
 * @brief A handful of simple, sequential statements with no branches or
 * varargs, meant as a clean target for exercising GDB's software single-step
 * ('s'/'vCont;s', i.e. `step`/`next`). The stub's do_software_step()
 * implements single-step by decoding the current instruction and placing a
 * temporary trap at the next one; this function gives an easy line-by-line
 * target to confirm that actually lands correctly.
 */
void SteppableFunction();

/**
 * @brief Dispatches a `monitor <text>` command received via GDB's qRcmd.
 *
 * This lets the same test paths triggered by gamepad buttons also be
 * triggered headlessly -- e.g. `(gdb) monitor crash illegal` -- so hardware
 * regression testing can be scripted instead of requiring a human at the
 * controller.
 */
void HandleMonitorCommand(const char* cmd);

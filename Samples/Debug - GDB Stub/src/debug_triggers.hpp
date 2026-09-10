#pragma once

#include <srl.hpp>

/**
 * @brief GDB-stub exercise triggers: deliberate SH-2 exceptions (one per
 * vector this sample's readme documents), a clean single-step target, a
 * watchpoint-friendly global, a Slave-CPU dispatch smoke test, and the
 * `monitor <text>` dispatcher that lets any of these be fired headlessly
 * over GDB's qRcmd channel instead of requiring a human at the gamepad.
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
 * @brief Task run a handful of times on the Slave SH-2 at startup, used to
 * exercise SRL::Slave::ExecuteOnSlave().
 *
 * @warning Hardware-confirmed: SRL::GDBStub::InstallSlaveFreezeHandler() does
 * NOT reliably work in a program that also uses SRL::Slave::ExecuteOnSlave(),
 * in either ordering. This file's own comments on InstallSlaveFreezeHandler()
 * called that "likely but unverified" -- real-hardware testing of this sample
 * confirmed it two different ways:
 *   1. Redispatching this task every frame (the original version of this
 *      sample): SRL::GDBStub::g_slave_ici_count -- which should tick up by
 *      exactly one per debug stop -- incremented only for the first stop or
 *      two after boot, then went permanently silent.
 *   2. Dispatching this task a fixed 5 times at startup ONLY, then installing
 *      the freeze handler last and never touching SRL::Slave again: this
 *      does NOT fix it either. g_slave_ici_count read exactly 5 (matching
 *      the dispatch count, not any debug stop) immediately after boot, then
 *      stayed at 5 across 8 further real Ctrl-C-triggered debug stops --
 *      it tracks past SRL::Slave activity, not live freeze pulses.
 *
 * The working theory: SGL's slSlaveFunc uses the slave's FRT input-capture
 * interrupt to wake the slave for each dispatched job, and appears to leave
 * the slave's on-chip TIER.ICIE (interrupt enable) bit disabled once it has
 * no more queued work. That bit lives in the slave's own private on-chip
 * peripheral space -- the master cannot poke it directly across the bus, and
 * the only sanctioned way to run code on the slave that could re-arm it is
 * SRL::Slave::ExecuteOnSlave() itself, which reopens the same conflict.
 * There is no ordering or dispatch-count workaround found so far; a real fix
 * would need either a from-scratch (non-SGL) slave wake-up path or a way to
 * safely coexist with SGL's own internal use of the interrupt.
 *
 * This task is kept here purely to demonstrate SRL::Slave::ExecuteOnSlave()
 * itself working (it does, reliably) -- see main.cxx for how it's dispatched.
 * SRL::GDBStub::InstallSlaveFreezeHandler() is intentionally NOT called by
 * this sample any more; see the readme for the full writeup.
 */
class SlaveCounterTask : public SRL::Types::ITask
{
public:
    SlaveCounterTask() : counter(0) {}

    uint32_t GetCounter() const
    {
        return this->counter;
    }

protected:
    void Do() override
    {
        this->counter = this->counter + 1;
    }

private:
    volatile uint32_t counter;
};

/**
 * @brief Dispatches a `monitor <text>` command received via GDB's qRcmd.
 *
 * This lets the same test paths triggered by gamepad buttons also be
 * triggered headlessly -- e.g. `(gdb) monitor crash illegal` -- so hardware
 * regression testing can be scripted instead of requiring a human at the
 * controller.
 */
void HandleMonitorCommand(const char* cmd);

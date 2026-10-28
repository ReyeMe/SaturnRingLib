#include "debug_triggers.hpp"

#include <srl_log.hpp>

using namespace SRL::Logger;

volatile int32_t g_testVariable = 0;

[[noreturn]] void CrashProgram()
{
    SRL::Debug::Print(1, 27, "*** CRASH TRIGGERED ***");
    SRL::Core::Synchronize(); // show the message before the CPU stops
    asm volatile(".word 0xFFFF" ::: "memory");
    __builtin_unreachable();
}

void ReservedInstruction()
{
    asm volatile(".word 0xFFFD" ::: "memory");
}

void SlotIllegalInstruction()
{
    asm volatile(
        "bra 1f\n\t"
        ".word 0xFFFF\n\t" // runs in the delay slot of the bra
        "1:\n\t" ::: "memory");
}

void GeneralIllegalInstruction()
{
    asm volatile(".word 0xFFFC" ::: "memory");
}

void SlotReservedInstruction()
{
    asm volatile(
        "bra 1f\n\t"
        ".word 0xFFFD\n\t" // runs in the delay slot of the bra
        "1:\n\t" ::: "memory");
}

void CPUAddressError()
{
    volatile uint8_t buf[8] = {};
    volatile uint32_t *misaligned = // buf + 1 is never 4-byte aligned
        reinterpret_cast<volatile uint32_t *>(&buf[1]);
    (void)*misaligned;
}

void DMAAddressError()
{
    // SH7604 DMAC channel 0 registers.
    volatile uint32_t *SAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF80U);
    volatile uint32_t *DAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF84U);
    volatile uint32_t *TCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF88U);
    volatile uint32_t *CHCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF8CU);
    volatile uint32_t *DMAOR = reinterpret_cast<volatile uint32_t *>(0xFFFFFFB0U);

    alignas(4) static uint8_t scratch[16] = {};

    // Status flags clear only on a write that follows a read.
    (void)*DMAOR;
    (void)*CHCR0;
    *CHCR0 = 0U; // stop the channel

    *SAR0 = reinterpret_cast<uint32_t>(&scratch[1]); // misaligned source
    *DAR0 = reinterpret_cast<uint32_t>(&scratch[8]); // aligned destination
    *TCR0 = 1U;                                      // transfer 1 unit
    *DMAOR = 0x00000001U;                            // DME=1: DMAC on
    // CHCR0: increment both addresses, 32-bit units, auto-request (start now), enable.
    *CHCR0 = 0x00005A01U; // the misaligned source raises vector 10 at once
}

// Writes UBC channel A directly, so it replaces any GDB `watch` or `hbreak` that uses channel A.
void UserBreakController()
{
    volatile uint32_t *BARA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF40U);
    volatile uint32_t *BAMRA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF44U);
    volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
    volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF78U);

    // PR holds the return address: the break fires there, back in the caller.
    uint32_t return_pc = 0;
    asm volatile("sts pr, %0" : "=r"(return_pc));

    *BARA = return_pc;
    *BAMRA = 0x0000U; // exact address match
    *BBRA = 0x0054U;  // CPU cycle, instruction fetch, read
    *BRCR = 0x1000U;  // SH7604 mode, break before the instruction runs
    asm volatile("nop" ::: "memory"); // let the BRCR write land before returning
}

void Trapa35()
{
    asm volatile("trapa #35" ::: "memory"); // TRAPA #N uses vector N; #3 would jump through the reset-SP vector
}

void SteppableFunction()
{
    int a = 1;
    int b = a + 1;
    int c = a + b;
    int d = c * 2;
    g_testVariable = d;
    SRL::Debug::Print(1, 29, "Steppable result: %d", d);
}

// Every instruction a stepi lands on has an sb_* label, so the test checks each landing by name.
// Unlabeled nops are paths that must not be taken. Delay slots flip T to prove they ran.
__asm__(
    ".pushsection .text\n"
    ".align 2\n"
    ".global _StepBranches\n"
    "_StepBranches:\n"
    "    sts.l   pr,@-r15\n"
    "    mov.l   .Lsb_jmp,r1\n"
    "    mov     #2,r2\n"              // braf: pc+4+2 = sb_braf_t
    "    mov     #4,r3\n"              // bsrf: pc+4+4 = sb_bsrf_t
    "    mov.l   .Lsb_jsr,r4\n"
    "    mov.l   .Lsb_rte,r5\n"
    "    sett\n"
    ".global _sb_start\n"
    "_sb_start:       bt      _sb_bt_taken\n"   // T=1: taken
    "                 nop\n"
    "_sb_bt_taken:    bf      _sb_fail\n"       // T=1: not taken
    "_sb_bf_not:      bt/s    _sb_bts_taken\n"  // T=1: taken
    "                 clrt\n"                   // slot: T=0
    "                 nop\n"
    "_sb_bts_taken:   bf/s    _sb_bfs_taken\n"  // T=0: taken
    "                 sett\n"                   // slot: T=1
    "                 nop\n"
    "_sb_bfs_taken:   bf/s    _sb_fail\n"       // T=1: not taken, the slot still runs
    "                 clrt\n"                   // slot: T=0
    "_sb_bfs_not:     bt/s    _sb_fail\n"       // T=0: not taken
    "                 nop\n"
    "_sb_bts_not:     bf      _sb_bf_taken\n"   // T=0: taken
    "                 nop\n"
    "_sb_bf_taken:    bt      _sb_fail\n"       // T=0: not taken
    "_sb_bt_not:      bra     _sb_bra_t\n"
    "                 nop\n"
    "                 nop\n"
    "_sb_bra_t:       bsr     _sb_bsr_t\n"
    "                 nop\n"
    "_sb_bsr_ret:     jmp     @r1\n"
    "                 nop\n"
    "                 nop\n"
    "_sb_jmp_t:       jsr     @r4\n"
    "                 nop\n"
    "_sb_jsr_ret:     braf    r2\n"
    "                 nop\n"
    "                 nop\n"
    "_sb_braf_t:      bsrf    r3\n"
    "                 nop\n"
    "_sb_bsrf_ret:    bra     _sb_rte_push_sr\n"
    "                 nop\n"
    "_sb_bsrf_t:      rts\n"
    "                 nop\n"
    "_sb_bsr_t:       rts\n"
    "                 nop\n"
    "_sb_jsr_t:       rts\n"
    "                 nop\n"
    "_sb_rte_push_sr: stc.l   sr,@-r15\n"       // RTE pops PC, then SR
    "_sb_rte_push_pc: mov.l   r5,@-r15\n"
    "_sb_rte:         rte\n"
    "                 nop\n"
    "                 nop\n"
    "_sb_rte_t:       lds.l   @r15+,pr\n"
    "                 rts\n"
    "                 nop\n"
    "_sb_fail:        bra     _sb_rte_t\n"
    "                 nop\n"
    ".align 2\n"
    ".Lsb_jmp:        .long   _sb_jmp_t\n"
    ".Lsb_jsr:        .long   _sb_jsr_t\n"
    ".Lsb_rte:        .long   _sb_rte_t\n"
    ".popsection\n");

void HandleMonitorCommand(const char* cmd)
{
    if (strcmp(cmd, "crash illegal") == 0) { CrashProgram(); }
    else if (strcmp(cmd, "crash addr") == 0) { CPUAddressError(); }
    else if (strcmp(cmd, "crash reserved") == 0) { ReservedInstruction(); }
    else if (strcmp(cmd, "crash slotillegal") == 0) { SlotIllegalInstruction(); }
    else if (strcmp(cmd, "crash slotreserved") == 0) { SlotReservedInstruction(); }
    else if (strcmp(cmd, "crash genillegal") == 0) { GeneralIllegalInstruction(); }
    else if (strcmp(cmd, "crash dma") == 0) { DMAAddressError(); }
    else if (strcmp(cmd, "crash ubc") == 0) { UserBreakController(); }
    else if (strcmp(cmd, "crash trapa35") == 0) { Trapa35(); }
    else if (strcmp(cmd, "step") == 0) { SteppableFunction(); }
    else if (strcmp(cmd, "stepbranches") == 0) { StepBranches(); }
    else if (strcmp(cmd, "touch") == 0) { g_testVariable = g_testVariable + 1; }
    else
    {
        // "regs slave" and "trace" are answered by the stub itself and never get here.
        Log::LogPrint("monitor: unknown command '%s' -- try crash illegal|addr|reserved|"
            "slotillegal|slotreserved|genillegal|dma|ubc|trapa35, step, stepbranches, touch, "
            "regs slave, or trace", cmd);
    }
}

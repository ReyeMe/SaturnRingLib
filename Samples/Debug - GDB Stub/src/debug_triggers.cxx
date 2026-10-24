#include "debug_triggers.hpp"

#include <srl_log.hpp>

using namespace SRL::Logger;

volatile int32_t g_testVariable = 0;

/**
 * @brief Compares two NUL-terminated strings for exact equality.
 */
static bool StrEquals(const char* a, const char* b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (*a != *b) { return false; }
        ++a;
        ++b;
    }

    return *a == *b;
}

[[noreturn]] void CrashProgram()
{
    SRL::Debug::Print(1, 27, "*** CRASH TRIGGERED ***");
    SRL::Core::Synchronize();
    // Emit 0xFFFF — the SH-2 Illegal Instruction opcode.
    // The GDB stub's exception thunk catches this and enters the RSP command loop.
    asm volatile(".word 0xFFFF" ::: "memory");
    // Tells the compiler that execution will never pass this point, preventing
    // it from generating a function epilogue or expecting a return value.
    __builtin_unreachable();
}

void ReservedInstruction()
{
    asm volatile(".word 0xFFFD" ::: "memory");
}

void SlotIllegalInstruction()
{
    asm volatile(
        "bra 1f\n\t"       // branch with delay slot
        ".word 0xFFFF\n\t" // illegal instruction IN the delay slot → vector 6
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
        ".word 0xFFFD\n\t" // reserved instruction in delay slot → vector 8
        "1:\n\t" ::: "memory");
}

void CPUAddressError()
{
    volatile uint8_t buf[8] = {};
    // Offset by 1 guarantees the pointer is never 4-byte aligned.
    volatile uint32_t *misaligned =
        reinterpret_cast<volatile uint32_t *>(&buf[1]);
    (void)*misaligned;
}

void DMAAddressError()
{
    // SH7604 DMAC channel 0 (same addresses as SGL's modules/sgl/IP/smpsys.c).
    volatile uint32_t *SAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF80U);
    volatile uint32_t *DAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF84U);
    volatile uint32_t *TCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF88U);
    volatile uint32_t *CHCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFFFF8CU);
    volatile uint32_t *DMAOR = reinterpret_cast<volatile uint32_t *>(0xFFFFFFB0U);

    // Scratch buffer in Work RAM — the destination side is kept valid;
    // only the source is misaligned to guarantee the address error.
    alignas(4) static uint8_t scratch[16] = {};

    (void)*DMAOR; // reading first lets the DMAOR write below clear a stale AE/NMIF flag
    (void)*CHCR0;
    *CHCR0 = 0U; // stop the channel; reading first lets the write clear a stale TE flag

    *SAR0 = reinterpret_cast<uint32_t>(&scratch[1]); // misaligned source
    *DAR0 = reinterpret_cast<uint32_t>(&scratch[8]); // aligned destination
    *TCR0 = 1U;                                      // transfer 1 unit
    *DMAOR = 0x00000001U;                            // DME=1: DMAC on
    // CHCR0: DM=01 (DAR increment), SM=01 (SAR increment), TS=10 (32-bit),
    //        AR=1 (auto-request: start now, no external DREQ), IE=0, TE=0, DE=1 (enable channel)
    *CHCR0 = 0x00005A01U;
    // The DMAC detects the misaligned SAR immediately and fires vector 10.
}

// @warning Programs the UBC (channel A) directly instead of going through
// SRL::GDBStub's own install_hardware_watchpoint()/remove_hardware_watchpoint() --
// there is only one physical channel, so calling this while a GDB `watch`/hardware
// breakpoint is active silently steals it out from under GDB (see
// g_ubc_channel_a_active's doc comment in srl_gdbstub.hpp). Fine for this sample's
// own one-shot test trigger in isolation; avoid combining with GDB-side watchpoints.
void UserBreakController()
{
    volatile uint32_t *BARA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF40U);
    volatile uint16_t *BAMRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF44U);
    volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
    volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF78U);

    // Capture the return address: whatever called this function will
    // be the first instruction executed after we re-enable the CPU.
    uint32_t return_pc = 0;
    asm volatile("sts pr, %0" : "=r"(return_pc));

    *BARA = return_pc; // break exactly at the return site
    *BAMRA = 0x0000U;  // no address masking — exact match
    // BBRA: CPU cycles (bits 7-6 = 01), instruction fetch (5-4 = 01), read (3-2 = 01); a 00 field never breaks
    *BBRA = 0x0054U;
    // BRCR: independent channels, break before the instruction runs
    *BRCR = 0x0000U;

    // The break fires on the instruction fetch at return_pc,
    // i.e. the first instruction the caller executes after this returns.
    asm volatile("nop" ::: "memory"); // ensure BRCR write is committed
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

void HandleMonitorCommand(const char* cmd)
{
    if (StrEquals(cmd, "crash illegal")) { CrashProgram(); }
    else if (StrEquals(cmd, "crash addr")) { CPUAddressError(); }
    else if (StrEquals(cmd, "crash reserved")) { ReservedInstruction(); }
    else if (StrEquals(cmd, "crash slotillegal")) { SlotIllegalInstruction(); }
    else if (StrEquals(cmd, "crash slotreserved")) { SlotReservedInstruction(); }
    else if (StrEquals(cmd, "crash genillegal")) { GeneralIllegalInstruction(); }
    else if (StrEquals(cmd, "crash dma")) { DMAAddressError(); }
    else if (StrEquals(cmd, "crash ubc")) { UserBreakController(); }
    else if (StrEquals(cmd, "crash trapa35")) { Trapa35(); }
    else if (StrEquals(cmd, "step")) { SteppableFunction(); }
    else if (StrEquals(cmd, "touch")) { g_testVariable = g_testVariable + 1; }
    else
    {
        // "regs slave" and "trace" aren't handled here -- they're
        // intercepted directly in srl_gdbstub.hpp's qRcmd handler (see
        // send_slave_regs_dump()/send_halt_trace_dump())
        // and never reach this dispatcher.
        Log::LogPrint("monitor: unknown command '%s' -- try crash illegal|addr|reserved|"
            "slotillegal|slotreserved|genillegal|dma|ubc|trapa35, step, touch, "
            "regs slave, or trace", cmd);
    }
}

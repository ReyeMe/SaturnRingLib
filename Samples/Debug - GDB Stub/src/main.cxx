#include <srl.hpp>
#include <srl_log.hpp>     // Logging systemcess
#include <srl_input.hpp>   // Gamepad input

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::DevCart;

/**
 * @brief Deliberately triggers an Illegal Instruction exception.
 *
 * Emits the SH-2 Illegal Instruction opcode (0xFFFF). The GDB stub's
 * exception thunk catches this and enters the RSP command loop, allowing
 * post-mortem inspection of the register state and call stack.
 */
[[noreturn]] static void CrashProgram()
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

/**
 * @brief Deliberately triggers a CPU Address Error.
 *
 * SH-2 requires 32-bit accesses to be 4-byte aligned and 16-bit accesses
 * to be 2-byte aligned. Violating this raises a CPU Address Error exception
 * (vector 9), which the GDB stub routes to its exception thunk.
 */
static inline void TriggerAlignmentCrash()
{
    // SH-2 requires 32-bit accesses to be 4-byte aligned and
    // 16-bit accesses to be 2-byte aligned. Violating this raises
    // a CPU Address Error exception (vector 9), which your stub
    // already routes to srl_gdbstub_exception_thunk.
    volatile uint8_t buf[8] = {};
    // Take a byte pointer and offset it by 1 so it is never 4-byte aligned.
    volatile uint32_t *misaligned = reinterpret_cast<volatile uint32_t *>(&buf[1]);
    (void)*misaligned; // read from misaligned address → CPU Address Error
}

/**
 * @brief Vector 5 — Reserved Instruction
 * 
 * Opcodes in the reserved space (not the same encoding table as
 * "illegal"). 0xFFFD is one such reserved slot on SH-2.
 * PC pushed = address of the reserved word.
 */
static inline void ReservedInstruction()
{
    asm volatile(".word 0xFFFD" ::: "memory");
}

/**
 * @brief Vector 6 — Slot Illegal Instruction
 * 
 * An illegal opcode placed in the delay slot of a branch.
 * The branch itself (BRA here) is valid; the word after it is not.
 * PC pushed = address of the delay slot word.
 */
static inline void SlotIllegalInstruction()
{
    asm volatile(
        "bra 1f\n\t"       // branch with delay slot
        ".word 0xFFFF\n\t" // illegal instruction IN the delay slot → vector 6
        "1:\n\t" ::: "memory");
}

/**
 * @brief Vector 7 — General Illegal Instruction
 * 
 * Triggered by executing a privileged instruction (e.g. LDC SR)
 * from user mode, or certain other encoding violations.
 * On the Saturn the CPU is always in privileged mode, so the most
 * reliable way to hit this is a truly undefined secondary opcode.
 * 0xFFFC sits in a general-illegal slot on SH-2.
 */
static inline void GeneralIllegalInstruction()
{
    asm volatile(".word 0xFFFC" ::: "memory");
}

/**
 * @brief Vector 8 — Slot Reserved Instruction
 * 
 * A reserved opcode in the delay slot of a branch.
 * Same structure as vector 6 but uses a reserved (not illegal) word.
 */
static inline void SlotReservedInstruction()
{
    asm volatile(
        "bra 1f\n\t"
        ".word 0xFFFD\n\t" // reserved instruction in delay slot → vector 8
        "1:\n\t" ::: "memory");
}

/**
 * @brief Vector 9 — CPU Address Error
 * 
 * SH-2 requires:  32-bit accesses aligned to 4 bytes
 *                 16-bit accesses aligned to 2 bytes
 * Misaligning either raises this exception.
 * The faulting address is latched in the TEA register (0xFFFFFFE4).
 */
static inline void CPUAddressError()
{
    volatile uint8_t buf[8] = {};
    // Offset by 1 guarantees the pointer is never 4-byte aligned.
    volatile uint32_t *misaligned =
        reinterpret_cast<volatile uint32_t *>(&buf[1]);
    (void)*misaligned;
}

/**
 * @brief Vector 10 — DMA Address Error
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
static inline void DMAAddressError()
{
    volatile uint32_t *SAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFF8000U);
    volatile uint32_t *DAR0 = reinterpret_cast<volatile uint32_t *>(0xFFFF8004U);
    volatile uint32_t *TCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFF8008U);
    volatile uint32_t *CHCR0 = reinterpret_cast<volatile uint32_t *>(0xFFFF800CU);
    volatile uint32_t *DMAOR = reinterpret_cast<volatile uint32_t *>(0xFFFF8040U);

    // Scratch buffer in Work RAM — the destination side is kept valid;
    // only the source is misaligned to guarantee the address error.
    static uint8_t scratch[16] = {};

    *SAR0 = reinterpret_cast<uint32_t>(&scratch[1]); // misaligned source
    *DAR0 = reinterpret_cast<uint32_t>(&scratch[8]); // aligned destination
    *TCR0 = 1U;                                      // transfer 1 unit
    // CHCR0: TS=2 (32-bit), DM=01 (DAR increment), SM=01 (SAR increment),
    //        IE=0, TE=0, DE=1 (enable channel)
    *CHCR0 = 0x00000401U;
    // DMAOR: enable DMA master
    *DMAOR = 0x00000001U;
    // The DMAC detects the misaligned SAR immediately and fires vector 10.
}

/**
 * @brief Vector 12 — User Break Controller (UBC)
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
static inline void UserBreakController()
{
    volatile uint32_t *BARA = reinterpret_cast<volatile uint32_t *>(0xFFFFFF40U);
    volatile uint16_t *BAMRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF44U);
    volatile uint16_t *BBRA = reinterpret_cast<volatile uint16_t *>(0xFFFFFF48U);
    volatile uint16_t *BRCR = reinterpret_cast<volatile uint16_t *>(0xFFFFFF60U);

    // Capture the return address: whatever called this function will
    // be the first instruction executed after we re-enable the CPU.
    uint32_t return_pc = 0;
    asm volatile("sts pr, %0" : "=r"(return_pc));

    *BARA = return_pc; // break exactly at the return site
    *BAMRA = 0x0000U;  // no address masking — exact match
    // BBRA: CPFETCH=1 (instruction fetch cycle), no data cycle
    *BBRA = 0x0010U;
    // BRCR: UBDE=1 (enable UBC), CMFAi=0 (no interrupt masking)
    *BRCR = 0x0001U;

    // The break fires on the instruction fetch at return_pc,
    // i.e. the first instruction the caller executes after this returns.
    asm volatile("nop" ::: "memory"); // ensure BRCR write is committed
}

/**
 * @brief Vector 35 — TRAPA #3  (legacy/fallback software breakpoint)
 * 
 * Executes the TRAPA instruction with immediate value 3.
 * The SH-2 pushes PC+2 and SR onto the stack and vectors through
 * VBR + 0x080 + (3 * 4) = VBR + 0x08C.
 * Note: PC pushed is the instruction AFTER the trapa, not the trapa
 * itself — adjust_pc_for_software_breakpoint handles this via the
 * TRAPA fallback path (subtracting 2).
 */
static inline void Trapa3()
{
    asm volatile("trapa #3" ::: "memory");
}

/**
 * @brief Main program entry point.
 *
 * Initializes the SaturnRingLib core, the GDB stub, and polls continuously
 * for USB gamepad inputs and GDB commands.
 *
 * @return Returns 0 on standard completion (though typically loops forever).
 */
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);
    Log::LogPrint("Before GDBStub::Init");

    Log::LogPrint("After GDBStub::Init");

    SRL::Debug::Print(1, 1, "GDB Stub Sample");
    SRL::Debug::Print(1, 2, "Stub active - connect GDB to break");
    SRL::Debug::Print(1, 4, "Press:");
    SRL::Debug::Print(2, 5, "B: Illegal Inst.");
    SRL::Debug::Print(2, 6, "A: CPU Addr Err");
    SRL::Debug::Print(2, 7, "C: Reserved Inst.");
    SRL::Debug::Print(2, 8, "X: Slot Illegal");
    SRL::Debug::Print(2, 9, "Y: Slot Reserved");
    SRL::Debug::Print(22, 5, "Z: Gen. Illegal");
    SRL::Debug::Print(22, 6, "L: DMA Addr Err");
    SRL::Debug::Print(22, 7, "R: UBC Break");
    SRL::Debug::Print(22, 8, "START: TRAPA 3");
    Log::LogPrint("GDB Stub active, waiting for GDB connection via Poll()");

    SRL::Core::Synchronize();
    // NOTE: Break() issues trapa #32 which blocks the Saturn in the RSP command loop
    // waiting for a GDB client. Only call it when GDB is already connected.
    // SRL::GDBStub::Break();

    int counter = 0;
    SRL::Input::Digital gamepad(0);

    while (true)
    {
        SRL::Debug::Print(1, 11, "Loop counter: %d", counter++);
        const bool usbConnected = SRL::DevCart::CS0::isConnected();
        const bool gdbConnected = SRL::GDBStub::IsConnected();

        SRL::Debug::Print(1, 13, "USB link: %s", usbConnected ? "connected" : "disconnected");
        SRL::Debug::Print(1, 14, "GDB session: %s", gdbConnected ? "active" : "waiting");
        SRL::Debug::Print(1, 15, "GDB handlers: %s", SRL::GDBStub::IsHandlersInstalled() ? "installed" : "pending");
        SRL::Debug::Print(1, 16, "GDB thunk count: %lu", static_cast<unsigned long>(SRL::GDBStub::GetExceptionThunkCount()));
        SRL::Debug::Print(1, 17, "GDB RX bytes:    %lu", static_cast<unsigned long>(SRL::GDBStub::GetRxDetectCount()));
        SRL::Debug::Print(1, 18, "GDB RX ready:    %lu", static_cast<unsigned long>(SRL::GDBStub::GetRxReadyCount()));
        SRL::Debug::Print(1, 19, "GDB cmd count:   %lu", static_cast<unsigned long>(SRL::GDBStub::g_command_count));
        SRL::Debug::Print(1, 20, "Last GDB cmd: %.45s", SRL::GDBStub::g_last_command[0] ? SRL::GDBStub::g_last_command : "<none>");
        SRL::Debug::Print(1, 21, "DevCart probe:   %s", SRL::GDBStub::IsDevCartReady() ? "ok" : "failed");
        SRL::Debug::Print(1, 22, "Port avail:      %s", SRL::GDBStub::IsDevCartPortAvailable() ? "yes" : "no");
        SRL::Debug::Print(1, 23, "USB_FLAGS:       0x%02X", static_cast<unsigned int>(SRL::GDBStub::GetLastUsbFlags()));
        SRL::Debug::Print(1, 24, "Poll fallback:   %lu", static_cast<unsigned long>(SRL::GDBStub::GetPollFallbackCount()));

        if (gamepad.WasPressed(SRL::Input::Digital::Button::B)) { CrashProgram(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::A)) { CPUAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::C)) { ReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::X)) { SlotIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Y)) { SlotReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Z)) { GeneralIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::L)) { DMAAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::R)) { UserBreakController(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::START)) { Trapa3(); }

        SRL::Core::Synchronize();
    }

    return 0;
}

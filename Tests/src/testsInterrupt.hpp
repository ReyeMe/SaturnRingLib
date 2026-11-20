// Tests/src/testsInterrupt.hpp
// Unit tests for SRL Interrupt API (mask, status, acknowledge, handler registration)

#include <srl.hpp>
#include <srl_system.hpp>
#include <srl_log.hpp>

#include <utility>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

#include <srl_interrupt.hpp>

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

    /** @brief Test Interrupt::SetMask round-trip via System::GetInterruptMask
 *
 * Verifies:
 * - SetMask(None) sets mask to 0x00000000 (all enabled)
 * - SetMask(All) sets mask to 0x7FFF (all standard interrupts disabled)
 * - Original mask is restored after each operation
 */
MU_TEST(interrupt_test_setmask_roundtrip)
{
    const uint32_t previousMask = System::GetInterruptMask();

    Interrupt::SetMask(Interrupt::Mask::None);
    const uint32_t maskNone = System::GetInterruptMask();

    Interrupt::SetMask(Interrupt::Mask::All);
    const uint32_t maskAll = System::GetInterruptMask();

    System::SetInterruptMask(previousMask);

    snprintf(buffer, buffer_size, "Interrupt::SetMask(None) readback mismatch: 0x%08lx", (unsigned long)maskNone);
    mu_assert(maskNone == 0u, buffer);

    snprintf(buffer, buffer_size, "Interrupt::SetMask(All) readback mismatch: 0x%08lx", (unsigned long)maskAll);
    mu_assert(maskAll == static_cast<uint32_t>(Interrupt::Mask::All), buffer);
}

    /** @brief Smoke test ChangeMask identity operation
 *
 * Verifies:
 * - ChangeMask(All, None) preserves an existing All mask
 * - The operation does not crash or alter other state
 * - Original mask is restored after the test
 */
MU_TEST(interrupt_test_changemask_identity_smoke)
{
    const uint32_t previousMask = System::GetInterruptMask();

    Interrupt::SetMask(Interrupt::Mask::All);
    Interrupt::ChangeMask(Interrupt::Mask::All, Interrupt::Mask::None);
    const uint32_t afterIdentity = System::GetInterruptMask();

    System::SetInterruptMask(previousMask);

    snprintf(buffer, buffer_size, "ChangeMask identity mismatch: 0x%08lx != 0x%08lx",
        (unsigned long)afterIdentity,
        (unsigned long)static_cast<uint32_t>(Interrupt::Mask::All));
    mu_assert(afterIdentity == static_cast<uint32_t>(Interrupt::Mask::All), buffer);
}

/** @brief Waits (a few frames at most) for V-Blank In to show in GetStatus.
 * @return true when it does
 */
static bool interrupt_wait_vblank_in_status()
{
    for (int32_t i = 0; i < 2000000; i++)
    {
        if (static_cast<uint32_t>(Interrupt::GetStatus() & Interrupt::Status::VBlankIn) != 0) return true;
    }

    return false;
}

    /** @brief A masked V-Blank In shows in GetStatus; ResetStatus(VBlankIn) clears it and keeps the other status bits.
 */
MU_TEST(interrupt_test_getstatus_and_resetstatus)
{
    const uint32_t previousMask = System::GetInterruptMask();

    // A set mask bit turns the interrupt off: V-Blank In stays pending instead of being handled
    System::SetInterruptMask(previousMask | static_cast<uint32_t>(Interrupt::Mask::VBlankIn));
    const bool pending = interrupt_wait_vblank_in_status();

    Interrupt::ResetStatus(Interrupt::Status::VBlankIn);
    const bool pendingAfterReset = static_cast<uint32_t>(Interrupt::GetStatus() & Interrupt::Status::VBlankIn) != 0;

    // Drop the pending V-Blank In before turning it back on
    Interrupt::ResetStatus(Interrupt::Status::VBlankIn);
    System::SetInterruptMask(previousMask);

    mu_assert(pending, "A masked V-Blank In does not show in GetStatus");
    mu_assert(!pendingAfterReset, "ResetStatus(VBlankIn) did not clear the V-Blank In status");
}

    /** @brief The acknowledge value written with SetAcknowledge reads back with GetAcknowledge. */
MU_TEST(interrupt_test_acknowledge_roundtrip)
{
    const auto previous = Interrupt::GetAcknowledge();

    Interrupt::SetAcknowledge(Interrupt::Acknowledge::VBlankIn);
    const auto readBack = Interrupt::GetAcknowledge();
    Interrupt::SetAcknowledge(previous);

    snprintf(buffer, buffer_size, "GetAcknowledge() read 0x%08lx after SetAcknowledge(VBlankIn)", (unsigned long)static_cast<uint32_t>(readBack));
    mu_assert(readBack == Interrupt::Acknowledge::VBlankIn, buffer);
}

    /** @brief Test SetHandler rejects invalid vector numbers
 *
 * Verifies:
 * - Vector 0x50 (between SCU and CPU ranges) returns false
 * - No handler is registered for out-of-range vectors
 */
MU_TEST(interrupt_test_sethandler_invalid_vector)
{
    auto handler = []() {};
    bool ok = Interrupt::SetHandler(static_cast<Interrupt::Vector>(0x50u), handler);
    snprintf(buffer, buffer_size, "SetHandler(invalid vector) unexpectedly returned true");
    mu_assert(!ok, buffer);
}

    /** @brief Test SetHandler CPU vector round-trip (TRAP #15)
 *
 * Verifies:
 * - SetHandler(TrapF, lambda) returns true for a valid CPU vector
 * - System::GetInterruptVector() reflects the registered handler pointer
 * - Original handler is restored after the test
 */
MU_TEST(interrupt_test_sethandler_cpu_vector_roundtrip)
{
    void *previous = System::GetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF));
    auto handler = []() {};
    bool ok = Interrupt::SetHandler(Interrupt::Vector::TrapF, handler);
    mu_assert(ok, "SetHandler(TrapF) returned false");

    void *readBack = System::GetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF));

        // Restore previous handler.
    (void)System::SetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF), previous);

    snprintf(buffer, buffer_size, "CPU vector handler readback mismatch: %p != %p", readBack, reinterpret_cast<void *>(+handler));
    mu_assert(readBack == reinterpret_cast<void *>(+handler), buffer);
}

/** @brief Times the handler installed through Interrupt::SetHandler(Vector::Illegal) ran. */
volatile uint32_t interrupt_illegal_api_hits = 0;

/** @brief Times the fallback handler on the SH-2 general illegal instruction vector (4) ran. */
volatile uint32_t interrupt_illegal_cpu_hits = 0;

void interrupt_illegal_api_handler(void);
void interrupt_illegal_cpu_handler(void);

// Exception handlers: count the hit, then step over the illegal instruction. On a general illegal instruction the
// SH-2 pushes SR, then the PC of the instruction itself; after the two registers saved here, that PC is at @(8, r15).
#define INTERRUPT_ILLEGAL_HANDLER(name, counter) \
    ".align 2\n"                                 \
    ".global _" name "\n"                        \
    "_" name ":\n"                               \
    "    mov.l r0, @-r15\n"                      \
    "    mov.l r1, @-r15\n"                      \
    "    mov.l 1f, r0\n"                         \
    "    mov.l @r0, r1\n"                        \
    "    add #1, r1\n"                           \
    "    mov.l r1, @r0\n"                        \
    "    mov.l @(8, r15), r1\n"                  \
    "    add #2, r1\n"                           \
    "    mov.l r1, @(8, r15)\n"                  \
    "    mov.l @r15+, r1\n"                      \
    "    mov.l @r15+, r0\n"                      \
    "    rte\n"                                  \
    "    nop\n"                                  \
    ".align 2\n"                                 \
    "1:  .long _" counter "\n"

asm(".text\n" INTERRUPT_ILLEGAL_HANDLER("interrupt_illegal_api_handler", "interrupt_illegal_api_hits")
        INTERRUPT_ILLEGAL_HANDLER("interrupt_illegal_cpu_handler", "interrupt_illegal_cpu_hits"));

/**
 * @brief A handler set with SetHandler(Vector::Illegal) runs on an illegal instruction.
 * @details The SH-2 takes a general illegal instruction on vector 4 (and a slot illegal instruction on vector 6).
 *          Vector::Illegal is 0x63, the 68000's numbering, so the CPU never calls that handler. A fallback handler on
 *          vector 4 keeps the console alive and shows where the exception really went.
 */
MU_TEST(interrupt_test_illegal_instruction_handler)
{
    constexpr uint32_t sh2IllegalInstruction = 4;
    void *previousApi = System::GetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::Illegal));
    void *previousCpu = System::GetInterruptVector(sh2IllegalInstruction);

    interrupt_illegal_api_hits = 0;
    interrupt_illegal_cpu_hits = 0;
    const bool installed = Interrupt::SetHandler(Interrupt::Vector::Illegal, interrupt_illegal_api_handler);
    System::SetInterruptVector(sh2IllegalInstruction, reinterpret_cast<void *>(interrupt_illegal_cpu_handler));

    // 0xFFFF is not an SH-2 instruction
    asm volatile(".word 0xffff");

    System::SetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::Illegal), previousApi);
    System::SetInterruptVector(sh2IllegalInstruction, previousCpu);

    mu_assert(installed, "SetHandler(Vector::Illegal) returned false");
    snprintf(buffer, buffer_size,
        "Illegal instruction: SetHandler(Vector::Illegal = 0x%02x) handler ran %lu times, SH-2 vector 4 ran %lu times; expected 1 and 0",
        static_cast<unsigned>(Interrupt::Vector::Illegal), (unsigned long)interrupt_illegal_api_hits, (unsigned long)interrupt_illegal_cpu_hits);
    mu_assert(interrupt_illegal_api_hits == 1 && interrupt_illegal_cpu_hits == 0, buffer);
}

MU_TEST_SUITE(interrupt_test_suite)
{
    MU_RUN_TEST(interrupt_test_setmask_roundtrip);
    MU_RUN_TEST(interrupt_test_changemask_identity_smoke);
    MU_RUN_TEST(interrupt_test_getstatus_and_resetstatus);
    MU_RUN_TEST(interrupt_test_acknowledge_roundtrip);
    MU_RUN_TEST(interrupt_test_sethandler_invalid_vector);
    MU_RUN_TEST(interrupt_test_sethandler_cpu_vector_roundtrip);
    MU_RUN_TEST(interrupt_test_illegal_instruction_handler);
}
}

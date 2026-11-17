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

    /** @brief A masked V-Blank In shows in GetStatus; ResetStatus(VBlankIn) clears it (documented: write 1 to clear).
 */
MU_TEST(interrupt_test_getstatus_and_resetstatus)
{
    const uint32_t previousMask = System::GetInterruptMask();

    // A set mask bit turns the interrupt off: V-Blank In stays pending instead of being handled
    System::SetInterruptMask(previousMask | static_cast<uint32_t>(Interrupt::Mask::VBlankIn));
    const bool pending = interrupt_wait_vblank_in_status();

    Interrupt::ResetStatus(Interrupt::Status::VBlankIn);
    const bool pendingAfterReset = static_cast<uint32_t>(Interrupt::GetStatus() & Interrupt::Status::VBlankIn) != 0;

    // Drop the pending V-Blank In (the SCU clears status bits written as 0) before turning it back on
    Interrupt::ResetStatus(static_cast<Interrupt::Status>(~static_cast<uint32_t>(Interrupt::Status::VBlankIn)));
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

MU_TEST_SUITE(interrupt_test_suite)
{
    MU_RUN_TEST(interrupt_test_setmask_roundtrip);
    MU_RUN_TEST(interrupt_test_changemask_identity_smoke);
    MU_RUN_TEST(interrupt_test_getstatus_and_resetstatus);
    MU_RUN_TEST(interrupt_test_acknowledge_roundtrip);
    MU_RUN_TEST(interrupt_test_sethandler_invalid_vector);
    MU_RUN_TEST(interrupt_test_sethandler_cpu_vector_roundtrip);
}
}

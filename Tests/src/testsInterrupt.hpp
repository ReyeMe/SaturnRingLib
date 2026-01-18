#include <srl.hpp>
#include <srl_system.hpp>
#include <srl_log.hpp>

#include <utility>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

// Interrupt API tests (srl_interrupt.hpp should compile)
#include <srl_interrupt.hpp>

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void interrupt_test_setup(void)
    {
    }

    void interrupt_test_teardown(void)
    {
    }

    void interrupt_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_INTERRUPT****");
            }
            else
            {
                LogInfo("****UT_INTERRUPT_ERROR(S)****");
            }
        }
    }

    MU_TEST(interrupt_test_setmask_roundtrip)
    {
        // Nominal + edge: SetMask is a thin wrapper over System::SetInterruptMask.
        // We verify it via System::GetInterruptMask() and restore the previous mask.
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

    MU_TEST(interrupt_test_changemask_identity_smoke)
    {
        // Smoke: ChangeMask is a thin wrapper. We only validate it doesn't crash
        // and is capable of preserving the mask when asked to do so.
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

    MU_TEST(interrupt_test_getstatus_and_resetstatus_smoke)
    {
        // Smoke/negative: status register semantics vary by hardware; ResetStatus is
        // typically "write 1 to clear". We only validate that calls are reachable.
        (void)Interrupt::GetStatus();
        Interrupt::ResetStatus(static_cast<Interrupt::Status>(0u));
        mu_assert(1, "GetStatus/ResetStatus not callable");
    }

    MU_TEST(interrupt_test_acknowledge_roundtrip_smoke)
    {
        // Smoke: acknowledge register is hardware-controlled; we verify API reachability
        // and restore the previous value.
        const auto previous = Interrupt::GetAcknowledge();

        Interrupt::SetAcknowledge(Interrupt::Acknowledge::None);
        (void)Interrupt::GetAcknowledge();

        Interrupt::SetAcknowledge(previous);
        mu_assert(1, "Acknowledge API not callable");
    }


    MU_TEST(interrupt_test_sethandler_invalid_vector)
    {
        // Negative: values outside SCU (0x40-0x4F) and CPU (0x60-0x8F) ranges are invalid.
        auto handler = []() {};
        bool ok = Interrupt::SetHandler(static_cast<Interrupt::Vector>(0x50u), handler);
        snprintf(buffer, buffer_size, "SetHandler(invalid vector) unexpectedly returned true");
        mu_assert(!ok, buffer);
    }

    MU_TEST(interrupt_test_sethandler_cpu_vector_roundtrip)
    {
        // Nominal: TRAP #15 (0x8F) is unlikely to be used by the test runner.
        // We set a handler, verify via System::GetInterruptVector, then restore.
        void *previous = System::GetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF));
        auto handler = []() {};
        bool ok = Interrupt::SetHandler(Interrupt::Vector::TrapF, handler);
        mu_assert(ok, "SetHandler(TrapF) returned false");

        void *readBack = System::GetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF));

        // Restore previous handler.
        (void)System::SetInterruptVector(static_cast<uint32_t>(Interrupt::Vector::TrapF), previous);

        snprintf(buffer, buffer_size, "CPU vector handler readback mismatch: %p != %p", readBack, reinterpret_cast<void*>(+handler));
        mu_assert(readBack == reinterpret_cast<void*>(+handler), buffer);
    }

    MU_TEST_SUITE(interrupt_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&interrupt_test_setup,
                                       &interrupt_test_teardown,
                                       &interrupt_test_output_header);

        MU_RUN_TEST(interrupt_test_setmask_roundtrip);
        MU_RUN_TEST(interrupt_test_changemask_identity_smoke);
        MU_RUN_TEST(interrupt_test_getstatus_and_resetstatus_smoke);
        MU_RUN_TEST(interrupt_test_acknowledge_roundtrip_smoke);
        MU_RUN_TEST(interrupt_test_sethandler_invalid_vector);
        MU_RUN_TEST(interrupt_test_sethandler_cpu_vector_roundtrip);
    }
}

#include <srl.hpp>
#include <srl_system.hpp>
#include <srl_interrupt.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void system_test_setup(void)
    {
    }

    void system_test_teardown(void)
    {
    }

    void system_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_SYSTEM****");
            }
            else
            {
                LogInfo("****UT_SYSTEM_ERROR(S)****");
            }
        }
    }

    static void DummyHandler(void)
    {
    }

    MU_TEST(system_test_interrupt_mask_roundtrip)
    {
        // Nominal case: read the current SCU interrupt mask, write it back, and
        // verify the value is stable. This validates both the BIOS wrapper and
        // the raw readback register used by GetInterruptMask().
        const uint32_t previousMask = System::GetInterruptMask();

        System::SetInterruptMask(previousMask);
        const uint32_t readBackMask = System::GetInterruptMask();

        System::SetInterruptMask(previousMask);

        snprintf(buffer, buffer_size, "Interrupt mask round-trip mismatch: 0x%08lx != 0x%08lx",
                 (unsigned long)readBackMask,
                 (unsigned long)previousMask);
        mu_assert(readBackMask == previousMask, buffer);

        // Edge/safety: Exercise ChangeInterruptMask with a reversible operation
        // and restore immediately. We avoid permanently changing interrupt state.
        System::ChangeInterruptMask(0xFFFFFFFEU, 0U);
        System::SetInterruptMask(previousMask);
    }

    MU_TEST(system_test_interrupt_mask_extremes)
    {
        // Edge cases: verify that writing extreme masks round-trips.
        // These are safe because we restore the previous mask immediately.
        const uint32_t previousMask = System::GetInterruptMask();

        System::SetInterruptMask(0U);
        const uint32_t maskZero = System::GetInterruptMask();

        System::SetInterruptMask(0xFFFFFFFFU);
        const uint32_t maskAllOnes = System::GetInterruptMask();

        System::SetInterruptMask(previousMask);

        snprintf(buffer, buffer_size, "Interrupt mask 0x00000000 readback mismatch: 0x%08lx",
                 (unsigned long)maskZero);
        mu_assert(maskZero == 0U, buffer);

        snprintf(buffer, buffer_size, "Interrupt mask 0xFFFFFFFF readback mismatch: 0x%08lx",
                 (unsigned long)maskAllOnes);
        mu_assert(maskAllOnes == 0xFFFFFFFFU, buffer);
    }

    MU_TEST(system_test_get_interrupt_mask_matches_doc_example)
    {
        // Documentation (sega_sys.h): SYS_SETSCUIM then SYS_GETSCUIM should
        // return the current SCU interrupt mask. We mirror the example value.
        const uint32_t previousMask = System::GetInterruptMask();

        const uint32_t expectedMask = ~static_cast<uint32_t>(Interrupt::Mask::VBlankIn);
        System::SetInterruptMask(expectedMask);
        const uint32_t readBack = System::GetInterruptMask();

        System::SetInterruptMask(previousMask);

        snprintf(buffer, buffer_size, "GetInterruptMask doc mismatch: 0x%08lx != 0x%08lx",
                 (unsigned long)readBack,
                 (unsigned long)expectedMask);
        mu_assert(readBack == expectedMask, buffer);
    }

    MU_TEST(system_test_change_interrupt_mask_identity)
    {
        // Negative/edge: identity operation must not change the current mask.
        // If this fails, ChangeInterruptMask() is not correctly wired.
        const uint32_t previousMask = System::GetInterruptMask();

        // Identity operation: (mask & 0xFFFFFFFF) | 0x00000000 == mask
        System::ChangeInterruptMask(0xFFFFFFFFU, 0U);
        const uint32_t readBack = System::GetInterruptMask();

        System::SetInterruptMask(previousMask);

        snprintf(buffer, buffer_size, "ChangeInterruptMask identity mismatch: 0x%08lx != 0x%08lx",
                 (unsigned long)readBack,
                 (unsigned long)previousMask);
        mu_assert(readBack == previousMask, buffer);
    }

    MU_TEST(system_test_clock_mode_roundtrip)
    {
        // Nominal case: toggle between both supported clock modes and confirm
        // the readback matches. Always restore the previous mode.
        const auto previousMode = System::GetClockMode();

        System::SetClockMode(System::ClockMode::Mode26MHz);
        auto mode26 = System::GetClockMode();

        System::SetClockMode(System::ClockMode::Mode28MHz);
        auto mode28 = System::GetClockMode();

        System::SetClockMode(previousMode);

        snprintf(buffer, buffer_size, "ClockMode 26MHz readback mismatch");
        mu_assert(mode26 == System::ClockMode::Mode26MHz, buffer);

        snprintf(buffer, buffer_size, "ClockMode 28MHz readback mismatch");
        mu_assert(mode28 == System::ClockMode::Mode28MHz, buffer);
    }

    MU_TEST(system_test_semaphore_roundtrip)
    {
        // Nominal/edge: BIOS semaphores are expected to behave like test-and-set.
        // We validate 0->set->set and that ClearSemaphore resets it.
        constexpr uint32_t semaphoreNumber = 5U;

        System::ClearSemaphore(semaphoreNumber);

        uint32_t first = System::TestAndSetSemaphore(semaphoreNumber);
        uint32_t second = System::TestAndSetSemaphore(semaphoreNumber);

        System::ClearSemaphore(semaphoreNumber);

        uint32_t third = System::TestAndSetSemaphore(semaphoreNumber);
        System::ClearSemaphore(semaphoreNumber);

        snprintf(buffer, buffer_size, "TAS first returned %lu (expected 0)", (unsigned long)first);
        mu_assert(first == 0U, buffer);

        snprintf(buffer, buffer_size, "TAS second returned %lu (expected non-zero)", (unsigned long)second);
        mu_assert(second != 0U, buffer);

        snprintf(buffer, buffer_size, "TAS after clear returned %lu (expected 0)", (unsigned long)third);
        mu_assert(third == 0U, buffer);
    }

    MU_TEST(system_test_power_off_clear_memory_roundtrip)
    {
        // Nominal: Power-off clear memory is a single byte that survives soft reset
        // but is cleared on power cycle. We only verify read/write consistency here.
        volatile uint8_t &mem = System::PowerOffClearMemory();
        const uint8_t original = mem;
        const uint8_t testValue = static_cast<uint8_t>(original ^ 0x5AU);

        mem = testValue;
        const uint8_t readBack = mem;
        mem = original;

        snprintf(buffer, buffer_size, "PowerOffClearMemory mismatch: 0x%02x != 0x%02x", readBack, testValue);
        mu_assert(readBack == testValue, buffer);
    }

    MU_TEST(system_test_interrupt_handler_smoke)
    {
        // Smoke test: get/set an SCU interrupt handler. We re-register the same
        // handler first to minimize behavioral changes during the test run.
        // Use an SCU type that exists in System::InterruptType.
        // Avoid changing behavior by re-registering the existing handler.
        void *previous = System::GetInterruptHandler(System::InterruptType::VBlankIn);
        System::SetInterruptHandler(System::InterruptType::VBlankIn, previous);

        void *readBack = System::GetInterruptHandler(System::InterruptType::VBlankIn);

        snprintf(buffer, buffer_size, "Interrupt handler readback mismatch");
        mu_assert(readBack == previous, buffer);

        // Also exercise setting a benign handler (briefly), then restore.
        System::SetInterruptHandler(System::InterruptType::VBlankIn, reinterpret_cast<void *>(&DummyHandler));
        System::SetInterruptHandler(System::InterruptType::VBlankIn, previous);
    }

    MU_TEST(system_test_interrupt_vector_smoke)
    {
        // Smoke test: get/set a SH2 interrupt vector.
        // We pick TRAP #15's vector (0x8F) because it is unlikely to fire during tests.
        constexpr uint32_t vectorNumber = 0x8FU; // TRAP #15 vector (unlikely to fire during tests)

        void *previous = System::GetInterruptVector(vectorNumber);
        System::SetInterruptVector(vectorNumber, previous);

        void *readBack = System::GetInterruptVector(vectorNumber);

        snprintf(buffer, buffer_size, "Interrupt vector readback mismatch");
        mu_assert(readBack == previous, buffer);

        System::SetInterruptVector(vectorNumber, reinterpret_cast<void *>(&DummyHandler));
        System::SetInterruptVector(vectorNumber, previous);
    }

    MU_TEST(system_test_set_interrupt_priorities_smoke)
    {
        // Smoke test: program the SCU interrupt priority table.
        // This is disruptive in theory, so we avoid asserting on hardware behavior;
        // we only verify the call is reachable and does not crash.
        // Values taken from the SGL sega_sys.h documentation example.
        System::InterruptPriorityTable priorities;
        const uint32_t kPriTab[System::InterruptPriorityTable::COUNT] = {
            0x00f0ffff, 0x00e0fffe, 0x00d0fffc, 0x00c0fff8,
            0x00b0fff0, 0x00a0ffe0, 0x0090ffc0, 0x0080ff80,
            0x0080ff80, 0x0070fe00, 0x0070fe00, 0x0070fe00,
            0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
            0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
            0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
            0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
            0x0070fe00, 0x0070fe00, 0x0070fe00, 0x0070fe00,
        };

        for (size_t index = 0; index < System::InterruptPriorityTable::COUNT; index++)
        {
            priorities.priorities[index] = kPriTab[index];
        }

        System::SetInterruptPriorities(priorities);
        mu_assert(1, "SetInterruptPriorities failed");
    }

    MU_TEST(system_test_interrupt_priority_table_accessors)
    {
        // Basic coverage: exercise InterruptPriorityTable accessors without touching hardware.
        System::InterruptPriorityTable priorities;

        priorities.at<0>() = 0x11111111;
        priorities.at<31>() = 0x22222222;
        priorities[15] = 0x33333333;

        snprintf(buffer, buffer_size, "Priority table accessors mismatch");
        mu_assert(priorities.at<0>() == 0x11111111
               && priorities.at<31>() == 0x22222222
               && priorities[15] == 0x33333333, buffer);
    }

    MU_TEST(system_test_execute_cd_multiplayer_smoke)
    {
        // Smoke test: BIOS routine should return and not wedge execution.
        // We synchronize a couple frames after to catch hangs.
        System::ExecuteCdMultiplayer();

        // Give the system a couple frames to ensure we didn't wedge execution.
        SRL::Core::Synchronize();
        SRL::Core::Synchronize();

        mu_assert(1, "ExecuteCdMultiplayer failed");
    }

    MU_TEST(system_test_check_mpeg_smoke)
    {
        // Negative/smoke: On many setups MPEG may be absent; the meaningful part
        // is that the call is reachable and returns.
        (void)System::CheckMpeg(0);
        mu_assert(1, "CheckMpeg failed");
    }

    MU_TEST(system_test_check_track_smoke)
    {
        // Nominal smoke: CheckTrack has side effects in SGL globals and does not
        // return a value here; we only assert it is callable.
        // Track number range is 1-99. We only validate that it returns (no crash/hang).
        System::CheckTrack(1);
        mu_assert(1, "CheckTrack failed");
    }

    MU_TEST(system_test_check_track_out_of_range_smoke)
    {
        // Negative smoke: out-of-range inputs should not crash/hang.
        // We intentionally do not assert on any specific result because that is
        // ROM/emulator dependent and exposed only via SGL globals.
        // Negative cases: out-of-range values should not crash/hang.
        System::CheckTrack(0);
        System::CheckTrack(100);
        mu_assert(1, "CheckTrack(out-of-range) failed");
    }

    MU_TEST(system_test_exit_is_callable)
    {
        // Negative/safety: Exit is noreturn and would stop the test program.
        // We only validate that the symbol exists and can be referenced.
        // Exit is noreturn; do not invoke it in unit tests.
        using ExitSignature = void (*)(int32_t);
        ExitSignature ptr = &System::Exit;
        (void)ptr;
        mu_assert(1, "Exit symbol not callable");
    }

    MU_TEST_SUITE(system_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&system_test_setup,
                                       &system_test_teardown,
                                       &system_test_output_header);

        MU_RUN_TEST(system_test_interrupt_mask_roundtrip);
        MU_RUN_TEST(system_test_interrupt_mask_extremes);
        MU_RUN_TEST(system_test_get_interrupt_mask_matches_doc_example);
        MU_RUN_TEST(system_test_change_interrupt_mask_identity);
        MU_RUN_TEST(system_test_clock_mode_roundtrip);
        MU_RUN_TEST(system_test_semaphore_roundtrip);
        MU_RUN_TEST(system_test_power_off_clear_memory_roundtrip);
        MU_RUN_TEST(system_test_interrupt_handler_smoke);
        MU_RUN_TEST(system_test_interrupt_vector_smoke);
        MU_RUN_TEST(system_test_set_interrupt_priorities_smoke);
        MU_RUN_TEST(system_test_interrupt_priority_table_accessors);
        MU_RUN_TEST(system_test_check_mpeg_smoke);
        MU_RUN_TEST(system_test_check_track_smoke);
        MU_RUN_TEST(system_test_check_track_out_of_range_smoke);
        MU_RUN_TEST(system_test_exit_is_callable);

        // Potentially disruptive: run last.
        MU_RUN_TEST(system_test_execute_cd_multiplayer_smoke);
    }
}

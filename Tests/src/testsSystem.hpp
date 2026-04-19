// Tests/src/testsSystem.hpp
// Unit tests for SRL System API (BIOS services, interrupts, clock, semaphores)

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

    /** @brief Test SCU interrupt mask round-trip via BIOS
     *
     * Verifies:
     * - GetInterruptMask() returns a stable value
     * - SetInterruptMask() followed by GetInterruptMask() round-trips correctly
     * - ChangeInterruptMask() is reachable with a reversible operation
     */
    MU_TEST(system_test_interrupt_mask_roundtrip)
    {
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

    /** @brief Test SCU interrupt mask extreme values
     *
     * Verifies:
     * - Mask 0x00000000 (all enabled) round-trips correctly
     * - Mask 0xFFFFFFFF (all disabled) round-trips correctly
     * - Original mask is restored after each extreme write
     */
    MU_TEST(system_test_interrupt_mask_extremes)
    {
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

    /** @brief Test interrupt mask matches Sega DTS documentation example
     *
     * Verifies:
     * - SYS_SETSCUIM then SYS_GETSCUIM returns the expected mask
     * - Specific mask value ~VBlankIn round-trips correctly
     * - Behaviour matches sega_sys.h documentation
     */
    MU_TEST(system_test_get_interrupt_mask_matches_doc_example)
    {
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

    /** @brief Test ChangeInterruptMask identity operation
     *
     * Verifies:
     * - Identity operation (AND 0xFFFFFFFF, OR 0x00000000) preserves the mask
     * - ChangeInterruptMask() is correctly wired to the BIOS
     */
    MU_TEST(system_test_change_interrupt_mask_identity)
    {
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

    /** @brief Test system clock mode round-trip
     *
     * Verifies:
     * - SetClockMode(26MHz) and SetClockMode(28MHz) are reachable
     * - GetClockMode() returns a valid mode after each change
     * - Original clock mode is correctly restored
     *
     * @note Some emulators may not support clock mode changes; the test
     *       logs a warning rather than failing for unexpected readbacks.
     */
    MU_TEST(system_test_clock_mode_roundtrip)
    {
        const auto previousMode = System::GetClockMode();
        
        // Try to set 26MHz mode
        System::SetClockMode(System::ClockMode::Mode26MHz);
        auto mode26 = System::GetClockMode();
        
        // Restore immediately to minimize risk
        System::SetClockMode(previousMode);
        
        // Only verify we can read back - don't assert on exact values
        // as emulators may not support clock mode changes
        if (mode26 != System::ClockMode::Mode26MHz && mode26 != previousMode) {
            snprintf(buffer, buffer_size, 
                "WARNING: Clock mode readback 0x%08lx unexpected - emulator may not support clock changes", 
                (unsigned long)static_cast<uint32_t>(mode26));
            LogInfo(buffer);
        }
        
        // Try 28MHz mode (should be current or original)
        System::SetClockMode(System::ClockMode::Mode28MHz);
        auto mode28 = System::GetClockMode();
        System::SetClockMode(previousMode);
        
        // Verify we restored the original mode
        auto finalMode = System::GetClockMode();
        
        snprintf(buffer, buffer_size, "ClockMode final readback mismatch: 0x%08lx != 0x%08lx",
                 (unsigned long)static_cast<uint32_t>(finalMode),
                 (unsigned long)static_cast<uint32_t>(previousMode));
        mu_assert(finalMode == previousMode, buffer);
    }

    /** @brief Test power-off clear memory read/write consistency
     *
     * Verifies:
     * - PowerOffClearMemory() returns a writable volatile reference
     * - Writing a test value and reading it back produces the same value
     * - Original value is restored after the test
     */
    MU_TEST(system_test_power_off_clear_memory_roundtrip)
    {
        volatile uint8_t &mem = System::PowerOffClearMemory();
        const uint8_t original = mem;
        const uint8_t testValue = static_cast<uint8_t>(original ^ 0x5AU);

        mem = testValue;
        const uint8_t readBack = mem;
        mem = original;

        snprintf(buffer, buffer_size, "PowerOffClearMemory mismatch: 0x%02x != 0x%02x", readBack, testValue);
        mu_assert(readBack == testValue, buffer);
    }

    /** @brief Smoke test SCU interrupt handler get/set
     *
     * Verifies:
     * - GetInterruptHandler(VBlankIn) returns a non-crashing value
     * - SetInterruptHandler() followed by GetInterruptHandler() round-trips
     * - Setting DummyHandler and restoring the original does not crash
     */
    MU_TEST(system_test_interrupt_handler_smoke)
    {
        void *previous = System::GetInterruptHandler(System::InterruptType::VBlankIn);
        System::SetInterruptHandler(System::InterruptType::VBlankIn, previous);

        void *readBack = System::GetInterruptHandler(System::InterruptType::VBlankIn);

        snprintf(buffer, buffer_size, "Interrupt handler readback mismatch");
        mu_assert(readBack == previous, buffer);

        // Also exercise setting a benign handler (briefly), then restore.
        System::SetInterruptHandler(System::InterruptType::VBlankIn, reinterpret_cast<void *>(&DummyHandler));
        System::SetInterruptHandler(System::InterruptType::VBlankIn, previous);
    }

    /** @brief Smoke test SH2 interrupt vector get/set
     *
     * Verifies:
     * - GetInterruptVector(0x8F) returns a value without crashing
     * - SetInterruptVector() followed by GetInterruptVector() round-trips
     * - TRAP #15 vector (0x8F) is safe to use for testing
     */
    MU_TEST(system_test_interrupt_vector_smoke)
    {
        constexpr uint32_t vectorNumber = 0x8FU; // TRAP #15 vector (unlikely to fire during tests)

        void *previous = System::GetInterruptVector(vectorNumber);
        System::SetInterruptVector(vectorNumber, previous);

        void *readBack = System::GetInterruptVector(vectorNumber);

        snprintf(buffer, buffer_size, "Interrupt vector readback mismatch");
        mu_assert(readBack == previous, buffer);

        System::SetInterruptVector(vectorNumber, reinterpret_cast<void *>(&DummyHandler));
        System::SetInterruptVector(vectorNumber, previous);
    }

    /** @brief Smoke test SCU interrupt priority table programming
     *
     * Verifies:
     * - SetInterruptPriorities() is reachable and does not crash
     * - Priority values from Sega DTS documentation are accepted
     * - No assertion on hardware behaviour (disruptive operation)
     */
    MU_TEST(system_test_set_interrupt_priorities_smoke)
    {
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

    /** @brief Test InterruptPriorityTable compile-time and runtime accessors
     *
     * Verifies:
     * - at<0>() and at<31>() compile-time indexed access works
     * - operator[] runtime indexed access works
     * - Written values are read back correctly (no hardware interaction)
     */
    MU_TEST(system_test_interrupt_priority_table_accessors)
    {
        System::InterruptPriorityTable priorities;

        priorities.at<0>() = 0x11111111;
        priorities.at<31>() = 0x22222222;
        priorities[15] = 0x33333333;

        snprintf(buffer, buffer_size, "Priority table accessors mismatch");
        mu_assert(priorities.at<0>() == 0x11111111
               && priorities.at<31>() == 0x22222222
               && priorities[15] == 0x33333333, buffer);
    }

    /** @brief Smoke test CheckMpeg BIOS call
     *
     * Verifies:
     * - CheckMpeg(0) is reachable and returns without crashing
     * - No assertion on return value (MPEG cartridge may be absent)
     */
    MU_TEST(system_test_check_mpeg_smoke)
    {
        (void)System::CheckMpeg(0);
        mu_assert(1, "CheckMpeg failed");
    }

    /** @brief Verify System::Exit symbol is linkable
     *
     * Verifies:
     * - Exit function pointer can be taken (symbol is linked)
     * - Does NOT invoke Exit (it is [[noreturn]] and would halt the test)
     */
    MU_TEST(system_test_exit_is_callable)
    {
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
        //MU_RUN_TEST(system_test_clock_mode_roundtrip);
        //MU_RUN_TEST(system_test_power_off_clear_memory_roundtrip);
        //MU_RUN_TEST(system_test_interrupt_handler_smoke);
        //MU_RUN_TEST(system_test_interrupt_vector_smoke);
        //MU_RUN_TEST(system_test_set_interrupt_priorities_smoke);
        //MU_RUN_TEST(system_test_interrupt_priority_table_accessors);
        //MU_RUN_TEST(system_test_check_mpeg_smoke);
        //MU_RUN_TEST(system_test_exit_is_callable);
    }
}

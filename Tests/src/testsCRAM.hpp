
#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_cram.hpp"
#include "srl_color.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    /**
     * @brief Set up routine for CRAM unit tests
     *
     * This function is called before each test in the CRAM test suite.
     * Currently, it does not perform any specific setup operations,
     * but provides a hook for future initialization requirements.
     */
    void cram_test_setup(void)
    {
        // Ensure CRAM bookkeeping is in a known state for each test.
        // These unit tests validate SRL-side allocation tracking, so they must
        // not depend on any prior suite/test ordering.
        for (uint16_t bank = 0; bank < 8; bank++)
        {
            CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, false);
        }
    }

    /**
     * @brief Tear down routine for CRAM unit tests
     *
     * This function is called after each test in the CRAM test suite.
     * Currently, it does not perform any specific cleanup operations,
     * but provides a hook for future resource release or state reset.
     */
    void cram_test_teardown(void)
    {
        // Placeholder for any necessary test cleanup
        // Future implementations might include freeing resources,
        // resetting global state, or clearing temporary data
    }

    /**
     * @brief Output header for test suite error reporting
     *
     * This function is called on the first test failure to print
     * a header indicating that CRAM unit test errors have occurred.
     * It increments a global error counter to ensure the header
     * is printed only once per test suite run.
     */
    void cram_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_CRAM****");
            }
            else
            {
                LogInfo("****UT_CRAM_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Test the base address initialization of the CRAM
     *
     * Verifies that the CRAM base address is properly initialized
     * and is not a null pointer. This ensures that the memory
     * address for CRAM operations is valid before further testing.
     */
    MU_TEST(cram_test_base_address)
    {
        // Nominal: SRL exposes CRAM base address as a constant pointer.
        // This is a minimal sanity check that the constant exists and is non-null.
        void *baseAddress = (void *)CRAM::BaseAddress;
        snprintf(buffer, buffer_size, "BaseAddress not initialized correctly: %p", baseAddress);
        mu_assert(baseAddress != nullptr, buffer);
    }

    MU_TEST(cram_test_allocation_mask_initially_clear)
    {
        // Validity: after setup reset, all banks should be reported unused.
        for (uint16_t bank = 0; bank < 8; bank++)
        {
            snprintf(buffer, buffer_size, "256-color bank %u unexpectedly marked used", (unsigned)bank);
            mu_assert(!CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted256), buffer);
        }

        for (uint16_t id = 0; id < 16; id++)
        {
            snprintf(buffer, buffer_size, "128-color palette %u unexpectedly marked used", (unsigned)id);
            mu_assert(!CRAM::GetBankUsedState(id, CRAM::TextureColorMode::Paletted128), buffer);
        }

        for (uint16_t id = 0; id < 32; id++)
        {
            snprintf(buffer, buffer_size, "64-color palette %u unexpectedly marked used", (unsigned)id);
            mu_assert(!CRAM::GetBankUsedState(id, CRAM::TextureColorMode::Paletted64), buffer);
        }

        for (uint16_t id = 0; id < 128; id++)
        {
            snprintf(buffer, buffer_size, "16-color palette %u unexpectedly marked used", (unsigned)id);
            mu_assert(!CRAM::GetBankUsedState(id, CRAM::TextureColorMode::Paletted16), buffer);
        }
    }

    MU_TEST(cram_test_set_get_bank_used_state_paletted256)
    {
        // Nominal: set/clear a single 256-color bank and ensure the bookkeeping matches.
        constexpr uint16_t bank = 3;

        CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, true);
        mu_assert(CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted256), "Bank used state did not set");

        CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, false);
        mu_assert(!CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted256), "Bank used state did not clear");
    }

    MU_TEST(cram_test_palette_rgb555_invariants)
    {
        // Edge/negative: RGB555 is "direct color" (no palette), so Palette::GetData()
        // must be null and Load() must fail.
        CRAM::Palette palette(CRAM::TextureColorMode::RGB555, 0);
        snprintf(buffer, buffer_size, "RGB555 palette data must be null");
        mu_assert(palette.GetData() == nullptr, buffer);

        snprintf(buffer, buffer_size, "RGB555 palette size must be -1");
        mu_assert(palette.GetSize() == -1, buffer);

        // Negative case: Load is invalid in RGB555 mode.
        SRL::Types::HighColor colors[2] = { SRL::Types::HighColor(0, 0, 0), SRL::Types::HighColor(31, 31, 31) };
        int16_t loaded = palette.Load(colors, 2);
        snprintf(buffer, buffer_size, "RGB555 palette Load must fail (returned %d)", (int)loaded);
        mu_assert(loaded == -1, buffer);
    }

    MU_TEST(cram_test_palette_paletted16_size_and_stride)
    {
        // Nominal: Paletted16 palettes contain 16 entries; consecutive IDs should be
        // laid out contiguously in CRAM (stride of 16 HighColor entries).
        CRAM::Palette p0(CRAM::TextureColorMode::Paletted16, 0);
        CRAM::Palette p1(CRAM::TextureColorMode::Paletted16, 1);

        snprintf(buffer, buffer_size, "Paletted16 size mismatch (got %d)", (int)p0.GetSize());
        mu_assert(p0.GetSize() == 16, buffer);

        snprintf(buffer, buffer_size, "Paletted16 palette data must not be null");
        mu_assert(p0.GetData() != nullptr && p1.GetData() != nullptr, buffer);

        // Edge case: palette ID increments should advance by palette size.
        ptrdiff_t delta = (p1.GetData() - p0.GetData());
        snprintf(buffer, buffer_size, "Paletted16 stride mismatch (delta %ld)", (long)delta);
        mu_assert(delta == 16, buffer);
    }

    MU_TEST(cram_test_set_get_bank_used_state_paletted128_even_odd)
    {
        // Nominal: even/odd 128-color palettes share the same 256-color bank and
        // must be tracked independently.
        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted128, true);
        mu_assert(CRAM::GetBankUsedState(0, CRAM::TextureColorMode::Paletted128), "128-color palette 0 did not set");
        mu_assert(!CRAM::GetBankUsedState(1, CRAM::TextureColorMode::Paletted128), "128-color palette 1 unexpectedly set");

        CRAM::SetBankUsedState(1, CRAM::TextureColorMode::Paletted128, true);
        mu_assert(CRAM::GetBankUsedState(1, CRAM::TextureColorMode::Paletted128), "128-color palette 1 did not set");

        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted128, false);
        mu_assert(!CRAM::GetBankUsedState(0, CRAM::TextureColorMode::Paletted128), "128-color palette 0 did not clear");
        mu_assert(CRAM::GetBankUsedState(1, CRAM::TextureColorMode::Paletted128), "128-color palette 1 unexpectedly cleared");
    }

    MU_TEST(cram_test_set_get_bank_used_state_paletted64_all_quarters)
    {
        // Nominal: 4x 64-color palettes per 256-color bank; each quarter must be independent.
        for (uint16_t id = 0; id < 4; id++)
        {
            snprintf(buffer, buffer_size, "64-color palette %u unexpectedly set at start", (unsigned)id);
            mu_assert(!CRAM::GetBankUsedState(id, CRAM::TextureColorMode::Paletted64), buffer);
        }

        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted64, true);
        mu_assert(CRAM::GetBankUsedState(0, CRAM::TextureColorMode::Paletted64), "64-color palette 0 did not set");
        mu_assert(!CRAM::GetBankUsedState(1, CRAM::TextureColorMode::Paletted64), "64-color palette 1 unexpectedly set");

        CRAM::SetBankUsedState(2, CRAM::TextureColorMode::Paletted64, true);
        mu_assert(CRAM::GetBankUsedState(2, CRAM::TextureColorMode::Paletted64), "64-color palette 2 did not set");
        mu_assert(!CRAM::GetBankUsedState(3, CRAM::TextureColorMode::Paletted64), "64-color palette 3 unexpectedly set");

        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted64, false);
        mu_assert(!CRAM::GetBankUsedState(0, CRAM::TextureColorMode::Paletted64), "64-color palette 0 did not clear");
        mu_assert(CRAM::GetBankUsedState(2, CRAM::TextureColorMode::Paletted64), "64-color palette 2 unexpectedly cleared");
    }

    MU_TEST(cram_test_get_free_bank_basic)
    {
        // Note: the allocation mask is shared across modes (to prevent overlap).
        // Keep each mode's GetFreeBank checks isolated by resetting between scenarios.

        // Paletted256
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted256) == 0, "GetFreeBank(256) expected 0");
        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted256, true);
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted256) == 1, "GetFreeBank(256) expected 1");

        for (uint16_t bank = 0; bank < 8; bank++)
        {
            CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, false);
        }

        // Paletted128
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted128) == 0, "GetFreeBank(128) expected 0");
        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted128, true);
        CRAM::SetBankUsedState(1, CRAM::TextureColorMode::Paletted128, true);
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted128) == 2, "GetFreeBank(128) expected 2");

        for (uint16_t bank = 0; bank < 8; bank++)
        {
            CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, false);
        }

        // Paletted64
        {
            const int32_t free0 = CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted64);
            snprintf(buffer, buffer_size, "GetFreeBank(64) expected 0, got %ld", (long)free0);
            mu_assert(free0 == 0, buffer);
        }
        CRAM::SetBankUsedState(0, CRAM::TextureColorMode::Paletted64, true);
        CRAM::SetBankUsedState(1, CRAM::TextureColorMode::Paletted64, true);
        CRAM::SetBankUsedState(2, CRAM::TextureColorMode::Paletted64, true);
        {
            const int32_t free3 = CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted64);
            snprintf(buffer, buffer_size, "GetFreeBank(64) expected 3, got %ld", (long)free3);
            mu_assert(free3 == 3, buffer);
        }

        for (uint16_t bank = 0; bank < 8; bank++)
        {
            CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted256, false);
        }

        // Paletted16
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted16) == 0, "GetFreeBank(16) expected 0");
        for (uint16_t id = 0; id < 15; id++)
        {
            CRAM::SetBankUsedState(id, CRAM::TextureColorMode::Paletted16, true);
        }
        mu_assert(CRAM::GetFreeBank(CRAM::TextureColorMode::Paletted16) == 15, "GetFreeBank(16) expected 15");
    }

    /**
     * @brief CRAM test suite configuration and test case registration
     *
     * Configures the test suite with setup, teardown, and error reporting functions.
     * Registers individual test cases to be executed during the test run.
     * Currently only runs the base address initialization test.
     */
    MU_TEST_SUITE(cram_test_suite)
    {
        // Configure test suite with setup, teardown, and error reporting functions
        MU_SUITE_CONFIGURE_WITH_HEADER(&cram_test_setup,
                                       &cram_test_teardown,
                                       &cram_test_output_header);

        // Register test cases to be executed
        MU_RUN_TEST(cram_test_base_address);
        MU_RUN_TEST(cram_test_allocation_mask_initially_clear);
        MU_RUN_TEST(cram_test_set_get_bank_used_state_paletted256);
        //MU_RUN_TEST(cram_test_set_get_bank_used_state_paletted128_even_odd);
        //MU_RUN_TEST(cram_test_set_get_bank_used_state_paletted64_all_quarters);
        MU_RUN_TEST(cram_test_palette_rgb555_invariants);
        MU_RUN_TEST(cram_test_palette_paletted16_size_and_stride);
        //MU_RUN_TEST(cram_test_get_free_bank_basic);
    }
}

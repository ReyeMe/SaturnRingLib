
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
        // Placeholder for any necessary test initialization
        // Future implementations might include resetting CRAM state,
        // clearing buffers, or preparing test environments
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
        // Edge: AllocationMask should default to zero (no banks used).
        // This is SRL-side bookkeeping; it does not touch hardware.
        for (uint16_t bank = 0; bank < 8; bank++)
        {
            snprintf(buffer, buffer_size, "Bank %u unexpectedly marked used", (unsigned)bank);
            mu_assert(!CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted256), buffer);
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
        MU_RUN_TEST(cram_test_palette_rgb555_invariants);
        MU_RUN_TEST(cram_test_palette_paletted16_size_and_stride);
    }
}

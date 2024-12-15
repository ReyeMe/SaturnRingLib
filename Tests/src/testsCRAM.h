
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
            LogInfo("****UT_CRAM_ERROR(S)****");
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
        void *baseAddress = (void *)CRAM::BaseAddress;
        snprintf(buffer, buffer_size, "BaseAddress not initialized correctly: %p", baseAddress);
        mu_assert(baseAddress != nullptr, buffer);
    }

    // Test: Setting and getting a color in CRAM
    // MU_TEST(cram_test_set_get_color)
    // {
    //     CRAM cram;
    //     HighColor inputColor = {1, 31, 15, 7}; // Opaque color with specific values
    //     uint16_t index = 5;
    //
    //     cram.SetColor(index, inputColor); // Assuming SetColor is implemented
    //     HighColor retrievedColor = cram.GetColor(index); // Assuming GetColor is implemented
    //
    //     snprintf(buffer, buffer_size, "Set/Get color failed: Red != %d", inputColor.Red);
    //     mu_assert(retrievedColor.Red == inputColor.Red, buffer);
    //     snprintf(buffer, buffer_size, "Set/Get color failed: Green != %d", inputColor.Green);
    //     mu_assert(retrievedColor.Green == inputColor.Green, buffer);
    //     snprintf(buffer, buffer_size, "Set/Get color failed: Blue != %d", inputColor.Blue);
    //     mu_assert(retrievedColor.Blue == inputColor.Blue, buffer);
    //     snprintf(buffer, buffer_size, "Set/Get color failed: Opaque != %d", inputColor.Opaque);
    //     mu_assert(retrievedColor.Opaque == inputColor.Opaque, buffer);
    // }

    // Test: Switching texture color modes
    // MU_TEST(cram_test_texture_color_mode) {
    //     CRAM cram;
    //     cram.SetTextureColorMode(CRAM::TextureColorMode::Paletted256); // Assuming SetTextureColorMode is implemented
    //     CRAM::TextureColorMode mode = cram.GetTextureColorMode(); // Assuming GetTextureColorMode is implemented
    //
    //     snprintf(buffer, buffer_size, "Texture color mode not set correctly: %d != Paletted256", (uint16_t)mode);
    //     mu_assert(mode == CRAM::TextureColorMode::Paletted256, buffer);
    // }

    // Test: Invalid color index
    // MU_TEST(cram_test_invalid_color_index) {
    //     CRAM cram;
    //     uint16_t invalidIndex = 1024; // Assuming CRAM size < 1024
    //     HighColor color = {1, 15, 15, 15};
    //
    //     bool success = cram.SetColor(invalidIndex, color); // Assuming SetColor returns a success flag
    //     snprintf(buffer, buffer_size, "Setting invalid index did not fail: index = %d", invalidIndex);
    //     mu_assert(!success, buffer);
    // }

    // Test: Invalid texture mode
    // MU_TEST(cram_test_invalid_texture_mode) {
    //     CRAM cram;
    //     uint16_t invalidMode = 9999; // Nonexistent mode value
    //
    //     bool success = cram.SetTextureColorMode(static_cast<CRAM::TextureColorMode>(invalidMode));
    //     snprintf(buffer, buffer_size, "Setting invalid texture mode did not fail: mode = %d", invalidMode);
    //     mu_assert(!success, buffer);
    // }

    // Test: Maximum color index
    // MU_TEST(cram_test_max_color_index) {
    //     CRAM cram;
    //     uint16_t maxIndex = 255; // Assuming CRAM supports 256 entries
    //     HighColor color = {1, 31, 31, 31};
    //
    //     bool success = cram.SetColor(maxIndex, color); // Assuming SetColor is implemented
    //     snprintf(buffer, buffer_size, "Setting max index failed: index = %d", maxIndex);
    //     mu_assert(success, buffer);
    //
    //     HighColor retrievedColor = cram.GetColor(maxIndex); // Assuming GetColor is implemented
    //     snprintf(buffer, buffer_size, "Retrieved color does not match for max index");
    //     mu_assert(retrievedColor.Red == color.Red &&
    //               retrievedColor.Green == color.Green &&
    //               retrievedColor.Blue == color.Blue &&
    //               retrievedColor.Opaque == color.Opaque, buffer);
    // }

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
        // MU_RUN_TEST(cram_test_set_get_color);
        // MU_RUN_TEST(cram_test_texture_color_mode);
        // MU_RUN_TEST(cram_test_invalid_color_index);
        // MU_RUN_TEST(cram_test_invalid_texture_mode);
        // MU_RUN_TEST(cram_test_max_color_index);
    }
}

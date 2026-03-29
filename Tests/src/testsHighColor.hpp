
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    /**
     * @brief Sets up the environment for HighColor unit tests.
     */
    void highcolor_test_setup(void)
    {
        // Placeholder for any future test initialization needs
    }

    /**
     * @brief Cleans up the environment after each HighColor unit test.
     */
    void highcolor_test_teardown(void)
    {
        // Placeholder for any future test cleanup requirements
    }

    /**
     * @brief Displays a header for the HighColor test suite upon the first error.
     */
    void highcolor_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_HIGHCOLOR****");
            }
            else
            {
                LogInfo("****UT_HIGHCOLOR_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Tests the initialization of a HighColor object with specific channel values.
     * @details Verifies that a HighColor object is correctly initialized with predefined values
     *          for its opacity, red, green, and blue channels.
     */
    MU_TEST(highcolor_test_initialization)
    {
        // Create a HighColor instance with specific channel values
        HighColor color = {1, 31, 15, 0}; // Opaque, Blue: 31, Green: 15, Red: 0

        // Validate each color channel and opacity setting
        snprintf(buffer, buffer_size, "Initialization failed: Opaque != 1");
        mu_assert(color.Opaque == 1, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Green != 15");
        mu_assert(color.Green == 15, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Red != 0");
        mu_assert(color.Red == 0, buffer);
    }

    /**
     * @brief Tests the initialization of a HighColor object with maximum channel values.
     * @details Verifies that all color channels (R, G, B) can be set to their maximum value of 31.
     */
    MU_TEST(highcolor_test_max_values)
    {
        // Create a HighColor instance with maximum channel values
        HighColor color = {1, 31, 31, 31}; // Opaque, all channels at max

        // Validate that all channels are set to their maximum value
        snprintf(buffer, buffer_size, "Max value test failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "Max value test failed: Green != 31");
        mu_assert(color.Green == 31, buffer);
        snprintf(buffer, buffer_size, "Max value test failed: Red != 31");
        mu_assert(color.Red == 31, buffer);
    }

    /**
     * @brief Tests the initialization of a HighColor object with minimum channel values.
     * @details Verifies that all color channels (R, G, B) and opacity can be set to their minimum value of 0.
     */
    MU_TEST(highcolor_test_min_values)
    {
        // Create a HighColor instance with minimum channel values
        HighColor color = {0, 0, 0, 0}; // Transparent, all channels at min

        // Validate that all channels are set to their minimum value
        snprintf(buffer, buffer_size, "Min value test failed: Opaque != 0");
        mu_assert(color.Opaque == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Blue != 0");
        mu_assert(color.Blue == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Green != 0");
        mu_assert(color.Green == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Red != 0");
        mu_assert(color.Red == 0, buffer);
    }

    /**
     * @brief Tests the ability to toggle the opacity of a HighColor object.
     * @details Verifies that the `Opaque` member can be changed from 1 (opaque) to 0 (transparent)
     *          and back again after initialization.
     */
    MU_TEST(highcolor_test_toggle_opacity)
    {
        // Create an initially opaque HighColor instance
        HighColor color = {1, 15, 15, 15}; // Initially opaque
        color.Opaque = 0;                  // Toggle to transparent

        // Validate opacity can be set to transparent
        snprintf(buffer, buffer_size, "Opacity toggle failed: Opaque != 0");
        mu_assert(color.Opaque == 0, buffer);

        // Toggle back to opaque and validate
        color.Opaque = 1; // Toggle back to opaque
        snprintf(buffer, buffer_size, "Opacity toggle failed: Opaque != 1");
        mu_assert(color.Opaque == 1, buffer);
    }

    /**
     * @brief Tests the color blending functionality between two HighColor objects.
     * @details Verifies that the `Blend` method correctly averages the RGB channel values of two colors.
     */
    MU_TEST(highcolor_test_blending)
    {
        // Create two distinct color instances for blending
        HighColor color1 = {1, 31, 0, 0}; // Pure blue
        HighColor color2 = {1, 0, 31, 0}; // Pure green

        // Validate the blended color's channel values
        HighColor blended = color1.Blend(color2); // Assuming Blend is implemented
        snprintf(buffer, buffer_size, "Blending failed: Blue != 15");
        mu_assert(blended.Blue == 15, buffer);
        snprintf(buffer, buffer_size, "Blending failed: Green != 15");
        mu_assert(blended.Green == 15, buffer);
        snprintf(buffer, buffer_size, "Blending failed: Red != 0");
        mu_assert(blended.Red == 0, buffer);
    }

    /**
     * @brief Tests the conversion of a HighColor object to its 16-bit integer representation.
     * @details Verifies that the `GetABGR` method correctly packs the color channels into a
     *          `uint16_t` in ABGR format.
     */
    MU_TEST(highcolor_test_to_integer)
    {
        // Create a maximum intensity color instance
        HighColor color = {1, 31, 31, 31}; // Max color
        uint16_t intValue = color.GetABGR();
        // Validate the integer conversion matches expected value
        snprintf(buffer, buffer_size, "ToInteger failed: %d != 0xFFFF", intValue);
        mu_assert(intValue == 0xFFFF, buffer);
    }

    /**
     * @brief Tests the creation of a HighColor object from a 16-bit integer.
     * @details Verifies that the `FromARGB15` static method correctly unpacks a `uint16_t`
     *          into the respective color channels of a HighColor object.
     */
    MU_TEST(highcolor_test_from_integer)
    {
        // Create a 16-bit integer representing a max color
        uint16_t intValue = 0xFFFF;                        // Max color
        HighColor color = HighColor::FromARGB15(intValue); // Assuming FromInteger is implemented

        // Validate that color channels are correctly reconstructed
        snprintf(buffer, buffer_size, "FromInteger failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "FromInteger failed: Green != 31");
        mu_assert(color.Green == 31, buffer);
        snprintf(buffer, buffer_size, "FromInteger failed: Red != 31");
        mu_assert(color.Red == 31, buffer);
    }

    /**
     * @brief Defines the test suite for all HighColor functionality.
     */
    MU_TEST_SUITE(highcolor_test_suite)
    {
        // Configure test suite with setup, teardown, and error reporting functions
        MU_SUITE_CONFIGURE_WITH_HEADER(&highcolor_test_setup,
                                       &highcolor_test_teardown,
                                       &highcolor_test_output_header);

        // Register individual test cases for execution
        MU_RUN_TEST(highcolor_test_initialization);
        MU_RUN_TEST(highcolor_test_max_values);
        MU_RUN_TEST(highcolor_test_min_values);
        MU_RUN_TEST(highcolor_test_toggle_opacity);
        MU_RUN_TEST(highcolor_test_blending);
        MU_RUN_TEST(highcolor_test_to_integer);
        MU_RUN_TEST(highcolor_test_from_integer);
    }
}

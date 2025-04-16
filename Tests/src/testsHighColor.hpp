
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
     * @brief Preparation routine for HighColor unit tests
     *
     * This function is called before each test in the HighColor test suite.
     * Currently serves as a placeholder for potential future test initialization
     * requirements, such as setting up test data or resetting test environment.
     */
    void highcolor_test_setup(void)
    {
        // Placeholder for any future test initialization needs
        // Can be expanded to include specific setup operations
        // for more complex HighColor testing scenarios
    }

    /**
     * @brief Cleanup routine for HighColor unit tests
     *
     * This function is called after each test in the HighColor test suite.
     * Currently serves as a placeholder for potential resource release
     * or state reset operations that might be needed during testing.
     */
    void highcolor_test_teardown(void)
    {
        // Placeholder for any future test cleanup requirements
        // Can be used to free resources, reset global states,
        // or perform any necessary post-test operations
    }

    /**
     * @brief Error reporting header for HighColor test suite
     *
     * Prints a standardized error header when the first test failure occurs.
     * Uses a global error counter to ensure the header is printed only once
     * during a test suite execution, preventing redundant error messages.
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
     * @brief Test the initialization of HighColor with specific values
     *
     * Verifies that a HighColor object can be correctly initialized
     * with predefined values for opacity, blue, green, and red channels.
     * Ensures that each channel is set to the expected value during creation.
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
     * @brief Test the maximum value limits for color channels
     *
     * Verifies that a HighColor object can be initialized with
     * maximum values (31) for all color channels, including opacity.
     * Ensures the color representation handles maximum intensity correctly.
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
     * @brief Test the minimum value limits for color channels
     *
     * Verifies that a HighColor object can be initialized with
     * minimum values (0) for all color channels, including opacity.
     * Ensures the color representation handles minimum intensity correctly.
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
     * @brief Test the opacity toggling functionality
     *
     * Verifies that the opacity of a HighColor object can be
     * dynamically changed between opaque (1) and transparent (0) states.
     * Ensures the opacity setting can be modified after initial creation.
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
     * @brief Test the color blending functionality
     *
     * Verifies that the Blend method correctly combines two separate
     * color instances by averaging their respective color channel values.
     * Ensures that color mixing produces the expected intermediate color.
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
     * @brief Test conversion of HighColor to 16-bit integer representation
     *
     * Verifies that the GetABGR method correctly converts a HighColor
     * instance to its corresponding 16-bit integer (ABGR) format.
     * Ensures accurate bit-level color representation conversion.
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
     * @brief Test conversion of 16-bit integer to HighColor representation
     *
     * Verifies that the FromARGB15 method correctly reconstructs a HighColor
     * instance from its 16-bit integer representation.
     * Ensures accurate bit-level color reconstruction from integer format.
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
     * @brief Configure and register HighColor test suite
     *
     * Sets up the test suite with initialization, cleanup, and error reporting
     * functions. Registers all individual test cases to be executed during
     * the HighColor unit testing process.
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

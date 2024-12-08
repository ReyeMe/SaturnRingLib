
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    void highcolor_test_setup(void)
    {
        // Setup for HighColor tests, if necessary
    }

    void highcolor_test_teardown(void)
    {
        // Cleanup for HighColor tests, if necessary
    }

    void highcolor_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_HIGHCOLOR_ERROR(S)****");
        }
    }

    // Test: Initialize HighColor with specific values
    MU_TEST(highcolor_test_initialization)
    {
        HighColor color = {1, 31, 15, 0}; // Opaque, Blue: 31, Green: 15, Red: 0
        snprintf(buffer, buffer_size, "Initialization failed: Opaque != 1");
        mu_assert(color.Opaque == 1, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Green != 15");
        mu_assert(color.Green == 15, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: Red != 0");
        mu_assert(color.Red == 0, buffer);
    }

    // Test: Maximum values for each channel
    MU_TEST(highcolor_test_max_values)
    {
        HighColor color = {1, 31, 31, 31}; // Opaque, all channels at max
        snprintf(buffer, buffer_size, "Max value test failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "Max value test failed: Green != 31");
        mu_assert(color.Green == 31, buffer);
        snprintf(buffer, buffer_size, "Max value test failed: Red != 31");
        mu_assert(color.Red == 31, buffer);
    }

    // Test: Minimum values for each channel
    MU_TEST(highcolor_test_min_values)
    {
        HighColor color = {0, 0, 0, 0}; // Transparent, all channels at min
        snprintf(buffer, buffer_size, "Min value test failed: Opaque != 0");
        mu_assert(color.Opaque == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Blue != 0");
        mu_assert(color.Blue == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Green != 0");
        mu_assert(color.Green == 0, buffer);
        snprintf(buffer, buffer_size, "Min value test failed: Red != 0");
        mu_assert(color.Red == 0, buffer);
    }

    // Test: Toggle opacity
    MU_TEST(highcolor_test_toggle_opacity)
    {
        HighColor color = {1, 15, 15, 15}; // Initially opaque
        color.Opaque = 0;                  // Toggle to transparent
        snprintf(buffer, buffer_size, "Opacity toggle failed: Opaque != 0");
        mu_assert(color.Opaque == 0, buffer);
        color.Opaque = 1; // Toggle back to opaque
        snprintf(buffer, buffer_size, "Opacity toggle failed: Opaque != 1");
        mu_assert(color.Opaque == 1, buffer);
    }

    // Test: Blending two colors
    MU_TEST(highcolor_test_blending)
    {
        HighColor color1 = {1, 31, 0, 0};         // Pure blue
        HighColor color2 = {1, 0, 31, 0};         // Pure green
        HighColor blended = color1.Blend(color2); // Assuming Blend is implemented
        snprintf(buffer, buffer_size, "Blending failed: Blue != 15");
        mu_assert(blended.Blue == 15, buffer);
        snprintf(buffer, buffer_size, "Blending failed: Green != 15");
        mu_assert(blended.Green == 15, buffer);
        snprintf(buffer, buffer_size, "Blending failed: Red != 0");
        mu_assert(blended.Red == 0, buffer);
    }

    // Test: Conversion to 16-bit integer
    MU_TEST(highcolor_test_to_integer)
    {
        HighColor color = {1, 31, 31, 31}; // Max color
        uint16_t intValue = color.GetABGR();
        snprintf(buffer, buffer_size, "ToInteger failed: %d != 0xFFFF", intValue);
        mu_assert(intValue == 0xFFFF, buffer);
    }

    // Test: Conversion from 16-bit integer
    MU_TEST(highcolor_test_from_integer)
    {
        uint16_t intValue = 0xFFFF;                        // Max color
        HighColor color = HighColor::FromARGB15(intValue); // Assuming FromInteger is implemented
        snprintf(buffer, buffer_size, "FromInteger failed: Blue != 31");
        mu_assert(color.Blue == 31, buffer);
        snprintf(buffer, buffer_size, "FromInteger failed: Green != 31");
        mu_assert(color.Green == 31, buffer);
        snprintf(buffer, buffer_size, "FromInteger failed: Red != 31");
        mu_assert(color.Red == 31, buffer);
    }

    MU_TEST_SUITE(highcolor_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&highcolor_test_setup,
                                       &highcolor_test_teardown,
                                       &highcolor_test_output_header);

        MU_RUN_TEST(highcolor_test_initialization);
        MU_RUN_TEST(highcolor_test_max_values);
        MU_RUN_TEST(highcolor_test_min_values);
        MU_RUN_TEST(highcolor_test_toggle_opacity);
        MU_RUN_TEST(highcolor_test_blending);
        MU_RUN_TEST(highcolor_test_to_integer);
        MU_RUN_TEST(highcolor_test_from_integer);
    }
}

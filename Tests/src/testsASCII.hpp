#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_bitmap.hpp> // for IBitmap

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Logger;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    // UT setup function, called before every tests
    void ascii_test_setup(void)
    {
        // Initialization logic, if necessary
    }

    // UT teardown function, called after every tests
    void ascii_test_teardown(void)
    {
        // Cleanup logic,
        ASCII::Clear();
        ASCII::SetPalette(0);
    }

    // UT output header function, called on the first test failure
    void ascii_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_ASCII****");
            }
            else
            {
                LogInfo("****UT_ASCII_ERROR(S)****");
            }
        }
    }

    // Test basic text display functionality
    // Verifies that simple text can be printed at a specific screen coordinate
    MU_TEST(ascii_test_display_simple_text)
    {
        const char *text = "Hello, World!";
        bool success = ASCII::Print(text, 0, 0); // Top-left corner
        snprintf(buffer, buffer_size, "Text display failed at (0, 0) for: %s", text);
        mu_assert(success, buffer);
    }

    // Test out-of-bounds text display handling
    // Ensures the ASCII display correctly handles attempts to print outside screen boundaries
    MU_TEST(ascii_test_display_out_of_bounds)
    {
        const char *text = "Out of bounds!";
        bool success = ASCII::Print(text, 127, 89); // Assuming these are out-of-bounds
        snprintf(buffer, buffer_size, "Out-of-bounds text display did not fail as expected");
        mu_assert(!success, buffer);
    }

    // Test color palette application
    // Verifies that the ASCII display can successfully set a color palette
    MU_TEST(ascii_test_apply_color_palette)
    {
        int paletteId = 2;
        bool success = ASCII::SetPalette(paletteId);
        snprintf(buffer, buffer_size, "Color palette application failed for palette ID: %d", paletteId);
        mu_assert(success, buffer);
    }

    // Test loading a font
    // Verifies that a font can be loaded into the ASCII display
    // MU_TEST(ascii_test_load_font)
    // {
    //     SRL::Bitmap::IBitmap* bmp = /* Initialize your bitmap here */;
    //     ASCII::LoadFont(bmp, 0);
    //     // Add assertions to verify the font was loaded correctly
    //     // For example, you can check specific memory locations or other indicators
    //     mu_assert(/* condition */, "Font loading failed");
    // }

    // Test loading a font using LoadFontSG
    // Verifies that a font can be loaded into the ASCII display using LoadFontSG
    // MU_TEST(ascii_test_load_font_sg)
    // {
    //     uint8_t source[0x1800] = {0}; // Initialize with appropriate data
    //     ASCII::LoadFontSG(source, 0);
    //     // Add assertions to verify the font was loaded correctly
    //     // For example, you can check specific memory locations or other indicators
    //     mu_assert(/* condition */, "Font loading failed");
    // }

    // Define the test suite for ASCII-related functionality
    // Configures and runs a comprehensive set of tests for the ASCII display class
    MU_TEST_SUITE(ascii_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&ascii_test_setup,
                                       &ascii_test_teardown,
                                       &ascii_test_output_header);

        MU_RUN_TEST(ascii_test_display_simple_text);
        MU_RUN_TEST(ascii_test_display_out_of_bounds);
        MU_RUN_TEST(ascii_test_apply_color_palette);
//        MU_RUN_TEST(ascii_test_load_font);
//        MU_RUN_TEST(ascii_test_load_font_sg);
    }
}

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
    extern uint32_t suite_error_counter;

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

    /**
     * @brief Tests the basic functionality of displaying a simple text string.
     * @details This test verifies that a standard text string can be printed successfully at a valid
     *          on-screen coordinate (in this case, the top-left corner).
     */
    MU_TEST(ascii_test_display_simple_text)
    {
        // Nominal: Print should succeed for in-bounds coordinates.
        const char *text = "Hello, World!";
        bool success = ASCII::Print(text, 0, 0); // Top-left corner
        snprintf(buffer, buffer_size, "Text display failed at (0, 0) for: %s", text);
        mu_assert(success, buffer);
    }

    /**
     * @brief Tests the handling of attempts to display text outside of the screen boundaries.
     * @details This test ensures that the ASCII display class correctly identifies and reports
     *          attempts to print text at coordinates that are off-screen.
     */
    MU_TEST(ascii_test_display_out_of_bounds)
    {
        // Negative: Print should fail when coordinates are out-of-bounds.
        // Note: SRL clamps internally, but still returns false to signal invalid input.
        const char *text = "Out of bounds!";
        bool success = ASCII::Print(text, 127, 89); // Assuming these are out-of-bounds
        snprintf(buffer, buffer_size, "Out-of-bounds text display did not fail as expected");
        mu_assert(!success, buffer);
    }

    /**
     * @brief Tests the ability to apply a valid color palette to the ASCII display.
     * @details This test verifies that setting a valid color palette by its index is a successful operation.
     */
    MU_TEST(ascii_test_apply_color_palette)
    {
        // Nominal: valid palette index should succeed.
        int paletteId = 2;
        bool success = ASCII::SetPalette(paletteId);
        snprintf(buffer, buffer_size, "Color palette application failed for palette ID: %d", paletteId);
        mu_assert(success, buffer);
    }

    /**
     * @brief Tests that setting a color palette with an out-of-range index is correctly handled.
     * @details This test ensures that `SetPalette` returns `false` when given an index that
     *          exceeds the valid range of palettes.
     */
    MU_TEST(ascii_test_set_palette_out_of_range)
    {
        // Negative: out-of-range palette index should return false.
        // We do not assert the clamped value because the internal state is private.
        bool success = ASCII::SetPalette(255);
        snprintf(buffer, buffer_size, "SetPalette(255) unexpectedly succeeded");
        mu_assert(!success, buffer);
    }

    /**
     * @brief Tests that setting a font with an out-of-range index is correctly handled.
     * @details This test ensures that `SetFont` returns `false` when given an index that
     *          exceeds the valid range of loaded fonts.
     */
    MU_TEST(ascii_test_set_font_out_of_range)
    {
        // Negative: out-of-range font index should return false.
        bool success = ASCII::SetFont(255);
        snprintf(buffer, buffer_size, "SetFont(255) unexpectedly succeeded");
        mu_assert(!success, buffer);
        // Reset font to valid value for subsequent tests
        ASCII::SetFont(0);
    }

    /**
     * @brief Tests that setting a color with an out-of-range index is correctly handled.
     * @details This test ensures that `SetColor` returns `false` when given an index that
     *          exceeds the valid range of colors within a palette.
     */
    MU_TEST(ascii_test_set_color_out_of_range)
    {
        // Negative: out-of-range color index should return false.
        bool success = ASCII::SetColor(0x7FFF, 255);
        snprintf(buffer, buffer_size, "SetColor(out-of-range) unexpectedly succeeded");
        mu_assert(!success, buffer);
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

    /**
     * @brief Defines the test suite for all ASCII-related functionality.
     * @details This suite configures and runs a comprehensive set of tests for the ASCII display class,
     *          covering text printing, bounds checking, and color/font management.
     */
    MU_TEST_SUITE(ascii_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&ascii_test_setup,
                                       &ascii_test_teardown,
                                       &ascii_test_output_header);

        MU_RUN_TEST(ascii_test_display_simple_text);
        MU_RUN_TEST(ascii_test_display_out_of_bounds);
        MU_RUN_TEST(ascii_test_apply_color_palette);
        MU_RUN_TEST(ascii_test_set_palette_out_of_range);
        MU_RUN_TEST(ascii_test_set_font_out_of_range);
        MU_RUN_TEST(ascii_test_set_color_out_of_range);
//        MU_RUN_TEST(ascii_test_load_font);
//        MU_RUN_TEST(ascii_test_load_font_sg);
    }
}
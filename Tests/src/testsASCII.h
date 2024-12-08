#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Logger;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    void ascii_test_setup(void)
    {
        // Initialization logic, if necessary
    }

    void ascii_test_teardown(void)
    {
        // Cleanup logic, if necessary
    }

    void ascii_test_output_header(void)
    {
        if(!suite_error_counter++)
        {
            LogInfo("****UT_ASCII_ERROR(S)****");
        }
    }

    MU_TEST(ascii_test_display_simple_text)
    {
        ASCII display;
        const char *text = "Hello, World!";
        bool success = display.Print(text, 0, 0); // Top-left corner
        snprintf(buffer, buffer_size, "Text display failed at (0, 0) for: %s", text);
        mu_assert(success, buffer);
    }

    MU_TEST(ascii_test_display_out_of_bounds)
    {
        ASCII display;
        const char *text = "Out of bounds!";
        bool success = display.Print(text, 127, 89); // Assuming these are out-of-bounds
        snprintf(buffer, buffer_size, "Out-of-bounds text display did not fail as expected");
        mu_assert(!success, buffer);
    }

    // MU_TEST(ascii_test_load_font) {
    //     ASCII display;
    //     int fontId = 1;
    //     bool success = display.LoadFont(fontId, "font_path");
    //     snprintf(buffer, buffer_size, "Font loading failed for font ID: %d", fontId);
    //     mu_assert(success, buffer);
    // }

    MU_TEST(ascii_test_apply_color_palette)
    {
        ASCII display;
        int paletteId = 2;
        bool success = display.SetPalette(paletteId);
        snprintf(buffer, buffer_size, "Color palette application failed for palette ID: %d", paletteId);
        mu_assert(success, buffer);
    }

    // MU_TEST(ascii_test_display_with_font_and_palette) {
    //     ASCII display;
    //     display.LoadFont(1, "font_path");
    //     display.ApplyColorPalette(2);
    //     const char* text = "Styled Text!";
    //     bool success = display.DisplayText(text, 10, 10);
    //     snprintf(buffer, buffer_size, "Styled text display failed at (10, 10) for: %s", text);
    //     mu_assert(success, buffer);
    // }

    // MU_TEST(ascii_test_invalid_character_handling) {
    //     ASCII display;
    //     const char* text = "Invalid\xFFChar!";
    //     bool success = display.Print(text, 0, 0);
    //     snprintf(buffer, buffer_size, "Invalid character handling failed for text: %s", text);
    //     mu_assert(!success, buffer);
    // }

    MU_TEST_SUITE(ascii_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&ascii_test_setup,
                                        &ascii_test_teardown,
                                        &ascii_test_output_header);

        MU_RUN_TEST(ascii_test_display_simple_text);
        MU_RUN_TEST(ascii_test_display_out_of_bounds);
        // MU_RUN_TEST(ascii_test_load_font);
        MU_RUN_TEST(ascii_test_apply_color_palette);
        // MU_RUN_TEST(ascii_test_display_with_font_and_palette);
        // MU_RUN_TEST(ascii_test_invalid_character_handling);
    }
}

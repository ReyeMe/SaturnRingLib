
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

    void cram_test_setup(void)
    {
        // Setup for CRAM tests, if necessary
    }

    void cram_test_teardown(void)
    {
        // Cleanup for CRAM tests, if necessary
    }

    // Test: BaseAddress initialization
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

    MU_TEST_SUITE(cram_test_suite)
    {
        MU_SUITE_CONFIGURE(&cram_test_setup, &cram_test_teardown);

        MU_RUN_TEST(cram_test_base_address);
        // MU_RUN_TEST(cram_test_set_get_color);
        // MU_RUN_TEST(cram_test_texture_color_mode);
        // MU_RUN_TEST(cram_test_invalid_color_index);
        // MU_RUN_TEST(cram_test_invalid_texture_mode);
        // MU_RUN_TEST(cram_test_max_color_index);
    }
}

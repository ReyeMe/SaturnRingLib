#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_bitmap.hpp"
#include "srl_color.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {

extern const uint8_t buffer_size;
extern char buffer[];

    /**
 * @brief Test the initialization of the Palette struct
 *
 * Verifies that the Palette struct is properly initialized with the correct
 * number of colors and that the Colors array is allocated correctly.
 */
MU_TEST(palette_test_initialization)
{
    size_t colorCount = 10;
    SRL::Bitmap::Palette palette(colorCount);

    snprintf(buffer, buffer_size, "Palette count not initialized correctly: %zu", palette.Count);
    mu_assert(palette.Count == colorCount, buffer);

    snprintf(buffer, buffer_size, "Palette colors not allocated correctly: %p", palette.Colors);
    mu_assert(palette.Colors != nullptr, buffer);
}

    /**
 * @brief Test the destruction of the Palette struct
 *
 * Deleting a palette frees its colors and itself: the heap has as many used blocks as before.
 */
MU_TEST(palette_test_destruction)
{
    const size_t before = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    SRL::Bitmap::Palette* palette = new SRL::Bitmap::Palette(10);
    delete palette;
    const size_t after = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;

    snprintf(buffer, buffer_size, "Used blocks %d -> %d after deleting a palette", before, after);
    mu_assert(after == before, buffer);
}

    /**
 * @brief Test the initialization of the BitmapInfo struct without a palette
 *
 * Verifies that the BitmapInfo struct is properly initialized with the correct
 * width, height, and default color mode.
 */
MU_TEST(bitmap_info_test_initialization_no_palette)
{
    uint16_t width = 100;
    uint16_t height = 200;
    SRL::Bitmap::BitmapInfo bitmapInfo(width, height);

    snprintf(buffer, buffer_size, "BitmapInfo width not initialized correctly: %u", bitmapInfo.Width);
    mu_assert(bitmapInfo.Width == width, buffer);

    snprintf(buffer, buffer_size, "BitmapInfo height not initialized correctly: %u", bitmapInfo.Height);
    mu_assert(bitmapInfo.Height == height, buffer);

    snprintf(buffer, buffer_size, "BitmapInfo color mode not initialized correctly: %d", bitmapInfo.ColorMode);
    mu_assert(bitmapInfo.ColorMode == SRL::CRAM::TextureColorMode::RGB555, buffer);
}

    /**
 * @brief Test the initialization of the BitmapInfo struct with a palette
 *
 * Verifies that the BitmapInfo struct is properly initialized with the correct
 * width, height, palette, and color mode based on the palette size.
 */
MU_TEST(bitmap_info_test_initialization_with_palette)
{
    uint16_t width = 100;
    uint16_t height = 200;
    size_t colorCount = 16;
    SRL::Bitmap::Palette palette(colorCount);
    SRL::Bitmap::BitmapInfo bitmapInfo(width, height, &palette);

    snprintf(buffer, buffer_size, "BitmapInfo width not initialized correctly: %u", bitmapInfo.Width);
    mu_assert(bitmapInfo.Width == width, buffer);

    snprintf(buffer, buffer_size, "BitmapInfo height not initialized correctly: %u", bitmapInfo.Height);
    mu_assert(bitmapInfo.Height == height, buffer);

    snprintf(buffer, buffer_size, "BitmapInfo palette not initialized correctly: %p", bitmapInfo.Palette);
    mu_assert(bitmapInfo.Palette == &palette, buffer);

    snprintf(buffer, buffer_size, "BitmapInfo color mode not initialized correctly: %d", bitmapInfo.ColorMode);
    mu_assert(bitmapInfo.ColorMode == SRL::CRAM::TextureColorMode::Paletted16, buffer);
}

    /**
 * @brief Test the GetData method of the IBitmap interface
 *
 * Verifies that the GetData method returns the correct data pointer.
 */
MU_TEST(ibitmap_test_get_data)
{
    uint8_t mockData[100];
    SRL::Bitmap::BitmapInfo mockInfo(100, 200);
    MockBitmap mockBitmap(mockData, mockInfo);

    snprintf(buffer, buffer_size, "IBitmap GetData method did not return the correct data pointer: %p", mockBitmap.GetData());
    mu_assert(mockBitmap.GetData() == mockData, buffer);
}

    /**
 * @brief Test the GetInfo method of the IBitmap interface
 *
 * Verifies that the GetInfo method returns the correct BitmapInfo object.
 */
MU_TEST(ibitmap_test_get_info)
{
    uint8_t mockData[100];
    SRL::Bitmap::BitmapInfo mockInfo(100, 200);
    MockBitmap mockBitmap(mockData, mockInfo);

    SRL::Bitmap::BitmapInfo returnedInfo = mockBitmap.GetInfo();

    snprintf(buffer, buffer_size, "IBitmap GetInfo method did not return the correct width: %u", returnedInfo.Width);
    mu_assert(returnedInfo.Width == mockInfo.Width, buffer);

    snprintf(buffer, buffer_size, "IBitmap GetInfo method did not return the correct height: %u", returnedInfo.Height);
    mu_assert(returnedInfo.Height == mockInfo.Height, buffer);

    snprintf(buffer, buffer_size, "IBitmap GetInfo method did not return the correct color mode: %d", returnedInfo.ColorMode);
    mu_assert(returnedInfo.ColorMode == mockInfo.ColorMode, buffer);
}

    /**
 * @brief bitmap test suite configuration and test case registration
 *
 * Configures the test suite with setup and teardown functions.
 * Registers individual test cases to be executed during the test run.
 * Currently runs the base address initialization test and palette tests.
 */
/**
 * @brief A palette built around the caller's colors (Palette(colors, count)) does not free them: the caller owns them.
 * @details The colors are a heap block here, so a wrong free shows up in the used block count instead of corrupting
 *          the heap (with a stack or static array, the destructor would hand the allocator a pointer it never gave).
 */
MU_TEST(palette_test_wrapped_colors_not_freed)
{
    SRL::Types::HighColor* colors = new SRL::Types::HighColor[16];
    const size_t before = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;

    {
        SRL::Bitmap::Palette palette(colors, 16);
    }

    const size_t after = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    if (after == before)
    {
        delete[] colors;
    }

    snprintf(buffer, buffer_size, "Used blocks %d -> %d: the palette freed the caller's colors", (int)before, (int)after);
    mu_assert(after == before, buffer);
}

MU_TEST_SUITE(bitmap_test_suite)
{
        // Register test cases to be executed
    MU_RUN_TEST(palette_test_initialization);
    MU_RUN_TEST(palette_test_destruction);
    MU_RUN_TEST(palette_test_wrapped_colors_not_freed);
    MU_RUN_TEST(bitmap_info_test_initialization_no_palette);
    MU_RUN_TEST(bitmap_info_test_initialization_with_palette);
    MU_RUN_TEST(ibitmap_test_get_data);
    MU_RUN_TEST(ibitmap_test_get_info);
}
}

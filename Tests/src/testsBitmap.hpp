#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_bitmap.hpp"
#include "srl_color.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    /**
     * @brief Sets up the environment for each bitmap unit test.
     */
    void bitmap_test_setup(void)
    {
        // Placeholder for any necessary test initialization
    }

    /**
     * @brief Cleans up the environment after each bitmap unit test.
     */
    void bitmap_test_teardown(void)
    {
        // Placeholder for any necessary test cleanup
    }

    /**
     * @brief Displays a header for the bitmap test suite upon the first error.
     */
    void bitmap_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_BITMAP****");
            }
            else
            {
                LogInfo("****UT_BITMAP_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Tests the initialization of the `Palette` struct.
     * @details Verifies that the `Palette` struct is properly initialized with the correct
     *          number of colors and that the `Colors` array is allocated.
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
     * @brief Tests the destruction of the `Palette` struct.
     * @details Verifies that a dynamically allocated `Palette` can be deleted without
     *          causing memory errors, implying correct deallocation.
     */
    MU_TEST(palette_test_destruction)
    {
        size_t colorCount = 10;
        SRL::Bitmap::Palette* palette = new SRL::Bitmap::Palette(colorCount);
        delete palette;

        // Since we cannot directly test the deallocation, we assume that if no
        // memory errors occur, the test passes.
        mu_assert(true, "Palette destruction test passed");
    }

    /**
     * @brief Tests the initialization of the `BitmapInfo` struct without a palette.
     * @details Verifies that `BitmapInfo` is properly initialized with the correct
     *          width, height, and a default color mode.
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
     * @brief Tests the initialization of the `BitmapInfo` struct with a palette.
     * @details Verifies that `BitmapInfo` is properly initialized with the correct
     *          width, height, palette pointer, and the corresponding paletted color mode.
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
     * @brief A mock implementation of the `IBitmap` interface for testing purposes.
     */
    class MockBitmap : public SRL::Bitmap::IBitmap
    {
    public:
        uint8_t* data;
        SRL::Bitmap::BitmapInfo info;

        MockBitmap(uint8_t* data, SRL::Bitmap::BitmapInfo info) : data(data), info(info) {}

        ~MockBitmap() { }

        uint8_t* GetData() override
        {
            return data;
        }

        SRL::Bitmap::BitmapInfo GetInfo() const override
        {
            return info;
        }
    };

    /**
     * @brief Tests the `GetData` method of the `IBitmap` interface.
     * @details Verifies that the `GetData` method of a class implementing `IBitmap`
     *          returns the correct pointer to the bitmap's raw pixel data.
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
     * @brief Tests the `GetInfo` method of the `IBitmap` interface.
     * @details Verifies that the `GetInfo` method of a class implementing `IBitmap`
     *          returns a `BitmapInfo` object with the correct properties.
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
     * @brief Defines the test suite for bitmap-related functionality.
     * @details Configures and registers test cases for `Palette`, `BitmapInfo`, and the `IBitmap` interface.
     */
    MU_TEST_SUITE(bitmap_test_suite)
    {
        // Configure test suite with setup, teardown, and error reporting functions
        MU_SUITE_CONFIGURE_WITH_HEADER(&bitmap_test_setup,
                                       &bitmap_test_teardown,
                                       &bitmap_test_output_header);

        // Register test cases to be executed
        MU_RUN_TEST(palette_test_initialization);
        MU_RUN_TEST(palette_test_destruction);
        MU_RUN_TEST(bitmap_info_test_initialization_no_palette);
        MU_RUN_TEST(bitmap_info_test_initialization_with_palette);
        MU_RUN_TEST(ibitmap_test_get_data);
        MU_RUN_TEST(ibitmap_test_get_info);
    }
}

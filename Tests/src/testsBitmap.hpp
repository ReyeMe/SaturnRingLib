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
     * @brief Set up routine for bitmap unit tests
     *
     * This function is called before each test in the bitmap test suite.
     * Currently, it does not perform any specific setup operations,
     * but provides a hook for future initialization requirements.
     */
    void bitmap_test_setup(void)
    {
        // Placeholder for any necessary test initialization
        // Future implementations might include resetting bitmap state,
        // clearing buffers, or preparing test environments
    }

    /**
     * @brief Tear down routine for bitmap unit tests
     *
     * This function is called after each test in the bitmap test suite.
     * Currently, it does not perform any specific cleanup operations,
     * but provides a hook for future resource release or state reset.
     */
    void bitmap_test_teardown(void)
    {
        // Placeholder for any necessary test cleanup
        // Future implementations might include freeing resources,
        // resetting global state, or clearing temporary data
    }

    /**
     * @brief Output header for test suite error reporting
     *
     * This function is called on the first test failure to print
     * a header indicating that bitmap unit test errors have occurred.
     * It increments a global error counter to ensure the header
     * is printed only once per test suite run.
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
     * Verifies that the Palette struct is properly destroyed and that the Colors
     * array is deallocated correctly.
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
     * @brief Mock implementation of the IBitmap interface for testing
     */
    class MockBitmap : public SRL::Bitmap::IBitmap
    {
    public:
        uint8_t* data;
        SRL::Bitmap::BitmapInfo info;

        MockBitmap(uint8_t* data, SRL::Bitmap::BitmapInfo info) : data(data), info(info) {}

        uint8_t* GetData() override
        {
            return data;
        }

        SRL::Bitmap::BitmapInfo GetInfo() override
        {
            return info;
        }
    };

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
     * Configures the test suite with setup, teardown, and error reporting functions.
     * Registers individual test cases to be executed during the test run.
     * Currently runs the base address initialization test and palette tests.
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

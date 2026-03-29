#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Types;
using namespace SRL::Math::Types;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    /**
     * @brief Sets up the environment for math-related unit tests.
     */
    void math_test_setup(void)
    {
        // Placeholder for any future test initialization needs
    }

    /**
     * @brief Cleans up the environment after each math-related unit test.
     */
    void math_test_teardown(void)
    {
        // Placeholder for any future test cleanup requirements
    }

    /**
     * @brief Displays a header for the math test suite upon the first error.
     */
    void math_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_MATH****");
            }
            else
            {
                LogInfo("****UT_MATH_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Tests the trigonometric sine function for standard angles (0, 90, 180, 360 degrees).
     * @details Verifies that the calculated sine values match their expected mathematical results
     *          within a small tolerance.
     */
    MU_TEST(math_test_sin_standard_angles)
    {
        // Calculate sine for standard angles using degree-based conversion
        Fxp sin_0 = Math::Trigonometry::Sin(Angle::FromDegrees(0));
        Fxp sin_90 = Math::Trigonometry::Sin(Angle::FromDegrees(90));
        Fxp sin_180 = Math::Trigonometry::Sin(Angle::FromDegrees(180));
        Fxp sin_360 = Math::Trigonometry::Sin(Angle::FromDegrees(360));

        // Validate sine values with a small floating-point tolerance
        snprintf(buffer, buffer_size, "Sin(0) failed: %f != 0.0", sin_0.As<float>());
        mu_assert(fabs(sin_0.As<float>() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(90) failed: %f != 1.0", sin_90.As<float>());
        mu_assert(fabs(sin_90.As<float>() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(180) failed: %f != 0.0", sin_180.As<float>());
        mu_assert(fabs(sin_180.As<float>() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(360) failed: %f != 0.0", sin_360.As<float>());
        mu_assert(fabs(sin_360.As<float>() - 0.0) < 1e-5, buffer);
    }

    /**
     * @brief Tests the trigonometric cosine function for standard angles (0, 90, 180, 360 degrees).
     * @details Verifies that the calculated cosine values match their expected mathematical results
     *          within a small tolerance.
     */
    MU_TEST(math_test_cos_standard_angles)
    {
        // Calculate cosine for standard angles using degree-based conversion
        Fxp cos_0 = Math::Trigonometry::Cos(Angle::FromDegrees(0));
        Fxp cos_90 = Math::Trigonometry::Cos(Angle::FromDegrees(90));
        Fxp cos_180 = Math::Trigonometry::Cos(Angle::FromDegrees(180));
        Fxp cos_360 = Math::Trigonometry::Cos(Angle::FromDegrees(360));

        // Validate cosine values with a small floating-point tolerance
        snprintf(buffer, buffer_size, "Cos(0) failed: %f != 1.0", cos_0.As<float>());
        mu_assert(fabs(cos_0.As<float>() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(90) failed: %f != 0.0", cos_90.As<float>());
        mu_assert(fabs(cos_90.As<float>() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(180) failed: %f != -1.0", cos_180.As<float>());
        mu_assert(fabs(cos_180.As<float>() - -1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(360) failed: %f != 1.0", cos_360.As<float>());
        mu_assert(fabs(cos_360.As<float>() - 1.0) < 1e-5, buffer);
    }

    /**
     * @brief Tests trigonometric functions for negative angles.
     * @details Validates that sine and cosine calculations for negative angles produce correct results,
     *          specifically testing with -90 degrees.
     */
    MU_TEST(math_test_negative_angles)
    {
        // Calculate sine and cosine for a negative angle
        Fxp sin_neg90 = Math::Trigonometry::Sin(Angle::FromDegrees(-90));
        Fxp cos_neg90 = Math::Trigonometry::Cos(Angle::FromDegrees(-90));

        // Validate trigonometric values for negative angle
        snprintf(buffer, buffer_size, "Sin(-90) failed: %f != -1.0", sin_neg90.As<float>());
        mu_assert(fabs(sin_neg90.As<float>() - -1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(-90) failed: %f != 0.0", cos_neg90.As<float>());
        mu_assert(fabs(cos_neg90.As<float>() - 0.0) < 1e-5, buffer);
    }

    /**
     * @brief Tests trigonometric functions for angles greater than 360 degrees.
     * @details Verifies that sine and cosine calculations correctly handle angle normalization
     *          by testing with an angle of 450 degrees (equivalent to 90 degrees).
     */
    MU_TEST(math_test_large_angles)
    {
        // Calculate sine and cosine for a large angle (450 degrees)
        Fxp sin_large = Math::Trigonometry::Sin(Angle::FromDegrees(450)); // 450° = 90° normalized
        Fxp cos_large = Math::Trigonometry::Cos(Angle::FromDegrees(450));

        // Validate trigonometric values for large angle
        snprintf(buffer, buffer_size, "Sin(450) failed: %f != 1.0", sin_large.As<float>());
        mu_assert(fabs(sin_large.As<float>() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(450) failed: %f != 0.0", cos_large.As<float>());
        mu_assert(fabs(cos_large.As<float>() - 0.0) < 1e-5, buffer);
    }

    /**
     * @brief Tests the precision of trigonometric functions for very small angles.
     * @details Evaluates sine and cosine for a small angle (0.1 degrees) to ensure the
     *          calculations are accurate to a high degree of precision.
     */
    MU_TEST(math_test_small_angles)
    {
        // Calculate sine and cosine for a very small angle
        Fxp sin_small = Math::Trigonometry::Sin(Angle::FromDegrees(0.1));
        Fxp cos_small = Math::Trigonometry::Cos(Angle::FromDegrees(0.1));

        // Validate trigonometric values for small angle with high precision
        snprintf(buffer, buffer_size, "Sin(0.1) precision check failed");
        mu_assert(fabs(sin_small.As<float>() - 0.00174533) < 1e-4, buffer);

        snprintf(buffer, buffer_size, "Cos(0.1) precision check failed");
        mu_assert(fabs(cos_small.As<float>() - 0.999998) < 1e-4, buffer);
    }

    /**
     * @brief Defines the test suite for all general math and trigonometry functionality.
     */
    MU_TEST_SUITE(math_test_suite)
    {
        // Configure test suite with setup, teardown, and error reporting functions
        MU_SUITE_CONFIGURE_WITH_HEADER(&math_test_setup,
                                       &math_test_teardown,
                                       &math_test_output_header);

        // Register individual test cases for execution
        MU_RUN_TEST(math_test_sin_standard_angles);
        MU_RUN_TEST(math_test_cos_standard_angles);
        MU_RUN_TEST(math_test_negative_angles);
        MU_RUN_TEST(math_test_large_angles);
        MU_RUN_TEST(math_test_small_angles);
    }
}

#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_math.hpp"
#include "srl_angle.hpp"

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
     * @brief Preparation routine for mathematical unit tests
     *
     * This function is called before each test in the Math test suite.
     * Currently serves as a placeholder for potential future test initialization
     * requirements, such as setting up test data or resetting test environment.
     * Provides a hook for any necessary pre-test setup operations.
     */
    void math_test_setup(void)
    {
        // Placeholder for potential future test initialization needs
        // Can be expanded to include specific setup operations
        // for more complex mathematical testing scenarios
    }

    /**
     * @brief Cleanup routine for mathematical unit tests
     *
     * This function is called after each test in the Math test suite.
     * Currently serves as a placeholder for potential resource release
     * or state reset operations that might be needed during testing.
     * Provides a mechanism for post-test cleanup and resource management.
     */
    void math_test_teardown(void)
    {
        // Placeholder for potential future test cleanup requirements
        // Can be used to free resources, reset global states,
        // or perform any necessary post-test operations
    }

    /**
     * @brief Error reporting header for Math test suite
     *
     * Prints a standardized error header when the first test failure occurs.
     * Utilizes a global error counter to ensure the header is printed only once
     * during a test suite execution, preventing redundant error messages.
     * Provides clear identification of mathematical unit test failures.
     */
    void math_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            LogInfo("****UT_MATH_ERROR(S)****");
        }
    }

    /**
     * @brief Test trigonometric sine function for standard angles
     *
     * Verifies the sine function's correctness for key standard angles:
     * 0°, 90°, 180°, and 360°. Checks that the calculated sine values
     * match the expected mathematical results within a small tolerance.
     *
     * Test cases cover:
     * - Zero angle (0°)
     * - Right angle (90°)
     * - Straight angle (180°)
     * - Full rotation (360°)
     */
    MU_TEST(math_test_sin_standard_angles)
    {
        // Calculate sine for standard angles using degree-based conversion
        Fxp sin_0 = Math::Sin(Angle::FromDegrees(0));
        Fxp sin_90 = Math::Sin(Angle::FromDegrees(90));
        Fxp sin_180 = Math::Sin(Angle::FromDegrees(180));
        Fxp sin_360 = Math::Sin(Angle::FromDegrees(360));

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
     * @brief Test trigonometric cosine function for standard angles
     *
     * Verifies the cosine function's correctness for key standard angles:
     * 0°, 90°, 180°, and 360°. Checks that the calculated cosine values
     * match the expected mathematical results within a small tolerance.
     *
     * Test cases cover:
     * - Zero angle (0°)
     * - Right angle (90°)
     * - Straight angle (180°)
     * - Full rotation (360°)
     */
    MU_TEST(math_test_cos_standard_angles)
    {
        // Calculate cosine for standard angles using degree-based conversion
        Fxp cos_0 = Math::Cos(Angle::FromDegrees(0));
        Fxp cos_90 = Math::Cos(Angle::FromDegrees(90));
        Fxp cos_180 = Math::Cos(Angle::FromDegrees(180));
        Fxp cos_360 = Math::Cos(Angle::FromDegrees(360));

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
     * @brief Test trigonometric functions for negative angles
     *
     * Validates sine and cosine calculations for negative angles,
     * specifically focusing on -90 degrees. Ensures that the mathematical
     * library correctly handles signed angle inputs and produces
     * mathematically accurate results.
     *
     * Test case covers:
     * - Sine of negative right angle (-90°)
     * - Cosine of negative right angle (-90°)
     */
    MU_TEST(math_test_negative_angles)
    {
        // Calculate sine and cosine for a negative angle
        Fxp sin_neg90 = Math::Sin(Angle::FromDegrees(-90));
        Fxp cos_neg90 = Math::Cos(Angle::FromDegrees(-90));

        // Validate trigonometric values for negative angle
        snprintf(buffer, buffer_size, "Sin(-90) failed: %f != -1.0", sin_neg90.As<float>());
        mu_assert(fabs(sin_neg90.As<float>() - -1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(-90) failed: %f != 0.0", cos_neg90.As<float>());
        mu_assert(fabs(cos_neg90.As<float>() - 0.0) < 1e-5, buffer);
    }

    /**
     * @brief Test trigonometric functions for large angle values
     *
     * Verifies sine and cosine calculations for angles beyond the standard
     * 360-degree range. Checks that the mathematical library correctly
     * normalizes large angles and produces expected trigonometric results.
     *
     * Test case covers:
     * - Sine of 450 degrees (equivalent to 90 degrees)
     * - Cosine of 450 degrees (equivalent to 90 degrees)
     */
    MU_TEST(math_test_large_angles)
    {
        // Calculate sine and cosine for a large angle (450 degrees)
        Fxp sin_large = Math::Sin(Angle::FromDegrees(450)); // 450° = 90° normalized
        Fxp cos_large = Math::Cos(Angle::FromDegrees(450));

        // Validate trigonometric values for large angle
        snprintf(buffer, buffer_size, "Sin(450) failed: %f != 1.0", sin_large.As<float>());
        mu_assert(fabs(sin_large.As<float>() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(450) failed: %f != 0.0", cos_large.As<float>());
        mu_assert(fabs(cos_large.As<float>() - 0.0) < 1e-5, buffer);
    }

    /**
     * @brief Test trigonometric functions for small angle precision
     *
     * Evaluates the mathematical library's precision for trigonometric
     * calculations with very small angle inputs. Ensures accurate
     * sine and cosine computations near zero degrees.
     *
     * Test case covers:
     * - Sine of a very small angle (0.1 degrees)
     * - Cosine of a very small angle (0.1 degrees)
     */
    MU_TEST(math_test_small_angles)
    {
        // Calculate sine and cosine for a very small angle
        Fxp sin_small = Math::Sin(Angle::FromDegrees(0.1));
        Fxp cos_small = Math::Cos(Angle::FromDegrees(0.1));

        // Validate trigonometric values for small angle with high precision
        snprintf(buffer, buffer_size, "Sin(0.1) precision check failed");
        mu_assert(fabs(sin_small.As<float>() - 0.00174533) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(0.1) precision check failed");
        mu_assert(fabs(cos_small.As<float>() - 0.999998) < 1e-5, buffer);
    }

    /**
     * @brief Configure and register mathematical test suite
     *
     * Sets up the test suite with initialization, cleanup, and error reporting
     * functions. Registers all individual trigonometric test cases to be
     * executed during the mathematical unit testing process.
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

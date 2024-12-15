#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Logger;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];
    extern uint32_t suite_error_counter;

    // UT setup function, called before every tests
    void angle_test_setup(void)
    {
        // Nothomg to do here
    }

    // UT teardown function, called after every tests
    void angle_test_teardown(void)
    {
        /* Nothing */
    }

    // UT output header function, called on the first test failure
    void angle_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_ANGLE_ERROR(S)****");
        }
    }

    // Test that an angle initialized with zero remains zero after conversion
    MU_TEST(angle_test_initialization_zero)
    {
        Fxp angle(0.0);
        Angle a1 = Angle::FromDegrees(angle);
        Fxp a2 = a1.AsRadians();
        snprintf(buffer, buffer_size, "%d != %d", angle.ToInt(), a2.ToInt());
        mu_assert(angle == a2, buffer);
    }

    // Test subtracting a quarter circle (90 degrees) from a half circle (180 degrees)
    MU_TEST(angle_test_subtraction_half_circle_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(180.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(90.0)) == a3, buffer);
    }

    // Test subtracting a quarter circle from zero degrees
    MU_TEST(angle_test_subtraction_zero_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(0.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != -90", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(-90.0)) == a3, buffer);
    }

    // Test subtracting zero from a quarter circle
    MU_TEST(angle_test_subtraction_quarter_circle_minus_zero)
    {
        Angle a1 = Angle::FromDegrees(Fxp(0.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a2 - a1;
        snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(90.0)) == a3, buffer);
    }

    // Test subtracting a quarter circle from a full circle
    MU_TEST(angle_test_subtraction_full_circle_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(360.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(270.0)) == a3, buffer);
    }

    // Test subtracting a quarter circle from two full circles
    MU_TEST(angle_test_subtraction_two_full_circles_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(720.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(270.0)) == a3, buffer);
    }

    // Test subtracting two full circles from a quarter circle
    MU_TEST(angle_test_subtraction_quarter_circle_minus_two_full_circles)
    {
        Angle a1 = Angle::FromDegrees(Fxp(720.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a2 - a1;
        snprintf(buffer, buffer_size, "%d != -630", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(-630.0)) == a3, buffer);
    }

    // Test adding two quarter circles (90 degrees each)
    MU_TEST(angle_test_addition_quarter_circle_plus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(90.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        Angle a3 = a1 + a2;
        snprintf(buffer, buffer_size, "%d != 180", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(180.0)) == a3, buffer);
    }

    // Test adding two half circles (180 degrees each)
    MU_TEST(angle_test_addition_half_circle_plus_half_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(180.0));
        Angle a2 = Angle::FromDegrees(Fxp(180.0));
        Angle a3 = a1 + a2;
        snprintf(buffer, buffer_size, "%d != 360", a3.AsDegrees().ToInt());
        mu_assert(Angle::FromDegrees(Fxp(360.0)) == a3, buffer);
    }

    // Test normalization of a positive angle greater than 360 degrees
    MU_TEST(angle_test_normalization_positive)
    {
        Angle a1 = Angle::FromDegrees(Fxp(450.0)); // 450 degrees should normalize to 90 degrees
        Angle normalized = a1;
        snprintf(buffer, buffer_size, "Normalization failed: %d != 90", normalized.AsDegrees().ToInt());
        mu_assert(normalized.AsDegrees() == Fxp(90.0), buffer);
    }

    // Test normalization of a negative angle
    MU_TEST(angle_test_normalization_negative)
    {
        Angle a1 = Angle::FromDegrees(Fxp(-90.0)); // -90 degrees should normalize to 270 degrees
        Angle normalized = a1;
        snprintf(buffer, buffer_size, "Normalization failed: %d != -90", normalized.AsDegrees().ToInt());
        mu_assert(normalized.AsDegrees() == Fxp(-90.0), buffer);
    }

    // Test arithmetic addition of two small angles
    MU_TEST(angle_test_arithmetic_addition)
    {
        Angle a1 = Angle::FromDegrees(Fxp(45.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 + a2;
        snprintf(buffer, buffer_size, "Addition failed: %d != 75", result.AsDegrees().ToInt());
        mu_assert(Fxp(74.9) < result.AsDegrees() && result.AsDegrees() < Fxp(75.1), buffer);
    }

    // Test arithmetic subtraction of two small angles
    MU_TEST(angle_test_arithmetic_subtraction)
    {
        Angle a1 = Angle::FromDegrees(Fxp(90.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 - a2;
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 60", result.AsDegrees().ToInt());
        mu_assert(Fxp(59.9) < result.AsDegrees() && result.AsDegrees() < Fxp(60.1), buffer);
    }

    // Test arithmetic multiplication of an angle
    MU_TEST(angle_test_arithmetic_multiplication)
    {
        Angle a1 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 * Angle::FromDegrees(Fxp::FromInt(2)); // Assuming multiplication is supported
        snprintf(buffer, buffer_size, "Multiplication failed: %d != 60", result.AsDegrees().ToInt());
        mu_assert(Fxp(59.9) < result.AsDegrees() && result.AsDegrees() < Fxp(60.1), buffer);
    }

    // Test arithmetic division of an angle
    MU_TEST(angle_test_arithmetic_division)
    {
        Angle a1 = Angle::FromDegrees(Fxp(60.0));
        Angle result = a1 / Angle::FromDegrees(Fxp::FromInt(2)); // Assuming division is supported
        snprintf(buffer, buffer_size, "Division failed: %d != 30", result.AsDegrees().ToInt());
        mu_assert(Fxp(29.9) < result.AsDegrees() && result.AsDegrees() < Fxp(30.1), buffer);
    }

    // Test greater than comparison between angles
    MU_TEST(angle_test_comparison_greater)
    {
        Angle a1 = Angle::FromDegrees(Fxp(90.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        snprintf(buffer, buffer_size, "Comparison failed: 90 <= 30");
        mu_assert(a1 > a2, buffer);
    }

    // Test less than comparison between angles
    MU_TEST(angle_test_comparison_less)
    {
        Angle a1 = Angle::FromDegrees(Fxp(30.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        snprintf(buffer, buffer_size, "Comparison failed: 30 >= 90");
        mu_assert(a1 < a2, buffer);
    }

    // Test conversion from degrees to radians
    MU_TEST(angle_test_conversion_to_radians)
    {
        Angle a1 = Angle::FromDegrees(Fxp(180.0));
        Fxp radians = a1.AsRadians();
        snprintf(buffer, buffer_size, "Conversion to radians failed: %d != 3.14159", radians.ToInt());
        mu_assert(Fxp::Abs(radians - 3.14159) < Fxp(1e-4), buffer);
    }

    // Test conversion from radians to degrees
    MU_TEST(angle_test_conversion_to_degrees)
    {
        Angle a1 = Angle::FromRadians(Fxp(3.14159));
        Fxp degrees = a1.AsDegrees();
        snprintf(buffer, buffer_size, "Conversion to degrees failed: %f != 180", degrees.ToInt());
        mu_assert(Fxp::Abs(degrees - 180.0) < Fxp(1e-4), buffer);
    }

    // Test handling of zero angle
    MU_TEST(angle_test_edge_case_zero)
    {
        Angle a1 = Angle::FromDegrees(Fxp(0.0));
        snprintf(buffer, buffer_size, "Zero angle failed: %d != 0", a1.AsDegrees().ToInt());
        mu_assert(a1.AsDegrees() == Fxp(0.0), buffer);
    }

    // Test handling of full circle angle
    MU_TEST(angle_test_edge_case_full_circle)
    {
        Angle a1 = Angle::FromDegrees(Fxp(360.0));
        snprintf(buffer, buffer_size, "Full circle failed: %d != 0", a1.AsDegrees().ToInt());
        mu_assert(a1.AsDegrees() == Fxp(0.0), buffer);
    }

    // Define the test suite with all unit tests
    MU_TEST_SUITE(angle_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&angle_test_setup,
                                       &angle_test_teardown,
                                       &angle_test_output_header);

        MU_RUN_TEST(angle_test_initialization_zero);
        MU_RUN_TEST(angle_test_subtraction_half_circle_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_zero_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_quarter_circle_minus_zero);
        MU_RUN_TEST(angle_test_subtraction_full_circle_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_two_full_circles_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_quarter_circle_minus_two_full_circles);
        MU_RUN_TEST(angle_test_addition_quarter_circle_plus_quarter_circle);
        MU_RUN_TEST(angle_test_addition_half_circle_plus_half_circle);

        // Additional tests
        MU_RUN_TEST(angle_test_normalization_positive);
        MU_RUN_TEST(angle_test_normalization_negative);
        MU_RUN_TEST(angle_test_arithmetic_addition);
        MU_RUN_TEST(angle_test_arithmetic_subtraction);
        MU_RUN_TEST(angle_test_arithmetic_multiplication);
        MU_RUN_TEST(angle_test_arithmetic_division);
        MU_RUN_TEST(angle_test_comparison_greater);
        MU_RUN_TEST(angle_test_comparison_less);
        MU_RUN_TEST(angle_test_conversion_to_radians);
        MU_RUN_TEST(angle_test_conversion_to_degrees);
        MU_RUN_TEST(angle_test_edge_case_zero);
        MU_RUN_TEST(angle_test_edge_case_full_circle);
    }
}

#include <srl.hpp>
#include <srl_log.hpp>

#include <climits>
#include <cfloat> 

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    // Setup function called before each test to prepare the test environment
    void fxp_test_setup(void)
    {
        // No initialization needed
    }

    // Teardown function called after each test to clean up the test environment
    void fxp_test_teardown(void)
    {
        // No cleanup required
    }

    // Outputs an error header when the first test failure occurs
    void fxp_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_FXP_ERROR(S)****");
        }
    }

    // Test: Verify fixed-point initialization with zero
    MU_TEST(fxp_initialization_zero)
    {
        Fxp a1 = 0;
        snprintf(buffer, buffer_size, "%d != 0", a1);
        mu_assert(a1 == 0, buffer);
    }

    // Test: Verify fixed-point initialization with one
    MU_TEST(fxp_initialization_one)
    {
        Fxp a1 = 1;
        snprintf(buffer, buffer_size, "%d != 1", a1);
        mu_assert(a1 == 1, buffer);
    }

    // Test: Verify the assignment operator for fixed-point numbers
    MU_TEST(fxp_assignment_operator)
    {
        Fxp a1 = 1;
        Fxp b1 = a1;
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    // Test: Verify the copy constructor for fixed-point numbers
    MU_TEST(fxp_copy_constructor)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    // Test: Verify equality comparison for fixed-point numbers
    MU_TEST(fxp_equality_check)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != %d", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    // Test: Verify initialization with double and float values
    MU_TEST(fxp_initialization_with_doubles)
    {
        Fxp a1(10.0);
        Fxp b1(10.0f);
        snprintf(buffer, buffer_size, "%f != %f", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    // Test: Verify inequality comparison for fixed-point numbers
    MU_TEST(fxp_inequality_check)
    {
        Fxp a1(10.0);
        Fxp b1(20.0f);
        snprintf(buffer, buffer_size, "%f == %d", a1, b1);
        mu_assert(a1 != b1, buffer);
    }

    // Test: Verify addition of fixed-point numbers
    MU_TEST(fxp_arithmetic_addition)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 + a2;
        snprintf(buffer, buffer_size, "%f + %f != %f", a1, a2, result);
        mu_assert(result == Fxp(15.75), buffer);
    }

    // Test: Verify subtraction of fixed-point numbers
    MU_TEST(fxp_arithmetic_subtraction)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 - a2;
        snprintf(buffer, buffer_size, "%f - %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.25), buffer);
    }

    // Test: Verify multiplication of fixed-point numbers
    MU_TEST(fxp_arithmetic_multiplication)
    {
        Fxp a1(3.0);
        Fxp a2(4.0);
        Fxp result = a1 * a2;
        snprintf(buffer, buffer_size, "%f * %f != %f", a1, a2, result);
        mu_assert(result == Fxp(12.0), buffer);
    }

    // Test: Verify division of fixed-point numbers
    MU_TEST(fxp_arithmetic_division)
    {
        Fxp a1(10.0);
        Fxp a2(2.0);
        Fxp result = a1 / a2;
        snprintf(buffer, buffer_size, "%f / %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.0), buffer);
    }

    // Test: Conversion of fixed-point number to float
    MU_TEST(fxp_conversion_to_float)
    {
        Fxp a1 = 10;
        float result = a1.As<float>();
        snprintf(buffer, buffer_size, "Conversion to float failed: %f != 10.0", result);
        mu_assert(result == 10.0f, buffer);
    }

    // Test: Verify maximum value of fixed-point number
    MU_TEST(fxp_max_value_check)
    {
        Fxp max = Fxp::MaxValue();
        snprintf(buffer, buffer_size, "Max value test failed: %f != Fxp::FxpMax", max);
        mu_assert(max == Fxp::MaxValue(), buffer);
    }

    // Test: Verify minimum value of fixed-point number
    MU_TEST(fxp_min_value_check)
    {
        Fxp min = Fxp::MinValue();
        snprintf(buffer, buffer_size, "Min value test failed: %f != Fxp::FxpMin", min);
        mu_assert(min == Fxp::MinValue(), buffer);
    }

    // Test: Modulo operation for positive numbers
    MU_TEST(fxp_ModuloTest_PositiveNumbers)
    {
        Fxp a1 = 10;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert((a1 % b1) == 1, buffer);

        a1 = 20;
        b1 = 5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert((a1 % b1) == 0, buffer);
    }

    // Test: Modulo operation with negative dividend
    MU_TEST(fxp_ModuloTest_NegativeDividend)
    {
        Fxp a1 = -10;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert((a1 % b1) == -1, buffer);

        a1 = -20;
        b1 = 5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert((a1 % b1) == 0, buffer);
    }

    // Test: Modulo operation with negative divisor
    MU_TEST(fxp_ModuloTest_NegativeDivisor)
    {
        Fxp a1 = 10;
        Fxp b1 = -3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert((a1 % b1) == 1, buffer);

        a1 = 20;
        b1 = -5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert((a1 % b1) == 0, buffer);
    }

    // Test: Modulo operation with both dividend and divisor negative
    MU_TEST(fxp_ModuloTest_NegativeDividendAndDivisor)
    {
        Fxp a1 = -10;
        Fxp b1 = -3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert((a1 % b1) == -1, buffer);

        a1 = -20;
        b1 = -5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert((a1 % b1) == 0, buffer);
    }

    // Test: Modulo operation with large numbers
    MU_TEST(fxp_ModuloTest_LargeNumbers)
    {
        Fxp a1 = SHRT_MAX;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert((a1 % b1) == 1, buffer);

        // FAILS : Mod value test failed: mod(-32767, 3) != -1
        // a1 = -SHRT_MAX;
        // b1 = 3;
        // snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        // mu_assert((a1 % b1) == -1, buffer);
    }

    // Test: Edge cases (smallest/largest integers)
    MU_TEST(fxp_ModuloTest_EdgeCases)
    {
        Fxp a1 = SHRT_MAX;
        Fxp b1 = 2;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert((a1 % b1) == 1, buffer);

        a1 = -SHRT_MAX;
        b1 = 2;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert((a1 % b1) == -1, buffer);
    }

    // Test: Positive numbers
    MU_TEST(fxp_GreaterThanTest_PositiveNumbers)
    {
        Fxp a1 = 5;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = 3;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = 3;
        b1 = 3;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Negative numbers
    MU_TEST(fxp_GreaterThanTest_NegativeNumbers)
    {
        Fxp a1 = -3;
        Fxp b1 = -5;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = -5;
        b1 = -3;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = -3;
        b1 = -3;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Mixed positive and negative numbers
    MU_TEST(fxp_GreaterThanTest_MixedNumbers)
    {
        Fxp a1 = 3;
        Fxp b1 = -5;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = -3;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Comparison with zero
    MU_TEST(fxp_GreaterThanTest_ComparisonWithZero)
    {
        Fxp a1 = 3;
        Fxp b1 = 0;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = 0;
        b1 = 3;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = 0;
        b1 = 0;
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Basic comparisons
    MU_TEST(fxp_GreaterThanFloatTest_BasicComparisons)
    {
        Fxp a1(5.5);
        Fxp b1(3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = Fxp(3.3);
        b1 = Fxp(5.5);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = Fxp(3.3);
        b1 = Fxp(3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Negative numbers
    MU_TEST(fxp_GreaterThanFloatTest_NegativeNumbers)
    {
        Fxp a1(-5.5);
        Fxp b1(-3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(b1 > a1, buffer);

        a1 = Fxp(-3.3);
        b1 = Fxp(-5.5);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);

        a1 = Fxp(-3.3);
        b1 = Fxp(-3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Mixed positive and negative numbers
    MU_TEST(fxp_GreaterThanFloatTest_MixedNumbers)
    {
        Fxp a1(5.5);
        Fxp b1(-3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = Fxp(-3.3);
        b1 = Fxp(5.5);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Comparison with zero
    MU_TEST(fxp_GreaterThanFloatTest_ComparisonWithZero)
    {
        Fxp a1(3.3);
        Fxp b1(0.0);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = Fxp(0.0);
        b1 = Fxp(-3.3);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);

        a1 = Fxp(0.0);
        b1 = Fxp(0.0);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Very small differences
    MU_TEST(fxp_GreaterThanFloatTest_VerySmallDifferences)
    {
        constexpr float a = 1.1f;
        constexpr float b = 1.0000000f;

        Fxp a1(a);
        Fxp b1(b);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);

        a1 = Fxp(1.01f);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);

        a1 = Fxp(1.001f);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);

        a1 = Fxp(1.0001f);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        mu_assert(!(b1 > a1), buffer);
    }

    // Test: Integer and negative float comparisons
    MU_TEST(fxp_GreaterThanMixedTest_IntAndNegativeFloat)
    {
        Fxp a1 = -3;
        Fxp b1(-5.5f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = -5;
        b1 = Fxp(-3.3f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = -3;
        b1 = Fxp(-3.0f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Mixed positive and negative values
    MU_TEST(fxp_GreaterThanMixedTest_MixedPositiveAndNegativeValues)
    {
        Fxp a1 = 5;
        Fxp b1(-3.3f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = -3;
        b1 = Fxp(5.5f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Integer comparison with zero float
    MU_TEST(fxp_GreaterThanMixedTest_IntWithZeroFloat)
    {
        Fxp a1 = 3;
        Fxp b1(0.0f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = 0;
        b1 = Fxp(3.3f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);

        a1 = 0;
        b1 = Fxp(0.0f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Precision edge cases
    MU_TEST(fxp_GreaterThanMixedTest_PrecisionEdgeCases)
    {
        Fxp a1 = 1;
        Fxp b1(0.9999999f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 > b1, buffer);

        a1 = 1;
        b1 = Fxp(1.0000001f);
        snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 > b1), buffer);
    }

    // Test: Less than comparison between fixed-point numbers
    MU_TEST(fxp_comparison_lessthan)
    {
        Fxp a1 = 5;
        Fxp b1 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 < b1, buffer);

        a1 = 10;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 < b1), buffer);

        a1 = 5;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 < b1), buffer);
    }

    // Test: Greater than or equal comparison between fixed-point numbers
    MU_TEST(fxp_comparison_greaterthan_or_equal)
    {
        Fxp a1 = 10;
        Fxp b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 >= b1, buffer);

        a1 = 5;
        b1 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 >= b1), buffer);

        a1 = 5;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 >= b1, buffer);
    }

    // Test: Less than or equal comparison between fixed-point numbers
    MU_TEST(fxp_comparison_lessthan_or_equal)
    {
        Fxp a1 = 5;
        Fxp b1 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 <= b1, buffer);

        a1 = 10;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d <= %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(!(a1 <= b1), buffer);

        a1 = 5;
        b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b1.As<int32_t>());
        mu_assert(a1 <= b1, buffer);
    }

    // Test: Greater than comparison between fixed-point and integer
    MU_TEST(fxp_comparison_greater_than_int)
    {
        Fxp a1 = 10;
        constexpr int b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d <= %d", a1.As<int32_t>(), b1);
        mu_assert(a1 > b1, buffer);

        a1 = 5;
        constexpr int b2 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b2);
        mu_assert(!(a1 > b2), buffer);

        a1 = 5;
        constexpr int b3 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b3);
        mu_assert(!(a1 > b3), buffer);
    }

    // Test: Less than comparison between fixed-point and integer
    MU_TEST(fxp_comparison_less_than_int)
    {
        Fxp a1 = 5;
        constexpr int b1 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %d", a1.As<int32_t>(), b1);
        mu_assert(a1 < b1, buffer);

        a1 = 10;
        constexpr int b2 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b2);
        mu_assert(!(a1 < b2), buffer);

        a1 = 5;
        constexpr int b3 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b3);
        mu_assert(!(a1 < b3), buffer);
    }

    // Test: Greater than or equal comparison between fixed-point and integer
    MU_TEST(fxp_comparison_greater_than_or_equal_int)
    {
        Fxp a1 = 10;
        constexpr int b1 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b1);
        mu_assert(a1 >= b1, buffer);

        a1 = 5;
        constexpr int b2 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %d", a1.As<int32_t>(), b2);
        mu_assert(!(a1 >= b2), buffer);

        a1 = 5;
        constexpr int b3 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %d", a1.As<int32_t>(), b3);
        mu_assert(a1 >= b3, buffer);
    }

    // Test: Less than or equal comparison between fixed-point and integer
    MU_TEST(fxp_comparison_less_than_or_equal_int)
    {
        Fxp a1 = 5;
        constexpr int b1 = 10;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b1);
        mu_assert(a1 <= b1, buffer);

        a1 = 10;
        constexpr int b2 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d <= %d", a1.As<int32_t>(), b2);
        mu_assert(!(a1 <= b2), buffer);

        a1 = 5;
        constexpr int b3 = 5;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %d", a1.As<int32_t>(), b3);
        mu_assert(a1 <= b3, buffer);
    }

    // Test: Greater than comparison between fixed-point and float
    MU_TEST(fxp_comparison_greater_than_float)
    {
        Fxp a1 = 10;
        constexpr float b1 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d <= %f", a1.As<int32_t>(), b1);
        mu_assert(a1 > b1, buffer);

        a1 = 5;
        constexpr float b2 = 10.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %f", a1.As<int32_t>(), b2);
        mu_assert(!(a1 > b2), buffer);

        a1 = 5;
        constexpr float b3 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %f", a1.As<int32_t>(), b3);
        mu_assert(!(a1 > b3), buffer);
    }

    // Test: Less than comparison between fixed-point and float
    MU_TEST(fxp_comparison_less_than_float)
    {
        Fxp a1 = 5;
        constexpr float b1 = 10.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %f", a1.As<int32_t>(), b1);
        mu_assert(a1 < b1, buffer);

        a1 = 10;
        constexpr float b2 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %f", a1.As<int32_t>(), b2);
        mu_assert(!(a1 < b2), buffer);

        a1 = 5;
        constexpr float b3 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %f", a1.As<int32_t>(), b3);
        mu_assert(!(a1 < b3), buffer);
    }

    // Test: Greater than or equal comparison between fixed-point and float
    MU_TEST(fxp_comparison_greater_than_or_equal_float)
    {
        Fxp a1 = 10;
        constexpr float b1 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %f", a1.As<int32_t>(), b1);
        mu_assert(a1 >= b1, buffer);

        a1 = 5;
        constexpr float b2 = 10.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d >= %f", a1.As<int32_t>(), b2);
        mu_assert(!(a1 >= b2), buffer);

        a1 = 5;
        constexpr float b3 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d < %f", a1.As<int32_t>(), b3);
        mu_assert(a1 >= b3, buffer);
    }

    // Test: Less than or equal comparison between fixed-point and float
    MU_TEST(fxp_comparison_less_than_or_equal_float)
    {
        Fxp a1 = 5;
        constexpr float b1 = 10.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %f", a1.As<int32_t>(), b1);
        mu_assert(a1 <= b1, buffer);

        a1 = 10;
        constexpr float b2 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d <= %f", a1.As<int32_t>(), b2);
        mu_assert(!(a1 <= b2), buffer);

        a1 = 5;
        constexpr float b3 = 5.0f;
        snprintf(buffer, buffer_size, "Comparison failed: %d > %f", a1.As<int32_t>(), b3);
        mu_assert(a1 <= b3, buffer);
    }

    // Test: Edge case for addition (overflow)
    // MU_TEST(fxp_arithmetic_addition_overflow)
    // {
    //     Fxp a1 = LLONG_MAX;
    //     Fxp a2 = 1;
    //     Fxp result = a1 + a2;
    //     snprintf(buffer, buffer_size, "Addition overflow test failed: %d + %d != %d", a1.As<int32_t>(), a2.As<int32_t>(), result.As<int32_t>());
    //     mu_assert(result == LLONG_MAX, buffer);
    // }

    // Test: Edge case for subtraction (underflow)
    // MU_TEST(fxp_arithmetic_subtraction_underflow)
    // {
    //     Fxp a1 = LLONG_MIN;
    //     Fxp a2 = 1;
    //     Fxp result = a1 - a2;
    //     snprintf(buffer, buffer_size, "Subtraction underflow test failed: %d - %d != %d", a1.As<int32_t>(), a2.As<int32_t>(), result.As<int32_t>());
    //     mu_assert(result == LLONG_MIN, buffer);
    // }

    // Test: Edge case for multiplication (overflow)
    // MU_TEST(fxp_arithmetic_multiplication_overflow)
    // {
    //     Fxp a1 = LLONG_MAX / 2;
    //     Fxp a2 = 3;
    //     Fxp result = a1 * a2;
    //     snprintf(buffer, buffer_size, "Multiplication overflow test failed: %d * %d != %d", a1.As<int32_t>(), a2.As<int32_t>(), result.As<int32_t>());
    //     mu_assert(result == LLONG_MAX, buffer);
    // }

    // Test: Edge case for division (division by zero)
    /*
MU_TEST(fxp_arithmetic_division_by_zero)
{
    Fxp a1 = 10;
    Fxp a2 = 0;
    try
    {
        Fxp result = a1 / a2;
        mu_fail("Division by zero did not throw an exception");
    }
    catch (const std::exception &e)
    {
        snprintf(buffer, buffer_size, "Division by zero test passed: %s", e.what());
        mu_assert(true, buffer);
    }
}
    */

    // Test: Edge case for division (minimum value divided by -1)
    // MU_TEST(fxp_arithmetic_division_min_by_negative_one)
    // {
    //     Fxp a1 = LLONG_MAX;
    //     Fxp a2 = -1;
    //     Fxp result = a1 / a2;
    //     snprintf(buffer, buffer_size, "Division min by -1 test failed: %d / %d != %d", a1.As<int32_t>(), a2.As<int32_t>(), result.As<int32_t>());
    //     mu_assert(result == LLONG_MAX, buffer);
    // }

    // Test: Edge case for addition with int (overflow)
    // MU_TEST(fxp_arithmetic_addition_int_overflow)
    // {
    //     Fxp a1 = LLONG_MAX;
    //     constexpr int a2 = 1;
    //     Fxp result = a1 + a2;
    //     snprintf(buffer, buffer_size, "Addition overflow test failed: %d + %d != %d", a1.As<int32_t>(), a2, result.As<int32_t>());
    //     mu_assert(result == LLONG_MAX, buffer);
    // }

    // Test: Edge case for subtraction with int (underflow)
    // MU_TEST(fxp_arithmetic_subtraction_int_underflow)
    // {
    //     Fxp a1 = LLONG_MIN;
    //     constexpr int a2 = 1;
    //     Fxp result = a1 - a2;
    //     snprintf(buffer, buffer_size, "Subtraction underflow test failed: %d - %d != %d", a1.As<int32_t>(), a2, result.As<int32_t>());
    //     mu_assert(result == LLONG_MIN, buffer);
    // }

    // Test: Edge case for multiplication with int (overflow)
    // MU_TEST(fxp_arithmetic_multiplication_int_overflow)
    // {
    //     Fxp a1 = LLONG_MAX / 2;
    //     constexpr int a2 = 3;
    //     Fxp result = a1 * a2;
    //     snprintf(buffer, buffer_size, "Multiplication overflow test failed: %d * %d != %d", a1.As<int32_t>(), a2, result.As<int32_t>());
    //     mu_assert(result == LLONG_MAX, buffer);
    // }

    // Test: Edge case for division with int (division by zero)
    /*
MU_TEST(fxp_arithmetic_division_int_by_zero)
{
    Fxp a1 = 10;
    int a2 = 0;
    try
    {
        Fxp result = a1 / a2;
        mu_fail("Division by zero did not throw an exception");
    }
    catch (const std::exception &e)
    {
        snprintf(buffer, buffer_size, "Division by zero test passed: %s", e.what());
        mu_assert(true, buffer);
    }
}
    */

    // Test: Edge case for addition with float (overflow)
    // MU_TEST(fxp_arithmetic_addition_float_overflow)
    // {
    //     Fxp a1 = FLT_MAX;
    //     constexpr float a2 = 1.0f;
    //     Fxp result = a1 + a2;
    //     snprintf(buffer, buffer_size, "Addition overflow test failed: %f + %f != %f", a1.As<float>(), a2, result.As<float>());
    //     mu_assert(result == FLT_MAX, buffer);
    // }

    // Test: Edge case for subtraction with float (underflow)
    // MU_TEST(fxp_arithmetic_subtraction_float_underflow)
    // {
    //     Fxp a1 = FLT_MIN;
    //     constexpr float a2 = 1.0f;
    //     Fxp result = a1 - a2;
    //     snprintf(buffer, buffer_size, "Subtraction underflow test failed: %f - %f != %f", a1.As<float>(), a2, result.As<float>());
    //     mu_assert(result == FLT_MIN, buffer);
    // }

    // Test: Edge case for multiplication with float (overflow)
    // MU_TEST(fxp_arithmetic_multiplication_float_overflow)
    // {
    //     Fxp a1 = FLT_MAX / 2.0f;
    //     constexpr float a2 = 3.0f;
    //     Fxp result = a1 * a2;
    //     snprintf(buffer, buffer_size, "Multiplication overflow test failed: %f * %f != %f", a1.As<float>(), a2, result.As<float>());
    //     mu_assert(result == FLT_MAX, buffer);
    // }

    // Test: Edge case for division with float (division by zero)
    /*
MU_TEST(fxp_arithmetic_division_float_by_zero)
{
    Fxp a1 = 10.0f;
    float a2 = 0.0f;
    try
    {
        Fxp result = a1 / a2;
        mu_fail("Division by zero did not throw an exception");
    }
    catch (const std::exception &e)
    {
        snprintf(buffer, buffer_size, "Division by zero test passed: %s", e.what());
        mu_assert(true, buffer);
    }
}
    */

    // Test: Edge case for addition with double (overflow)
    // MU_TEST(fxp_arithmetic_addition_double_overflow)
    // {
    //     Fxp a1 = DBL_MAX;
    //     constexpr double a2 = 1.0;
    //     Fxp result = a1 + a2;
    //     snprintf(buffer, buffer_size, "Addition overflow test failed: %f + %f != %f", a1.As<double>(), a2, result.As<double>());
    //     mu_assert(result == DBL_MAX, buffer);
    // }

    // Test: Edge case for subtraction with double (underflow)
    // MU_TEST(fxp_arithmetic_subtraction_double_underflow)
    // {
    //     Fxp a1 = DBL_MIN;
    //     constexpr double a2 = 1.0;
    //     Fxp result = a1 - a2;
    //     snprintf(buffer, buffer_size, "Subtraction underflow test failed: %f - %f != %f", a1.As<double>(), a2, result.As<double>());
    //     mu_assert(result == DBL_MIN, buffer);
    // }

    // Test: Edge case for multiplication with double (overflow)
    // MU_TEST(fxp_arithmetic_multiplication_double_overflow)
    // {
    //     Fxp a1 = DBL_MAX / 2.0;
    //     constexpr double a2 = 3.0;
    //     Fxp result = a1 * a2;
    //     snprintf(buffer, buffer_size, "Multiplication overflow test failed: %f * %f != %f", a1.As<double>(), a2, result.As<double>());
    //     mu_assert(result == DBL_MAX, buffer);
    // }

    // Test: Edge case for division with double (division by zero)
    /*
MU_TEST(fxp_arithmetic_division_double_by_zero)
{
    Fxp a1 = 10.0;
    double a2 = 0.0;
    try
    {
        Fxp result = a1 / a2;
        mu_fail("Division by zero did not throw an exception");
    }
    catch (const std::exception &e)
    {
        snprintf(buffer, buffer_size, "Division by zero test passed: %s", e.what());
        mu_assert(true, buffer);
    }
}
    */

    // Test: Verify fixed-point initialization with unsigned int
    MU_TEST(fxp_initialization_unsigned_int)
    {
        constexpr unsigned int value = 10;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%u != %u", a1.As<unsigned int>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with int
    MU_TEST(fxp_initialization_int)
    {
        constexpr int value = -10;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with float
    MU_TEST(fxp_initialization_float)
    {
        constexpr float value = 10.5f;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%f != %f", a1.As<float>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with double
    MU_TEST(fxp_initialization_double)
    {
        constexpr double value = 20.25;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%f != %f", a1.As<double>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with char
    MU_TEST(fxp_initialization_char)
    {
        constexpr char value = 'A';
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with bool
    MU_TEST(fxp_initialization_bool)
    {
        constexpr bool value1 = true;
        Fxp a1 = value1;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value1);
        mu_assert(a1 == value1, buffer);

        constexpr bool value2 = false;
        a1 = value2;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value2);
        mu_assert(a1 == value2, buffer);
    }

    // Test: Verify fixed-point initialization with short
    MU_TEST(fxp_initialization_short)
    {
        short value = 32767;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<short>(), value);
        mu_assert(a1 == value, buffer);
    }

    // Test: Verify fixed-point initialization with long
    // MU_TEST(fxp_initialization_long)
    // {
    //     long value = 2147483647;
    //     Fxp a1 = value;
    //     snprintf(buffer, buffer_size, "%ld != %ld", a1.As<long>(), value);
    //     mu_assert(a1 == value, buffer);
    // }

    // Test: Verify fixed-point initialization with long long
    // MU_TEST(fxp_initialization_long_long)
    // {
    //     long long value = 9223372036854775807LL;
    //     Fxp a1 = value;
    //     snprintf(buffer, buffer_size, "%lld != %lld", a1.As<long long>(), value);
    //     mu_assert(a1 == value, buffer);
    // }

    MU_TEST_SUITE(fxp_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&fxp_test_setup,
                                       &fxp_test_teardown,
                                       &fxp_test_output_header);

        MU_RUN_TEST(fxp_initialization_zero);
        MU_RUN_TEST(fxp_initialization_one);
        MU_RUN_TEST(fxp_initialization_unsigned_int);
        MU_RUN_TEST(fxp_initialization_int);
        MU_RUN_TEST(fxp_initialization_float);
        MU_RUN_TEST(fxp_initialization_double);
        MU_RUN_TEST(fxp_initialization_char);
        MU_RUN_TEST(fxp_initialization_bool);
        MU_RUN_TEST(fxp_initialization_short);
        //MU_RUN_TEST(fxp_initialization_long);
        //MU_RUN_TEST(fxp_initialization_long_long);
        MU_RUN_TEST(fxp_assignment_operator);
        MU_RUN_TEST(fxp_copy_constructor);
        MU_RUN_TEST(fxp_equality_check);
        MU_RUN_TEST(fxp_initialization_with_doubles);
        MU_RUN_TEST(fxp_inequality_check);
        MU_RUN_TEST(fxp_arithmetic_addition);
        MU_RUN_TEST(fxp_arithmetic_subtraction);
        MU_RUN_TEST(fxp_arithmetic_multiplication);
        MU_RUN_TEST(fxp_arithmetic_division);
        MU_RUN_TEST(fxp_conversion_to_float);
        MU_RUN_TEST(fxp_max_value_check);
        MU_RUN_TEST(fxp_min_value_check);

        MU_RUN_TEST(fxp_ModuloTest_PositiveNumbers);
        MU_RUN_TEST(fxp_ModuloTest_NegativeDividend); //Mod value test failed: mod(-10, 3) != -1
        MU_RUN_TEST(fxp_ModuloTest_NegativeDivisor);
        MU_RUN_TEST(fxp_ModuloTest_NegativeDividendAndDivisor); //Mod value test failed: mod(-10, -3) != -1
        MU_RUN_TEST(fxp_ModuloTest_LargeNumbers);
        MU_RUN_TEST(fxp_ModuloTest_EdgeCases);

        MU_RUN_TEST(fxp_GreaterThanTest_PositiveNumbers);
        MU_RUN_TEST(fxp_GreaterThanTest_NegativeNumbers);
        MU_RUN_TEST(fxp_GreaterThanTest_MixedNumbers);
        MU_RUN_TEST(fxp_GreaterThanTest_ComparisonWithZero);

        MU_RUN_TEST(fxp_GreaterThanFloatTest_BasicComparisons);
        MU_RUN_TEST(fxp_GreaterThanFloatTest_NegativeNumbers);
        MU_RUN_TEST(fxp_GreaterThanFloatTest_MixedNumbers);
        MU_RUN_TEST(fxp_GreaterThanFloatTest_ComparisonWithZero);
        MU_RUN_TEST(fxp_GreaterThanFloatTest_VerySmallDifferences);

        MU_RUN_TEST(fxp_GreaterThanMixedTest_IntAndNegativeFloat);
        MU_RUN_TEST(fxp_GreaterThanMixedTest_MixedPositiveAndNegativeValues);
        MU_RUN_TEST(fxp_GreaterThanMixedTest_IntWithZeroFloat);
        MU_RUN_TEST(fxp_GreaterThanMixedTest_PrecisionEdgeCases);

        // Additional comparison tests
        MU_RUN_TEST(fxp_comparison_lessthan);
        MU_RUN_TEST(fxp_comparison_greaterthan_or_equal);
        MU_RUN_TEST(fxp_comparison_lessthan_or_equal);
        MU_RUN_TEST(fxp_comparison_greater_than_int);
        MU_RUN_TEST(fxp_comparison_less_than_int);
        MU_RUN_TEST(fxp_comparison_greater_than_or_equal_int);
        MU_RUN_TEST(fxp_comparison_less_than_or_equal_int);
        MU_RUN_TEST(fxp_comparison_greater_than_float);
        MU_RUN_TEST(fxp_comparison_less_than_float);
        MU_RUN_TEST(fxp_comparison_greater_than_or_equal_float);
        MU_RUN_TEST(fxp_comparison_less_than_or_equal_float);

        // Edge case tests
        //MU_RUN_TEST(fxp_arithmetic_division_min_by_negative_one);
        //MU_RUN_TEST(fxp_arithmetic_addition_int_overflow);
        //MU_RUN_TEST(fxp_arithmetic_subtraction_int_underflow);
        //MU_RUN_TEST(fxp_arithmetic_multiplication_int_overflow);
        // MU_RUN_TEST(fxp_arithmetic_division_int_by_zero);
        //MU_RUN_TEST(fxp_arithmetic_addition_float_overflow);
        //MU_RUN_TEST(fxp_arithmetic_subtraction_float_underflow);
        //MU_RUN_TEST(fxp_arithmetic_multiplication_float_overflow);
        // MU_RUN_TEST(fxp_arithmetic_division_float_by_zero);
        //MU_RUN_TEST(fxp_arithmetic_addition_double_overflow);
        //MU_RUN_TEST(fxp_arithmetic_subtraction_double_underflow);
        //MU_RUN_TEST(fxp_arithmetic_multiplication_double_overflow);
        // MU_RUN_TEST(fxp_arithmetic_division_double_by_zero);
    }
}

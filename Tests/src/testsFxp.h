#include <srl.hpp>
#include <srl_log.hpp>

#include <climits>
#include <limits> // std::numeric_limits

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Logger;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    // Setup function called before each test to prepare the test environment
    // Currently no specific setup is required for these tests
    void fxp_test_setup(void)
    {
        // No initialization needed
    }

    // Teardown function called after each test to clean up the test environment
    // Currently no specific teardown actions are performed
    void fxp_test_teardown(void)
    {
        /* No cleanup required */
    }

    // Outputs an error header when the first test failure occurs
    // Helps in identifying when a test suite encounters errors
    void fxp_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_FXP_ERROR(S)****");
        }
    }

    // Test: Verify fixed-point initialization with zero
    // Ensures that a fixed-point number initialized with zero is correctly represented
    MU_TEST(fxp_initialization_zero)
    {
        Fxp a1 = 0;
        snprintf(buffer, buffer_size, "%d != 0", a1);
        mu_assert(a1 == 0, buffer);
    }

    // Test: Verify fixed-point initialization with one
    // Checks that a fixed-point number initialized with one is correctly represented
    MU_TEST(fxp_initialization_one)
    {
        Fxp a1 = 1;
        snprintf(buffer, buffer_size, "%d != 1", a1);
        mu_assert(a1 == 1, buffer);
    }

    // Test: Verify the assignment operator for fixed-point numbers
    // Ensures that a fixed-point number can be correctly assigned to another variable
    MU_TEST(fxp_assignment_operator)
    {
        Fxp a1 = 1;
        Fxp b1 = a1;
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    // Test: Verify the copy constructor for fixed-point numbers
    // Checks that a new fixed-point number can be created as a copy of another
    MU_TEST(fxp_copy_constructor)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    // Test: Verify equality comparison for fixed-point numbers
    // Ensures that two fixed-point numbers with the same value are considered equal
    MU_TEST(fxp_equality_check)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != %d", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    // Test: Verify initialization with double and float values
    // Checks that fixed-point numbers can be created from both double and float types
    MU_TEST(fxp_initialization_with_doubles)
    {
        Fxp a1(10.0);
        Fxp b1(10.0f);
        snprintf(buffer, buffer_size, "%f != %f", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    // Test: Verify inequality comparison for fixed-point numbers
    // Ensures that different fixed-point numbers are correctly identified as unequal
    MU_TEST(fxp_inequality_check)
    {
        Fxp a1(10.0);
        Fxp b1(20.0f);
        snprintf(buffer, buffer_size, "%f == %d", a1, b1);
        mu_assert(a1 != b1, buffer);
    }

    // Test: Verify less than or equal comparison
    // Checks that less than or equal comparison works correctly for fixed-point numbers
    MU_TEST(fxp_comparison_lessthan_or_equal)
    {
        Fxp a1(10.0);
        snprintf(buffer, buffer_size, "%f > 20", a1);
        mu_assert(a1 <= 20, buffer);
    }

    // Test: Verify addition of fixed-point numbers
    // Checks that addition of fixed-point numbers produces the correct result
    MU_TEST(fxp_arithmetic_addition)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 + a2;
        snprintf(buffer, buffer_size, "%f + %f != %f", a1, a2, result);
        mu_assert(result == Fxp(15.75), buffer);
    }

    // Test: Verify subtraction of fixed-point numbers
    // Ensures that subtraction of fixed-point numbers yields the correct result
    MU_TEST(fxp_arithmetic_subtraction)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 - a2;
        snprintf(buffer, buffer_size, "%f - %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.25), buffer);
    }

    // Test: Verify multiplication of fixed-point numbers
    // Checks that multiplication of fixed-point numbers produces the correct result
    MU_TEST(fxp_arithmetic_multiplication)
    {
        Fxp a1(3.0);
        Fxp a2(4.0);
        Fxp result = a1 * a2;
        snprintf(buffer, buffer_size, "%f * %f != %f", a1, a2, result);
        mu_assert(result == Fxp(12.0), buffer);
    }

    // Test: Verify division of fixed-point numbers
    // Ensures that division of fixed-point numbers yields the correct result
    MU_TEST(fxp_arithmetic_division)
    {
        Fxp a1(10.0);
        Fxp a2(2.0);
        Fxp result = a1 / a2;
        snprintf(buffer, buffer_size, "%f / %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.0), buffer);
    }

    // Test: Conversion of fixed-point number to float
    // Checks that converting a fixed-point number to float preserves its value
    MU_TEST(fxp_conversion_to_float)
    {
        Fxp a1 = 10;
        float result = a1.As<float>();
        snprintf(buffer, buffer_size, "Conversion to float failed: %f != 10.0", result);
        mu_assert(result == 10.0f, buffer);
    }

    // Test: Verify maximum value of fixed-point number
    // Ensures that the maximum representable fixed-point value is correctly returned
    MU_TEST(fxp_max_value_check)
    {
        Fxp max = Fxp::MaxValue();
        snprintf(buffer, buffer_size, "Max value test failed: %f != Fxp::FxpMax", max);
        mu_assert(max == Fxp::MaxValue(), buffer);
    }

    // Test: Verify minimum value of fixed-point number
    // Checks that the minimum representable fixed-point value is correctly returned
    MU_TEST(fxp_min_value_check)
    {
        Fxp min = Fxp::MinValue();
        snprintf(buffer, buffer_size, "Min value test failed: %f != Fxp::FxpMin", min);
        mu_assert(min == Fxp::MinValue(), buffer);
    }

    // Test series: Modulo operation for positive numbers
    // Verifies the modulo operation works correctly for positive integers
    MU_TEST(fxp_ModuloTest_PositiveNumbers)
    {
        // Scenario 1: Non-divisible numbers
        Fxp a1 = 10;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert(Fxp::Mod(a1, b1) == 1, buffer);

        // Scenario 2: Perfectly divisible numbers
        a1 = 20;
        b1 = 5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert(Fxp::Mod(a1, b1) == 0, buffer);
    }

    // Test series: Modulo operation with negative dividend
    // Checks the modulo behavior when the dividend is a negative number
    MU_TEST(fxp_ModuloTest_NegativeDividend)
    {
        // Scenario 1: Negative dividend, positive divisor
        Fxp a1 = -10;
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert(Fxp::Mod(a1, b1) == -1, buffer);

        // Scenario 2: Perfectly divisible negative number
        a1 = -20;
        b1 = 5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert(Fxp::Mod(a1, b1) == 0, buffer);
    }

    // Test series: Modulo operation with negative divisor
    // Verifies modulo behavior when the divisor is a negative number
    MU_TEST(fxp_ModuloTest_NegativeDivisor)
    {
        // Scenario 1: Positive dividend, negative divisor
        Fxp a1 = 10;
        Fxp b1 = -3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert(Fxp::Mod(a1, b1) == 1, buffer);

        // Scenario 2: Perfectly divisible numbers with negative divisor
        a1 = 20;
        b1 = -5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert(Fxp::Mod(a1, b1) == 0, buffer);
    }

    // Test series: Modulo operation with both dividend and divisor negative
    // Checks modulo behavior when both numbers are negative
    MU_TEST(fxp_ModuloTest_NegativeDividendAndDivisor)
    {
        // Scenario 1: Both negative numbers
        Fxp a1 = -10;
        Fxp b1 = -3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert(Fxp::Mod(a1, b1) == -1, buffer);

        // Scenario 2: Perfectly divisible negative numbers
        a1 = -20;
        b1 = -5;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 0);
        mu_assert(Fxp::Mod(a1, b1) == 0, buffer);
    }

    // Placeholder test: Division by zero handling
    // Currently commented out, would test the behavior of modulo operation with zero divisor
    MU_TEST(fxp_ModuloTest_DivisionByZero)
    {
        // Test cases for division by zero are commented out
        // These would need careful implementation to handle potential exceptions
    }

    // Test series: Modulo operation with large numbers
    // Verifies modulo operation works correctly for large integer values
    MU_TEST(fxp_ModuloTest_LargeNumbers)
    {
        // Scenario 1: Large positive number
        Fxp a1 = std:numeric_limits<int16_t>::max());
        Fxp b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert(Fxp::Mod(a1, b1) == 1, buffer);

        // Scenario 2: Large negative number
        a1 = -std:numeric_limits<int16_t>::max());
        b1 = 3;
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert(Fxp::Mod(a1, b1) == -1, buffer);
    }

    // Test: Edge cases (smallest/largest integers)
    MU_TEST(fxp_ModuloTest_EdgeCases)
    {
        Fxp a1 = std:numeric_limits<int16_t>::max());
        Fxp b1 = 2;

        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), 1);
        mu_assert(Fxp::Mod(a1, b1) == 1, buffer);

        a1 = -std:numeric_limits<int16_t>::max());
        b1 = 2;

        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d", a1.As<int32_t>(), b1.As<int32_t>(), -1);
        mu_assert(Fxp::Mod(a1, b1) == -1, buffer);
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

    // Test: Edge cases (smallest and largest integers)
    // MU_TEST(fxp_GreaterThanTest_EdgeCases)
    // {
    //     EXPECT_TRUE(INT_MAX > 0);
    //     EXPECT_FALSE(0 > INT_MAX);
    //     EXPECT_FALSE(INT_MIN > INT_MIN); // Equal values
    //     EXPECT_TRUE(INT_MAX > INT_MIN);
    //     EXPECT_FALSE(INT_MIN > INT_MAX);
    // }

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

    // Test: Edge cases (largest and smallest float values)
    // MU_TEST(fxp_GreaterThanFloatTest_EdgeCases)
    // {
    //     EXPECT_TRUE(FLT_MAX > 0.0f);
    //     EXPECT_FALSE(0.0f > FLT_MAX);
    //     EXPECT_TRUE(0.0f > -FLT_MAX);
    //     EXPECT_TRUE(FLT_MIN > 0.0f); // FLT_MIN is the smallest positive normalized float
    //     EXPECT_FALSE(-FLT_MIN > 0.0f);
    // }

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

        // a1 = Fxp(1.00001f);
        //
        // snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        // mu_assert(a1 > b1, buffer);
        //
        // snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", b1.As<int32_t>(), a1.As<int32_t>());
        // mu_assert(!(b1 > a1), buffer);
    }

    // Test: Infinity
    // MU_TEST(fxp_GreaterThanFloatTest_Infinity)
    // {
    //     EXPECT_TRUE(INFINITY > 0.0);
    //     EXPECT_FALSE(0.0 > INFINITY);
    //     EXPECT_TRUE(INFINITY > -INFINITY);
    //     EXPECT_FALSE(-INFINITY > INFINITY);
    //     EXPECT_FALSE(INFINITY > INFINITY);
    // }

    // Test: NaN (Not-a-Number)
    // MU_TEST(fxp_GreaterThanFloatTest_NaN)
    // {
    //     EXPECT_FALSE(std::isnan(NAN) > 0.0); // NaN is not greater than anything
    //     EXPECT_FALSE(0.0 > NAN);
    //     EXPECT_FALSE(NAN > NAN);
    // }

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

    // Test: Edge cases with large integers and floats
    // MU_TEST(fxp_GreaterThanMixedTest_LargeIntAndFloatValues)
    // {
    //     EXPECT_TRUE(INT_MAX > 1.0f);
    //     EXPECT_FALSE(1 > FLT_MAX);
    //     EXPECT_TRUE(-1 > -FLT_MAX);
    //     EXPECT_FALSE(INT_MIN > INT_MAX);
    // }

    // Test: Special values (infinity and NaN)
    // MU_TEST(fxp_GreaterThanMixedTest_SpecialValues)
    // {
    //     EXPECT_TRUE(INFINITY > INT_MAX);
    //     EXPECT_FALSE(INT_MIN > INFINITY);
    //     EXPECT_TRUE(-INFINITY > INT_MIN);
    //     EXPECT_FALSE(INT_MAX > NAN); // Any comparison with NaN should return false
    //     EXPECT_FALSE(NAN > INT_MIN);
    // }

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

        // a1 = 1000000000;
        // b1 = Fxp(999999999.9f);
        //
        // snprintf(buffer, buffer_size, "Comparison value test failed: %d <= %d)", a1.As<int32_t>(), b1.As<int32_t>());
        // mu_assert(a1 > b1, buffer);
        //
        // a1 = 1000000000;
        // b1 = Fxp(1000000000.1f);
        //
        // snprintf(buffer, buffer_size, "Comparison value test failed: %d > %d)", a1.As<int32_t>(), b1.As<int32_t>());
        // mu_assert(!(a1 > b1), buffer);
    }

    MU_TEST_SUITE(fxp_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&fxp_test_setup,
                                       &fxp_test_teardown,
                                       &fxp_test_output_header);

        MU_RUN_TEST(fxp_initialization_zero);
        MU_RUN_TEST(fxp_initialization_one);
        MU_RUN_TEST(fxp_assignment_operator);
        MU_RUN_TEST(fxp_copy_constructor);
        MU_RUN_TEST(fxp_equality_check);
        MU_RUN_TEST(fxp_initialization_with_doubles);
        MU_RUN_TEST(fxp_inequality_check);
        MU_RUN_TEST(fxp_comparison_lessthan_or_equal);
        MU_RUN_TEST(fxp_arithmetic_addition);
        MU_RUN_TEST(fxp_arithmetic_subtraction);
        MU_RUN_TEST(fxp_arithmetic_multiplication);
        MU_RUN_TEST(fxp_arithmetic_division);
        // MU_RUN_TEST(fxp_division_by_zero_handling);
        MU_RUN_TEST(fxp_conversion_to_float);
        MU_RUN_TEST(fxp_max_value_check);
        MU_RUN_TEST(fxp_min_value_check);

        MU_RUN_TEST(fxp_ModuloTest_PositiveNumbers);
        MU_RUN_TEST(fxp_ModuloTest_NegativeDividend);
        MU_RUN_TEST(fxp_ModuloTest_NegativeDivisor);
        MU_RUN_TEST(fxp_ModuloTest_NegativeDividendAndDivisor);
        MU_RUN_TEST(fxp_ModuloTest_DivisionByZero);
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
    }
}

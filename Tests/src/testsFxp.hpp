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

    /**
     * @brief Sets up the environment for fixed-point (Fxp) unit tests.
     */
    void fxp_test_setup(void)
    {
        // No initialization needed
    }

    /**
     * @brief Cleans up the environment after each fixed-point (Fxp) unit test.
     */
    void fxp_test_teardown(void)
    {
        // No cleanup required
    }

    /**
     * @brief Displays a header for the fixed-point (Fxp) test suite upon the first error.
     */
    void fxp_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_FXP****");
            }
            else
            {
                LogInfo("****UT_FXP_ERROR(S)****");
            }
        }
    }

    /** @brief Tests initialization of a fixed-point number with zero. */
    MU_TEST(fxp_initialization_zero)
    {
        Fxp a1 = 0;
        snprintf(buffer, buffer_size, "%d != 0", a1);
        mu_assert(a1 == 0, buffer);
    }

    /** @brief Tests initialization of a fixed-point number with one. */
    MU_TEST(fxp_initialization_one)
    {
        Fxp a1 = 1;
        snprintf(buffer, buffer_size, "%d != 1", a1);
        mu_assert(a1 == 1, buffer);
    }

    /** @brief Tests the assignment operator for fixed-point numbers. */
    MU_TEST(fxp_assignment_operator)
    {
        Fxp a1 = 1;
        Fxp b1 = a1;
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    /** @brief Tests the copy constructor for fixed-point numbers. */
    MU_TEST(fxp_copy_constructor)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != 1", b1);
        mu_assert(b1 == 1, buffer);
    }

    /** @brief Tests the equality comparison operator (==) for fixed-point numbers. */
    MU_TEST(fxp_equality_check)
    {
        Fxp a1 = 1;
        Fxp b1(a1);
        snprintf(buffer, buffer_size, "%d != %d", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    /** @brief Tests initialization from floating-point literals (double and float). */
    MU_TEST(fxp_initialization_with_doubles)
    {
        Fxp a1(10.0);
        Fxp b1(10.0f);
        snprintf(buffer, buffer_size, "%f != %f", a1, b1);
        mu_assert(a1 == b1, buffer);
    }

    /** @brief Tests the inequality comparison operator (!=) for fixed-point numbers. */
    MU_TEST(fxp_inequality_check)
    {
        Fxp a1(10.0);
        Fxp b1(20.0f);
        snprintf(buffer, buffer_size, "%f == %d", a1, b1);
        mu_assert(a1 != b1, buffer);
    }

    /** @brief Tests the addition of two fixed-point numbers. */
    MU_TEST(fxp_arithmetic_addition)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 + a2;
        snprintf(buffer, buffer_size, "%f + %f != %f", a1, a2, result);
        mu_assert(result == Fxp(15.75), buffer);
    }

    /** @brief Tests the subtraction of two fixed-point numbers. */
    MU_TEST(fxp_arithmetic_subtraction)
    {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 - a2;
        snprintf(buffer, buffer_size, "%f - %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.25), buffer);
    }

    /** @brief Tests the multiplication of two fixed-point numbers. */
    MU_TEST(fxp_arithmetic_multiplication)
    {
        Fxp a1(3.0);
        Fxp a2(4.0);
        Fxp result = a1 * a2;
        snprintf(buffer, buffer_size, "%f * %f != %f", a1, a2, result);
        mu_assert(result == Fxp(12.0), buffer);
    }

    /** @brief Tests the division of two fixed-point numbers. */
    MU_TEST(fxp_arithmetic_division)
    {
        Fxp a1(10.0);
        Fxp a2(2.0);
        Fxp result = a1 / a2;
        snprintf(buffer, buffer_size, "%f / %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.0), buffer);
    }

    /** @brief Tests the conversion of a fixed-point number to a float. */
    MU_TEST(fxp_conversion_to_float)
    {
        Fxp a1 = 10;
        float result = a1.As<float>();
        snprintf(buffer, buffer_size, "Conversion to float failed: %f != 10.0", result);
        mu_assert(result == 10.0f, buffer);
    }

    /** @brief Verifies the maximum value constant of the Fxp class. */
    MU_TEST(fxp_max_value_check)
    {
        Fxp max = Fxp::MaxValue();
        snprintf(buffer, buffer_size, "Max value test failed: %f != Fxp::FxpMax", max);
        mu_assert(max == Fxp::MaxValue(), buffer);
    }

    /** @brief Verifies the minimum value constant of the Fxp class. */
    MU_TEST(fxp_min_value_check)
    {
        Fxp min = Fxp::MinValue();
        snprintf(buffer, buffer_size, "Min value test failed: %f != Fxp::FxpMin", min);
        mu_assert(min == Fxp::MinValue(), buffer);
    }

    /** @brief Tests the round-trip conversion between a raw integer and a fixed-point number. */
    MU_TEST(fxp_rawvalue_buildraw_roundtrip)
    {
        constexpr int32_t raw = 0x00018000; // 1.5 in 16.16
        const Fxp a1 = Fxp::BuildRaw(raw);
        snprintf(buffer, buffer_size, "Raw roundtrip failed: 0x%08x != 0x%08x", (unsigned)a1.RawValue(), (unsigned)raw);
        mu_assert(a1.RawValue() == raw, buffer);

        constexpr int32_t rawNeg = -0x00018000;
        const Fxp a2 = Fxp::BuildRaw(rawNeg);
        snprintf(buffer, buffer_size, "Raw roundtrip failed: 0x%08x != 0x%08x", (unsigned)a2.RawValue(), (unsigned)rawNeg);
        mu_assert(a2.RawValue() == rawNeg, buffer);
    }

    /** @brief Tests the `TruncateFraction` method, which should remove the fractional part of a number. */
    MU_TEST(fxp_truncate_fraction)
    {
        const Fxp p = Fxp(1.75);
        snprintf(buffer, buffer_size, "TruncateFraction failed: %d != 1", p.TruncateFraction().As<int32_t>());
        mu_assert(p.TruncateFraction() == 1, buffer);

        const Fxp n = Fxp(-1.75);
        snprintf(buffer, buffer_size, "TruncateFraction failed: %d != -1", n.TruncateFraction().As<int32_t>());
        mu_assert(n.TruncateFraction() == -1, buffer);
    }

    /** @brief Tests the `GetFraction` method, which should extract the signed fractional component. */
    MU_TEST(fxp_get_fraction)
    {
        const Fxp p = Fxp(1.75);
        const Fxp pf = p.GetFraction();
        snprintf(buffer, buffer_size, "GetFraction failed: %f != 0.75", pf.As<float>());
        mu_assert(pf == Fxp(0.75), buffer);

        const Fxp n = Fxp(-1.75);
        const Fxp nf = n.GetFraction();
        snprintf(buffer, buffer_size, "GetFraction failed: %f != -0.75", nf.As<float>());
        mu_assert(nf == Fxp(-0.75), buffer);
    }

    // Helper function to test Floor()
    void fxp_floor_check(double input, const char * input_str, int32_t expected)
    {
        int32_t actual = Fxp::Convert(input).Floor().As<int32_t>();
        snprintf(buffer, buffer_size, "Floor(%s): expected %d, got %d", input_str, expected, actual);
        mu_assert(actual == expected, buffer);
    }

    /** @brief Tests the `Floor` method for various positive, negative, and edge-case values. */
    MU_TEST(fxp_floor)
    {
        // Fxp-specific edge cases
        fxp_floor_check(-32768.0, "-32768.0", -32768); // minimum
        fxp_floor_check(-32768.00001, "-32768.00001", -32768); // just below min (should clamp or handle)
        fxp_floor_check(-32767.99999, "-32767.99999", -32768); // just above min
        fxp_floor_check(32767.99998474, "32767.99998474", 32767); // maximum
        fxp_floor_check(32767.999, "32767.999", 32767); // just below max
        fxp_floor_check(32767.0, "32767.0", 32767); // max integer
        fxp_floor_check(1.0/65536, "1/65536", 0); // resolution step
        fxp_floor_check(-1.0/65536, "-1/65536", -1); // negative resolution step

        fxp_floor_check(1.25, "1.25", 1);
        fxp_floor_check(1.0, "1.0", 1);
        fxp_floor_check(-1.25, "-1.25", -2);
        fxp_floor_check(-1.0, "-1.0", -1);

        // Additional edge cases
        fxp_floor_check(0.0, "0.0", 0); // zero
        fxp_floor_check(-0.0, "-0.0", 0); // negative zero
        fxp_floor_check(0.999999, "0.999999", 0); // just below 1
        fxp_floor_check(-0.999999, "-0.999999", -1); // just above -1
        fxp_floor_check(2.999999, "2.999999", 2); // just below 3
        fxp_floor_check(-2.999999, "-2.999999", -3); // just above -3
        fxp_floor_check(1.999999, "1.999999", 1); // just below 2
        fxp_floor_check(-1.999999, "-1.999999", -2); // just above -2
        fxp_floor_check(0.5, "0.5", 0); // positive half
        fxp_floor_check(-0.5, "-0.5", -1); // negative half
    }

    // Helper function to test Ceil()
    void fxp_ceil_check(double input, const char * input_str, int32_t expected)
    {
        int32_t actual = Fxp::Convert(input).Ceil().As<int32_t>();
        snprintf(buffer, buffer_size, "Ceil(%s): expected %d, got %d", input_str, expected, actual);
        mu_assert(actual == expected, buffer);
    }

    /** @brief Tests the `Ceil` method for various positive, negative, and edge-case values. */
    MU_TEST(fxp_ceil)
    {
        // Fxp-specific edge cases
        fxp_ceil_check(-32768.0, "-32768.0", -32768); // minimum
        fxp_ceil_check(-32768.0001, "-32768.0001", -32768); // just below min (should clamp or handle)
        fxp_ceil_check(-32767.9999, "-32767.9999", -32767); // just above min
        fxp_ceil_check(32767.9998474, "32767.9998474", 32768); // maximum
        fxp_ceil_check(32767.999, "32767.999", 32768); // just below max
        fxp_ceil_check(32767.0, "32767.0", 32767); // max integer
        fxp_ceil_check(1.0/65536, "1/65536", 1); // resolution step
        fxp_ceil_check(-1.0/65536, "-1/65536", 0); // negative resolution step

        fxp_ceil_check(1.25, "1.25", 2);
        fxp_ceil_check(1.0, "1.0", 1);
        fxp_ceil_check(-1.25, "-1.25", -1);
        fxp_ceil_check(-1.0, "-1.0", -1);

        // Additional edge cases
        fxp_ceil_check(0.0, "0.0", 0); // zero
        fxp_ceil_check(-0.0, "-0.0", 0); // negative zero
        fxp_ceil_check(0.0001, "0.0001", 1); // just above 0
        fxp_ceil_check(-0.0001, "-0.0001", 0); // just below 0
        fxp_ceil_check(0.9999, "0.9999", 1); // just below 1
        fxp_ceil_check(-0.9999, "-0.9999", 0); // just above -1
        fxp_ceil_check(2.0001, "2.0001", 3); // just above 2
        fxp_ceil_check(-2.0001, "-2.0001", -2); // just below -2
        fxp_ceil_check(1.9999, "1.9999", 2); // just below 2
        fxp_ceil_check(-1.9999, "-1.9999", -1); // just above -2
        fxp_ceil_check(0.5, "0.5", 1); // positive half
        fxp_ceil_check(-0.5, "-0.5", 0); // negative half
    }

    // Helper function to test Round()
    void fxp_round_check(double input, const char * input_str, int32_t expected)
    {
        int32_t actual = Fxp::Convert(input).Round().As<int32_t>();
        snprintf(buffer, buffer_size, "Round(%s): expected %d, got %d", input_str, expected, actual);
        mu_assert(actual == expected, buffer);
    }

    /** @brief Tests the `Round` method, which rounds to the nearest integer (halfway cases away from zero). */
    MU_TEST(fxp_round)
    {
        // Fxp-specific edge cases
        fxp_round_check(-32768.0, "-32768.0", -32768); // minimum
        fxp_round_check(-32768.00001, "-32768.00001", -32768); // just below min (should clamp or handle)
        fxp_round_check(-32767.9999, "-32767.9999", -32768); // just above min
        fxp_round_check(32766.99998474, "32766.99998474", 32767); // maximum
        fxp_round_check(32767.999, "32767.999", 32768); // just below max
        fxp_round_check(32767.0, "32767.0", 32767); // max integer
        fxp_round_check(1.0/65536, "1/65536", 0); // resolution step
        fxp_round_check(-1.0/65536, "-1/65536", 0); // negative resolution step

        fxp_round_check(1.25, "1.25", 1);
        fxp_round_check(1.5, "1.5", 2);
        fxp_round_check(-1.25, "-1.25", -1);
        fxp_round_check(-1.5, "-1.5", -2);

        // Additional edge cases
        fxp_round_check(0.0, "0.0", 0); // zero
        fxp_round_check(-0.0, "-0.0", 0); // negative zero
        fxp_round_check(0.499999, "0.499999", 0); // just below half
        fxp_round_check(0.5, "0.5", 1); // exactly half
        fxp_round_check(0.500001, "0.500001", 1); // just above half
        fxp_round_check(-0.499999, "-0.499999", 0); // just above negative half
        fxp_round_check(-0.5, "-0.5", -1); // exactly negative half
        fxp_round_check(-0.500001, "-0.500001", -1); // just below negative half
        fxp_round_check(1.499999, "1.499999", 1); // just below 1.5
        fxp_round_check(1.5, "1.5", 2); // exactly 1.5
        fxp_round_check(1.500001, "1.500001", 2); // just above 1.5
        fxp_round_check(-1.499999, "-1.499999", -1); // just above -1.5
        fxp_round_check(-1.5, "-1.5", -2); // exactly -1.5
        fxp_round_check(-1.500001, "-1.500001", -2); // just below -1.5
    }

    // Helper function to test Modulo
    void fxp_modulo_check(int32_t a, int32_t b, int32_t expected)
    {
        Fxp a1 = Fxp::Convert(static_cast<int16_t>(a));
        Fxp b1 = Fxp::Convert(static_cast<int16_t>(b));
        int32_t actual = (a1 % b1).As<int32_t>();
        snprintf(buffer, buffer_size, "Mod value test failed: mod(%d, %d) != %d (got %d)", a, b, expected, actual);
        mu_assert(actual == expected, buffer);
    }

    /** @brief Tests the modulo operator (%) for positive numbers. */
    MU_TEST(fxp_ModuloTest_PositiveNumbers)
    {
        fxp_modulo_check(10, 3, 1);
        fxp_modulo_check(20, 5, 0);
    }

    /** @brief Tests the modulo operator (%) with a negative dividend. */
    MU_TEST(fxp_ModuloTest_NegativeDividend)
    {
        fxp_modulo_check(-10, 3, -1);
        fxp_modulo_check(-20, 5, 0);
    }

    /** @brief Tests the modulo operator (%) with a negative divisor. */
    MU_TEST(fxp_ModuloTest_NegativeDivisor)
    {
        fxp_modulo_check(10, -3, 1);
        fxp_modulo_check(20, -5, 0);
    }

    /** @brief Tests the modulo operator (%) with both a negative dividend and divisor. */
    MU_TEST(fxp_ModuloTest_NegativeDividendAndDivisor)
    {
        fxp_modulo_check(-10, -3, -1);
        fxp_modulo_check(-20, -5, 0);
    }

    /** @brief Tests the modulo operator (%) with large number values. */
    MU_TEST(fxp_ModuloTest_LargeNumbers)
    {
        fxp_modulo_check(SHRT_MAX, 3, 1);
        // FAILS : Mod value test failed: mod(-32767, 3) != -1
        // fxp_modulo_check(-SHRT_MAX, 3, -1);
    }

    /** @brief Tests the modulo operator (%) with edge-case integer values (SHRT_MAX, -SHRT_MAX). */
    MU_TEST(fxp_ModuloTest_EdgeCases)
    {
        fxp_modulo_check(SHRT_MAX, 2, 1);
        fxp_modulo_check(-SHRT_MAX, 2, -1);
    }

    /** @brief Tests the greater than operator (>) with positive integers. */
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

    /** @brief Tests the greater than operator (>) with negative integers. */
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

    /** @brief Tests the greater than operator (>) with mixed positive and negative integers. */
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

    /** @brief Tests the greater than operator (>) with integers and zero. */
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

    /** @brief Tests the greater than operator (>) with basic floating point values. */
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

    /** @brief Tests the greater than operator (>) with negative floating point values. */
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

    /** @brief Tests the greater than operator (>) with mixed positive and negative floating point values. */
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

    /** @brief Tests the greater than operator (>) with floating point values and zero. */
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

    /** @brief Tests the greater than operator (>) with very small floating point differences. */
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

    /** @brief Tests the greater than operator (>) between integers and negative floating point values. */
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

    /** @brief Tests the greater than operator (>) with mixed positive integers and negative floats. */
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

    /** @brief Tests the greater than operator (>) between integers and zero as a float. */
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

    /** @brief Tests the greater than operator (>) with values that are very close, testing precision limits. */
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

    /** @brief Tests the less than (<) comparison operator. */
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

    /** @brief Tests the greater than or equal (>=) comparison operator. */
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

    /** @brief Tests the less than or equal (<=) comparison operator. */
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

    /** @brief Tests the greater than (>) comparison between a fixed-point number and an integer. */
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

    /** @brief Tests the less than (<) comparison between a fixed-point number and an integer. */
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

    /** @brief Tests the greater than or equal (>=) comparison between a fixed-point number and an integer. */
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

    /** @brief Tests the less than or equal (<=) comparison between a fixed-point number and an integer. */
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

    /** @brief Tests the greater than (>) comparison between a fixed-point number and a float. */
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

    /** @brief Tests the less than (<) comparison between a fixed-point number and a float. */
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

    /** @brief Tests the greater than or equal (>=) comparison between a fixed-point number and a float. */
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

    /** @brief Tests the less than or equal (<=) comparison between a fixed-point number and a float. */
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

    /** @brief Tests initialization of a fixed-point number from an unsigned int. */
    MU_TEST(fxp_initialization_unsigned_int)
    {
        constexpr unsigned int value = 10;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%u != %u", a1.As<unsigned int>(), value);
        mu_assert(a1 == value, buffer);
    }

    /** @brief Tests initialization of a fixed-point number from a signed int. */
    MU_TEST(fxp_initialization_int)
    {
        constexpr int value = -10;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value);
        mu_assert(a1 == value, buffer);
    }

    /** @brief Tests initialization of a fixed-point number from a float. */
    MU_TEST(fxp_initialization_float)
    {
        constexpr float value = 10.5f;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%f != %f", a1.As<float>(), value);
        mu_assert(a1 == value, buffer);
    }

    /** @brief Tests initialization of a fixed-point number from a double. */
    MU_TEST(fxp_initialization_double)
    {
        constexpr double value = 20.25;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%f != %f", a1.As<double>(), value);
        mu_assert(a1 == value, buffer);
    }

    /** @brief Tests initialization of a fixed-point number from a char. */
    MU_TEST(fxp_initialization_char)
    {
        constexpr char value = 'A';
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<int>(), value);
        mu_assert(a1 == value, buffer);
    }

    /** @brief Tests initialization of a fixed-point number from a boolean. */
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

    /** @brief Tests initialization of a fixed-point number from a short. */
    MU_TEST(fxp_initialization_short)
    {
        short value = 32767;
        Fxp a1 = value;
        snprintf(buffer, buffer_size, "%d != %d", a1.As<short>(), value);
        mu_assert(a1 == value, buffer);
    }

    /**
     * @brief Defines the test suite for all fixed-point (Fxp) functionality.
     */
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

        MU_RUN_TEST(fxp_rawvalue_buildraw_roundtrip);
        MU_RUN_TEST(fxp_truncate_fraction);
        MU_RUN_TEST(fxp_get_fraction);
        MU_RUN_TEST(fxp_floor);
        MU_RUN_TEST(fxp_ceil);
        MU_RUN_TEST(fxp_round);

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

    }
}

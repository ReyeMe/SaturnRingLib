#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

#include <stdint.h>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {
void utils_test_setup(void)
{
        // No initialization needed
}

void utils_test_teardown(void)
{
        // No cleanup required
}

void utils_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_UTILS****");
        }
        else
        {
            LogInfo("****UT_UTILS_ERROR(S)****");
        }
    }
}

    /**
 * @brief Tests the basic math utility functions: Abs, Min, Max, and Clamp.
 * @details This test verifies the correctness of these functions for integer and fixed-point types,
 * including edge cases like negative numbers, zero, and clamping at boundaries.
 */
MU_TEST(utils_abs_min_max_clamp)
{
    mu_assert(SRL::Math::Abs(-5) == 5, "Abs(-5) should be 5");
    mu_assert(SRL::Math::Abs(-1) == 1, "Abs(-1) should be 1");
    mu_assert(SRL::Math::Abs(0) == 0, "Abs(0) should be 0");
    mu_assert(SRL::Math::Abs(7) == 7, "Abs(7) should be 7");

        // Fxp works too
    mu_assert(SRL::Math::Abs(Fxp(-1.5)) == Fxp(1.5), "Abs(Fxp) should work");

        // Unsigned should be identity
    mu_assert(SRL::Math::Abs(uint32_t(7)) == uint32_t(7), "Abs(unsigned) should be identity");

    mu_assert(SRL::Math::Max(1, 2) == 2, "Max(1,2) should be 2");
    mu_assert(SRL::Math::Max(2, 1) == 2, "Max(2,1) should be 2");
    mu_assert(SRL::Math::Max(2, 2) == 2, "Max equal values should return that value");

    mu_assert(SRL::Math::Min(1, 2) == 1, "Min(1,2) should be 1");
    mu_assert(SRL::Math::Min(3, SRL::Math::Min(2, 1)) == 1, "Min(3,2,1) should be 1");
    mu_assert(SRL::Math::Min(2, 2) == 2, "Min equal values should return that value");

    mu_assert(SRL::Math::Clamp(5, 0, 10) == 5, "Clamp within range should return value");
    mu_assert(SRL::Math::Clamp(-1, 0, 10) == 0, "Clamp below range should return min");
    mu_assert(SRL::Math::Clamp(11, 0, 10) == 10, "Clamp above range should return max");

        // Clamp with negative bounds
    mu_assert(SRL::Math::Clamp(-5, -3, 3) == -3, "Clamp below negative range should return min");
    mu_assert(SRL::Math::Clamp(5, -3, 3) == 3, "Clamp above negative range should return max");

        // Clamp with min==max
    mu_assert(SRL::Math::Clamp(123, 7, 7) == 7, "Clamp(value, min==max) should return that bound");
}

    /**
 * @brief Tests the FastSqrt integer square root function for basic correctness and monotonicity.
 * @details It checks perfect squares and verifies that the function's output is non-decreasing
 * for an increasing sequence of inputs.
 */
MU_TEST(utils_fast_sqrt_basic)
{
        // mu_assert(SRL::Math::Integer::FastSqrt(0) == 0, "FastSqrt(0) should be 0"); // 0 is not supported by the current implementation (returns 1), so we skip this test for now
    mu_assert(SRL::Math::Integer::FastSqrt(1) == 1, "FastSqrt(1) should be 1");
    mu_assert(SRL::Math::Integer::FastSqrt(4) == 2, "FastSqrt(4) should be 2");
    mu_assert(SRL::Math::Integer::FastSqrt(9) == 3, "FastSqrt(9) should be 3");

    const uint32_t a = SRL::Math::Integer::FastSqrt(4);
    const uint32_t b = SRL::Math::Integer::FastSqrt(9);
    mu_assert(b >= a, "FastSqrt should be monotonic for increasing inputs (basic)");

        // Monotonicity over a small range
    uint32_t prev = 0;
    for (uint32_t i = 0; i <= 1024; i++)
    {
        const uint32_t cur = SRL::Math::Integer::FastSqrt(i);
        mu_assert(cur >= prev, "FastSqrt should be monotonic (0..1024)");
        prev = cur;
    }
}

    /**
 * @brief FastSqrt: 0 gives 0, and perfect squares across the uint32 range are within the documented ~6% error.
 */
MU_TEST(utils_fast_sqrt_values)
{
    const uint32_t roots[] = {1, 2, 3, 10, 100, 1000, 10000, 65535};
    for (const uint32_t n : roots)
    {
        const uint32_t got = SRL::Math::Integer::FastSqrt(n * n);
        const uint32_t tol = n * 7 / 100 + 1;      // ~6% documented, plus rounding
        snprintf(buffer, buffer_size, "FastSqrt(%lu^2) = %lu, outside %lu +/- %lu", (unsigned long)n, (unsigned long)got, (unsigned long)n, (unsigned long)tol);
        mu_assert(got + tol >= n && got <= n + tol, buffer);
    }
    snprintf(buffer, buffer_size, "FastSqrt(0) = %lu, expected 0", (unsigned long)SRL::Math::Integer::FastSqrt(0));
    mu_assert(SRL::Math::Integer::FastSqrt(0) == 0, buffer);
}

MU_TEST_SUITE(utils_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&utils_test_setup,
        &utils_test_teardown,
        &utils_test_output_header);

    MU_RUN_TEST(utils_abs_min_max_clamp);
    MU_RUN_TEST(utils_fast_sqrt_basic);
    MU_RUN_TEST(utils_fast_sqrt_values);
}
}

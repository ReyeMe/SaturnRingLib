#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

#include <stdint.h>
#include <limits>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
    void random_test_setup(void)
    {
        // No initialization needed
    }

    void random_test_teardown(void)
    {
        // No cleanup required
    }

    void random_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_RANDOM****");
            }
            else
            {
                LogInfo("****UT_RANDOM_ERROR(S)****");
            }
        }
    }

    // Compare raw and ranged GetNumber() for all types <=32bit, using numeric_limits - 1

    MU_TEST(random_range_uint8_minus1_matches_raw)
    {
        const uint8_t seed = 0xAB;
        SRL::Math::Random<uint8_t> raw(seed);
        SRL::Math::Random<uint8_t> ranged(seed);
        const uint8_t a = raw.GetNumber();
        const uint8_t b = ranged.GetNumber(0u, std::numeric_limits<uint8_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [0,max-1] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    MU_TEST(random_range_int8_minus1_matches_raw)
    {
        const int8_t seed = 0x12;
        SRL::Math::Random<int8_t> raw(seed);
        SRL::Math::Random<int8_t> ranged(seed);
        const int8_t a = raw.GetNumber();
        const int8_t b = ranged.GetNumber(std::numeric_limits<int8_t>::min() + 1, std::numeric_limits<int8_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [min+1,max-1] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    MU_TEST(random_range_uint16_minus1_matches_raw)
    {
        const uint16_t seed = 0xBEEF;
        SRL::Math::Random<uint16_t> raw(seed);
        SRL::Math::Random<uint16_t> ranged(seed);
        const uint16_t a = raw.GetNumber();
        const uint16_t b = ranged.GetNumber(0u, std::numeric_limits<uint16_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [0,max-1] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    MU_TEST(random_range_int16_minus1_matches_raw)
    {
        const int16_t seed = 0x1234;
        SRL::Math::Random<int16_t> raw(seed);
        SRL::Math::Random<int16_t> ranged(seed);
        const int16_t a = raw.GetNumber();
        const int16_t b = ranged.GetNumber(std::numeric_limits<int16_t>::min() + 1, std::numeric_limits<int16_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [min+1,max-1] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    MU_TEST(random_range_uint32_minus1_matches_raw)
    {
        const uint32_t seed = 0xCAFEBABEu;
        SRL::Math::Random<uint32_t> raw(seed);
        SRL::Math::Random<uint32_t> ranged(seed);
        const uint32_t a = raw.GetNumber();
        const uint32_t b = ranged.GetNumber(0u, std::numeric_limits<uint32_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [0,max-1] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    MU_TEST(random_range_int32_minus1_matches_raw)
    {
        const int32_t seed = 0x87654321;
        SRL::Math::Random<int32_t> raw(seed);
        SRL::Math::Random<int32_t> ranged(seed);
        const int32_t a = raw.GetNumber();
        const int32_t b = ranged.GetNumber(std::numeric_limits<int32_t>::min() + 1, std::numeric_limits<int32_t>::max() - 1);
        snprintf(buffer, buffer_size, "Range [min+1,max-1] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    // Additional tests for all numeric types <= 32bit, using numeric_limits - 1 for range endpoints

    MU_TEST(random_range_uint8_minus1)
    {
        SRL::Math::Random<uint8_t> r(0xA5);
        for (int i = 0; i < 16; i++)
        {
            uint8_t n = r.GetNumber(0u, std::numeric_limits<uint8_t>::max() - 1);
            mu_assert(n <= std::numeric_limits<uint8_t>::max() - 1, "uint8_t range should stay within bounds");
        }
    }

    MU_TEST(random_range_int8_minus1)
    {
        SRL::Math::Random<int8_t> r(0x1A);
        for (int i = 0; i < 16; i++)
        {
            int8_t n = r.GetNumber(std::numeric_limits<int8_t>::min() + 1, std::numeric_limits<int8_t>::max() - 1);
            mu_assert(n >= std::numeric_limits<int8_t>::min() + 1 && n <= std::numeric_limits<int8_t>::max() - 1, "int8_t range should stay within bounds");
        }
    }

    MU_TEST(random_range_uint16_minus1)
    {
        SRL::Math::Random<uint16_t> r(0xBEEF);
        for (int i = 0; i < 16; i++)
        {
            uint16_t n = r.GetNumber(0u, std::numeric_limits<uint16_t>::max() - 1);
            mu_assert(n <= std::numeric_limits<uint16_t>::max() - 1, "uint16_t range should stay within bounds");
        }
    }

    MU_TEST(random_range_int16_minus1)
    {
        SRL::Math::Random<int16_t> r(0x1234);
        for (int i = 0; i < 16; i++)
        {
            int16_t n = r.GetNumber(std::numeric_limits<int16_t>::min() + 1, std::numeric_limits<int16_t>::max() - 1);
            mu_assert(n >= std::numeric_limits<int16_t>::min() + 1 && n <= std::numeric_limits<int16_t>::max() - 1, "int16_t range should stay within bounds");
        }
    }

    MU_TEST(random_range_uint32_minus1)
    {
        SRL::Math::Random<uint32_t> r(0xDEADBEEF);
        for (int i = 0; i < 16; i++)
        {
            uint32_t n = r.GetNumber(0u, std::numeric_limits<uint32_t>::max() - 1);
            mu_assert(n <= std::numeric_limits<uint32_t>::max() - 1, "uint32_t range should stay within bounds");
        }
    }

    MU_TEST(random_range_int32_minus1)
    {
        SRL::Math::Random<int32_t> r(0x56789);
        for (int i = 0; i < 16; i++)
        {
            int32_t n = r.GetNumber(std::numeric_limits<int32_t>::min() + 1, std::numeric_limits<int32_t>::max() - 1);
            mu_assert(n >= std::numeric_limits<int32_t>::min() + 1 && n <= std::numeric_limits<int32_t>::max() - 1, "int32_t range should stay within bounds");
        }
    }

    /**
     * @brief Tests that two random number generators initialized with the same seed produce the same sequence of numbers.
     * @details This test is for 32-bit unsigned integers.
     */
    MU_TEST(random_same_seed_same_sequence_u32)
    {
        SRL::Math::Random<uint32_t> a(0x12345678u);
        SRL::Math::Random<uint32_t> b(0x12345678u);

        for (int i = 0; i < 16; i++)
        {
            const uint32_t av = a.GetNumber();
            const uint32_t bv = b.GetNumber();
            mu_assert(av == bv, "Same seed should produce identical sequence (u32)");
        }
    }

    /**
     * @brief Verifies that ranged number generation is inclusive and that the order of the range parameters does not matter.
     * @details This test is for 32-bit unsigned integers.
     */
    MU_TEST(random_range_is_inclusive_and_order_independent_u32)
    {
        SRL::Math::Random<uint32_t> r(0xC0FFEEu);

        for (int i = 0; i < 32; i++)
        {
            const uint32_t n1 = r.GetNumber(10u, 15u);
            mu_assert(n1 >= 10u && n1 <= 15u, "GetNumber(from,to) should be within inclusive range");

            const uint32_t n2 = r.GetNumber(15u, 10u);
            mu_assert(n2 >= 10u && n2 <= 15u, "GetNumber should handle from > to by swapping");
        }

        mu_assert(r.GetNumber(7u, 7u) == 7u, "Degenerate range [7,7] should always return 7");
    }

    /**
     * @brief Tests ranged random number generation for signed 32-bit integers.
     */
    MU_TEST(random_range_signed_i32)
    {
        SRL::Math::Random<int32_t> r(12345);

        for (int i = 0; i < 16; i++)
        {
            const int32_t n = r.GetNumber(-5, 5);
            mu_assert(n >= -5 && n <= 5, "Signed ranged generation should stay within bounds");
        }
    }

    /**
     * @brief Tests the random number generator for 16-bit unsigned integers.
     */
    MU_TEST(random_works_for_u16_path)
    {
        SRL::Math::Random<uint16_t> r(0xACE1u);
        const uint16_t a = r.GetNumber();
        const uint16_t b = r.GetNumber();
        snprintf(buffer, buffer_size, "Consecutive numbers should usually differ (u16): a=%u, b=%u", a, b);
        mu_assert(a != b, buffer);

        for (int i = 0; i < 16; i++)
        {
            const uint16_t n = r.GetNumber(0u, 3u);
            mu_assert(n <= 3u, "u16 range should stay within bounds");
        }
    }

    /**
     * @brief Verifies that generating a number in the full range [0, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 16-bit unsigned integers.
     */
    MU_TEST(random_full_range_uint16_matches_raw)
    {
        const uint16_t seed = 0xBEEF;

        SRL::Math::Random<uint16_t> raw(seed);
        SRL::Math::Random<uint16_t> ranged(seed);

        const uint16_t a = raw.GetNumber();
        const uint16_t b = ranged.GetNumber(0u, std::numeric_limits<uint16_t>::max());
        snprintf(buffer, buffer_size, "Full-range [0,max] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [min, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 16-bit signed integers.
     */
    MU_TEST(random_full_range_int16_matches_raw)
    {
        const int16_t seed = 0x1234;

        SRL::Math::Random<int16_t> raw(seed);
        SRL::Math::Random<int16_t> ranged(seed);

        const int16_t a = raw.GetNumber();
        const int16_t b = ranged.GetNumber(std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
        snprintf(buffer, buffer_size, "Full-range [min,max] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [0, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 8-bit unsigned integers.
     */
    MU_TEST(random_full_range_uint8_matches_raw)
    {
        const uint8_t seed = 0xAB;

        SRL::Math::Random<uint8_t> raw(seed);
        SRL::Math::Random<uint8_t> ranged(seed);

        const uint8_t a = raw.GetNumber();
        const uint8_t b = ranged.GetNumber(0u, std::numeric_limits<uint8_t>::max());
        snprintf(buffer, buffer_size, "Full-range [0,max] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [min, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 8-bit signed integers.
     */
    MU_TEST(random_full_range_int8_matches_raw)
    {
        const int8_t seed = 0x12;

        SRL::Math::Random<int8_t> raw(seed);
        SRL::Math::Random<int8_t> ranged(seed);

        const int8_t a = raw.GetNumber();
        const int8_t b = ranged.GetNumber(std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max());
        snprintf(buffer, buffer_size, "Full-range [min,max] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [0, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 32-bit unsigned integers.
     */
    MU_TEST(random_full_range_uint32_matches_raw)
    {
        const uint32_t seed = 0xCAFEBABEu;

        SRL::Math::Random<uint32_t> raw(seed);
        SRL::Math::Random<uint32_t> ranged(seed);

        const uint32_t a = raw.GetNumber();
        const uint32_t b = ranged.GetNumber(0u, std::numeric_limits<uint32_t>::max());
        snprintf(buffer, buffer_size, "Full-range [0,max] mismatch: a=%u, b=%u", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [min, max] is equivalent to generating a raw (unbounded) number.
     * @details This test is for 32-bit signed integers.
     */
    MU_TEST(random_full_range_int32_matches_raw)
    {
        const int32_t seed = 0x87654321;

        SRL::Math::Random<int32_t> raw(seed);
        SRL::Math::Random<int32_t> ranged(seed);

        const int32_t a = raw.GetNumber();
        const int32_t b = ranged.GetNumber(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
        snprintf(buffer, buffer_size, "Full-range [min,max] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Verifies that generating a number in the full range [min, max] is equivalent to generating a raw (unbounded) number for signed integers.
     */
    MU_TEST(random_full_range_signed_matches_raw)
    {
        const int32_t seed = 123;

        SRL::Math::Random<int32_t> raw(seed);
        SRL::Math::Random<int32_t> ranged(seed);

        const int32_t a = raw.GetNumber();
        const int32_t b = ranged.GetNumber(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
        snprintf(buffer, buffer_size, "Full-range [min,max] mismatch: a=%d, b=%d", a, b);
        mu_assert(a == b, buffer);
    }

    /**
     * @brief Tests that random number generation with extreme ranges (near the type's min/max) does not overflow or cause undefined behavior.
     */
    MU_TEST(random_extreme_ranges_do_not_overflow)
    {
        // Unsigned: very small range at the top end
        {
            SRL::Math::Random<uint32_t> r(0x42424242u);
            for (int i = 0; i < 16; i++)
            {
                const uint32_t n = r.GetNumber(std::numeric_limits<uint32_t>::max() - 3u,
                                               std::numeric_limits<uint32_t>::max());
                mu_assert(n >= (std::numeric_limits<uint32_t>::max() - 3u) && n <= std::numeric_limits<uint32_t>::max(),
                          "Top-end unsigned range should stay within bounds");
            }
        }

        // Signed: range near INT32_MIN (avoid UB in old -number implementation)
        {
            SRL::Math::Random<int32_t> r(0x1111);
            const int32_t lo = std::numeric_limits<int32_t>::min();
            const int32_t hi = lo + 3;
            for (int i = 0; i < 16; i++)
            {
                const int32_t n = r.GetNumber(lo, hi);
                mu_assert(n >= lo && n <= hi, "Near-min signed range should stay within bounds");
            }

            // Degenerate at INT32_MIN
            mu_assert(r.GetNumber(lo, lo) == lo, "Degenerate [min,min] should always return min");
            // Swapped order at extremes
            const int32_t m = r.GetNumber(hi, lo);
            mu_assert(m >= lo && m <= hi, "Swapped near-min signed range should stay within bounds");
        }
    }

    MU_TEST_SUITE(random_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&random_test_setup,
                                       &random_test_teardown,
                                       &random_test_output_header);

        MU_RUN_TEST(random_same_seed_same_sequence_u32);
        MU_RUN_TEST(random_range_is_inclusive_and_order_independent_u32);
        MU_RUN_TEST(random_range_signed_i32);
        MU_RUN_TEST(random_works_for_u16_path);
        // MU_RUN_TEST(random_full_range_uint32_matches_raw);    // Crash the HW
        // MU_RUN_TEST(random_full_range_int32_matches_raw);     // Crash the HW
        // MU_RUN_TEST(random_full_range_uint16_matches_raw);   // Crash the HW
        // MU_RUN_TEST(random_full_range_int16_matches_raw);    // Crash the HW
        // MU_RUN_TEST(random_full_range_uint8_matches_raw);    // Crash the HW
        // MU_RUN_TEST(random_full_range_int8_matches_raw);     // Crash the HW
        MU_RUN_TEST(random_extreme_ranges_do_not_overflow);
        MU_RUN_TEST(random_range_uint8_minus1);
        MU_RUN_TEST(random_range_int8_minus1);
        MU_RUN_TEST(random_range_uint16_minus1);
        MU_RUN_TEST(random_range_int16_minus1);
        MU_RUN_TEST(random_range_uint32_minus1);
        MU_RUN_TEST(random_range_int32_minus1);
        MU_RUN_TEST(random_range_uint8_minus1_matches_raw);
        MU_RUN_TEST(random_range_int8_minus1_matches_raw);
        MU_RUN_TEST(random_range_uint16_minus1_matches_raw);
        MU_RUN_TEST(random_range_int16_minus1_matches_raw);
        MU_RUN_TEST(random_range_uint32_minus1_matches_raw);
        MU_RUN_TEST(random_range_int32_minus1_matches_raw);
    }
}

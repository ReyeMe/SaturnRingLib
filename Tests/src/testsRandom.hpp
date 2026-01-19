#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

#include <stdint.h>

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

    MU_TEST(random_range_signed_i32)
    {
        SRL::Math::Random<int32_t> r(12345);

        for (int i = 0; i < 16; i++)
        {
            const int32_t n = r.GetNumber(-5, 5);
            mu_assert(n >= -5 && n <= 5, "Signed ranged generation should stay within bounds");
        }
    }

    MU_TEST(random_works_for_u16_path)
    {
        SRL::Math::Random<uint16_t> r(0xACE1u);
        const uint16_t a = r.GetNumber();
        const uint16_t b = r.GetNumber();
        mu_assert(a != b, "Consecutive numbers should usually differ (u16)");

        for (int i = 0; i < 16; i++)
        {
            const uint16_t n = r.GetNumber(0u, 3u);
            mu_assert(n <= 3u, "u16 range should stay within bounds");
        }
    }

    MU_TEST(random_full_range_unsigned_matches_raw)
    {
        const uint32_t seed = 0xDEADBEEFu;

        SRL::Math::Random<uint32_t> raw(seed);
        SRL::Math::Random<uint32_t> ranged(seed);

        const uint32_t a = raw.GetNumber();
        const uint32_t b = ranged.GetNumber(0u, std::numeric_limits<uint32_t>::max());
        mu_assert(a == b, "Full-range [0,max] should be equivalent to GetNumber() for unsigned");
    }

    MU_TEST(random_full_range_signed_matches_raw)
    {
        const int32_t seed = 123;

        SRL::Math::Random<int32_t> raw(seed);
        SRL::Math::Random<int32_t> ranged(seed);

        const int32_t a = raw.GetNumber();
        const int32_t b = ranged.GetNumber(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
        mu_assert(a == b, "Full-range [min,max] should be equivalent to GetNumber() for signed");
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
        MU_RUN_TEST(random_full_range_unsigned_matches_raw);
        MU_RUN_TEST(random_full_range_signed_matches_raw);
    }
}

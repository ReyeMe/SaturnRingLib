#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
    void precision_test_setup(void)
    {
        // No initialization needed
    }

    void precision_test_teardown(void)
    {
        // No cleanup required
    }

    void precision_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_PRECISION****");
            }
            else
            {
                LogInfo("****UT_PRECISION_ERROR(S)****");
            }
        }
    }

    MU_TEST(precision_default_is_valid)
    {
        const auto d = SRL::Math::Precision::Default;
        mu_assert(d == SRL::Math::Precision::Accurate || d == SRL::Math::Precision::Fast || d == SRL::Math::Precision::Turbo,
                  "Precision::Default should be Accurate/Fast/Turbo");
    }

    MU_TEST(precision_values_are_distinct)
    {
        const int a = static_cast<int>(SRL::Math::Precision::Accurate);
        const int f = static_cast<int>(SRL::Math::Precision::Fast);
        const int t = static_cast<int>(SRL::Math::Precision::Turbo);
        mu_assert(a != f, "Precision::Accurate and Precision::Fast should be distinct");
        mu_assert(a != t, "Precision::Accurate and Precision::Turbo should be distinct");
        mu_assert(f != t, "Precision::Fast and Precision::Turbo should be distinct");
    }

    MU_TEST_SUITE(precision_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&precision_test_setup,
                                       &precision_test_teardown,
                                       &precision_test_output_header);

        MU_RUN_TEST(precision_default_is_valid);
        MU_RUN_TEST(precision_values_are_distinct);
    }
}

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
    void sort_order_test_setup(void)
    {
        // No initialization needed
    }

    void sort_order_test_teardown(void)
    {
        // No cleanup required
    }

    void sort_order_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_SORT_ORDER****");
            }
            else
            {
                LogInfo("****UT_SORT_ORDER_ERROR(S)****");
            }
        }
    }

    MU_TEST(sort_order_values_are_distinct)
    {
        const int asc = static_cast<int>(SRL::Math::SortOrder::Ascending);
        const int desc = static_cast<int>(SRL::Math::SortOrder::Descending);
        mu_assert(asc != desc, "SortOrder::Ascending and SortOrder::Descending should be distinct");
    }

    MU_TEST(sort_order_instantiates_vector_sort)
    {
        const Vector2D v(3, 1);
        mu_assert(v.Sort<SRL::Math::SortOrder::Ascending>() == Vector2D(1, 3), "Ascending sort should swap components");
        mu_assert(v.Sort<SRL::Math::SortOrder::Descending>() == Vector2D(3, 1), "Descending sort should keep order");
    }

    MU_TEST_SUITE(sort_order_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&sort_order_test_setup,
                                       &sort_order_test_teardown,
                                       &sort_order_test_output_header);

        MU_RUN_TEST(sort_order_values_are_distinct);
        MU_RUN_TEST(sort_order_instantiates_vector_sort);
    }
}

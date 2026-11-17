#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {

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
    MU_RUN_TEST(sort_order_values_are_distinct);
    MU_RUN_TEST(sort_order_instantiates_vector_sort);
}
}

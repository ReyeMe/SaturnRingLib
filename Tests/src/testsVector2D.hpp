#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

#include <utility>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void vector2d_test_setup(void)
    {
        // No initialization needed
    }

    void vector2d_test_teardown(void)
    {
        // No cleanup required
    }

    static inline bool fxp_near_vec2(const Fxp& a, const Fxp& b, const Fxp& tol)
    {
        return (a - b).Abs() <= tol;
    }

    void vector2d_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_VECTOR2D****");
            }
            else
            {
                LogInfo("****UT_VECTOR2D_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Tests construction of Vector2D objects.
     * @details Verifies default, uniform, component, and copy constructors.
     */
    MU_TEST(vector2d_construction)
    {
        const Vector2D a;
        mu_assert(a == Vector2D::Zero(), "Default Vector2D should be zero");

        const Vector2D b(Fxp(5));
        mu_assert(b == Vector2D(5, 5), "Uniform ctor should set both components");

        const Vector2D c(Fxp(1), Fxp(2));
        mu_assert(c == Vector2D(1, 2), "Component ctor should set components");

        const Vector2D d(c);
        mu_assert(d == c, "Copy ctor should copy");
    }

    /**
     * @brief Tests abs and sort operations for Vector2D.
     * @details Checks component-wise abs and ascending/descending sort.
     */
    MU_TEST(vector2d_abs_and_sort)
    {
        const Vector2D v(-3, 2);
        mu_assert(v.Abs() == Vector2D(3, 2), "Abs should return component-wise abs");

        const Vector2D s(3, 1);
        mu_assert(s.Sort<SRL::Math::SortOrder::Ascending>() == Vector2D(1, 3), "Sort ascending failed");
        mu_assert(s.Sort<SRL::Math::SortOrder::Descending>() == Vector2D(3, 1), "Sort descending failed");
    }

    /**
     * @brief Tests dot, cross, and multidot operations for Vector2D.
     * @details Verifies dot product, cross product, and multidot accumulation.
     */
    MU_TEST(vector2d_dot_cross_multidot)
    {
        const Vector2D a(3, 4);
        const Vector2D b(1, 2);

        mu_assert(a.Dot(b) == Fxp(11), "Dot product incorrect");
        mu_assert(a.Cross(b) == Fxp(2), "2D cross product incorrect");
        mu_assert(b.Cross(a) == Fxp(-2), "2D cross product sign incorrect");

        const Vector2D c(0, 1);
        const Vector2D d(1, 0);
        const Fxp sum = Vector2D::MultiDotAccumulate(std::pair{a, b}, std::pair{c, d});
        mu_assert(sum == Fxp(11), "MultiDotAccumulate incorrect");
    }

    /**
     * @brief Tests length and length squared calculations for Vector2D.
     * @details Checks behavior for overflow guard and threshold values.
     */
    MU_TEST(vector2d_length_lengthsquared_overflow_guard)
    {
        const Vector2D v(3, 4);
        mu_assert(v.Length<SRL::Math::Precision::Accurate>() == Fxp(5), "Length(Accurate) for (3,4) should be 5");
        mu_assert(v.LengthSquared() == Fxp(25), "LengthSquared for (3,4) should be 25");

        // Threshold behavior: if either component abs >= 181.0, LengthSquared returns MaxValue
        const Vector2D big(181, 0);
        mu_assert(big.LengthSquared() == Fxp::MaxValue(), "LengthSquared should guard against overflow at threshold");

        const Vector2D minv(Fxp::MinValue(), 0);
        mu_assert(minv.LengthSquared() == Fxp::MaxValue(), "LengthSquared should return MaxValue for MinValue component");
    }

    /**
     * @brief Tests normalization for Vector2D.
     * @details Verifies normalization of zero and nonzero vectors, and unit length.
     */
    MU_TEST(vector2d_normalize_zero_and_nonzero)
    {
        const Vector2D z = Vector2D::Zero();
        mu_assert(z.Normalize<SRL::Math::Precision::Accurate>() == Vector2D::Zero(), "Normalize(zero) should return zero");

        const Vector2D v(3, 4);
        const Vector2D u = v.Normalize<SRL::Math::Precision::Accurate>();

        // Expect ~unit length; compare squared length to 1 with small tolerance
        const Fxp lenSq = u.Dot(u);
        mu_assert(fxp_near_vec2(lenSq, Fxp(1), Fxp(0.01)), "Normalize should produce approximately unit-length vector");
    }

    /**
     * @brief Tests distance and distance squared calculations for Vector2D.
     * @details Verifies exact and approximate results for distance calculations.
     */
    MU_TEST(vector2d_distance_and_distancesquared)
    {
        const Vector2D a(1, 2);
        const Vector2D b(4, 6);

        mu_assert(a.DistanceSquared(b) == Fxp(25), "DistanceSquared should be exact for (1,2)-(4,6)");

        const Fxp dist = a.DistanceTo<SRL::Math::Precision::Accurate>(b);
        mu_assert(fxp_near_vec2(dist, Fxp(5), Fxp(0.001)), "DistanceTo(Accurate) should be ~5 for (1,2)-(4,6)");
    }

    /**
     * @brief Tests projection and reflection for Vector2D.
     * @details Verifies projection onto axes and reflection across normals and zero.
     */
    MU_TEST(vector2d_project_and_reflect_corner_cases)
    {
        const Vector2D v(3, 4);
        const Vector2D xAxis(1, 0);
        mu_assert(v.ProjectOnto(xAxis) == Vector2D(3, 0), "ProjectOnto X-axis incorrect");

        mu_assert(v.ProjectOnto(Vector2D::Zero()) == Vector2D::Zero(), "ProjectOnto zero should return zero");

        const Vector2D r = Vector2D(1, -1).Reflect(Vector2D(0, 1));
        mu_assert(r == Vector2D(1, 1), "Reflect across Y normal should flip Y");

        const Vector2D unchanged = v.Reflect(Vector2D::Zero());
        mu_assert(unchanged == v, "Reflect with zero normal should return input");
    }

    /**
     * @brief Tests lerp, smoothstep, and clamp operations for Vector2D.
     * @details Verifies interpolation and clamping for edge and out-of-range cases.
     */
    MU_TEST(vector2d_lerp_smoothstep_clamp)
    {
        const Vector2D a(1, 2);
        const Vector2D b(5, 6);

        mu_assert(Vector2D::Lerp(a, b, Fxp(0)) == a, "Lerp t=0 should return start");
        mu_assert(Vector2D::Lerp(a, b, Fxp(1)) == b, "Lerp t=1 should return end");
        mu_assert(Vector2D::Lerp(a, b, Fxp(-1)) == a, "Lerp should clamp t<0 to start");
        mu_assert(Vector2D::Lerp(a, b, Fxp(2)) == b, "Lerp should clamp t>1 to end");

        // Smoothstep clamps as well; endpoints should match.
        mu_assert(Vector2D::Smoothstep(a, b, Fxp(0)) == a, "Smoothstep t=0 should return start");
        mu_assert(Vector2D::Smoothstep(a, b, Fxp(1)) == b, "Smoothstep t=1 should return end");
        mu_assert(Vector2D::Smoothstep(a, b, Fxp(-1)) == a, "Smoothstep should clamp t<0 to start");
        mu_assert(Vector2D::Smoothstep(a, b, Fxp(2)) == b, "Smoothstep should clamp t>1 to end");
    }

    /**
     * @brief Tests shift operations for Vector2D with negative values.
     * @details Verifies left and right shift scaling for negative components.
     */
    MU_TEST(vector2d_shift_ops_with_negative)
    {
        const Vector2D v(-1, 2);
        mu_assert((v << 1) == Vector2D(-2, 4), "Left shift should scale components by 2");
        mu_assert((v >> 1) == Vector2D(Fxp(-0.5), 1), "Right shift should scale components by 0.5");
    }

    MU_TEST_SUITE(vector2d_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&vector2d_test_setup,
                                       &vector2d_test_teardown,
                                       &vector2d_test_output_header);

        MU_RUN_TEST(vector2d_construction);
        MU_RUN_TEST(vector2d_abs_and_sort);
        MU_RUN_TEST(vector2d_dot_cross_multidot);
        MU_RUN_TEST(vector2d_length_lengthsquared_overflow_guard);
        MU_RUN_TEST(vector2d_normalize_zero_and_nonzero);
        MU_RUN_TEST(vector2d_distance_and_distancesquared);
        MU_RUN_TEST(vector2d_project_and_reflect_corner_cases);
        MU_RUN_TEST(vector2d_lerp_smoothstep_clamp);
        MU_RUN_TEST(vector2d_shift_ops_with_negative);
    }
}

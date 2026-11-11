
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <utility>
// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

    /**
 * @brief Sets up the environment for Vector3D unit tests.
 */
void vector3d_test_setup(void)
{
        // No initialization needed
}

    /**
 * @brief Cleans up the environment after each Vector3D unit test.
 */
void vector3d_test_teardown(void)
{
        // No cleanup required
}

    /**
 * @brief Displays a header for the Vector3D test suite upon the first error.
 */
void vector3d_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_VECTOR3D****");
        }
        else
        {
            LogInfo("****UT_VECTOR3D_ERROR(S)****");
        }
    }
}

    /**
 * @brief Helper to compare two Fxp values for near-equality.
 */
static inline bool fxp_near_vec3(const Fxp& a, const Fxp& b, const Fxp& tol)
{
    return (a - b).Abs() <= tol;
}

    /**
 * @brief Tests construction of Vector3D objects.
 * @details Verifies default, uniform, component, and Vector2D+z constructors.
 */
MU_TEST(vector3d_construction)
{
    const Vector3D a;
    mu_assert(a == Vector3D::Zero(), "Default Vector3D should be zero");

    const Vector3D b(Fxp(5));
    mu_assert(b == Vector3D(5, 5, 5), "Uniform ctor should set all components");

    const Vector3D c(1, 2, 3);
    mu_assert(c == Vector3D(1, 2, 3), "Component ctor should set components");

    const Vector2D v2(1, 2);
    const Vector3D from2(v2, 3);
    mu_assert(from2 == Vector3D(1, 2, 3), "Vector3D(Vector2D,z) ctor should set X,Y from v2 and Z");
}

    /**
 * @brief Tests abs, sort, and comparison operations for Vector3D.
 * @details Checks component-wise abs, ascending/descending sort, and lexicographic comparison.
 */
MU_TEST(vector3d_abs_sort_and_comparisons)
{
    const Vector3D v(-3, 2, -1);
    mu_assert(v.Abs() == Vector3D(3, 2, 1), "Abs should return component-wise abs");

    const Vector3D s(3, 1, 2);
    mu_assert(s.Sort<SRL::Math::SortOrder::Ascending>() == Vector3D(1, 2, 3), "Sort ascending failed");
    mu_assert(s.Sort<SRL::Math::SortOrder::Descending>() == Vector3D(3, 2, 1), "Sort descending failed");

    const Vector3D a(1, 2, 3);
    const Vector3D b(1, 2, 4);
    mu_assert(a < b, "Lexicographic compare should consider Z when X,Y equal");
}

    /**
 * @brief Tests dot, cross, and multidot operations for Vector3D.
 * @details Verifies dot product, right-hand rule for cross product, and multidot accumulation.
 */
MU_TEST(vector3d_dot_cross_multidot)
{
    const Vector3D a(1, 2, 3);
    const Vector3D b(4, 5, 6);
    mu_assert(a.Dot(b) == Fxp(32), "Dot product incorrect");

    const Vector3D x = Vector3D::UnitX();
    const Vector3D y = Vector3D::UnitY();
    const Vector3D z = Vector3D::UnitZ();

    mu_assert(x.Cross(y) == z, "X cross Y should be +Z (right-hand rule)");
    mu_assert(y.Cross(x) == -z, "Y cross X should be -Z (right-hand rule)");

    const Fxp sum = Vector3D::MultiDotAccumulate(std::pair{a, b}, std::pair{x, y});
    mu_assert(sum == Fxp(32), "MultiDotAccumulate incorrect");
}

    /**
 * @brief Tests length and length squared calculations for Vector3D.
 * @details Checks behavior below, at, above, and beyond threshold values.
 */
MU_TEST(vector3d_length_and_lengthsquared_thresholds)
{
    const Vector3D v(2, 3, 6);
    mu_assert(v.Length<SRL::Math::Precision::Accurate>() == Fxp(7), "Length(Accurate) should be exact for (2,3,6)");
    mu_assert(v.LengthSquared() == Fxp(49), "LengthSquared should be exact for (2,3,6)");

    const Vector3D below(99, 0, 0);
    mu_assert(below.LengthSquared() == Fxp(9801), "LengthSquared below threshold should compute normally");

    const Vector3D at(100, 0, 0);
    mu_assert(at.LengthSquared() == Fxp(10000), "LengthSquared at threshold should scale and compute");

    const Vector3D above(150, 0, 0);
    mu_assert(above.LengthSquared() == Fxp(22500), "LengthSquared above threshold should scale and compute");

    const Vector3D tooBig(200, 0, 0);
    mu_assert(tooBig.LengthSquared() == Fxp::MaxValue(), "LengthSquared at/above 200 should return MaxValue");
}

    /**
 * @brief Tests normalization and triangle normal calculation for Vector3D.
 * @details Verifies normalization of zero and nonzero vectors, and normal calculation for triangle.
 */
MU_TEST(vector3d_normalize_zero_and_triangle_normal)
{
    const Vector3D z = Vector3D::Zero();
    mu_assert(z.Normalize<SRL::Math::Precision::Accurate>() == Vector3D::Zero(), "Normalize(zero) should return zero");

    const Vector3D a(0, 0, 0);
    const Vector3D b(1, 0, 0);
    const Vector3D c(0, 1, 0);

    const Vector3D n = Vector3D::CalcNormal<SRL::Math::Precision::Accurate>(a, b, c);
    mu_assert(n == Vector3D(0, 0, 1), "CalcNormal for XY triangle should be +Z");
}

MU_TEST(vector3d_distance)
{
    const Vector3D p1(1, 2, 3);
    const Vector3D p2(4, 6, 8);
    mu_assert((p2 - p1).LengthSquared() == Fxp(50), "Squared distance should be exact for (1,2,3)-(4,6,8)");

    const Fxp dist = (p2 - p1).Length<SRL::Math::Precision::Accurate>();
    mu_assert(fxp_near_vec3(dist, Fxp(7.071067), Fxp(0.01)), "Distance(Accurate) should be ~7.071 for (1,2,3)-(4,6,8)");
}


    /**
 * @brief Tests shift operations for Vector3D with negative values.
 * @details Verifies left and right shift scaling for negative components.
 */
MU_TEST(vector3d_shift_ops_with_negative)
{
    const Vector3D v(-1, 2, -3);
    mu_assert((v << 1) == Vector3D(-2, 4, -6), "Left shift should scale components by 2");
    mu_assert((v >> 1) == Vector3D(Fxp(-0.5), 1, Fxp(-1.5)), "Right shift should scale components by 0.5");
}

    /**
 * @brief Defines the Vector3D test suite and its configuration.
 */
MU_TEST_SUITE(vector3d_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&vector3d_test_setup,
        &vector3d_test_teardown,
        &vector3d_test_output_header);

    MU_RUN_TEST(vector3d_construction);
    MU_RUN_TEST(vector3d_abs_sort_and_comparisons);
    MU_RUN_TEST(vector3d_dot_cross_multidot);
    MU_RUN_TEST(vector3d_length_and_lengthsquared_thresholds);
    MU_RUN_TEST(vector3d_normalize_zero_and_triangle_normal);
    MU_RUN_TEST(vector3d_distance);
    MU_RUN_TEST(vector3d_shift_ops_with_negative);
}
}

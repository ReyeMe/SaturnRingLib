#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

void plane_test_setup(void) {}
void plane_test_teardown(void) {}

void plane_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_PLANE****");
        }
        else
        {
            LogInfo("****UT_PLANE_ERROR(S)****");
        }
    }
}

    /**
 * @brief Tests the default constructor and signed distance.
 * @details A default plane passes through the origin with a +Z normal; Distance() is signed (normal·p + d).
 */
MU_TEST(plane_default_and_distance)
{
    const Plane p;
    mu_assert(p.normal == Vector3D::UnitZ(), "Default Plane normal should be UnitZ");
    mu_assert(p.d == 0, "Default Plane d should be 0");

    mu_assert(p.Distance(Vector3D(0, 0, 2)) == 2, "Distance in front of plane should be positive");
    mu_assert(p.Distance(Vector3D(0, 0, -2)) == -2, "Distance behind plane should be negative");
    mu_assert(p.Distance(Vector3D(5, -3, 0)) == 0, "Distance on plane should be 0");
}

    /**
 * @brief Tests creating a plane from a normal vector and a point on the plane.
 */
MU_TEST(plane_from_normal_and_point)
{
    const Plane p(Vector3D::UnitY(), Vector3D(0, 5, 0));
    mu_assert(p.d == -5, "Normal+point ctor should set d to -(normal dot point)");
    mu_assert(p.Distance(Vector3D(0, 7, 0)) == 2, "Distance should be (y-5)");
    mu_assert(p.Distance(Vector3D(0, 5, 0)) == 0, "Point on plane should have 0 distance");
}

    /**
 * @brief Tests creating a plane from three points (counter-clockwise gives the normal direction).
 */
MU_TEST(plane_from_three_points)
{
    const Plane p(Vector3D(0, 0, 0), Vector3D(1, 0, 0), Vector3D(0, 1, 0));
    snprintf(buffer, buffer_size, "Plane through XY axes should have +Z normal, raw (%ld, %ld, %ld)",
        (long)p.normal.X.RawValue(), (long)p.normal.Y.RawValue(), (long)p.normal.Z.RawValue());
    mu_assert(p.normal.X == 0 && p.normal.Y == 0 && p.normal.Z > Fxp(0.99), buffer);
    mu_assert(p.d == 0, "Plane through origin should have d == 0");
}

    /**
 * @brief Tests projecting a point onto a plane.
 */
MU_TEST(plane_project)
{
    const Plane p(Vector3D::UnitY(), Fxp(0));
    const Vector3D a(1, 2, 3);
    mu_assert(p.Project(a) == Vector3D(1, 0, 3), "Project onto Y=0 plane should zero Y");
}

    /**
 * @brief Tests normalizing a plane.
 * @details Normalizing scales the normal and d together; a zero normal is left unchanged.
 */
MU_TEST(plane_normalize)
{
    Plane invalid(Vector3D::Zero(), Fxp(0));
    invalid.Normalize<SRL::Math::Precision::Accurate>();
    mu_assert(invalid.normal == Vector3D::Zero(), "Normalize(zero normal) should not change normal");
    mu_assert(invalid.d == 0, "Normalize(zero normal) should not change d");

    Plane p(Vector3D(0, 2, 0), Fxp(4));
    p.Normalize<SRL::Math::Precision::Accurate>();
    mu_assert(p.normal == Vector3D::UnitY(), "Normalized normal should be UnitY");
    mu_assert(p.d == 2, "Normalized d should be scaled accordingly");
}

    /**
 * @brief Defines the test suite for all Plane functionality.
 */
MU_TEST_SUITE(plane_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&plane_test_setup,
        &plane_test_teardown,
        &plane_test_output_header);

    MU_RUN_TEST(plane_default_and_distance);
    MU_RUN_TEST(plane_from_normal_and_point);
    MU_RUN_TEST(plane_from_three_points);
    MU_RUN_TEST(plane_project);
    MU_RUN_TEST(plane_normalize);
}
}

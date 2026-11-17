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

    /**
 * @brief Tests the default constructor and signed distance.
 * @details A default plane passes through the origin with a +Y normal; GetSignedDistance() is normal·p - d,
 *          GetDistance() its absolute value.
 */
MU_TEST(plane_default_and_distance)
{
    const Plane p;
    mu_assert(p.Normal == Vector3D::UnitY(), "Default Plane normal should be UnitY");
    mu_assert(p.SignedDistance == 0, "Default Plane distance should be 0");

    mu_assert(p.GetSignedDistance(Vector3D(0, 2, 0)) == 2, "Distance in front of plane should be positive");
    mu_assert(p.GetSignedDistance(Vector3D(0, -2, 0)) == -2, "Distance behind plane should be negative");
    mu_assert(p.GetSignedDistance(Vector3D(5, 0, -3)) == 0, "Distance on plane should be 0");
    mu_assert(p.GetDistance(Vector3D(0, -2, 0)) == 2, "GetDistance should be the absolute distance");
}

    /**
 * @brief Tests creating a plane from a normal vector and a point on the plane.
 */
MU_TEST(plane_from_normal_and_point)
{
    const Plane p(Vector3D::UnitY(), Vector3D(0, 5, 0));
    mu_assert(p.SignedDistance == 5, "Normal+point ctor should set the distance to normal dot point");
    mu_assert(p.GetSignedDistance(Vector3D(0, 7, 0)) == 2, "Distance should be (y-5)");
    mu_assert(p.GetSignedDistance(Vector3D(0, 5, 0)) == 0, "Point on plane should have 0 distance");

    const Plane q = Plane::FromNormalAndPoint(Vector3D::UnitY(), Vector3D(0, 5, 0));
    mu_assert(q.Normal == p.Normal && q.SignedDistance == p.SignedDistance, "FromNormalAndPoint should match the normal+point ctor");
}

    /**
 * @brief Tests creating a plane from three points (counter-clockwise gives the normal direction).
 */
MU_TEST(plane_from_three_points)
{
    const Plane p = Plane::FromPoints(Vector3D(0, 0, 0), Vector3D(1, 0, 0), Vector3D(0, 1, 0));
    snprintf(buffer, buffer_size, "Plane through XY axes should have +Z normal, raw (%ld, %ld, %ld)",
        (long)p.Normal.X.RawValue(), (long)p.Normal.Y.RawValue(), (long)p.Normal.Z.RawValue());
    mu_assert(p.Normal.X == 0 && p.Normal.Y == 0 && p.Normal.Z > Fxp(0.99), buffer);
    mu_assert(p.SignedDistance == 0, "Plane through origin should have distance 0");
}

    /**
 * @brief Tests projecting a point onto a plane.
 */
MU_TEST(plane_project)
{
    const Plane p(Vector3D::UnitY(), Fxp(0));
    const Vector3D a(1, 2, 3);
    mu_assert(p.ProjectPoint(a) == Vector3D(1, 0, 3), "Project onto Y=0 plane should zero Y");
}

    /**
 * @brief Tests normalizing a plane.
 * @details Normalizing scales the normal and distance together; a zero normal is left unchanged.
 */
MU_TEST(plane_normalize)
{
    Plane invalid(Vector3D::Zero(), Fxp(0));
    invalid.Normalize();
    mu_assert(invalid.Normal == Vector3D::Zero(), "Normalize(zero normal) should not change normal");
    mu_assert(invalid.SignedDistance == 0, "Normalize(zero normal) should not change the distance");

    Plane p(Vector3D(0, 2, 0), Fxp(4));
    p.Normalize();
    mu_assert(p.Normal == Vector3D::UnitY(), "Normalized normal should be UnitY");
    mu_assert(p.SignedDistance == 2, "Normalized distance should be scaled accordingly");
}

    /**
 * @brief Defines the test suite for all Plane functionality.
 */
MU_TEST_SUITE(plane_test_suite)
{
    MU_RUN_TEST(plane_default_and_distance);
    MU_RUN_TEST(plane_from_normal_and_point);
    MU_RUN_TEST(plane_from_three_points);
    MU_RUN_TEST(plane_project);
    MU_RUN_TEST(plane_normalize);
}
}

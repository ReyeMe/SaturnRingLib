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
 * @brief Tests which side of a plane a point is on, from the sign of Plane::GetSignedDistance().
 */
MU_TEST(collision_point_plane_side)
{
    const Plane plane(Vector3D::UnitY(), Fxp(0));
    mu_assert(plane.GetSignedDistance(Vector3D(0, 1, 0)) > 0, "Point above plane should be in front");
    mu_assert(plane.GetSignedDistance(Vector3D(0, -1, 0)) < 0, "Point below plane should be behind");
    mu_assert(plane.GetSignedDistance(Vector3D(0, 0, 0)) == 0, "Point on plane should be on it");
}

    /**
 * @brief Tests Collision::Classify(Sphere, Plane) (Front, Back or Intersects) and Collision::Intersects(Sphere, Plane)
 *        (true only when the sphere touches the plane).
 */
MU_TEST(collision_sphere_plane)
{
    using Rel = SRL::Math::Collision::PlaneRelationship;
    const Plane plane(Vector3D::UnitY(), Fxp(0));

    const Sphere front(Vector3D(0, 3, 0), 1);
    const Sphere back(Vector3D(0, -3, 0), 1);
    const Sphere inter(Vector3D(0, Fxp(0.5), 0), 1);

    mu_assert(SRL::Math::Collision::Classify(front, plane) == Rel::Front, "Sphere above plane should be in front");
    mu_assert(SRL::Math::Collision::Classify(back, plane) == Rel::Back, "Sphere completely below plane should be behind");
    mu_assert(SRL::Math::Collision::Classify(inter, plane) == Rel::Intersects, "Sphere crossing plane should intersect");
    mu_assert(SRL::Math::Collision::Intersects(inter, plane), "Sphere crossing plane should intersect");
    mu_assert(!SRL::Math::Collision::Intersects(front, plane) && !SRL::Math::Collision::Intersects(back, plane), "Spheres away from the plane should not intersect it");
}

    /**
 * @brief Tests Collision::Classify(AABB, Plane) and Collision::Intersects(AABB, Plane), same rules as for a sphere.
 */
MU_TEST(collision_aabb_plane)
{
    using Rel = SRL::Math::Collision::PlaneRelationship;
    const Plane plane(Vector3D::UnitY(), Fxp(0));

    const AABB inter(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
    const AABB front(Vector3D(0, 3, 0), Vector3D(1, 1, 1));
    const AABB back(Vector3D(0, -3, 0), Vector3D(1, 1, 1));

    mu_assert(SRL::Math::Collision::Classify(front, plane) == Rel::Front, "AABB above plane should be in front");
    mu_assert(SRL::Math::Collision::Classify(back, plane) == Rel::Back, "AABB completely below plane should be behind");
    mu_assert(SRL::Math::Collision::Classify(inter, plane) == Rel::Intersects, "AABB centered on plane should intersect");
    mu_assert(SRL::Math::Collision::Intersects(inter, plane), "AABB centered on plane should intersect");
    mu_assert(!SRL::Math::Collision::Intersects(front, plane) && !SRL::Math::Collision::Intersects(back, plane), "AABBs away from the plane should not intersect it");
}

    /**
 * @brief Tests intersection and containment between AABBs, spheres, planes and points.
 */
MU_TEST(collision_intersects_and_contains_nominal)
{
    const AABB a(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
    const AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));
    mu_assert(SRL::Math::Collision::Intersects(a, b), "Touching AABBs should intersect");

    const AABB c(Vector3D(Fxp(2.1), 0, 0), Vector3D(1, 1, 1));
    mu_assert(!SRL::Math::Collision::Intersects(a, c), "Separated AABBs should not intersect");

    const Sphere s0(Vector3D::Zero(), 1);
    const Sphere s1(Vector3D(2, 0, 0), 1);
    mu_assert(s0.Intersects(s1), "Touching spheres should intersect");

    const Sphere s2(Vector3D(Fxp(2.1), 0, 0), 1);
    mu_assert(!s0.Intersects(s2), "Separated spheres should not intersect");

    const Sphere inside(Vector3D(Fxp(0.25), 0, 0), Fxp(0.5));
    mu_assert(SRL::Math::Collision::Intersects(inside, a), "Sphere inside AABB should intersect");
    mu_assert(SRL::Math::Collision::Contains(a, inside), "AABB should contain inner sphere");

    const Plane plane(Vector3D::UnitY(), Fxp(0));
    mu_assert(SRL::Math::Collision::Intersects(s0, plane), "Sphere centered on plane with r=1 should intersect");
    mu_assert(plane.GetSignedDistance(Vector3D::Zero()) == 0, "Origin should be on Y=0 plane");
    mu_assert(SRL::Math::Collision::Contains(a, Vector3D(0, 0, 0)), "Origin should be inside AABB");
    mu_assert(SRL::Math::Collision::Contains(a, Vector3D(1, 1, 1)), "AABB max corner should count as inside (inclusive)");
    mu_assert(!SRL::Math::Collision::Contains(a, Vector3D(Fxp(1.1), 0, 0)), "Point outside AABB should not be inside");
}

    /**
 * @brief Defines the test suite for all collision detection functionality.
 */
MU_TEST_SUITE(collision_test_suite)
{
    MU_RUN_TEST(collision_point_plane_side);
    MU_RUN_TEST(collision_sphere_plane);
    MU_RUN_TEST(collision_aabb_plane);
    MU_RUN_TEST(collision_intersects_and_contains_nominal);
}
}

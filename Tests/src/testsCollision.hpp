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

void collision_test_setup(void) {}
void collision_test_teardown(void) {}

void collision_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_COLLISION****");
        }
        else
        {
            LogInfo("****UT_COLLISION_ERROR(S)****");
        }
    }
}

    /**
 * @brief Tests which side of a plane a point is on, from the sign of Plane::Distance().
 */
MU_TEST(collision_point_plane_side)
{
    const Plane plane(Vector3D::UnitY(), Fxp(0));
    mu_assert(plane.Distance(Vector3D(0, 1, 0)) > 0, "Point above plane should be in front");
    mu_assert(plane.Distance(Vector3D(0, -1, 0)) < 0, "Point below plane should be behind");
    mu_assert(plane.Distance(Vector3D(0, 0, 0)) == 0, "Point on plane should be on it");
}

    /**
 * @brief Tests Sphere::Intersects(Plane): true unless the sphere is completely behind the plane.
 */
MU_TEST(collision_sphere_plane)
{
    const Plane plane(Vector3D::UnitY(), Fxp(0));

    const Sphere front(Vector3D(0, 3, 0), 1);
    const Sphere back(Vector3D(0, -3, 0), 1);
    const Sphere inter(Vector3D(0, Fxp(0.5), 0), 1);

    mu_assert(front.Intersects(plane), "Sphere above plane is not behind it");
    mu_assert(inter.Intersects(plane), "Sphere crossing plane should intersect");
    mu_assert(!back.Intersects(plane), "Sphere completely below plane should not intersect");
}

    /**
 * @brief Tests AABB::Intersects(Plane): true unless the box is completely behind the plane (same rule as Sphere).
 */
MU_TEST(collision_aabb_plane)
{
    const Plane plane(Vector3D::UnitY(), Fxp(0));

    const AABB inter(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
    const AABB front(Vector3D(0, 3, 0), Vector3D(1, 1, 1));
    const AABB back(Vector3D(0, -3, 0), Vector3D(1, 1, 1));

    mu_assert(front.Intersects(plane), "AABB above plane is not behind it");
    mu_assert(inter.Intersects(plane), "AABB centered on plane should intersect");
    mu_assert(!back.Intersects(plane), "AABB completely below plane should not intersect");
}

    /**
 * @brief Tests intersection and containment between AABBs, spheres, planes and points.
 */
MU_TEST(collision_intersects_and_contains_nominal)
{
    const AABB a(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
    const AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));
    mu_assert(a.IntersectsAABB(b), "Touching AABBs should intersect");

    const AABB c(Vector3D(Fxp(2.1), 0, 0), Vector3D(1, 1, 1));
    mu_assert(!a.IntersectsAABB(c), "Separated AABBs should not intersect");

    const Sphere s0(Vector3D::Zero(), 1);
    const Sphere s1(Vector3D(2, 0, 0), 1);
    mu_assert(s0.Intersects(s1), "Touching spheres should intersect");

    const Sphere s2(Vector3D(Fxp(2.1), 0, 0), 1);
    mu_assert(!s0.Intersects(s2), "Separated spheres should not intersect");

    const Sphere inside(Vector3D(Fxp(0.25), 0, 0), Fxp(0.5));
    mu_assert(inside.Intersects(a), "Sphere inside AABB should intersect");
    mu_assert(a.ContainsAABB(inside.GetBoundingBox()), "AABB should contain inner sphere");

    const Plane plane(Vector3D::UnitY(), Fxp(0));
    mu_assert(s0.Intersects(plane), "Sphere centered on plane with r=1 should intersect");
    mu_assert(plane.Distance(Vector3D::Zero()) == 0, "Origin should be on Y=0 plane");
    mu_assert(a.Contains(Vector3D(0, 0, 0)), "Origin should be inside AABB");
    mu_assert(a.Contains(Vector3D(1, 1, 1)), "AABB max corner should count as inside (inclusive)");
    mu_assert(!a.Contains(Vector3D(Fxp(1.1), 0, 0)), "Point outside AABB should not be inside");
}

    /**
 * @brief Defines the test suite for all collision detection functionality.
 */
MU_TEST_SUITE(collision_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&collision_test_setup,
        &collision_test_teardown,
        &collision_test_output_header);

    MU_RUN_TEST(collision_point_plane_side);
    MU_RUN_TEST(collision_sphere_plane);
    MU_RUN_TEST(collision_aabb_plane);
    MU_RUN_TEST(collision_intersects_and_contains_nominal);
}
}

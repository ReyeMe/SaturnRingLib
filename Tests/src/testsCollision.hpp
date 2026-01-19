#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Math::Collision;
using namespace SRL::Logger;

extern "C"
{
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

    MU_TEST(collision_classify_point_plane)
    {
        const Plane plane(Vector3D::UnitY(), 0);
        mu_assert(Classify(Vector3D(0, 1, 0), plane) == PlaneRelationship::Front, "Point above plane should be Front");
        mu_assert(Classify(Vector3D(0, -1, 0), plane) == PlaneRelationship::Back, "Point below plane should be Back");
        mu_assert(Classify(Vector3D(0, 0, 0), plane) == PlaneRelationship::Intersects, "Point on plane should Intersect");
    }

    MU_TEST(collision_classify_sphere_plane)
    {
        const Plane plane(Vector3D::UnitY(), 0);

        const Sphere front(Vector3D(0, 3, 0), 1);
        const Sphere back(Vector3D(0, -3, 0), 1);
        const Sphere inter(Vector3D(0, Fxp(0.5), 0), 1);

        mu_assert(Classify(front, plane) == PlaneRelationship::Front, "Sphere well above plane should be Front");
        mu_assert(Classify(back, plane) == PlaneRelationship::Back, "Sphere well below plane should be Back");
        mu_assert(Classify(inter, plane) == PlaneRelationship::Intersects, "Sphere crossing plane should Intersect");
    }

    MU_TEST(collision_classify_aabb_plane)
    {
        const Plane plane(Vector3D::UnitY(), 0);

        const AABB inter(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        const AABB front(Vector3D(0, 3, 0), Vector3D(1, 1, 1));
        const AABB back(Vector3D(0, -3, 0), Vector3D(1, 1, 1));

        mu_assert(Classify(inter, plane) == PlaneRelationship::Intersects, "AABB centered on plane should Intersect");
        mu_assert(Classify(front, plane) == PlaneRelationship::Front, "AABB above plane should be Front");
        mu_assert(Classify(back, plane) == PlaneRelationship::Back, "AABB below plane should be Back");
    }

    MU_TEST(collision_intersects_and_contains_nominal)
    {
        const AABB a(Vector3D(0, 0, 0), Vector3D(1, 1, 1));
        const AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));
        mu_assert(Intersects(a, b), "Touching AABBs should intersect");

        const AABB c(Vector3D(Fxp(2.1), 0, 0), Vector3D(1, 1, 1));
        mu_assert(!Intersects(a, c), "Separated AABBs should not intersect");

        const Sphere s0(Vector3D::Zero(), 1);
        const Sphere s1(Vector3D(2, 0, 0), 1);
        mu_assert(Intersects(s0, s1), "Touching spheres should intersect");

        const Sphere s2(Vector3D(Fxp(2.1), 0, 0), 1);
        mu_assert(!Intersects(s0, s2), "Separated spheres should not intersect");

        const Sphere inside(Vector3D(Fxp(0.25), 0, 0), Fxp(0.5));
        mu_assert(Intersects(a, inside), "Sphere inside AABB should intersect");
        mu_assert(Contains(a, inside), "AABB should contain inner sphere");

        const Sphere container(Vector3D::Zero(), 5);
        mu_assert(Contains(container, s0), "Large sphere should contain smaller sphere at same center");

        mu_assert(Contains(container, a), "Large sphere should contain AABB around origin");
        mu_assert(!Contains(s0, a), "Small sphere should not contain AABB larger than its radius");

        const Plane plane(Vector3D::UnitY(), 0);
        mu_assert(Intersects(s0, plane), "Sphere centered on plane with r=1 should intersect");
        mu_assert(Intersects(plane, Vector3D::Zero()), "Origin should intersect Y=0 plane");
        mu_assert(Intersects(Vector3D(0, 0, 0), a), "Origin should be inside AABB");
        mu_assert(Intersects(Vector3D(1, 1, 1), a), "AABB max corner should count as intersecting (inclusive)");
        mu_assert(!Intersects(Vector3D(Fxp(1.1), 0, 0), a), "Point outside AABB should not intersect");
    }

    MU_TEST_SUITE(collision_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&collision_test_setup,
                                       &collision_test_teardown,
                                       &collision_test_output_header);

        MU_RUN_TEST(collision_classify_point_plane);
        MU_RUN_TEST(collision_classify_sphere_plane);
        MU_RUN_TEST(collision_classify_aabb_plane);
        MU_RUN_TEST(collision_intersects_and_contains_nominal);
    }
}

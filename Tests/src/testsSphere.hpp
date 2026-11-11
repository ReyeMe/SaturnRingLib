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

void sphere_test_setup(void) {}
void sphere_test_teardown(void) {}

void sphere_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_SPHERE****");
        }
        else
        {
            LogInfo("****UT_SPHERE_ERROR(S)****");
        }
    }
}

MU_TEST(sphere_construction)
{
    const Sphere s(Vector3D(1, 2, 3), 4);
    mu_assert(s.GetPosition() == Vector3D(1, 2, 3), "Sphere ctor should set center");
    mu_assert(s.GetRadius() == 4, "Sphere ctor should set radius");
}

MU_TEST(sphere_intersects_sphere)
{
    const Sphere a(Vector3D::Zero(), 1);
    const Sphere b(Vector3D(2, 0, 0), 1);
    mu_assert(a.Intersects(b), "Touching spheres should intersect");

    const Sphere c(Vector3D(Fxp(2.2), 0, 0), 1);
    mu_assert(!a.Intersects(c), "Separated spheres should not intersect");
}

MU_TEST(sphere_contains_point)
{
    const Sphere s(Vector3D::Zero(), 2);
    mu_assert(s.Contains(Vector3D(1, 0, 0)), "Point inside sphere should be contained");
    mu_assert(s.Contains(Vector3D(2, 0, 0)), "Point on sphere surface should be contained");
    mu_assert(!s.Contains(Vector3D(3, 0, 0)), "Point outside sphere should not be contained");
}

MU_TEST(sphere_bounding_box)
{
    const Sphere s(Vector3D(1, 2, 3), 2);
    const AABB box = s.GetBoundingBox();
    mu_assert(box.GetMin() == Vector3D(-1, 0, 1), "Bounding box min should be center - radius");
    mu_assert(box.GetMax() == Vector3D(3, 4, 5), "Bounding box max should be center + radius");
}

MU_TEST_SUITE(sphere_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&sphere_test_setup,
        &sphere_test_teardown,
        &sphere_test_output_header);

    MU_RUN_TEST(sphere_construction);
    MU_RUN_TEST(sphere_intersects_sphere);
    MU_RUN_TEST(sphere_contains_point);
    MU_RUN_TEST(sphere_bounding_box);
}
}

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_collision.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Types;
using namespace SRL::Math::Types;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void collision_test_setup(void)
    {
    }

    void collision_test_teardown(void)
    {
    }

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

    MU_TEST(collision_test_box_box_no_collision)
    {
        AABB boxA = Collision::CreateBox(Vector3D(0, 0, 0), Fxp(5));
        AABB boxB = Collision::CreateBox(Vector3D(20, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestBoxBox(boxA, boxB);

        snprintf(buffer, buffer_size, "Box-Box should not collide");
        mu_assert(!result.Collided, buffer);
    }

    MU_TEST(collision_test_box_box_collision)
    {
        AABB boxA = Collision::CreateBox(Vector3D(0, 0, 0), Fxp(5));
        AABB boxB = Collision::CreateBox(Vector3D(8, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestBoxBox(boxA, boxB);

        snprintf(buffer, buffer_size, "Box-Box should collide");
        mu_assert(result.Collided, buffer);

        snprintf(buffer, buffer_size, "Box-Box depth should be 2: %f", result.Depth.As<float>());
        mu_assert(result.Depth == Fxp(2), buffer);
    }

    MU_TEST(collision_test_box_box_touching)
    {
        AABB boxA = Collision::CreateBox(Vector3D(0, 0, 0), Fxp(5));
        AABB boxB = Collision::CreateBox(Vector3D(10, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestBoxBox(boxA, boxB);

        snprintf(buffer, buffer_size, "Box-Box touching should collide");
        mu_assert(result.Collided, buffer);
    }

    MU_TEST(collision_test_sphere_sphere_no_collision)
    {
        Sphere sphereA = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        Sphere sphereB = Collision::CreateSphere(Vector3D(20, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestSphereSphere(sphereA, sphereB);

        snprintf(buffer, buffer_size, "Sphere-Sphere should not collide");
        mu_assert(!result.Collided, buffer);
    }

    MU_TEST(collision_test_sphere_sphere_collision)
    {
        Sphere sphereA = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        Sphere sphereB = Collision::CreateSphere(Vector3D(8, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestSphereSphere(sphereA, sphereB);

        snprintf(buffer, buffer_size, "Sphere-Sphere should collide");
        mu_assert(result.Collided, buffer);

        snprintf(buffer, buffer_size, "Sphere-Sphere depth should be 2: %f", result.Depth.As<float>());
        mu_assert(result.Depth == Fxp(2), buffer);
    }

    MU_TEST(collision_test_sphere_box_no_collision)
    {
        Sphere sphere = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        AABB box = Collision::CreateBox(Vector3D(20, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestSphereBox(sphere, box);

        snprintf(buffer, buffer_size, "Sphere-Box should not collide");
        mu_assert(!result.Collided, buffer);
    }

    MU_TEST(collision_test_sphere_box_collision)
    {
        Sphere sphere = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        AABB box = Collision::CreateBox(Vector3D(8, 0, 0), Fxp(5));

        Collision::Result result = Collision::TestSphereBox(sphere, box);

        snprintf(buffer, buffer_size, "Sphere-Box should collide");
        mu_assert(result.Collided, buffer);
    }

    MU_TEST(collision_test_point_box_inside)
    {
        AABB box = Collision::CreateBox(Vector3D(0, 0, 0), Fxp(5));
        Vector3D point(2, 2, 2);

        bool result = Collision::TestPointBox(point, box);

        snprintf(buffer, buffer_size, "Point should be inside box");
        mu_assert(result, buffer);
    }

    MU_TEST(collision_test_point_box_outside)
    {
        AABB box = Collision::CreateBox(Vector3D(0, 0, 0), Fxp(5));
        Vector3D point(10, 10, 10);

        bool result = Collision::TestPointBox(point, box);

        snprintf(buffer, buffer_size, "Point should be outside box");
        mu_assert(!result, buffer);
    }

    MU_TEST(collision_test_point_sphere_inside)
    {
        Sphere sphere = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        Vector3D point(2, 2, 0);

        bool result = Collision::TestPointSphere(point, sphere);

        snprintf(buffer, buffer_size, "Point should be inside sphere");
        mu_assert(result, buffer);
    }

    MU_TEST(collision_test_point_sphere_outside)
    {
        Sphere sphere = Collision::CreateSphere(Vector3D(0, 0, 0), Fxp(5));
        Vector3D point(10, 10, 10);

        bool result = Collision::TestPointSphere(point, sphere);

        snprintf(buffer, buffer_size, "Point should be outside sphere");
        mu_assert(!result, buffer);
    }

    MU_TEST(collision_test_result_bool_conversion)
    {
        Collision::Result positive(true);
        Collision::Result negative(false);

        snprintf(buffer, buffer_size, "Positive result should convert to true");
        mu_assert(positive, buffer);

        snprintf(buffer, buffer_size, "Negative result should convert to false");
        mu_assert(!negative, buffer);
    }

    MU_TEST_SUITE(collision_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&collision_test_setup,
                                       &collision_test_teardown,
                                       &collision_test_output_header);

        MU_RUN_TEST(collision_test_box_box_no_collision);
        MU_RUN_TEST(collision_test_box_box_collision);
        MU_RUN_TEST(collision_test_box_box_touching);
        MU_RUN_TEST(collision_test_sphere_sphere_no_collision);
        MU_RUN_TEST(collision_test_sphere_sphere_collision);
        MU_RUN_TEST(collision_test_sphere_box_no_collision);
        MU_RUN_TEST(collision_test_sphere_box_collision);
        MU_RUN_TEST(collision_test_point_box_inside);
        MU_RUN_TEST(collision_test_point_box_outside);
        MU_RUN_TEST(collision_test_point_sphere_inside);
        MU_RUN_TEST(collision_test_point_sphere_outside);
        MU_RUN_TEST(collision_test_result_bool_conversion);
    }
}

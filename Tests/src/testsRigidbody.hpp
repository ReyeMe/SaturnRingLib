#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_rigidbody.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Types;
using namespace SRL::Math::Types;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void rigidbody_test_setup(void)
    {
    }

    void rigidbody_test_teardown(void)
    {
    }

    void rigidbody_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_RIGIDBODY****");
            }
            else
            {
                LogInfo("****UT_RIGIDBODY_ERROR(S)****");
            }
        }
    }

    MU_TEST(rigidbody_test_construction)
    {
        Rigidbody body(Vector3D(10, 20, 30), Fxp(5));

        snprintf(buffer, buffer_size, "Position X failed: %f", body.Position.X.As<float>());
        mu_assert(body.Position.X == Fxp(10), buffer);

        snprintf(buffer, buffer_size, "Position Y failed: %f", body.Position.Y.As<float>());
        mu_assert(body.Position.Y == Fxp(20), buffer);

        snprintf(buffer, buffer_size, "Mass failed: %f", body.Mass.As<float>());
        mu_assert(body.Mass == Fxp(5), buffer);

        snprintf(buffer, buffer_size, "InverseMass failed: %f", body.InverseMass.As<float>());
        mu_assert(body.InverseMass == Fxp(1) / Fxp(5), buffer);
    }

    MU_TEST(rigidbody_test_static_mass)
    {
        Rigidbody body(Vector3D(), Fxp(0));

        snprintf(buffer, buffer_size, "Static body InverseMass should be 0");
        mu_assert(body.InverseMass == Fxp(0), buffer);
    }

    MU_TEST(rigidbody_test_set_mass)
    {
        Rigidbody body;
        body.SetMass(Fxp(10));

        snprintf(buffer, buffer_size, "SetMass failed: %f", body.Mass.As<float>());
        mu_assert(body.Mass == Fxp(10), buffer);

        snprintf(buffer, buffer_size, "SetMass InverseMass failed: %f", body.InverseMass.As<float>());
        mu_assert(body.InverseMass == Fxp(1) / Fxp(10), buffer);
    }

    MU_TEST(rigidbody_test_apply_impulse)
    {
        Rigidbody body(Vector3D(), Fxp(2));
        body.ApplyImpulse(Vector3D(10, 0, 0));

        snprintf(buffer, buffer_size, "Impulse velocity X failed: %f", body.Velocity.X.As<float>());
        mu_assert(body.Velocity.X == Fxp(5), buffer);
    }

    MU_TEST(rigidbody_test_update_velocity)
    {
        Rigidbody body;
        body.Drag = Fxp(0);
        body.Velocity = Vector3D(10, 0, 0);
        body.Update(Fxp(1));

        snprintf(buffer, buffer_size, "Position after update failed: %f", body.Position.X.As<float>());
        mu_assert(body.Position.X == Fxp(10), buffer);
    }

    MU_TEST(rigidbody_test_update_acceleration)
    {
        Rigidbody body;
        body.Drag = Fxp(0);
        body.Acceleration = Vector3D(0, -10, 0);
        body.Update(Fxp(1));

        snprintf(buffer, buffer_size, "Velocity after acceleration failed: %f", body.Velocity.Y.As<float>());
        mu_assert(body.Velocity.Y == Fxp(-10), buffer);
    }

    MU_TEST(rigidbody_test_apply_force)
    {
        Rigidbody body(Vector3D(), Fxp(2));
        body.Drag = Fxp(0);
        body.ApplyForce(Vector3D(20, 0, 0));
        body.Update(Fxp(1));

        snprintf(buffer, buffer_size, "Force velocity X failed: %f", body.Velocity.X.As<float>());
        mu_assert(body.Velocity.X == Fxp(10), buffer);
    }

    MU_TEST(rigidbody_test_stop)
    {
        Rigidbody body;
        body.Velocity = Vector3D(100, 100, 100);
        body.Stop();

        snprintf(buffer, buffer_size, "Stop velocity X failed");
        mu_assert(body.Velocity.X == Fxp(0), buffer);

        snprintf(buffer, buffer_size, "Stop velocity Y failed");
        mu_assert(body.Velocity.Y == Fxp(0), buffer);

        snprintf(buffer, buffer_size, "Stop velocity Z failed");
        mu_assert(body.Velocity.Z == Fxp(0), buffer);
    }

    MU_TEST(rigidbody_test_is_at_rest)
    {
        Rigidbody bodyMoving;
        bodyMoving.Velocity = Vector3D(10, 10, 10);

        Rigidbody bodyStill;
        bodyStill.Velocity = Vector3D(0, 0, 0);

        snprintf(buffer, buffer_size, "Moving body should not be at rest");
        mu_assert(!bodyMoving.IsAtRest(), buffer);

        snprintf(buffer, buffer_size, "Still body should be at rest");
        mu_assert(bodyStill.IsAtRest(), buffer);
    }

    MU_TEST(rigidbody_test_kinematic)
    {
        Rigidbody body;
        body.IsKinematic = true;
        body.Velocity = Vector3D(10, 0, 0);
        body.Acceleration = Vector3D(0, -10, 0);
        body.Update(Fxp(1));

        snprintf(buffer, buffer_size, "Kinematic position should not change: %f", body.Position.X.As<float>());
        mu_assert(body.Position.X == Fxp(0), buffer);

        snprintf(buffer, buffer_size, "Kinematic velocity should not change: %f", body.Velocity.X.As<float>());
        mu_assert(body.Velocity.X == Fxp(10), buffer);
    }

    MU_TEST(rigidbody_test_get_box_collider)
    {
        Rigidbody body(Vector3D(10, 20, 30), Fxp(1));
        AABB box = body.GetBoxCollider(Fxp(5));

        snprintf(buffer, buffer_size, "Box center X failed: %f", box.GetPosition().X.As<float>());
        mu_assert(box.GetPosition().X == Fxp(10), buffer);

        snprintf(buffer, buffer_size, "Box center Y failed: %f", box.GetPosition().Y.As<float>());
        mu_assert(box.GetPosition().Y == Fxp(20), buffer);
    }

    MU_TEST(rigidbody_test_get_sphere_collider)
    {
        Rigidbody body(Vector3D(10, 20, 30), Fxp(1));
        Sphere sphere = body.GetSphereCollider(Fxp(5));

        snprintf(buffer, buffer_size, "Sphere center X failed: %f", sphere.GetPosition().X.As<float>());
        mu_assert(sphere.GetPosition().X == Fxp(10), buffer);

        snprintf(buffer, buffer_size, "Sphere radius failed: %f", sphere.GetRadius().As<float>());
        mu_assert(sphere.GetRadius() == Fxp(5), buffer);
    }

    MU_TEST(rigidbody_test_resolve_collision_static)
    {
        Rigidbody body(Vector3D(0, 0, 0), Fxp(1));
        body.Velocity = Vector3D(0, -10, 0);
        body.Restitution = Fxp(1);

        Collision::Result collision(true, Fxp(2), Vector3D(0, 1, 0));
        Physics::ResolveCollisionStatic(body, collision);

        snprintf(buffer, buffer_size, "Position after static resolve Y failed: %f", body.Position.Y.As<float>());
        mu_assert(body.Position.Y == Fxp(2), buffer);

        snprintf(buffer, buffer_size, "Velocity after static resolve should be positive: %f", body.Velocity.Y.As<float>());
        mu_assert(body.Velocity.Y > Fxp(0), buffer);
    }

    MU_TEST_SUITE(rigidbody_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&rigidbody_test_setup,
                                       &rigidbody_test_teardown,
                                       &rigidbody_test_output_header);

        MU_RUN_TEST(rigidbody_test_construction);
        MU_RUN_TEST(rigidbody_test_static_mass);
        MU_RUN_TEST(rigidbody_test_set_mass);
        MU_RUN_TEST(rigidbody_test_apply_impulse);
        MU_RUN_TEST(rigidbody_test_update_velocity);
        MU_RUN_TEST(rigidbody_test_update_acceleration);
        MU_RUN_TEST(rigidbody_test_apply_force);
        MU_RUN_TEST(rigidbody_test_stop);
        MU_RUN_TEST(rigidbody_test_is_at_rest);
        MU_RUN_TEST(rigidbody_test_kinematic);
        MU_RUN_TEST(rigidbody_test_get_box_collider);
        MU_RUN_TEST(rigidbody_test_get_sphere_collider);
        MU_RUN_TEST(rigidbody_test_resolve_collision_static);
    }
}

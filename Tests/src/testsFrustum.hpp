#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void frustum_test_setup(void) {}
    void frustum_test_teardown(void) {}

    void frustum_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_FRUSTUM****");
            }
            else
            {
                LogInfo("****UT_FRUSTUM_ERROR(S)****");
            }
        }
    }

    static Frustum make_test_frustum()
    {
        const Angle fov = Angle::FromDegrees(Fxp(int16_t{ 90 }));
        const Fxp aspect = Fxp(int16_t{ 4 }) / Fxp(int16_t{ 3 });
        const Fxp nearDist = Fxp(int16_t{ 1 });
        const Fxp farDist = Fxp(int16_t{ 10 });
        return Frustum(fov, aspect, nearDist, farDist);
    }

    MU_TEST(frustum_construction_and_plane_orientation)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        mu_assert(f.NearDist > 0, "NearDist should be positive");
        mu_assert(f.FarDist > f.NearDist, "FarDist should be greater than NearDist");

        // Near plane should face -Z, far plane should face +Z with identity view
        mu_assert(f.GetPlane(Frustum::PLANE_NEAR).Normal == Vector3D(int16_t{ 0 }, int16_t{ 0 }, int16_t{ -1 }), "Near plane normal should be -Z");
        mu_assert(f.GetPlane(Frustum::PLANE_FAR).Normal == Vector3D(int16_t{ 0 }, int16_t{ 0 }, int16_t{ 1 }), "Far plane normal should be +Z");

        // Planes should remain valid after Update()
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Frustum planes should be valid after Update()");
    }

    MU_TEST(frustum_classify_point_sphere_aabb)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        const Vector3D insidePoint(int16_t{ 0 }, int16_t{ 0 }, int16_t{ -5 });
        const Vector3D nearPlaneCenter(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -f.NearDist);
        const Vector3D farPlaneCenter(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -f.FarDist);
        const Vector3D behindNear(int16_t{ 0 }, int16_t{ 0 }, int16_t{ 0 });
        const Vector3D beyondFar(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -f.FarDist - Fxp(int16_t{ 1 }));

        mu_assert(f.Classify(insidePoint) != Frustum::FrustumRelationship::Outside, "Point inside should not be Outside");
        mu_assert(f.Classify(nearPlaneCenter) != Frustum::FrustumRelationship::Outside, "Point on near plane should not be Outside");
        mu_assert(f.Classify(farPlaneCenter) != Frustum::FrustumRelationship::Outside, "Point on far plane should not be Outside");
        mu_assert(f.Classify(behindNear) == Frustum::FrustumRelationship::Outside, "Point behind near should be Outside");
        mu_assert(f.Classify(beyondFar) == Frustum::FrustumRelationship::Outside, "Point beyond far should be Outside");

        mu_assert(f.Intersects(insidePoint), "Intersects(point) should be true for inside point");
        mu_assert(!f.Intersects(behindNear), "Intersects(point) should be false for outside point");

        const Fxp quarter = Fxp::BuildRaw(0x00004000);
        const Fxp half = Fxp::BuildRaw(0x00008000);

        const Sphere insideSphere(Vector3D(int16_t{ 0 }, int16_t{ 0 }, int16_t{ -5 }), Fxp(int16_t{ 1 }));
        const Sphere nearIntersectSphere(Vector3D(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -(f.NearDist + quarter)), half);
        const Sphere farIntersectSphere(Vector3D(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -(f.FarDist - quarter)), half);
        const Sphere outsideSphere(Vector3D(int16_t{ 0 }, int16_t{ 0 }, int16_t{ 0 }), quarter);
        mu_assert(f.Classify(insideSphere) != Frustum::FrustumRelationship::Outside, "Sphere inside should not be Outside");
        mu_assert(f.Classify(nearIntersectSphere) != Frustum::FrustumRelationship::Outside, "Sphere intersecting near plane should not be Outside");
        mu_assert(f.Classify(farIntersectSphere) != Frustum::FrustumRelationship::Outside, "Sphere intersecting far plane should not be Outside");
        mu_assert(f.Classify(outsideSphere) == Frustum::FrustumRelationship::Outside, "Sphere behind near should be Outside");

        const AABB insideAabb(Vector3D(int16_t{ 0 }, int16_t{ 0 }, int16_t{ -5 }), Vector3D(int16_t{ 1 }, int16_t{ 1 }, int16_t{ 1 }));
        const AABB nearIntersectAabb(Vector3D(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -(f.NearDist + quarter)), Vector3D(int16_t{ 1 }, int16_t{ 1 }, int16_t{ 1 }));
        const AABB farIntersectAabb(Vector3D(Fxp(int16_t{ 0 }), Fxp(int16_t{ 0 }), -(f.FarDist - quarter)), Vector3D(int16_t{ 1 }, int16_t{ 1 }, int16_t{ 1 }));
        const AABB outsideAabb(Vector3D(int16_t{ 100 }, int16_t{ 0 }, int16_t{ -5 }), Vector3D(int16_t{ 1 }, int16_t{ 1 }, int16_t{ 1 }));
        mu_assert(f.Classify(insideAabb) != Frustum::FrustumRelationship::Outside, "AABB near center should not be Outside");
        mu_assert(f.Classify(nearIntersectAabb) != Frustum::FrustumRelationship::Outside, "AABB intersecting near plane should not be Outside");
        mu_assert(f.Classify(farIntersectAabb) != Frustum::FrustumRelationship::Outside, "AABB intersecting far plane should not be Outside");
        mu_assert(f.Classify(outsideAabb) == Frustum::FrustumRelationship::Outside, "Far X AABB should be Outside");

        mu_assert(f.Intersects(insideSphere), "Intersects(sphere) should be true for inside sphere");
        mu_assert(!f.Intersects(outsideSphere), "Intersects(sphere) should be false for outside sphere");
    }

    MU_TEST(frustum_update_rotated_view_smoke)
    {
        // Rotate view so forward axis changes; smoke-test plane normals stay valid and we can still classify.
        const Matrix33 rot = Matrix33::CreateRotation(Angle::Zero(), Angle::FromDegrees(Fxp(int16_t{ 90 })), Angle::Zero());
        const Matrix43 view(rot, Vector3D::Zero());

        Frustum f = make_test_frustum();
        f.Update(view);

        // All planes should remain valid (non-zero normal)
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Rotated frustum planes should be valid");

        // Points on/inside the rotated frustum should not be classified as Outside.
        const Vector3D nearCenter = view.Row3 - view.Row2 * f.NearDist;
        const Vector3D midPoint = view.Row3 - view.Row2 * ((f.NearDist + f.FarDist) / Fxp(int16_t{ 2 }));
        mu_assert(f.Classify(nearCenter) != Frustum::FrustumRelationship::Outside, "Near plane center should not be Outside in rotated view");
        mu_assert(f.Classify(midPoint) != Frustum::FrustumRelationship::Outside, "Mid frustum point should not be Outside in rotated view");
    }

    MU_TEST_SUITE(frustum_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&frustum_test_setup,
                                       &frustum_test_teardown,
                                       &frustum_test_output_header);

        MU_RUN_TEST(frustum_construction_and_plane_orientation);
        MU_RUN_TEST(frustum_classify_point_sphere_aabb);
        MU_RUN_TEST(frustum_update_rotated_view_smoke);
    }
}

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

    static constexpr Frustum make_test_frustum()
    {
        const Angle fov = Angle::FromDegrees(90);
        const Fxp aspect = Fxp(4) / 3;
        const Fxp nearDist = 1;
        const Fxp farDist = 10;
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
        mu_assert(f.GetPlane(Frustum::PLANE_NEAR).Normal == Vector3D(0, 0, -1), "Near plane normal should be -Z");
        mu_assert(f.GetPlane(Frustum::PLANE_FAR).Normal == Vector3D(0, 0, 1), "Far plane normal should be +Z");

        // Side plane normals should point inward (sign checks)
        mu_assert(f.GetPlane(Frustum::PLANE_LEFT).Normal.X > 0, "Left plane normal X should be positive");
        mu_assert(f.GetPlane(Frustum::PLANE_RIGHT).Normal.X < 0, "Right plane normal X should be negative");
        mu_assert(f.GetPlane(Frustum::PLANE_TOP).Normal.Y < 0, "Top plane normal Y should be negative");
        mu_assert(f.GetPlane(Frustum::PLANE_BOTTOM).Normal.Y > 0, "Bottom plane normal Y should be positive");
    }

    MU_TEST(frustum_classify_point_sphere_aabb)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        const Vector3D insidePoint(0, 0, -5);
        const Vector3D nearPlaneCenter(0, 0, -f.NearDist);
        const Vector3D behindNear(0, 0, 0);
        const Vector3D beyondFar(0, 0, -f.FarDist - 1);

        mu_assert(f.Classify(insidePoint) == Frustum::FrustumRelationship::Inside, "Point inside should be Inside");
        mu_assert(f.Classify(nearPlaneCenter) == Frustum::FrustumRelationship::Intersects, "Point on near plane should Intersect");
        mu_assert(f.Classify(behindNear) == Frustum::FrustumRelationship::Outside, "Point behind near should be Outside");
        mu_assert(f.Classify(beyondFar) == Frustum::FrustumRelationship::Outside, "Point beyond far should be Outside");

        mu_assert(f.Intersects(insidePoint), "Intersects(point) should be true for inside point");
        mu_assert(!f.Intersects(behindNear), "Intersects(point) should be false for outside point");

        const Sphere insideSphere(Vector3D(0, 0, -5), 1);
        const Sphere outsideSphere(Vector3D(0, 0, 0), Fxp(0.25));
        mu_assert(f.Classify(insideSphere) == Frustum::FrustumRelationship::Inside, "Sphere inside should be Inside");
        mu_assert(f.Classify(outsideSphere) == Frustum::FrustumRelationship::Outside, "Sphere behind near should be Outside");

        const AABB insideAabb(Vector3D(0, 0, -5), Vector3D(1, 1, 1));
        const AABB outsideAabb(Vector3D(100, 0, -5), Vector3D(1, 1, 1));
        mu_assert(f.Classify(insideAabb) != Frustum::FrustumRelationship::Outside, "AABB near center should not be Outside");
        mu_assert(f.Classify(outsideAabb) == Frustum::FrustumRelationship::Outside, "Far X AABB should be Outside");

        mu_assert(f.Intersects(insideSphere), "Intersects(sphere) should be true for inside sphere");
        mu_assert(!f.Intersects(outsideSphere), "Intersects(sphere) should be false for outside sphere");
    }

    MU_TEST_SUITE(frustum_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&frustum_test_setup,
                                       &frustum_test_teardown,
                                       &frustum_test_output_header);

        MU_RUN_TEST(frustum_construction_and_plane_orientation);
        MU_RUN_TEST(frustum_classify_point_sphere_aabb);
    }
}

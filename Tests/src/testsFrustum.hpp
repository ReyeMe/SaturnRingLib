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
    const Angle fov = Angle::FromDegrees(Fxp(int16_t{90}));
    const Fxp aspect = Fxp(int16_t{4}) / Fxp(int16_t{3});
    const Fxp nearDist = Fxp(int16_t{1});
    const Fxp farDist = Fxp(int16_t{10});
    return Frustum(fov, aspect, nearDist, farDist);
}

// Additional completeness tests
MU_TEST(frustum_extreme_values)
{
    constexpr Matrix43 view = Matrix43::Identity();
    // Extremely large FOV
    Frustum f_large_fov(Angle::FromDegrees(Fxp(int16_t{179})), Fxp(int16_t{1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
    f_large_fov.Update(view);
    mu_assert(f_large_fov.NearHeight > Fxp(int16_t{0}), "NearHeight should be positive for large FOV");

    // Extremely small FOV
    Frustum f_small_fov(Angle::FromDegrees(Fxp(int16_t{1})), Fxp(int16_t{1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
    f_small_fov.Update(view);
    mu_assert(f_small_fov.NearHeight > Fxp(int16_t{0}), "NearHeight should be positive for small FOV");

    // Extremely large aspect ratio
    Frustum f_large_aspect(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{1000}), Fxp(int16_t{1}), Fxp(int16_t{10}));
    f_large_aspect.Update(view);
    mu_assert(f_large_aspect.NearWidth > Fxp(int16_t{0}), "NearWidth should be positive for large aspect ratio");

    // Extremely small aspect ratio
    Frustum f_small_aspect(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
    f_small_aspect.Update(view);
    mu_assert(f_small_aspect.NearWidth > Fxp(int16_t{0}), "NearWidth should be positive for small aspect ratio");

    // Extremely large near/far distances
    Frustum f_large_dist(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{1}), Fxp(int16_t{10000}), Fxp(int16_t{20000}));
    f_large_dist.Update(view);
    mu_assert(f_large_dist.FarDist > f_large_dist.NearDist, "FarDist should be greater than NearDist for large distances");
}

MU_TEST(frustum_degenerate_values)
{
    constexpr Matrix43 view = Matrix43::Identity();
    // All parameters zero
    Frustum f_zero(Angle::Zero(), Fxp(int16_t{0}), Fxp(int16_t{0}), Fxp(int16_t{0}));
    f_zero.Update(view);
    mu_assert(f_zero.NearHeight == Fxp(int16_t{0}), "NearHeight should be 0 for zero params");
    mu_assert(f_zero.NearWidth == Fxp(int16_t{0}), "NearWidth should be 0 for zero params");
    mu_assert(f_zero.NearDist == Fxp(int16_t{0}), "NearDist should be 0 for zero params");
    mu_assert(f_zero.FarDist == Fxp(int16_t{0}), "FarDist should be 0 for zero params");
}

MU_TEST(frustum_copy_move_semantics)
{
    Frustum f1 = make_test_frustum();
    Frustum f2(f1); // Copy constructor
    mu_assert(f2.NearDist == f1.NearDist, "Copy constructor NearDist");
    Frustum f3 = std::move(f1); // Move constructor
    mu_assert(f3.NearDist == f2.NearDist, "Move constructor NearDist");
    Frustum f4 = make_test_frustum();
    f4 = f2; // Copy assignment
    mu_assert(f4.NearDist == f2.NearDist, "Copy assignment NearDist");
    Frustum f5 = make_test_frustum();
    f5 = std::move(f3); // Move assignment
    mu_assert(f5.NearDist == f2.NearDist, "Move assignment NearDist");
}

MU_TEST(frustum_frustum_intersection)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f1 = make_test_frustum();
    Frustum f2 = make_test_frustum();
    f1.Update(view);
    f2.Update(view);
    // Place a point inside both frustums
    const Vector3D pt(int16_t{0}, int16_t{0}, int16_t{-5});
    mu_assert(f1.Classify(pt) != Frustum::FrustumRelationship::Outside, "pt inside f1");
    mu_assert(f2.Classify(pt) != Frustum::FrustumRelationship::Outside, "pt inside f2");
    // Place a point outside both frustums
    const Vector3D pt_out(int16_t{100}, int16_t{100}, int16_t{100});
    mu_assert(f1.Classify(pt_out) == Frustum::FrustumRelationship::Outside, "pt_out outside f1");
    mu_assert(f2.Classify(pt_out) == Frustum::FrustumRelationship::Outside, "pt_out outside f2");
}
    MU_TEST(frustum_construction_and_plane_orientation)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        mu_assert(f.NearDist > 0, "NearDist should be positive");
        mu_assert(f.FarDist > f.NearDist, "FarDist should be greater than NearDist");

        // Near plane should face -Z, far plane should face +Z with identity view
        mu_assert(f.GetPlane(Frustum::PLANE_NEAR).Normal == Vector3D(int16_t{0}, int16_t{0}, int16_t{-1}), "Near plane normal should be -Z");
        mu_assert(f.GetPlane(Frustum::PLANE_FAR).Normal == Vector3D(int16_t{0}, int16_t{0}, int16_t{1}), "Far plane normal should be +Z");

        // Planes should remain valid after Update()
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Frustum planes should be valid after Update()");
    }

    MU_TEST(frustum_classify_point_sphere_aabb)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        const Vector3D insidePoint(int16_t{0}, int16_t{0}, int16_t{-5});
        const Vector3D nearPlaneCenter(Fxp(int16_t{0}), Fxp(int16_t{0}), -f.NearDist);
        const Vector3D farPlaneCenter(Fxp(int16_t{0}), Fxp(int16_t{0}), -f.FarDist);
        const Vector3D behindNear(int16_t{0}, int16_t{0}, int16_t{0});
        const Vector3D beyondFar(Fxp(int16_t{0}), Fxp(int16_t{0}), -f.FarDist - Fxp(int16_t{1}));

        mu_assert(f.Classify(insidePoint) != Frustum::FrustumRelationship::Outside, "Point inside should not be Outside");
        mu_assert(f.Classify(nearPlaneCenter) != Frustum::FrustumRelationship::Outside, "Point on near plane should not be Outside");
        mu_assert(f.Classify(farPlaneCenter) != Frustum::FrustumRelationship::Outside, "Point on far plane should not be Outside");
        mu_assert(f.Classify(behindNear) == Frustum::FrustumRelationship::Outside, "Point behind near should be Outside");
        mu_assert(f.Classify(beyondFar) == Frustum::FrustumRelationship::Outside, "Point beyond far should be Outside");

        mu_assert(f.Intersects(insidePoint), "Intersects(point) should be true for inside point");
        mu_assert(!f.Intersects(behindNear), "Intersects(point) should be false for outside point");

        const Fxp quarter = Fxp::BuildRaw(0x00004000);
        const Fxp half = Fxp::BuildRaw(0x00008000);

        const Sphere insideSphere(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5}), Fxp(int16_t{1}));
        const Sphere nearIntersectSphere(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(f.NearDist + quarter)), half);
        const Sphere farIntersectSphere(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(f.FarDist - quarter)), half);
        const Sphere outsideSphere(Vector3D(int16_t{0}, int16_t{0}, int16_t{0}), quarter);
        mu_assert(f.Classify(insideSphere) != Frustum::FrustumRelationship::Outside, "Sphere inside should not be Outside");
        mu_assert(f.Classify(nearIntersectSphere) != Frustum::FrustumRelationship::Outside, "Sphere intersecting near plane should not be Outside");
        mu_assert(f.Classify(farIntersectSphere) != Frustum::FrustumRelationship::Outside, "Sphere intersecting far plane should not be Outside");
        mu_assert(f.Classify(outsideSphere) == Frustum::FrustumRelationship::Outside, "Sphere behind near should be Outside");

        const AABB insideAabb(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5}), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
        const AABB nearIntersectAabb(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(f.NearDist + quarter)), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
        const AABB farIntersectAabb(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(f.FarDist - quarter)), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
        const AABB outsideAabb(Vector3D(int16_t{100}, int16_t{0}, int16_t{-5}), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
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
        const Matrix33 rot = Matrix33::CreateRotation(Angle::Zero(), Angle::FromDegrees(Fxp(int16_t{90})), Angle::Zero());
        const Matrix43 view(rot, Vector3D::Zero());

        Frustum f = make_test_frustum();
        f.Update(view);

        // All planes should remain valid (non-zero normal)
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Rotated frustum planes should be valid");

        // Points on/inside the rotated frustum should not be classified as Outside.
        const Vector3D nearCenter = view.Row3 - view.Row2 * f.NearDist;
        const Vector3D midPoint = view.Row3 - view.Row2 * ((f.NearDist + f.FarDist) / Fxp(int16_t{2}));
        mu_assert(f.Classify(nearCenter) != Frustum::FrustumRelationship::Outside, "Near plane center should not be Outside in rotated view");
        mu_assert(f.Classify(midPoint) != Frustum::FrustumRelationship::Outside, "Mid frustum point should not be Outside in rotated view");
    }

    MU_TEST(frustum_invalid_construction)
    {
        constexpr Matrix43 view = Matrix43::Identity();

        // Test with zero FOV
        Frustum f_zero_fov(Angle::Zero(), Fxp(int16_t{1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
        f_zero_fov.Update(view);
        mu_assert(f_zero_fov.NearHeight == Fxp(int16_t{0}), "NearHeight should be 0 for 0 FOV");
        mu_assert(f_zero_fov.NearWidth == Fxp(int16_t{0}), "NearWidth should be 0 for 0 FOV");

        // Test with negative FOV
        Frustum f_neg_fov(Angle::FromDegrees(Fxp(int16_t{-90})), Fxp(int16_t{1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
        f_neg_fov.Update(view);
        mu_assert(f_neg_fov.NearHeight < Fxp(int16_t{0}), "NearHeight should be negative for negative FOV");

        // Test with zero aspect ratio
        Frustum f_zero_aspect(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{0}), Fxp(int16_t{1}), Fxp(int16_t{10}));
        f_zero_aspect.Update(view);
        mu_assert(f_zero_aspect.NearWidth == Fxp(int16_t{0}), "NearWidth should be 0 for 0 aspect ratio");

        // Test with negative aspect ratio
        Frustum f_neg_aspect(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{-1}), Fxp(int16_t{1}), Fxp(int16_t{10}));
        f_neg_aspect.Update(view);
        mu_assert(f_neg_aspect.NearWidth < Fxp(int16_t{0}), "NearWidth should be negative for negative aspect ratio");

        // Test with near >= far
        Frustum f_near_far(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{1}), Fxp(int16_t{10}), Fxp(int16_t{1}));
        f_near_far.Update(view);
        mu_assert(f_near_far.NearDist >= f_near_far.FarDist, "NearDist is >= FarDist");
        // We expect that things might not work correctly, but it shouldn't crash.
        // Let's check if a point inside the "inverted" frustum is still classified as outside.
        const Vector3D point_in_inverted(int16_t{0}, int16_t{0}, int16_t{-5});
        mu_assert(f_near_far.Classify(point_in_inverted) == Frustum::FrustumRelationship::Outside, "Point should be outside an inverted frustum");
    }
    MU_TEST(frustum_boundary_conditions)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        // Point on near plane
        const Vector3D point_on_near(int16_t{0}, int16_t{0}, -f.NearDist);
        mu_assert(f.Classify(point_on_near) == Frustum::FrustumRelationship::Intersects, "Point on near plane should be Intersects");

        // Point on far plane
        const Vector3D point_on_far(int16_t{0}, int16_t{0}, -f.FarDist);
        mu_assert(f.Classify(point_on_far) == Frustum::FrustumRelationship::Intersects, "Point on far plane should be Intersects");

        // Sphere touching near plane
        const Sphere sphere_touching_near(Vector3D(int16_t{0}, int16_t{0}, -f.NearDist - Fxp(int16_t{1})), Fxp(int16_t{1}));
        mu_assert(f.Classify(sphere_touching_near) == Frustum::FrustumRelationship::Intersects, "Sphere touching near plane should be Intersects");

        // Sphere touching far plane
        const Sphere sphere_touching_far(Vector3D(int16_t{0}, int16_t{0}, -f.FarDist + Fxp(int16_t{1})), Fxp(int16_t{1}));
        mu_assert(f.Classify(sphere_touching_far) == Frustum::FrustumRelationship::Intersects, "Sphere touching far plane should be Intersects");

        // AABB touching near plane
        const AABB aabb_touching_near(Vector3D(int16_t{0}, int16_t{0}, -f.NearDist - Fxp(int16_t{1})), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
        mu_assert(f.Classify(aabb_touching_near) == Frustum::FrustumRelationship::Intersects, "AABB touching near plane should be Intersects");

        // AABB touching far plane
        const AABB aabb_touching_far(Vector3D(int16_t{0}, int16_t{0}, -f.FarDist + Fxp(int16_t{1})), Vector3D(int16_t{1}, int16_t{1}, int16_t{1}));
        mu_assert(f.Classify(aabb_touching_far) == Frustum::FrustumRelationship::Intersects, "AABB touching far plane should be Intersects");
    }
    MU_TEST(frustum_containing_aabb)
    {
        constexpr Matrix43 view = Matrix43::Identity();
        Frustum f = make_test_frustum();
        f.Update(view);

        const Vector3D center = Vector3D(int16_t{0}, int16_t{0}, -(f.NearDist + f.FarDist) / Fxp(int16_t{2}));
        const Vector3D size = Vector3D(f.FarWidth, f.FarHeight, (f.FarDist - f.NearDist) / Fxp(int16_t{2})) * Fxp(int16_t{2});
        const AABB containing_aabb(center, size);

        mu_assert(f.Classify(containing_aabb) == Frustum::FrustumRelationship::Intersects, "AABB containing frustum should be Intersects");
    }
    MU_TEST(frustum_translated_view)
    {
        const Vector3D translation(int16_t{10}, int16_t{20}, int16_t{30});
        const Matrix43 view = Matrix43::CreateTranslation(translation);

        Frustum f = make_test_frustum();
        f.Update(view);

        // All planes should remain valid (non-zero normal)
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Translated frustum planes should be valid");

        // A point inside the translated frustum should be classified as Inside.
        const Vector3D insidePoint = translation + Vector3D(int16_t{0}, int16_t{0}, -(f.NearDist + f.FarDist) / Fxp(int16_t{2}));
        mu_assert(f.Classify(insidePoint) != Frustum::FrustumRelationship::Outside, "Point inside translated frustum should not be Outside");
    }
    MU_TEST(frustum_translated_rotated_view)
    {
        const Vector3D translation(int16_t{10}, int16_t{20}, int16_t{30});
        const Matrix33 rotation = Matrix33::CreateRotation(Angle::FromDegrees(Fxp(int16_t{45})), Angle::FromDegrees(Fxp(int16_t{45})), Angle::FromDegrees(Fxp(int16_t{45})));
        const Matrix43 view(rotation, translation);

        Frustum f = make_test_frustum();
        f.Update(view);

        // All planes should remain valid (non-zero normal)
        for (size_t i = 0; i < Frustum::PLANE_COUNT; i++)
            mu_assert(f.GetPlane(i).IsValid(), "Translated/rotated frustum planes should be valid");

        // A point inside the transformed frustum should be classified as Inside.
        const Vector3D insidePoint = translation - view.Row2 * ((f.NearDist + f.FarDist) / Fxp(int16_t{2}));
        mu_assert(f.Classify(insidePoint) != Frustum::FrustumRelationship::Outside, "Point inside translated/rotated frustum should not be Outside");
    }

    MU_TEST_SUITE(frustum_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&frustum_test_setup,
                                       &frustum_test_teardown,
                                       &frustum_test_output_header);

        MU_RUN_TEST(frustum_construction_and_plane_orientation);
        MU_RUN_TEST(frustum_classify_point_sphere_aabb);
        MU_RUN_TEST(frustum_update_rotated_view_smoke);

        // Merged extended tests
        MU_RUN_TEST(frustum_invalid_construction);
        MU_RUN_TEST(frustum_boundary_conditions);
        MU_RUN_TEST(frustum_containing_aabb);
        MU_RUN_TEST(frustum_translated_view);
        MU_RUN_TEST(frustum_translated_rotated_view);
    }
}

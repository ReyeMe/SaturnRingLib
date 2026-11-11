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

// Test frustum: 90 degree vertical FOV, 4:3 aspect, near 1, far 10, looking down -Z.
// Frustum has no accessors in this SaturnMathPP version, so tests use these values directly.
static const Fxp kNear = Fxp(int16_t{1});
static const Fxp kFar = Fxp(int16_t{10});
static const Fxp kFarHeight = kFar;                                  // tan(45 degrees) * far
static const Fxp kFarWidth = kFarHeight * Fxp(int16_t{4}) / Fxp(int16_t{3});

static Frustum make_test_frustum()
{
    return Frustum(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{4}) / Fxp(int16_t{3}), kNear, kFar);
}

    /**
 * @brief Tests that two identical frustums agree on a point inside and a point outside.
 */
MU_TEST(frustum_frustum_intersection)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f1 = make_test_frustum();
    Frustum f2 = make_test_frustum();
    f1.Update(view);
    f2.Update(view);

    const Vector3D pt(int16_t{0}, int16_t{0}, int16_t{-5});
    mu_assert(f1.Contains(pt), "pt inside f1");
    mu_assert(f2.Contains(pt), "pt inside f2");

    const Vector3D pt_out(int16_t{100}, int16_t{100}, int16_t{100});
    mu_assert(!f1.Contains(pt_out), "pt_out outside f1");
    mu_assert(!f2.Contains(pt_out), "pt_out outside f2");
}

    /**
 * @brief Tests Contains() for points, spheres and AABBs inside, crossing and outside the frustum.
 * @details Contains() is true for shapes at least partially inside.
 */
MU_TEST(frustum_contains_point_sphere_aabb)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f = make_test_frustum();
    f.Update(view);

        // Point tests
    mu_assert(f.Contains(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5})), "Point inside should be contained");
    mu_assert(f.Contains(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -kNear)), "Point on near plane should be contained");
    mu_assert(f.Contains(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -kFar)), "Point on far plane should be contained");
    mu_assert(!f.Contains(Vector3D(int16_t{0}, int16_t{0}, int16_t{0})), "Point behind near should be outside");
    mu_assert(!f.Contains(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -kFar - Fxp(int16_t{1}))), "Point beyond far should be outside");

        // Sphere tests
    const Fxp quarter = Fxp::BuildRaw(0x00004000);
    const Fxp half = Fxp::BuildRaw(0x00008000);

    mu_assert(f.Contains(Sphere(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5}), Fxp(int16_t{1}))), "Sphere inside should be contained");
    mu_assert(f.Contains(Sphere(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kNear + quarter)), half)), "Sphere crossing near plane should be contained");
    mu_assert(f.Contains(Sphere(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kFar - quarter)), half)), "Sphere crossing far plane should be contained");
    mu_assert(!f.Contains(Sphere(Vector3D(int16_t{0}, int16_t{0}, int16_t{0}), quarter)), "Sphere behind near should be outside");
    mu_assert(f.Contains(Sphere(Vector3D(0, 0, -5), Fxp(int16_t{10}))), "Sphere containing frustum should be contained");

        // AABB tests
    const Vector3D one(int16_t{1}, int16_t{1}, int16_t{1});
    mu_assert(f.Contains(AABB(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5}), one)), "AABB near center should be contained");
    mu_assert(f.Contains(AABB(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kNear + quarter)), one)), "AABB crossing near plane should be contained");
    mu_assert(f.Contains(AABB(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kFar - quarter)), one)), "AABB crossing far plane should be contained");
    mu_assert(!f.Contains(AABB(Vector3D(int16_t{100}, int16_t{0}, int16_t{-5}), one)), "Far X AABB should be outside");
    mu_assert(f.Contains(AABB(Vector3D(0, 0, -5), Vector3D(10, 10, 10))), "AABB containing frustum should be contained");
}

    /**
 * @brief Tests that points along the view direction stay inside after rotating the view.
 */
MU_TEST(frustum_update_rotated_view)
{
    const Matrix33 rot = Matrix33::CreateRotation(Angle::Zero(), Angle::FromDegrees(Fxp(int16_t{90})), Angle::Zero());
    const Matrix43 view(rot, Vector3D::Zero());

    Frustum f = make_test_frustum();
    f.Update(view);

    const Vector3D nearCenter = view.Row3 - view.Row2 * kNear;
    const Vector3D midPoint = view.Row3 - view.Row2 * ((kNear + kFar) / Fxp(int16_t{2}));
    mu_assert(f.Contains(nearCenter), "Near plane center should be inside in rotated view");
    mu_assert(f.Contains(midPoint), "Mid frustum point should be inside in rotated view");
}

    /**
 * @brief Tests that an inverted frustum (near >= far) contains nothing.
 */
MU_TEST(frustum_invalid_construction)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f_near_far(Angle::FromDegrees(Fxp(int16_t{90})), Fxp(int16_t{1}), Fxp(int16_t{10}), Fxp(int16_t{1}));
    f_near_far.Update(view);
    mu_assert(!f_near_far.Contains(Vector3D(int16_t{0}, int16_t{0}, int16_t{-5})), "Point should be outside an inverted frustum");
}

    /**
 * @brief Tests points, spheres and AABBs exactly on or touching the near and far planes.
 */
MU_TEST(frustum_boundary_conditions)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f = make_test_frustum();
    f.Update(view);

    mu_assert(f.Contains(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -kNear)), "Point on near plane should be contained");
    mu_assert(f.Contains(Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -kFar)), "Point on far plane should be contained");

    const Fxp zero = Fxp(int16_t{0});
    const Fxp one = Fxp(int16_t{1});
    mu_assert(f.Contains(Sphere(Vector3D(zero, zero, -kNear - one), one)), "Sphere touching near plane should be contained");
    mu_assert(f.Contains(Sphere(Vector3D(zero, zero, -kFar + one), one)), "Sphere touching far plane should be contained");
    mu_assert(f.Contains(AABB(Vector3D(zero, zero, -kNear - one), Vector3D(one, one, one))), "AABB touching near plane should be contained");
    mu_assert(f.Contains(AABB(Vector3D(zero, zero, -kFar + one), Vector3D(one, one, one))), "AABB touching far plane should be contained");
}

    /**
 * @brief Tests an AABB that encloses the whole frustum, and one far away.
 */
MU_TEST(frustum_containing_aabb)
{
    constexpr Matrix43 view = Matrix43::Identity();
    Frustum f = make_test_frustum();
    f.Update(view);

    const Vector3D center(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kNear + kFar) / Fxp(int16_t{2}));
    const Vector3D size = Vector3D(kFarWidth, kFarHeight, (kFar - kNear) / Fxp(int16_t{2})) * Fxp(int16_t{2});
    mu_assert(f.Contains(AABB(center, size)), "AABB containing frustum should be contained");

    const AABB far_aabb(Vector3D(int16_t{1000}, int16_t{1000}, int16_t{1000}), Vector3D(int16_t{10}, int16_t{10}, int16_t{10}));
    mu_assert(!f.Contains(far_aabb), "AABB far from frustum should be outside");
}

    /**
 * @brief Tests a frustum moved by a view matrix (translation only).
 */
MU_TEST(frustum_translated_view)
{
    const Vector3D eye(10, 20, 30);
    const Vector3D target = eye + Vector3D(0, 0, -1);
    // Matrix43::CreateLookAt does not compile in this SaturnMathPP version; the camera looks down -Row2
    const Matrix43 view(Vector3D::UnitY(), eye - target, eye);

    Frustum f = make_test_frustum();
    f.Update(view);

    const Vector3D insidePoint = eye + Vector3D(Fxp(int16_t{0}), Fxp(int16_t{0}), -(kNear + kFar) / Fxp(int16_t{2}));
    mu_assert(f.Contains(insidePoint), "Point inside translated frustum should be contained");
}

    /**
 * @brief Tests a frustum moved and rotated by a view matrix looking at a target.
 */
MU_TEST(frustum_translated_rotated_view)
{
    const Vector3D eye(10, 20, 30);
    const Vector3D target(5, 15, 25);
    // Matrix43::CreateLookAt does not compile in this SaturnMathPP version; the camera looks down -Row2
    const Matrix43 view(Vector3D::UnitY(), eye - target, eye);

    Frustum f = make_test_frustum();
    f.Update(view);

    const Vector3D forward = (target - eye).Normalize<SRL::Math::Precision::Accurate>();
    const Vector3D insidePoint = eye + forward * ((kNear + kFar) / Fxp(int16_t{2}));
    mu_assert(f.Contains(insidePoint), "Point inside translated/rotated frustum should be contained");
}

MU_TEST_SUITE(frustum_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&frustum_test_setup,
        &frustum_test_teardown,
        &frustum_test_output_header);

    MU_RUN_TEST(frustum_frustum_intersection);
    MU_RUN_TEST(frustum_contains_point_sphere_aabb);
    MU_RUN_TEST(frustum_update_rotated_view);
    MU_RUN_TEST(frustum_invalid_construction);
    MU_RUN_TEST(frustum_boundary_conditions);
    MU_RUN_TEST(frustum_containing_aabb);
    MU_RUN_TEST(frustum_translated_view);
    MU_RUN_TEST(frustum_translated_rotated_view);
}
}

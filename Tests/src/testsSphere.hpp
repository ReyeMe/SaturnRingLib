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

    void sphere_test_setup(void) {}
    void sphere_test_teardown(void) {}

    static inline bool fxp_near_sphere(const Fxp& a, const Fxp& b, const Fxp& tol)
    {
        return (a - b).Abs() <= tol;
    }

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

    MU_TEST(sphere_default_and_basic_properties)
    {
        const Sphere s;
        mu_assert(s.GetPosition() == Vector3D::Zero(), "Default sphere center should be origin");
        mu_assert(s.GetRadius() == 1, "Default sphere radius should be 1");
        mu_assert(s.IsValid(), "Default sphere should be valid");
        mu_assert(s.GetDiameter() == 2, "Diameter should be 2*r");

        // Formula checks (avoid hardcoded Pi decimal)
        mu_assert(s.GetSurfaceArea() == (4 * Fxp::Pi() * 1 * 1), "Surface area should be 4*pi*r^2");
        mu_assert(s.GetVolume() == ((Fxp(4) / 3) * Fxp::Pi() * 1 * 1 * 1), "Volume should be (4/3)*pi*r^3");
    }

    MU_TEST(sphere_validity_and_degenerate_radius)
    {
        const Sphere zero(Vector3D(1, 2, 3), 0);
        mu_assert(zero.IsValid(), "Zero-radius sphere should be valid (point sphere)");

        const Sphere neg(Vector3D::Zero(), -1);
        mu_assert(!neg.IsValid(), "Negative-radius sphere should be invalid");

        // Degenerate case: GetClosestPoint returns center when radius <= 0
        mu_assert(zero.GetClosestPoint<SRL::Math::Precision::Accurate>(Vector3D(9, 9, 9)) == zero.GetPosition(),
                  "GetClosestPoint on zero-radius sphere should return center");
        mu_assert(neg.GetClosestPoint<SRL::Math::Precision::Accurate>(Vector3D(9, 9, 9)) == neg.GetPosition(),
                  "GetClosestPoint on negative-radius sphere should return center");
    }

    MU_TEST(sphere_intersects_translate_scale)
    {
        const Sphere a(Vector3D::Zero(), 1);
        const Sphere b(Vector3D(2, 0, 0), 1);
        mu_assert(a.Intersects(b), "Touching spheres should intersect");

        const Sphere c(Vector3D(Fxp(2.2), 0, 0), 1);
        mu_assert(!a.Intersects(c), "Separated spheres should not intersect");

        const Sphere t = a.Translate(Vector3D(1, 2, 3));
        mu_assert(t.GetPosition() == Vector3D(1, 2, 3), "Translate should move center");
        mu_assert(t.GetRadius() == 1, "Translate should not change radius");

        const Sphere s2 = t.Scale(2);
        mu_assert(s2.GetPosition() == Vector3D(2, 4, 6), "Scale(uniform) should scale center");
        mu_assert(s2.GetRadius() == 2, "Scale(uniform) should scale radius");

        const Sphere snu = Sphere(Vector3D(1, 2, 3), 10).Scale(Vector3D(2, 3, 1));
        mu_assert(snu.GetPosition() == Vector3D(2, 6, 3), "Scale(non-uniform) should scale position component-wise");
        mu_assert(snu.GetRadius() == 10, "Scale(non-uniform) should use min scale component for radius");
    }

    MU_TEST(sphere_closest_point_cases)
    {
        const Sphere s(Vector3D::Zero(), 2);

        // At center: should return (r,0,0) offset
        mu_assert(s.GetClosestPoint<SRL::Math::Precision::Accurate>(Vector3D::Zero()) == Vector3D(2, 0, 0),
                  "Closest point from center should be (r,0,0) from center");

        // Inside: returns the point itself
        const Vector3D inside(1, 0, 0);
        mu_assert(s.GetClosestPoint<SRL::Math::Precision::Accurate>(inside) == inside,
                  "Closest point for inside point should be point itself");

        // Outside along axis: should clamp to surface
        const Vector3D outside(4, 0, 0);
        mu_assert(s.GetClosestPoint<SRL::Math::Precision::Accurate>(outside) == Vector3D(2, 0, 0),
                  "Closest point for outside point on +X should be (r,0,0)");

        // Outside diagonal: result should be on surface (length ~= r)
        const Vector3D diag(4, 4, 4);
        const Vector3D closest = s.GetClosestPoint<SRL::Math::Precision::Accurate>(diag);
        const Fxp len = closest.Length<SRL::Math::Precision::Accurate>();
        mu_assert(fxp_near_sphere(len, Fxp(2), Fxp(0.05)), "Closest point should lie on sphere surface");
    }

    MU_TEST_SUITE(sphere_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&sphere_test_setup,
                                       &sphere_test_teardown,
                                       &sphere_test_output_header);

        MU_RUN_TEST(sphere_default_and_basic_properties);
        MU_RUN_TEST(sphere_validity_and_degenerate_radius);
        MU_RUN_TEST(sphere_intersects_translate_scale);
        MU_RUN_TEST(sphere_closest_point_cases);
    }
}

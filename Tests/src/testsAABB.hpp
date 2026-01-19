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

    void aabb_test_setup(void)
    {
        // No initialization needed
    }

    void aabb_test_teardown(void)
    {
        // No cleanup required
    }

    void aabb_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_AABB****");
            }
            else
            {
                LogInfo("****UT_AABB_ERROR(S)****");
            }
        }
    }

    MU_TEST(aabb_default_construction)
    {
        constexpr AABB box;
        mu_assert(box.GetPosition() == Vector3D::Zero(), "Default AABB position is not zero");
        mu_assert(box.GetHalfExtents() == Vector3D::Zero(), "Default AABB half-extents are not zero");
        mu_assert(box.IsDegenerate(), "Default AABB should be degenerate");
        mu_assert(box.GetMin() == Vector3D::Zero(), "Default AABB min should be zero");
        mu_assert(box.GetMax() == Vector3D::Zero(), "Default AABB max should be zero");
    }

    MU_TEST(aabb_construction_center_and_uniform_size)
    {
        constexpr Vector3D center(1, 2, 3);
        constexpr Fxp size(4);
        constexpr AABB box(center, size);
        mu_assert(box.GetPosition() == center, "AABB center+size ctor did not set position");
        mu_assert(box.GetHalfExtents() == Vector3D(size, size, size), "AABB center+size ctor did not set half-extents");
        mu_assert(!box.IsDegenerate(), "Non-zero uniform AABB should not be degenerate");
    }

    MU_TEST(aabb_construction_negative_uniform_size)
    {
        constexpr Vector3D center(1, 2, 3);
        constexpr Fxp negSize(-4);
        constexpr AABB box(center, negSize);
        mu_assert(box.GetPosition() == center, "Negative size ctor should keep center");
        mu_assert(box.GetHalfExtents() == Vector3D(4, 4, 4), "Negative size ctor should use magnitude");
        mu_assert(box.GetMin() == Vector3D(-3, -2, -1), "Negative size ctor should produce correct min");
        mu_assert(box.GetMax() == Vector3D(5, 6, 7), "Negative size ctor should produce correct max");
    }

    MU_TEST(aabb_construction_center_and_half_extents)
    {
        constexpr Vector3D center(1, 2, 3);
        constexpr Vector3D halfExtents(1, 2, 3);
        constexpr AABB box(center, halfExtents);
        mu_assert(box.GetPosition() == center, "AABB center+halfExtents ctor did not set position");
        mu_assert(box.GetHalfExtents() == halfExtents, "AABB center+halfExtents ctor did not set half-extents");
        mu_assert(box.GetMin() == Vector3D(0, 0, 0), "AABB min incorrect for center+halfExtents");
        mu_assert(box.GetMax() == Vector3D(2, 4, 6), "AABB max incorrect for center+halfExtents");
    }

    MU_TEST(aabb_construction_negative_half_extents_components)
    {
        constexpr Vector3D center(1, 2, 3);
        constexpr Vector3D halfExtents(-1, 2, -3);
        constexpr AABB box(center, halfExtents);
        mu_assert(box.GetHalfExtents() == Vector3D(1, 2, 3), "Negative half-extents components should be normalized");
        mu_assert(box.GetMin() == Vector3D(0, 0, 0), "Normalized half-extents should give correct min");
        mu_assert(box.GetMax() == Vector3D(2, 4, 6), "Normalized half-extents should give correct max");
    }

    MU_TEST(aabb_from_min_max)
    {
        constexpr Vector3D min(-1, 0, -3);
        constexpr Vector3D max(3, 2, 1);
        constexpr AABB box = AABB::FromMinMax(min, max);
        mu_assert(box.GetPosition() == Vector3D(1, 1, -1), "FromMinMax center incorrect");
        mu_assert(box.GetHalfExtents() == Vector3D(2, 1, 2), "FromMinMax halfExtents incorrect");
        mu_assert(box.GetMin() == min, "FromMinMax min incorrect");
        mu_assert(box.GetMax() == max, "FromMinMax max incorrect");
    }

    MU_TEST(aabb_from_min_max_swapped_inputs)
    {
        constexpr Vector3D a(3, 2, 1);
        constexpr Vector3D b(-1, 0, -3);
        constexpr AABB box = AABB::FromMinMax(a, b);
        mu_assert(box.GetMin() == Vector3D(-1, 0, -3), "FromMinMax should normalize swapped inputs (min)");
        mu_assert(box.GetMax() == Vector3D(3, 2, 1), "FromMinMax should normalize swapped inputs (max)");
    }

    MU_TEST(aabb_from_min_max_equal_points_degenerate)
    {
        constexpr Vector3D p(1, 2, 3);
        constexpr AABB box = AABB::FromMinMax(p, p);
        mu_assert(box.GetPosition() == p, "FromMinMax(equal) center should be the point");
        mu_assert(box.GetHalfExtents() == Vector3D::Zero(), "FromMinMax(equal) halfExtents should be zero");
        mu_assert(box.IsDegenerate(), "FromMinMax(equal) should be degenerate");
        mu_assert(box.GetMin() == p, "FromMinMax(equal) min should equal point");
        mu_assert(box.GetMax() == p, "FromMinMax(equal) max should equal point");
    }

    MU_TEST(aabb_is_degenerate_any_axis)
    {
        constexpr AABB xZero(Vector3D::Zero(), Vector3D(0, 1, 1));
        constexpr AABB yZero(Vector3D::Zero(), Vector3D(1, 0, 1));
        constexpr AABB zZero(Vector3D::Zero(), Vector3D(1, 1, 0));
        constexpr AABB noneZero(Vector3D::Zero(), Vector3D(1, 1, 1));

        mu_assert(xZero.IsDegenerate(), "AABB with X half-extent == 0 should be degenerate");
        mu_assert(yZero.IsDegenerate(), "AABB with Y half-extent == 0 should be degenerate");
        mu_assert(zZero.IsDegenerate(), "AABB with Z half-extent == 0 should be degenerate");
        mu_assert(!noneZero.IsDegenerate(), "AABB with all non-zero half-extents should not be degenerate");
    }

    MU_TEST(aabb_volume_and_surface_area)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
        snprintf(buffer, buffer_size, "Volume mismatch: %f != 48", box.GetVolume());
        mu_assert(box.GetVolume() == 48, buffer);

        snprintf(buffer, buffer_size, "Surface area mismatch: %f != 88", box.GetSurfaceArea());
        mu_assert(box.GetSurfaceArea() == 88, buffer);

        constexpr AABB flat(Vector3D::Zero(), Vector3D(1, 0, 3));
        mu_assert(flat.GetVolume() == 0, "Degenerate AABB volume should be 0");
    }

    MU_TEST(aabb_expand)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
        constexpr AABB expanded = box.Expand(1);
        mu_assert(expanded.GetPosition() == Vector3D::Zero(), "Expand should not change position");
        mu_assert(expanded.GetHalfExtents() == Vector3D(2, 3, 4), "Expand should add margin to all axes");

        constexpr AABB same = box.Expand(0);
        mu_assert(same.GetHalfExtents() == box.GetHalfExtents(), "Expand(0) should not change halfExtents");
    }

    MU_TEST(aabb_expand_negative_margin_shrinks_and_clamps)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));

        constexpr AABB shrunk = box.Expand(Fxp(-1));
        mu_assert(shrunk.GetHalfExtents() == Vector3D(0, 1, 2), "Expand(-1) should shrink and clamp at 0");
        mu_assert(shrunk.IsDegenerate(), "Shrunk AABB with any axis 0 should be degenerate");

        constexpr AABB collapsed = box.Expand(Fxp(-100));
        mu_assert(collapsed.GetHalfExtents() == Vector3D::Zero(), "Expand(large negative) should clamp to zero");
        mu_assert(collapsed.GetMin() == Vector3D::Zero(), "Collapsed AABB min should equal center");
        mu_assert(collapsed.GetMax() == Vector3D::Zero(), "Collapsed AABB max should equal center");
    }

    MU_TEST(aabb_expand_negative_margin_partial_collapse)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(Fxp(0.5), 1, 1));
        constexpr AABB shrunk = box.Expand(Fxp(-0.75));
        mu_assert(shrunk.GetHalfExtents() == Vector3D(0, Fxp(0.25), Fxp(0.25)), "Expand(-0.75) should clamp X to 0 and shrink others");
    }

    MU_TEST(aabb_encapsulate_point_inside_and_outside)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));

        // Inside point -> should remain unchanged
        constexpr AABB inside = box.Encapsulate(Vector3D(0, 0, 0));
        mu_assert(inside.GetMin() == box.GetMin(), "Encapsulate(inside) should keep min");
        mu_assert(inside.GetMax() == box.GetMax(), "Encapsulate(inside) should keep max");

        // Outside point -> should expand minimally
        constexpr AABB expanded = box.Encapsulate(Vector3D(2, 0, 0));
        mu_assert(expanded.GetMin() == Vector3D(-1, -1, -1), "Encapsulate(point) min incorrect");
        mu_assert(expanded.GetMax() == Vector3D(2, 1, 1), "Encapsulate(point) max incorrect");
        mu_assert(expanded.GetPosition() == Vector3D(Fxp(0.5), 0, 0), "Encapsulate(point) center incorrect");
        mu_assert(expanded.GetHalfExtents() == Vector3D(Fxp(1.5), 1, 1), "Encapsulate(point) halfExtents incorrect");
    }

    MU_TEST(aabb_encapsulate_point_on_boundary_no_change)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));
        constexpr AABB same = box.Encapsulate(Vector3D(1, 0, 0));
        mu_assert(same.GetMin() == box.GetMin(), "Encapsulate(boundary) should keep min");
        mu_assert(same.GetMax() == box.GetMax(), "Encapsulate(boundary) should keep max");
    }

    MU_TEST(aabb_encapsulate_point_from_degenerate_box)
    {
        constexpr AABB pointBox(Vector3D::Zero(), Vector3D::Zero());
        constexpr AABB expanded = pointBox.Encapsulate(Vector3D(1, 0, 0));
        mu_assert(expanded.GetMin() == Vector3D(0, 0, 0), "Encapsulate from pointBox should set min correctly");
        mu_assert(expanded.GetMax() == Vector3D(1, 0, 0), "Encapsulate from pointBox should set max correctly");
        mu_assert(expanded.GetPosition() == Vector3D(Fxp(0.5), 0, 0), "Encapsulate from pointBox center incorrect");
        mu_assert(expanded.GetHalfExtents() == Vector3D(Fxp(0.5), 0, 0), "Encapsulate from pointBox halfExtents incorrect");
    }

    MU_TEST(aabb_encapsulate_aabb)
    {
        constexpr AABB a(Vector3D::Zero(), Vector3D(1, 1, 1));
        constexpr AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));

        constexpr AABB ab = a.Encapsulate(b);
        mu_assert(ab.GetMin() == Vector3D(-1, -1, -1), "Encapsulate(AABB) min incorrect");
        mu_assert(ab.GetMax() == Vector3D(3, 1, 1), "Encapsulate(AABB) max incorrect");
        mu_assert(ab.GetPosition() == Vector3D(1, 0, 0), "Encapsulate(AABB) center incorrect");
        mu_assert(ab.GetHalfExtents() == Vector3D(2, 1, 1), "Encapsulate(AABB) halfExtents incorrect");

        // If B is inside A, result should be A
        constexpr AABB inner(Vector3D(0, 0, 0), Vector3D(Fxp(0.5), Fxp(0.5), Fxp(0.5)));
        constexpr AABB ai = a.Encapsulate(inner);
        mu_assert(ai.GetMin() == a.GetMin(), "Encapsulate(inner AABB) should keep min");
        mu_assert(ai.GetMax() == a.GetMax(), "Encapsulate(inner AABB) should keep max");
    }

    MU_TEST(aabb_scale)
    {
        constexpr AABB box(Vector3D(1, 2, 3), Vector3D(1, 2, 3));
        constexpr AABB scaled = box.Scale(2);
        mu_assert(scaled.GetPosition() == box.GetPosition(), "Scale should not change position");
        mu_assert(scaled.GetHalfExtents() == Vector3D(2, 4, 6), "Scale should multiply half-extents");

        constexpr AABB zeroed = box.Scale(0);
        mu_assert(zeroed.GetHalfExtents() == Vector3D::Zero(), "Scale(0) should produce zero half-extents");
        mu_assert(zeroed.IsDegenerate(), "Scale(0) should be degenerate");
    }

    MU_TEST(aabb_scale_negative_factor_uses_magnitude)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
        constexpr AABB scaled = box.Scale(Fxp(-2));
        mu_assert(scaled.GetHalfExtents() == Vector3D(2, 4, 6), "Scale(-2) should behave like Scale(2)");
    }

    MU_TEST(aabb_scale_fractional)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(2, 4, 6));
        constexpr AABB scaled = box.Scale(Fxp(0.5));
        mu_assert(scaled.GetHalfExtents() == Vector3D(1, 2, 3), "Scale(0.5) should scale half-extents down");
    }

    MU_TEST(aabb_closest_point)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));

        mu_assert(box.GetClosestPoint(Vector3D(0, 0, 0)) == Vector3D(0, 0, 0), "ClosestPoint for inside point should be itself");
        mu_assert(box.GetClosestPoint(Vector3D(2, 0, 0)) == Vector3D(1, 0, 0), "ClosestPoint clamp failed on X");
        mu_assert(box.GetClosestPoint(Vector3D(2, -3, Fxp(0.5))) == Vector3D(1, -1, Fxp(0.5)), "ClosestPoint clamp failed on multiple axes");
    }

    MU_TEST(aabb_closest_point_degenerate_box)
    {
        constexpr AABB pointBox(Vector3D(1, 2, 3), Vector3D::Zero());
        mu_assert(pointBox.GetClosestPoint(Vector3D(999, -999, 0)) == Vector3D(1, 2, 3), "Degenerate AABB closest point should be its center");
    }

    MU_TEST(aabb_vertices)
    {
        constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
        constexpr auto v = box.GetVertices();

        mu_assert(v[0] == Vector3D(-1, -2, -3), "Vertex 0 incorrect");
        mu_assert(v[1] == Vector3D(1, -2, -3), "Vertex 1 incorrect");
        mu_assert(v[2] == Vector3D(1, 2, -3), "Vertex 2 incorrect");
        mu_assert(v[3] == Vector3D(-1, 2, -3), "Vertex 3 incorrect");
        mu_assert(v[4] == Vector3D(-1, -2, 3), "Vertex 4 incorrect");
        mu_assert(v[5] == Vector3D(1, -2, 3), "Vertex 5 incorrect");
        mu_assert(v[6] == Vector3D(1, 2, 3), "Vertex 6 incorrect");
        mu_assert(v[7] == Vector3D(-1, 2, 3), "Vertex 7 incorrect");
    }

    MU_TEST(aabb_vertices_degenerate)
    {
        constexpr AABB box(Vector3D(1, 2, 3), Vector3D::Zero());
        constexpr auto v = box.GetVertices();
        for (int i = 0; i < 8; i++)
        {
            mu_assert(v[i] == Vector3D(1, 2, 3), "Degenerate AABB should have all vertices equal to center");
        }
    }

    MU_TEST(aabb_set_position)
    {
        AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));
        box.SetPosition(Vector3D(3, 4, 5));
        mu_assert(box.GetPosition() == Vector3D(3, 4, 5), "SetPosition did not update position");
    }

    MU_TEST(aabb_merge)
    {
        constexpr AABB a(Vector3D::Zero(), Vector3D(1, 1, 1));
        constexpr AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));
        constexpr AABB merged = a.Merge(b);

        mu_assert(merged.GetMin() == Vector3D(-1, -1, -1), "Merge min incorrect");
        mu_assert(merged.GetMax() == Vector3D(3, 1, 1), "Merge max incorrect");
        mu_assert(merged.GetPosition() == Vector3D(1, 0, 0), "Merge center incorrect");
        mu_assert(merged.GetHalfExtents() == Vector3D(2, 1, 1), "Merge halfExtents incorrect");
    }

    MU_TEST(aabb_infinite)
    {
        constexpr AABB inf = AABB::Infinite();
        constexpr Vector3D he = inf.GetHalfExtents();
        mu_assert(inf.GetPosition() == Vector3D::Zero(), "Infinite AABB center should be zero");
        mu_assert(he.X == Fxp::MaxValue() && he.Y == Fxp::MaxValue() && he.Z == Fxp::MaxValue(), "Infinite AABB halfExtents should be MaxValue");
    }

    MU_TEST_SUITE(aabb_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&aabb_test_setup,
                                       &aabb_test_teardown,
                                       &aabb_test_output_header);

        MU_RUN_TEST(aabb_default_construction);
        MU_RUN_TEST(aabb_construction_center_and_uniform_size);
        MU_RUN_TEST(aabb_construction_negative_uniform_size);
        MU_RUN_TEST(aabb_construction_center_and_half_extents);
        MU_RUN_TEST(aabb_construction_negative_half_extents_components);
        MU_RUN_TEST(aabb_from_min_max);
        MU_RUN_TEST(aabb_from_min_max_swapped_inputs);
        MU_RUN_TEST(aabb_from_min_max_equal_points_degenerate);
        MU_RUN_TEST(aabb_is_degenerate_any_axis);
        MU_RUN_TEST(aabb_volume_and_surface_area);
        MU_RUN_TEST(aabb_expand);
        MU_RUN_TEST(aabb_expand_negative_margin_shrinks_and_clamps);
        MU_RUN_TEST(aabb_expand_negative_margin_partial_collapse);
        MU_RUN_TEST(aabb_encapsulate_point_inside_and_outside);
        MU_RUN_TEST(aabb_encapsulate_point_on_boundary_no_change);
        MU_RUN_TEST(aabb_encapsulate_point_from_degenerate_box);
        MU_RUN_TEST(aabb_encapsulate_aabb);
        MU_RUN_TEST(aabb_scale);
        MU_RUN_TEST(aabb_scale_negative_factor_uses_magnitude);
        MU_RUN_TEST(aabb_scale_fractional);
        MU_RUN_TEST(aabb_closest_point);
        MU_RUN_TEST(aabb_closest_point_degenerate_box);
        MU_RUN_TEST(aabb_vertices);
        MU_RUN_TEST(aabb_vertices_degenerate);
        MU_RUN_TEST(aabb_set_position);
        MU_RUN_TEST(aabb_merge);
        MU_RUN_TEST(aabb_infinite);
    }
}

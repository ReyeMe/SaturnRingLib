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

    /** @brief Setup routine for AABB unit tests.
 */
void aabb_test_setup(void)
{
        // No initialization needed
}

    /** @brief Tear down routine for AABB unit tests.
 */
void aabb_test_teardown(void)
{
        // No cleanup required
}

    /** @brief Output header for AABB test suite error reporting.
 */
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

    /**
 * @brief Tests that a default-constructed AABB is a zero-sized box at the origin.
 */
MU_TEST(aabb_default_construction)
{
    constexpr AABB box;
    mu_assert(box.GetPosition() == Vector3D::Zero(), "Default AABB position is not zero");
    mu_assert(box.GetSize() == Vector3D::Zero(), "Default AABB half-extents are not zero");
    mu_assert(box.GetMin() == Vector3D::Zero(), "Default AABB min should be zero");
    mu_assert(box.GetMax() == Vector3D::Zero(), "Default AABB max should be zero");
}

    /**
 * @brief Tests the AABB constructor that takes a center point and a uniform size.
 */
MU_TEST(aabb_construction_center_and_uniform_size)
{
    constexpr Vector3D center(1, 2, 3);
    constexpr Fxp size(4);
    constexpr AABB box(center, size);
    mu_assert(box.GetPosition() == center, "AABB center+size ctor did not set position");
    mu_assert(box.GetSize() == Vector3D(size, size, size), "AABB center+size ctor did not set half-extents");
}


    /**
 * @brief Tests the AABB constructor that takes a center point and non-uniform half-extents.
 */
MU_TEST(aabb_construction_center_and_half_extents)
{
    constexpr Vector3D center(1, 2, 3);
    constexpr Vector3D halfExtents(1, 2, 3);
    constexpr AABB box(center, halfExtents);
    mu_assert(box.GetPosition() == center, "AABB center+halfExtents ctor did not set position");
    mu_assert(box.GetSize() == halfExtents, "AABB center+halfExtents ctor did not set half-extents");
    mu_assert(box.GetMin() == Vector3D(0, 0, 0), "AABB min incorrect for center+halfExtents");
    mu_assert(box.GetMax() == Vector3D(2, 4, 6), "AABB max incorrect for center+halfExtents");
}


    /**
 * @brief Tests creating an AABB from two points (min and max corners).
 */
MU_TEST(aabb_from_min_max)
{
    constexpr Vector3D min(-1, 0, -3);
    constexpr Vector3D max(3, 2, 1);
    constexpr AABB box = AABB::FromMinMax(min, max);
    mu_assert(box.GetPosition() == Vector3D(1, 1, -1), "FromMinMax center incorrect");
    mu_assert(box.GetSize() == Vector3D(2, 1, 2), "FromMinMax halfExtents incorrect");
    mu_assert(box.GetMin() == min, "FromMinMax min incorrect");
    mu_assert(box.GetMax() == max, "FromMinMax max incorrect");
}


    /**
 * @brief Tests that `FromMinMax` with two equal points creates a degenerate (zero-sized) AABB at that point.
 */
MU_TEST(aabb_from_min_max_equal_points_degenerate)
{
    constexpr Vector3D p(1, 2, 3);
    constexpr AABB box = AABB::FromMinMax(p, p);
    mu_assert(box.GetPosition() == p, "FromMinMax(equal) center should be the point");
    mu_assert(box.GetSize() == Vector3D::Zero(), "FromMinMax(equal) halfExtents should be zero");
    mu_assert(box.GetMin() == p, "FromMinMax(equal) min should equal point");
    mu_assert(box.GetMax() == p, "FromMinMax(equal) max should equal point");
}


    /**
 * @brief Tests the calculation of the AABB's volume and surface area.
 */
MU_TEST(aabb_volume_and_surface_area)
{
    constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
    snprintf(buffer, buffer_size, "Volume mismatch: %d != 48", box.GetVolume().As<int32_t>());
    mu_assert(box.GetVolume() == 48, buffer);

    snprintf(buffer, buffer_size, "Surface area mismatch: %d != 88", box.GetSurfaceArea().As<int32_t>());
    mu_assert(box.GetSurfaceArea() == 88, buffer);

    constexpr AABB flat(Vector3D::Zero(), Vector3D(1, 0, 3));
    mu_assert(flat.GetVolume() == 0, "Degenerate AABB volume should be 0");
}

    /**
 * @brief Tests expanding an AABB by a given margin.
 */
MU_TEST(aabb_expand)
{
    constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
    constexpr AABB expanded = box.Expand(1);
    mu_assert(expanded.GetPosition() == Vector3D::Zero(), "Expand should not change position");
    mu_assert(expanded.GetSize() == Vector3D(2, 3, 4), "Expand should add margin to all axes");

    constexpr AABB same = box.Expand(0);
    mu_assert(same.GetSize() == box.GetSize(), "Expand(0) should not change halfExtents");
}







    /**
 * @brief Tests scaling an AABB by a uniform factor.
 */
MU_TEST(aabb_scale)
{
    constexpr AABB box(Vector3D(1, 2, 3), Vector3D(1, 2, 3));
    constexpr AABB scaled = box.Scale(2);
    mu_assert(scaled.GetPosition() == box.GetPosition(), "Scale should not change position");
    mu_assert(scaled.GetSize() == Vector3D(2, 4, 6), "Scale should multiply half-extents");

    constexpr AABB zeroed = box.Scale(0);
    mu_assert(zeroed.GetSize() == Vector3D::Zero(), "Scale(0) should produce zero half-extents");
}


    /**
 * @brief Tests scaling an AABB by a fractional factor.
 */
MU_TEST(aabb_scale_fractional)
{
    constexpr AABB box(Vector3D::Zero(), Vector3D(2, 4, 6));
    constexpr AABB scaled = box.Scale(Fxp(0.5));
    mu_assert(scaled.GetSize() == Vector3D(1, 2, 3), "Scale(0.5) should scale half-extents down");
}

    /**
 * @brief Tests the `GetClosestPoint` method to find the point on the AABB's surface closest to a given point.
 */
MU_TEST(aabb_closest_point)
{
    constexpr AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));

    mu_assert(box.GetClosestPoint(Vector3D(0, 0, 0)) == Vector3D(0, 0, 0), "ClosestPoint for inside point should be itself");
    mu_assert(box.GetClosestPoint(Vector3D(2, 0, 0)) == Vector3D(1, 0, 0), "ClosestPoint clamp failed on X");
    mu_assert(box.GetClosestPoint(Vector3D(2, -3, Fxp(0.5))) == Vector3D(1, -1, Fxp(0.5)), "ClosestPoint clamp failed on multiple axes");
}

    /**
 * @brief Tests `GetClosestPoint` on a degenerate AABB, which should always return the AABB's center.
 */
MU_TEST(aabb_closest_point_degenerate_box)
{
    constexpr AABB pointBox(Vector3D(1, 2, 3), Vector3D::Zero());
    mu_assert(pointBox.GetClosestPoint(Vector3D(999, -999, 0)) == Vector3D(1, 2, 3), "Degenerate AABB closest point should be its center");
}

    /**
 * @brief Tests that `GetVertices` returns the 8 corner points of the AABB in the correct order.
 */
MU_TEST(aabb_vertices)
{
    constexpr AABB box(Vector3D::Zero(), Vector3D(1, 2, 3));
    const auto v = box.GetVertices();

    mu_assert(v[0] == Vector3D(-1, -2, -3), "Vertex 0 incorrect");
    mu_assert(v[1] == Vector3D(1, -2, -3), "Vertex 1 incorrect");
    mu_assert(v[2] == Vector3D(1, 2, -3), "Vertex 2 incorrect");
    mu_assert(v[3] == Vector3D(-1, 2, -3), "Vertex 3 incorrect");
    mu_assert(v[4] == Vector3D(-1, -2, 3), "Vertex 4 incorrect");
    mu_assert(v[5] == Vector3D(1, -2, 3), "Vertex 5 incorrect");
    mu_assert(v[6] == Vector3D(1, 2, 3), "Vertex 6 incorrect");
    mu_assert(v[7] == Vector3D(-1, 2, 3), "Vertex 7 incorrect");
}

    /**
 * @brief Tests that `GetVertices` for a degenerate AABB returns 8 vertices all at the center point.
 */
MU_TEST(aabb_vertices_degenerate)
{
    constexpr AABB box(Vector3D(1, 2, 3), Vector3D::Zero());
    const auto v = box.GetVertices();
    for (int i = 0; i < 8; ++i)
    {
        mu_assert(v[i] == Vector3D(1, 2, 3), "Degenerate AABB should have all vertices equal to center");
    }
}

    /**
 * @brief Tests the `SetPosition` method of the AABB.
 */
MU_TEST(aabb_set_position)
{
    AABB box(Vector3D::Zero(), Vector3D(1, 1, 1));
    box.SetPosition(Vector3D(3, 4, 5));
    mu_assert(box.GetPosition() == Vector3D(3, 4, 5), "SetPosition did not update position");
}

    /**
 * @brief Tests merging two AABBs into one that contains both.
 */
MU_TEST(aabb_merge)
{
    constexpr AABB a(Vector3D::Zero(), Vector3D(1, 1, 1));
    constexpr AABB b(Vector3D(2, 0, 0), Vector3D(1, 1, 1));
    const AABB merged = a.Merge(b);

    mu_assert(merged.GetMin() == Vector3D(-1, -1, -1), "Merge min incorrect");
    mu_assert(merged.GetMax() == Vector3D(3, 1, 1), "Merge max incorrect");
    mu_assert(merged.GetPosition() == Vector3D(1, 0, 0), "Merge center incorrect");
    mu_assert(merged.GetSize() == Vector3D(2, 1, 1), "Merge halfExtents incorrect");

        // If B is inside A, result should be A
    const AABB inner(Vector3D(0, 0, 0), Vector3D(Fxp(0.5), Fxp(0.5), Fxp(0.5)));
    const AABB ai = a.Merge(inner);
    mu_assert(ai.GetMin() == a.GetMin(), "Merge(inner AABB) should keep min");
    mu_assert(ai.GetMax() == a.GetMax(), "Merge(inner AABB) should keep max");
}


MU_TEST_SUITE(aabb_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&aabb_test_setup,
        &aabb_test_teardown,
        &aabb_test_output_header);

    MU_RUN_TEST(aabb_default_construction);
    MU_RUN_TEST(aabb_construction_center_and_uniform_size);
    MU_RUN_TEST(aabb_construction_center_and_half_extents);
    MU_RUN_TEST(aabb_from_min_max);
    MU_RUN_TEST(aabb_from_min_max_equal_points_degenerate);
    MU_RUN_TEST(aabb_volume_and_surface_area);
    MU_RUN_TEST(aabb_expand);
    MU_RUN_TEST(aabb_scale);
    MU_RUN_TEST(aabb_scale_fractional);
    MU_RUN_TEST(aabb_closest_point);
    MU_RUN_TEST(aabb_closest_point_degenerate_box);
    MU_RUN_TEST(aabb_vertices);
    MU_RUN_TEST(aabb_vertices_degenerate);
    MU_RUN_TEST(aabb_set_position);
    MU_RUN_TEST(aabb_merge);
}
}

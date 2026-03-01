
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

    /**
     * @brief Sets up the environment for Plane unit tests.
     */
    void plane_test_setup(void) {}

    /**
     * @brief Cleans up the environment after each Plane unit test.
     */
    void plane_test_teardown(void) {}

    /**
     * @brief Displays a header for the Plane test suite upon the first error.
     */
    void plane_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_PLANE****");
            }
            else
            {
                LogInfo("****UT_PLANE_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Tests the default constructor and distance calculation methods of the Plane class.
     * @details Verifies that a default-constructed plane is at the origin with a normal pointing up (UnitY)
     *          and that signed and absolute distance calculations are correct.
     */
    MU_TEST(plane_default_and_distance)
    {
        const Plane p;
        mu_assert(p.Normal == Vector3D::UnitY(), "Default Plane normal should be UnitY");
        mu_assert(p.SignedDistance == 0, "Default Plane signed distance should be 0");

        mu_assert(p.GetSignedDistance(Vector3D(0, 2, 0)) == 2, "Signed distance above plane should be positive");
        mu_assert(p.GetSignedDistance(Vector3D(0, -2, 0)) == -2, "Signed distance below plane should be negative");
        mu_assert(p.GetSignedDistance(Vector3D(5, 0, -3)) == 0, "Signed distance on plane should be 0");
        mu_assert(p.GetDistance(Vector3D(0, -2, 0)) == 2, "Absolute distance should be positive");
    }

    /**
     * @brief Tests creating a plane from a normal vector and a point on the plane.
     * @details Verifies that the plane's signed distance is correctly calculated and that points
     *          are correctly classified relative to it.
     */
    MU_TEST(plane_from_normal_and_point)
    {
        const Plane p = Plane::FromNormalAndPoint(Vector3D::UnitY(), Vector3D(0, 5, 0));
        mu_assert(p.SignedDistance == 5, "FromNormalAndPoint should set signed distance to normal dot point");
        mu_assert(p.GetSignedDistance(Vector3D(0, 7, 0)) == 2, "Signed distance should be (y-5)");
        mu_assert(p.GetSignedDistance(Vector3D(0, 5, 0)) == 0, "Point on plane should have 0 distance");
    }

    /**
     * @brief Tests projecting and reflecting points and vectors across a plane.
     * @details Verifies projection and reflection operations for points and vectors.
     */
    MU_TEST(plane_project_and_reflect)
    {
        const Plane p(Vector3D::UnitY(), 0);
        const Vector3D a(1, 2, 3);

        mu_assert(p.ProjectPoint(a) == Vector3D(1, 0, 3), "ProjectPoint onto Y=0 plane should zero Y");
        mu_assert(p.ReflectPoint(a) == Vector3D(1, -2, 3), "ReflectPoint across Y=0 plane should invert Y");
        mu_assert(p.ReflectVector(a) == Vector3D(1, -2, 3), "ReflectVector across plane normal should invert Y component");
    }

    /**
     * @brief Tests the normalization of a plane and the validity check.
     * @details Verifies that a plane with a zero normal is invalid and that normalizing a valid
     *          plane correctly scales its normal and distance.
     */
    MU_TEST(plane_normalize_and_validity)
    {
        Plane invalid(Vector3D::Zero(), 0);
        mu_assert(!invalid.IsValid(), "Plane with zero normal should be invalid");

        invalid.Normalize<SRL::Math::Precision::Accurate>();
        mu_assert(invalid.Normal == Vector3D::Zero(), "Normalize(zero normal) should not change normal");
        mu_assert(invalid.SignedDistance == 0, "Normalize(zero normal) should not change signed distance");

        const Plane p(Vector3D(0, 2, 0), 4);
        const Plane n = p.Normalized();
        mu_assert(n.IsValid(), "Normalized plane should be valid");
        mu_assert(n.Normal == Vector3D::UnitY(), "Normalized normal should be UnitY");
        mu_assert(n.SignedDistance == 2, "Normalized signed distance should be scaled accordingly");
    }

    /**
     * @brief Tests that creating a plane from three collinear (degenerate) points returns a default plane.
     * @details Verifies that degenerate input falls back to a default plane.
     */
    MU_TEST(plane_from_points_degenerate_returns_default)
    {
        // Collinear points -> normal length ~0 -> returns Plane() (default)
        const Plane p = Plane::FromPoints(Vector3D(0, 0, 0), Vector3D(1, 0, 0), Vector3D(2, 0, 0));
        mu_assert(p.Normal == Vector3D::UnitY(), "FromPoints(collinear) should fall back to default plane normal");
        mu_assert(p.SignedDistance == 0, "FromPoints(collinear) should fall back to default plane distance");
    }

    /**
     * @brief Defines the test suite for all Plane functionality.
     */
    MU_TEST_SUITE(plane_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&plane_test_setup,
                                       &plane_test_teardown,
                                       &plane_test_output_header);

        MU_RUN_TEST(plane_default_and_distance);
        MU_RUN_TEST(plane_from_normal_and_point);
        MU_RUN_TEST(plane_project_and_reflect);
        MU_RUN_TEST(plane_normalize_and_validity);
        MU_RUN_TEST(plane_from_points_degenerate_returns_default);
    }
}

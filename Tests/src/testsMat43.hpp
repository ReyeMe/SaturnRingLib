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

    void mat43_test_setup(void) {}
    void mat43_test_teardown(void) {}

    void mat43_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_MAT43****");
            }
            else
            {
                LogInfo("****UT_MAT43_ERROR(S)****");
            }
        }
    }

    MU_TEST(mat43_identity_transform)
    {
        constexpr Matrix43 I = Matrix43::Identity();
        const Vector3D p(1, 2, 3);
        mu_assert(I.TransformPoint(p) == p, "Identity should not change points");
        mu_assert(I.TransformVector(p) == p, "Identity should not change vectors");
    }

    MU_TEST(mat43_translation_and_invert)
    {
        const Matrix43 t = Matrix43::CreateTranslation(Vector3D(5, -2, 1));
        const Vector3D p(1, 2, 3);

        mu_assert(t.TransformPoint(p) == Vector3D(6, 0, 4), "Translation should add to point");
        mu_assert(t.TransformVector(p) == p, "Translation should not affect vectors");

        const Matrix43 inv = t.Invert();
        mu_assert(inv.TransformPoint(t.TransformPoint(p)) == p, "Invert should undo translation for points");
    }

    MU_TEST(mat43_multiplication_combines_translations)
    {
        const Matrix43 a = Matrix43::CreateTranslation(Vector3D(1, 0, 0));
        const Matrix43 b = Matrix43::CreateTranslation(Vector3D(0, 2, 0));
        const Matrix43 c = a * b;
        mu_assert(c.Row3 == Vector3D(1, 2, 0), "Translation multiplication should add translations");
    }

    MU_TEST(mat43_rotation_y_90_vector)
    {
        const Matrix43 r = Matrix43::CreateRotationY(Angle::FromDegrees(90));
        const Vector3D forward(0, 0, -1);
        mu_assert(r.TransformVector(forward) == Vector3D(1, 0, 0), "Yaw +90 should rotate -Z to +X");

        // Rotations are orthogonal; Invert should undo rotation
        const Matrix43 inv = r.Invert();
        mu_assert(inv.TransformVector(r.TransformVector(forward)) == forward, "Invert should undo rotation for vectors");
    }

    MU_TEST_SUITE(mat43_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&mat43_test_setup,
                                       &mat43_test_teardown,
                                       &mat43_test_output_header);

        MU_RUN_TEST(mat43_identity_transform);
        MU_RUN_TEST(mat43_translation_and_invert);
        MU_RUN_TEST(mat43_multiplication_combines_translations);
        MU_RUN_TEST(mat43_rotation_y_90_vector);
    }
}

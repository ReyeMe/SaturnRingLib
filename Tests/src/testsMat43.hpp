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

    /**
 * @brief Sets up the environment for 4x3 Matrix (Mat43) unit tests.
 */
void mat43_test_setup(void) {}
    /**
 * @brief Cleans up the environment after each 4x3 Matrix (Mat43) unit test.
 */
void mat43_test_teardown(void) {}

    /**
 * @brief Displays a header for the 4x3 Matrix (Mat43) test suite upon the first error.
 */
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

    /**
 * @brief Tests that an identity matrix transformation does not alter points or vectors.
 */
MU_TEST(mat43_identity_transform)
{
    constexpr Matrix43 I = Matrix43::Identity();
    const Vector3D p(1, 2, 3);
    mu_assert(I.TransformPoint(p) == p, "Identity should not change points");
    mu_assert(I.TransformVector(p) == p, "Identity should not change vectors");
}

    /**
 * @brief Tests the creation of a translation matrix and its inversion.
 * @details Verifies that a translation matrix correctly transforms points (but not vectors)
 *          and that its inverse correctly undoes the transformation.
 */
MU_TEST(mat43_translation_and_invert)
{
    const Matrix43 t = Matrix43::CreateTranslation(Vector3D(5, -2, 1));
    const Vector3D p(1, 2, 3);

    mu_assert(t.TransformPoint(p) == Vector3D(6, 0, 4), "Translation should add to point");
    mu_assert(t.TransformVector(p) == p, "Translation should not affect vectors");

    const Matrix43 inv = t.Invert();
    mu_assert(inv.TransformPoint(t.TransformPoint(p)) == p, "Invert should undo translation for points");
}

    /**
 * @brief Tests that multiplying two translation matrices correctly combines their translations.
 */
MU_TEST(mat43_multiplication_combines_translations)
{
    const Matrix43 a = Matrix43::CreateTranslation(Vector3D(1, 0, 0));
    const Matrix43 b = Matrix43::CreateTranslation(Vector3D(0, 2, 0));
    const Matrix43 c = a * b;
    mu_assert(c.Row3 == Vector3D(1, 2, 0), "Translation multiplication should add translations");
}

    /**
 * @brief Tests the transformation of a vector by a 90-degree rotation matrix around the Y-axis (documented: +X to -Z).
 * @details Verifies that the rotation correctly transforms a vector and that its inverse restores the original vector.
 */
MU_TEST(mat43_rotation_y_90_vector)
{
    const Matrix43 r(Matrix33::CreateRotationY(Angle::FromDegrees(90)), Vector3D::Zero());
    const Vector3D forward(1, 0, 0);
    mu_assert(r.TransformVector(forward) == Vector3D(0, 0, -1), "Yaw +90 should rotate +X to -Z");

        // Rotations are orthogonal; Invert should undo rotation
    const Matrix43 inv = r.Invert();
    mu_assert(inv.TransformVector(r.TransformVector(forward)) == forward, "Invert should undo rotation for vectors");
}

    /**
 * @brief Defines the test suite for all 4x3 Matrix (Mat43) functionality.
 */
/** @brief True if every component of a is within tol raw units of b. */
static bool m43_vec_near(const Vector3D& a, const Vector3D& b, int32_t tol)
{
    auto near = [tol](const Fxp& x, const Fxp& y) { const int32_t d = x.RawValue() - y.RawValue(); return d >= -tol && d <= tol; };
    return near(a.X, b.X) && near(a.Y, b.Y) && near(a.Z, b.Z);
}

    /**
 * @brief Translation(x, y, z) equals CreateTranslation, and Translate() adds to the existing translation.
 */
MU_TEST(mat43_translate_and_translation)
{
    const Matrix43 a = Matrix43::Translation(1, 2, 3);
    const Matrix43 b = Matrix43::CreateTranslation(Vector3D(1, 2, 3));
    mu_assert(m43_vec_near(a.Row3, b.Row3, 0) && m43_vec_near(a.Row0, b.Row0, 0) && m43_vec_near(a.Row2, b.Row2, 0), "Translation(1,2,3) should equal CreateTranslation((1,2,3))");

    Matrix43 m = Matrix43::Identity();
    m.Translate(Vector3D(1, 0, 0)).Translate(Vector3D(0, 2, -3));
    mu_assert(m.Row3 == Vector3D(1, 2, -3), "Translate should accumulate");
    mu_assert(m.TransformPoint(Vector3D(1, 1, 1)) == Vector3D(2, 3, -2), "Translated point incorrect");
    mu_assert(m.TransformVector(Vector3D(1, 1, 1)) == Vector3D(1, 1, 1), "Translate should not affect vectors");
}

    /**
 * @brief The Scale factories (uniform, per axis, from a vector) scale points and have no translation.
 */
MU_TEST(mat43_scale_factories)
{
    const Vector3D p(1, 1, 1);
    mu_assert(Matrix43::Scale(2).TransformPoint(p) == Vector3D(2, 2, 2), "Scale(2) should scale uniformly");
    mu_assert(Matrix43::Scale(2, 3, Fxp(0.5)).TransformPoint(p) == Vector3D(2, 3, Fxp(0.5)), "Scale(2,3,0.5) should scale per axis");
    mu_assert(Matrix43::Scale(Vector3D(2, 3, 4)).TransformPoint(p) == Vector3D(2, 3, 4), "Scale(Vector3D) should scale per axis");
    mu_assert(Matrix43::Scale(Vector3D(2, 3, 4)).Row3 == Vector3D::Zero(), "Scale matrix should have no translation");
}

    /**
 * @brief A * B transforms like A(B(p)), and multiplying by a Matrix33 keeps the translation (documented).
 */
MU_TEST(mat43_multiply_composes)
{
    const Matrix43 a(Matrix33::CreateRotationY(Angle::FromDegrees(Fxp(90))), Vector3D(1, 2, 3));
    const Matrix43 b(Matrix33::CreateScale(Vector3D(2, 3, 4)), Vector3D(5, 0, -1));
    const Vector3D p(1, -2, Fxp(0.5));

    const Vector3D composed = (a * b).TransformPoint(p);
    const Vector3D chained = a.TransformPoint(b.TransformPoint(p));
    snprintf(buffer, buffer_size, "(A*B)p raw (%ld,%ld,%ld) vs A(Bp) (%ld,%ld,%ld)",
        (long)composed.X.RawValue(), (long)composed.Y.RawValue(), (long)composed.Z.RawValue(),
        (long)chained.X.RawValue(), (long)chained.Y.RawValue(), (long)chained.Z.RawValue());
    mu_assert(m43_vec_near(composed, chained, 4), buffer);

    const Matrix43 rotated = a * Matrix33::CreateRotationX(Angle::FromDegrees(Fxp(30)));
    mu_assert(rotated.Row3 == a.Row3, "Matrix43 * Matrix33 should keep the translation");
}

    /**
 * @brief Invert() undoes a rotation + translation for points and for vectors.
 */
MU_TEST(mat43_invert_rigid_transform)
{
    const Matrix43 m(Matrix33::CreateRotation(Angle::FromDegrees(Fxp(30)), Angle::FromDegrees(Fxp(45)), Angle::FromDegrees(Fxp(60))), Vector3D(4, -2, 7));
    const Matrix43 inv = m.Invert();
    const Vector3D p(1, 2, 3);

    const Vector3D back = inv.TransformPoint(m.TransformPoint(p));
    snprintf(buffer, buffer_size, "Inv(M)(M(p)) raw (%ld,%ld,%ld) != (1,2,3)",
        (long)back.X.RawValue(), (long)back.Y.RawValue(), (long)back.Z.RawValue());
    mu_assert(m43_vec_near(back, p, 1311), buffer);     // within 0.02: rotation error grows with distance

    const Vector3D vBack = inv.TransformVector(m.TransformVector(p));
    mu_assert(m43_vec_near(vBack, p, 1311), "Inv(M) should undo M for vectors");
}

    /**
 * @brief CreateTransform combines translation, Euler rotation and scale (documented example: 90 degrees around Y, scale 2).
 */
MU_TEST(mat43_create_transform)
{
    const Matrix43 noRot = Matrix43::CreateTransform(Vector3D(1, 2, 3), EulerAngles(), Vector3D(2, 3, 4));
    mu_assert(noRot.TransformPoint(Vector3D(1, 1, 1)) == Vector3D(3, 5, 7), "Scale then translate without rotation");

    const Matrix43 m = Matrix43::CreateTransform(Vector3D(1, 2, 3), EulerAngles(Angle::Zero(), Angle::HalfPi(), Angle::Zero()), Vector3D(2, 2, 2));
    const Vector3D p = m.TransformPoint(Vector3D(1, 0, 0));
    snprintf(buffer, buffer_size, "CreateTransform point raw (%ld,%ld,%ld) != (1,2,1)",
        (long)p.X.RawValue(), (long)p.Y.RawValue(), (long)p.Z.RawValue());
    mu_assert(m43_vec_near(p, Vector3D(1, 2, 1), 8), buffer);     // 2 * RotY(90)(1,0,0) + (1,2,3) = 2*(0,0,-1) + (1,2,3)

    const Matrix43 identity = Matrix43::CreateTransform(Vector3D::Zero(), EulerAngles());
    mu_assert(identity.TransformPoint(Vector3D(5, 6, 7)) == Vector3D(5, 6, 7), "Default CreateTransform should be identity");
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
    MU_RUN_TEST(mat43_translate_and_translation);
    MU_RUN_TEST(mat43_scale_factories);
    MU_RUN_TEST(mat43_multiply_composes);
    MU_RUN_TEST(mat43_invert_rigid_transform);
    MU_RUN_TEST(mat43_create_transform);
}
}

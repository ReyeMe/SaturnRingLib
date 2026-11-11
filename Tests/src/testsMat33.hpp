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
 * @brief Sets up the environment for 3x3 Matrix (Mat33) unit tests.
 */
void mat33_test_setup(void) {}
    /**
 * @brief Cleans up the environment after each 3x3 Matrix (Mat33) unit test.
 */
void mat33_test_teardown(void) {}

    /**
 * @brief Displays a header for the 3x3 Matrix (Mat33) test suite upon the first error.
 */
void mat33_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_MAT33****");
        }
        else
        {
            LogInfo("****UT_MAT33_ERROR(S)****");
        }
    }
}

/** @brief Row-by-row equality (Matrix33 has no operator==). */
static bool mat33_equal(const Matrix33& a, const Matrix33& b)
{
    return a.Row0 == b.Row0 && a.Row1 == b.Row1 && a.Row2 == b.Row2;
}

/** @brief Returns a transposed copy (Transpose() works in place). */
static Matrix33 mat33_transposed(Matrix33 m)
{
    return m.Transpose();
}

    /**
 * @brief Tests the properties of the identity matrix.
 * @details Verifies that multiplying by the identity matrix does not change a vector,
 *          that its determinant is 1, and that its transpose is itself.
 */
MU_TEST(mat33_identity_and_vector_multiply)
{
    constexpr Matrix33 I = Matrix33::Identity();
    const Vector3D v(1, 2, 3);
    mu_assert((I * v) == v, "Identity matrix should not change vector");

    mu_assert(I.Determinant() == 1, "Identity determinant should be 1");
    mu_assert(mat33_equal(mat33_transposed(I), I), "Identity transpose should be identity");
}

    /**
 * @brief Tests the properties of a scale matrix.
 * @details Verifies that a scale matrix correctly scales a vector, and that its determinant
 *          and transpose are calculated as expected.
 */
MU_TEST(mat33_scale_determinant_transpose)
{
    const Matrix33 s = Matrix33::CreateScale(Vector3D(2, 3, 4));
    mu_assert((s * Vector3D(1, 1, 1)) == Vector3D(2, 3, 4), "Scale matrix should scale axes");
    mu_assert(s.Determinant() == 24, "Scale determinant should equal product of diagonal");
    mu_assert(mat33_equal(mat33_transposed(s), s), "Diagonal scale matrix should equal its transpose");
}

    /**
 * @brief Tests that the transpose operation is an involution (i.e., applying it twice returns the original matrix).
 */
MU_TEST(mat33_transpose_involution)
{
    const Matrix33 m(
        Vector3D(1, 2, 3),
        Vector3D(4, 5, 6),
        Vector3D(7, 8, 9));

    const Matrix33 tt = mat33_transposed(mat33_transposed(m));
    mu_assert(mat33_equal(tt, m), "Transpose(Transpose(M)) should equal M");
}

    /**
 * @brief Tests the `TryInverse` method for both invertible and non-invertible (singular) matrices.
 */
MU_TEST(mat33_tryinverse_success_and_failure)
{
        // Failure: zero matrix has det=0
    const Matrix33 z;
    Matrix33 inv;
    mu_assert(!z.TryInverse(inv), "TryInverse should fail for singular matrix");

        // Success: uniform scale by 2 has exact inverse in fixed-point
    const Matrix33 s2 = Matrix33::CreateScale(Vector3D(2, 2, 2));
    mu_assert(s2.TryInverse(inv), "TryInverse should succeed for invertible matrix");

    constexpr Matrix33 I = Matrix33::Identity();
    const Matrix33 prod = s2 * inv;
    mu_assert(mat33_equal(prod, I), "M * Inv(M) should equal Identity for uniform scale 2");
}

    /**
 * @brief Defines the test suite for all 3x3 Matrix (Mat33) functionality.
 */
/** @brief True if every component of a is within tol raw units of b. */
static bool m33_vec_near(const Vector3D& a, const Vector3D& b, int32_t tol)
{
    auto near = [tol](const Fxp& x, const Fxp& y) { const int32_t d = x.RawValue() - y.RawValue(); return d >= -tol && d <= tol; };
    return near(a.X, b.X) && near(a.Y, b.Y) && near(a.Z, b.Z);
}

/** @brief True if every element of a is within tol raw units of b. */
static bool m33_near(const Matrix33& a, const Matrix33& b, int32_t tol)
{
    return m33_vec_near(a.Row0, b.Row0, tol) && m33_vec_near(a.Row1, b.Row1, tol) && m33_vec_near(a.Row2, b.Row2, tol);
}

// Rotations come from Sin/Cos (table accuracy ~0.0012), so compare them within 0.005
static constexpr int32_t kRotTol = 328;

    /**
 * @brief The documented examples of CreateRotationX/Y/Z, and a 0 degree rotation is the identity.
 */
MU_TEST(mat33_rotation_documented_examples)
{
    const Angle a90 = Angle::FromDegrees(Fxp(90));
    mu_assert(m33_vec_near(Matrix33::CreateRotationX(a90) * Vector3D(0, 1, 0), Vector3D(0, 0, 1), 4), "RotationX(90) should rotate (0,1,0) to (0,0,1)");
    mu_assert(m33_vec_near(Matrix33::CreateRotationY(a90) * Vector3D(1, 0, 0), Vector3D(0, 0, -1), 4), "RotationY(90) should rotate (1,0,0) to (0,0,-1)");
    mu_assert(m33_vec_near(Matrix33::CreateRotationZ(a90) * Vector3D(1, 0, 0), Vector3D(0, 1, 0), 4), "RotationZ(90) should rotate (1,0,0) to (0,1,0)");

    const Angle zero = Angle::Zero();
    mu_assert(m33_near(Matrix33::CreateRotationX(zero), Matrix33::Identity(), 0), "RotationX(0) should be identity");
    mu_assert(m33_near(Matrix33::CreateRotationY(zero), Matrix33::Identity(), 0), "RotationY(0) should be identity");
    mu_assert(m33_near(Matrix33::CreateRotationZ(zero), Matrix33::Identity(), 0), "RotationZ(0) should be identity");
}

    /**
 * @brief Rotations have determinant 1, R * R^T = I, and TryInverse gives the transpose.
 */
MU_TEST(mat33_rotation_determinant_and_inverse)
{
    const Matrix33 r = Matrix33::CreateRotation(Angle::FromDegrees(Fxp(30)), Angle::FromDegrees(Fxp(45)), Angle::FromDegrees(Fxp(60)));

    const Fxp det = r.Determinant();
    snprintf(buffer, buffer_size, "Rotation determinant raw %ld != 1", (long)det.RawValue());
    mu_assert(det.RawValue() >= 65536 - kRotTol && det.RawValue() <= 65536 + kRotTol, buffer);

    Matrix33 rt = r;
    rt.Transpose();
    mu_assert(m33_near(r * rt, Matrix33::Identity(), kRotTol), "R * R^T should be identity");

    Matrix33 inv;
    mu_assert(r.TryInverse(inv), "TryInverse should succeed for a rotation");
    mu_assert(m33_near(inv, rt, kRotTol), "Inverse of a rotation should equal its transpose");
}

    /**
 * @brief RotateX/Y/Z in place on the identity give CreateRotationX/Y/Z, and on any M give M * CreateRotation.
 */
MU_TEST(mat33_rotate_in_place_matches_create)
{
    const Angle a = Angle::FromDegrees(Fxp(35));

    Matrix33 mx = Matrix33::Identity(); mx.RotateX(a);
    Matrix33 my = Matrix33::Identity(); my.RotateY(a);
    Matrix33 mz = Matrix33::Identity(); mz.RotateZ(a);
    mu_assert(m33_near(mx, Matrix33::CreateRotationX(a), 4), "Identity.RotateX(a) should equal CreateRotationX(a)");
    mu_assert(m33_near(my, Matrix33::CreateRotationY(a), 4), "Identity.RotateY(a) should equal CreateRotationY(a)");
    mu_assert(m33_near(mz, Matrix33::CreateRotationZ(a), 4), "Identity.RotateZ(a) should equal CreateRotationZ(a)");

    const Matrix33 base = Matrix33::CreateRotation(Angle::FromDegrees(Fxp(10)), Angle::FromDegrees(Fxp(20)), Angle::FromDegrees(Fxp(30)));
    Matrix33 bx = base; bx.RotateX(a);
    Matrix33 by = base; by.RotateY(a);
    Matrix33 bz = base; bz.RotateZ(a);
    mu_assert(m33_near(bx, base * Matrix33::CreateRotationX(a), kRotTol), "M.RotateX(a) should equal M * CreateRotationX(a)");
    mu_assert(m33_near(by, base * Matrix33::CreateRotationY(a), kRotTol), "M.RotateY(a) should equal M * CreateRotationY(a)");
    mu_assert(m33_near(bz, base * Matrix33::CreateRotationZ(a), kRotTol), "M.RotateZ(a) should equal M * CreateRotationZ(a)");
}

    /**
 * @brief CreateRotation(x, y, z) is documented as Rx * Ry * Rz.
 */
MU_TEST(mat33_create_rotation_order)
{
    const Angle x = Angle::FromDegrees(Fxp(30)), y = Angle::FromDegrees(Fxp(45)), z = Angle::FromDegrees(Fxp(60));
    const Matrix33 combined = Matrix33::CreateRotation(x, y, z);
    const Matrix33 product = Matrix33::CreateRotationX(x) * Matrix33::CreateRotationY(y) * Matrix33::CreateRotationZ(z);
    snprintf(buffer, buffer_size, "CreateRotation row0 raw (%ld,%ld,%ld) vs Rx*Ry*Rz (%ld,%ld,%ld)",
        (long)combined.Row0.X.RawValue(), (long)combined.Row0.Y.RawValue(), (long)combined.Row0.Z.RawValue(),
        (long)product.Row0.X.RawValue(), (long)product.Row0.Y.RawValue(), (long)product.Row0.Z.RawValue());
    mu_assert(m33_near(combined, product, kRotTol), buffer);
}

    /**
 * @brief Scale(v) in place is documented as multiplying each row by its factor (row 0 by sx, row 1 by sy, row 2 by sz).
 */
MU_TEST(mat33_scale_in_place_documented)
{
    Matrix33 m(Vector3D(1, 2, 3), Vector3D(4, 5, 6), Vector3D(7, 8, 9));
    m.Scale(Vector3D(2, 3, 4));
    const Matrix33 expected(Vector3D(2, 4, 6), Vector3D(12, 15, 18), Vector3D(28, 32, 36));
    snprintf(buffer, buffer_size, "Scale row0 (%ld,%ld,%ld) != (2,4,6)",
        (long)m.Row0.X.As<int32_t>(), (long)m.Row0.Y.As<int32_t>(), (long)m.Row0.Z.As<int32_t>());
    mu_assert(m33_near(m, expected, 0), buffer);

    Matrix33 id = Matrix33::Identity();
    id.Scale(Vector3D(2, 1, Fxp(0.5)));
    mu_assert(m33_near(id, Matrix33::CreateScale(Vector3D(2, 1, Fxp(0.5))), 0), "Identity.Scale(v) should equal CreateScale(v)");
}

    /**
 * @brief TryInverse of a non-uniform scale gives the reciprocal scale exactly.
 */
MU_TEST(mat33_tryinverse_non_uniform_scale)
{
    const Matrix33 s = Matrix33::CreateScale(Vector3D(2, 4, Fxp(0.5)));
    Matrix33 inv;
    mu_assert(s.TryInverse(inv), "TryInverse should succeed for a non-uniform scale");
    mu_assert(m33_near(inv, Matrix33::CreateScale(Vector3D(Fxp(0.5), Fxp(0.25), 2)), 1), "Inverse of scale(2,4,0.5) should be scale(0.5,0.25,2)");
    mu_assert(m33_near(s * inv, Matrix33::Identity(), 1), "S * Inv(S) should be identity");
}

MU_TEST_SUITE(mat33_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&mat33_test_setup,
        &mat33_test_teardown,
        &mat33_test_output_header);

    MU_RUN_TEST(mat33_identity_and_vector_multiply);
    MU_RUN_TEST(mat33_scale_determinant_transpose);
    MU_RUN_TEST(mat33_transpose_involution);
    MU_RUN_TEST(mat33_tryinverse_success_and_failure);
    MU_RUN_TEST(mat33_rotation_documented_examples);
    MU_RUN_TEST(mat33_rotation_determinant_and_inverse);
    MU_RUN_TEST(mat33_rotate_in_place_matches_create);
    MU_RUN_TEST(mat33_create_rotation_order);
    MU_RUN_TEST(mat33_scale_in_place_documented);
    MU_RUN_TEST(mat33_tryinverse_non_uniform_scale);
}
}

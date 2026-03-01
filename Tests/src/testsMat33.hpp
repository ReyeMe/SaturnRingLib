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
        mu_assert(I.Transposed() == I, "Identity transpose should be identity");
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
        mu_assert(s.Transposed() == s, "Diagonal scale matrix should equal its transpose");
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

        const Matrix33 tt = m.Transposed().Transposed();
        mu_assert(tt == m, "Transpose(Transpose(M)) should equal M");
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
        mu_assert(prod == I, "M * Inv(M) should equal Identity for uniform scale 2");
    }

    /**
     * @brief Defines the test suite for all 3x3 Matrix (Mat33) functionality.
     */
    MU_TEST_SUITE(mat33_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&mat33_test_setup,
                                       &mat33_test_teardown,
                                       &mat33_test_output_header);

        MU_RUN_TEST(mat33_identity_and_vector_multiply);
        MU_RUN_TEST(mat33_scale_determinant_transpose);
        MU_RUN_TEST(mat33_transpose_involution);
        MU_RUN_TEST(mat33_tryinverse_success_and_failure);
    }
}

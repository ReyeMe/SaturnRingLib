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
    /**
     * @brief Sets up the environment for Matrix Stack unit tests.
     */
    void matrix_stack_test_setup(void)
    {
        // No initialization needed
    }

    /**
     * @brief Cleans up the environment after each Matrix Stack unit test.
     */
    void matrix_stack_test_teardown(void)
    {
        // No cleanup required
    }

    /**
     * @brief Displays a header for the Matrix Stack test suite upon the first error.
     */
    void matrix_stack_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_MATRIX_STACK****");
            }
            else
            {
                LogInfo("****UT_MATRIX_STACK_ERROR(S)****");
            }
        }
    }

    static inline bool matrix43_is_identity(const Matrix43& m)
    {
        return m.Row0 == Vector3D(1, 0, 0) &&
               m.Row1 == Vector3D(0, 1, 0) &&
               m.Row2 == Vector3D(0, 0, 1) &&
               m.Row3 == Vector3D(0, 0, 0);
    }

    /**
     * @brief Tests the initial state of a newly constructed matrix stack.
     * @details Verifies that a new stack is empty (contains only the base identity matrix),
     *          has a depth of 0, and its top matrix is the identity matrix.
     */
    MU_TEST(matrix_stack_construction_and_identity)
    {
        const MatrixStack s;
        mu_assert(s.IsEmpty(), "New stack should be empty (identity only)");
        mu_assert(s.GetDepth() == 0, "New stack depth should be 0");
        mu_assert(matrix43_is_identity(s.Top()), "Top of new stack should be identity");
    }

    /**
     * @brief Tests the core stack operations: Push, Pop, and Clear.
     * @details Verifies that `Push` increases stack depth and updates the top matrix,
     *          `Pop` decreases depth and restores the previous matrix, and `Clear` resets
     *          the stack to its initial identity state.
     */
    MU_TEST(matrix_stack_push_pop_clear)
    {
        MatrixStack s;

        Matrix43 m = Matrix43::Identity();
        m.Row3.X = 42;
        s.Push(m);
        mu_assert(s.GetDepth() == 1, "Push should increase depth");
        mu_assert(s.Top().Row3.X == Fxp(42), "Top translation X should match pushed matrix");

        s.Pop();
        mu_assert(s.GetDepth() == 0, "Pop should decrease depth");
        mu_assert(matrix43_is_identity(s.Top()), "After pop, top should be identity");

        s.Push(m);
        s.Clear();
        mu_assert(s.IsEmpty(), "Clear should reset to empty (identity only)");
        mu_assert(matrix43_is_identity(s.Top()), "After clear, top should be identity");
    }

    /**
     * @brief Tests that the matrix stack does not overflow its predefined maximum depth.
     * @details Verifies that pushing more matrices than the stack's capacity does not
     *          increase the depth beyond `MAX_DEPTH - 1`.
     */
    MU_TEST(matrix_stack_overflow_is_ignored)
    {
        MatrixStack s;
        const Matrix43 id = Matrix43::Identity();

        for (int i = 0; i < 32; i++)
        {
            s.Push(id);
        }

        mu_assert(s.GetDepth() == (MatrixStack::MAX_DEPTH - 1), "Depth should not exceed MAX_DEPTH-1");
    }

    /**
     * @brief Tests applying transformations to the top of the stack and transforming points/vectors.
     * @details Verifies that `TranslateTop`, `ScaleTop`, and `RotateTop` modify the top matrix correctly,
     *          and that `TransformPoint` and `TransformVector` apply the cumulative transformation as expected.
     */
    MU_TEST(matrix_stack_translate_scale_and_transform)
    {
        MatrixStack s;

        s.TranslateTop(Vector3D(1, 2, 3));
        mu_assert(s.Top().Row3 == Vector3D(1, 2, 3), "TranslateTop should update Row3");

        const Vector3D p = s.TransformPoint(Vector3D(2, 0, 0));
        mu_assert(p == Vector3D(3, 2, 3), "TransformPoint should apply translation");

        const Vector3D v = s.TransformVector(Vector3D(2, 0, 0));
        mu_assert(v == Vector3D(2, 0, 0), "TransformVector should not apply translation");

        s.Clear();
        s.ScaleTop(Vector3D(2, 3, 4));
        mu_assert(s.Top().Row0 == Vector3D(2, 0, 0), "ScaleTop should scale Row0");
        mu_assert(s.Top().Row1 == Vector3D(0, 3, 0), "ScaleTop should scale Row1");
        mu_assert(s.Top().Row2 == Vector3D(0, 0, 4), "ScaleTop should scale Row2");

        // Smoke: rotation with zeros should preserve identity
        s.Clear();
        s.RotateTop(Angle::Zero(), Angle::Zero(), Angle::Zero());
        mu_assert(matrix43_is_identity(s.Top()), "RotateTop with zero angles should keep identity");
    }

    /**
     * @brief Tests stack underflow behavior and the restoration of the parent matrix after a pop.
     * @details Verifies that calling `Pop` on an empty stack does not cause an underflow and
     *          that after a push/pop sequence, the original parent matrix is correctly restored.
     */
    MU_TEST(matrix_stack_pop_underflow_and_parent_restore)
    {
        MatrixStack s;
        s.Pop();
        mu_assert(s.GetDepth() == 0, "Pop at depth 0 should not underflow");
        mu_assert(matrix43_is_identity(s.Top()), "Pop at depth 0 should keep identity");

        // Parent/child behavior: push current, translate child, pop returns parent
        s.Clear();
        const Matrix43 parent = s.Top();
        s.Push(parent);
        s.TranslateTop(Vector3D(1, 0, 0));
        mu_assert(s.Top().Row3 == Vector3D(1, 0, 0), "Child translation should apply on top");
        s.Pop();
        mu_assert(s.Top().Row3 == Vector3D(0, 0, 0), "After pop, should restore parent matrix");
    }

    /**
     * @brief Defines the test suite for all Matrix Stack functionality.
     */
    MU_TEST_SUITE(matrix_stack_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&matrix_stack_test_setup,
                                       &matrix_stack_test_teardown,
                                       &matrix_stack_test_output_header);

        MU_RUN_TEST(matrix_stack_construction_and_identity);
        MU_RUN_TEST(matrix_stack_push_pop_clear);
        MU_RUN_TEST(matrix_stack_overflow_is_ignored);
        MU_RUN_TEST(matrix_stack_translate_scale_and_transform);
        MU_RUN_TEST(matrix_stack_pop_underflow_and_parent_restore);
    }
}

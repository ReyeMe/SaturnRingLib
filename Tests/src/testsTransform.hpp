// Tests/src/testsTransform.hpp
// SRL::Scene3D matrix operations (SGL's current matrix): the matrix stack, translate, rotate, scale, multiply, invert,
// transpose, identity parts, LookAt, and passing SRL::Math::Matrix43 values in and out.
// Each test starts on its own identity matrix (setup pushes one, teardown pops it).
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_scene3d.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Math::Matrix43;
using SRL::Math::Trigonometry;
using SRL::Math::Types::Angle;
using SRL::Math::Types::Fxp;
using SRL::Math::Types::Vector3D;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void transform_test_setup(void) { Scene3D::PushIdentityMatrix(); }
void transform_test_teardown(void) { Scene3D::PopMatrix(); }

void transform_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_TRANSFORM_ERROR(S)****");
    }
}

/** @brief Fxp value in thousandths, for messages. */
static int32_t transform_milli(const Fxp &value) { return static_cast<int32_t>((static_cast<int64_t>(value.RawValue()) * 1000) >> 16); }

/** @brief Two values within 0.03 of each other: SGL's sine tables and 16.16 rounding leave small errors. */
static bool transform_near(const Fxp &a, const Fxp &b) { return (a - b).Abs().RawValue() <= 0x800; }

static bool transform_near(const Vector3D &a, const Vector3D &b)
{
    return transform_near(a.X, b.X) && transform_near(a.Y, b.Y) && transform_near(a.Z, b.Z);
}

/** @brief Writes "<label> (x, y, z), expected (x, y, z)" in thousandths to the message buffer. */
static const char *transform_message(const char *label, const Vector3D &actual, const Vector3D &expected)
{
    snprintf(buffer, buffer_size, "%s (%d, %d, %d)/1000, expected (%d, %d, %d)/1000", label, transform_milli(actual.X),
        transform_milli(actual.Y), transform_milli(actual.Z), transform_milli(expected.X), transform_milli(expected.Y),
        transform_milli(expected.Z));
    return buffer;
}

/** @brief The identity matrix leaves points and directions unchanged. */
MU_TEST(transform_identity)
{
    const Vector3D p(1.5, -2, 300);
    mu_assert(transform_near(Scene3D::TransformPoint(p), p), transform_message("Point", Scene3D::TransformPoint(p), p));
    mu_assert(transform_near(Scene3D::TransformVector(p), p), transform_message("Vector", Scene3D::TransformVector(p), p));
}

/** @brief Translate moves points, not directions; translations add up; both overloads do the same. */
MU_TEST(transform_translate)
{
    const Vector3D p(1, 2, 3);
    Scene3D::Translate(10, 20, 30);
    const Vector3D once = Scene3D::TransformPoint(p);
    const Vector3D direction = Scene3D::TransformVector(p);
    Scene3D::Translate(Vector3D(-1, 0, 5));
    const Vector3D twice = Scene3D::TransformPoint(p);

    mu_assert(transform_near(once, Vector3D(11, 22, 33)), transform_message("Translated point", once, Vector3D(11, 22, 33)));
    mu_assert(transform_near(direction, p), transform_message("Translated direction", direction, p));
    mu_assert(transform_near(twice, Vector3D(10, 22, 38)), transform_message("Point after two translations", twice, Vector3D(10, 22, 38)));
}

/** @brief PushMatrix saves the current matrix and PopMatrix brings it back; PushIdentityMatrix starts from identity. */
MU_TEST(transform_push_pop)
{
    const Vector3D origin(0, 0, 0);
    Scene3D::Translate(5, 0, 0);
    Scene3D::PushMatrix();
    Scene3D::Translate(0, 7, 0);
    const Vector3D pushed = Scene3D::TransformPoint(origin);
    Scene3D::PushIdentityMatrix();
    const Vector3D identity = Scene3D::TransformPoint(origin);
    Scene3D::PopMatrix();
    Scene3D::PopMatrix();
    const Vector3D popped = Scene3D::TransformPoint(origin);

    mu_assert(transform_near(pushed, Vector3D(5, 7, 0)), transform_message("After PushMatrix and Translate", pushed, Vector3D(5, 7, 0)));
    mu_assert(transform_near(identity, origin), transform_message("After PushIdentityMatrix", identity, origin));
    mu_assert(transform_near(popped, Vector3D(5, 0, 0)), transform_message("After both PopMatrix", popped, Vector3D(5, 0, 0)));
}

/** @brief A quarter turn moves one axis onto another and keeps the rotation axis; four quarter turns change nothing. */
MU_TEST(transform_rotate_quarter_turns)
{
    const Angle quarter = Angle::FromDegrees(90);

    Scene3D::RotateX(quarter);
    const Vector3D xKeepsX = Scene3D::TransformPoint(Vector3D(1, 0, 0));
    const Vector3D xMovesY = Scene3D::TransformPoint(Vector3D(0, 1, 0));
    Scene3D::LoadIdentity();
    Scene3D::RotateY(quarter);
    const Vector3D yMovesX = Scene3D::TransformPoint(Vector3D(1, 0, 0));
    Scene3D::LoadIdentity();
    Scene3D::RotateZ(quarter);
    const Vector3D zMovesX = Scene3D::TransformPoint(Vector3D(1, 0, 0));
    Scene3D::RotateZ(quarter);
    Scene3D::RotateZ(quarter);
    Scene3D::RotateZ(quarter);
    const Vector3D fullTurn = Scene3D::TransformPoint(Vector3D(1, 2, 3));

    mu_assert(transform_near(xKeepsX, Vector3D(1, 0, 0)), transform_message("RotateX(90) moved the X axis to", xKeepsX, Vector3D(1, 0, 0)));
    mu_assert(transform_near(xMovesY.Y, 0) && transform_near(xMovesY.Z.Abs(), 1), transform_message("RotateX(90) of the Y axis", xMovesY, Vector3D(0, 0, 1)));
    mu_assert(transform_near(yMovesX.X, 0) && transform_near(yMovesX.Z.Abs(), 1), transform_message("RotateY(90) of the X axis", yMovesX, Vector3D(0, 0, 1)));
    mu_assert(transform_near(zMovesX.X, 0) && transform_near(zMovesX.Y.Abs(), 1), transform_message("RotateZ(90) of the X axis", zMovesX, Vector3D(0, 1, 0)));
    mu_assert(transform_near(fullTurn, Vector3D(1, 2, 3)), transform_message("Four RotateZ(90)", fullTurn, Vector3D(1, 2, 3)));
}

/** @brief The sine/cosine overloads of RotateX/Y/Z turn the same way as the angle overloads. */
MU_TEST(transform_rotate_sin_cos_matches_angle)
{
    const Angle angle = Angle::FromDegrees(30);
    const Fxp sin = Trigonometry::Sin(angle);
    const Fxp cos = Trigonometry::Cos(angle);
    const Vector3D p(1, 2, 3);
    Vector3D byAngle[3];
    Vector3D bySinCos[3];

    Scene3D::RotateX(angle);
    byAngle[0] = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::RotateX(sin, cos);
    bySinCos[0] = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::RotateY(angle);
    byAngle[1] = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::RotateY(sin, cos);
    bySinCos[1] = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::RotateZ(angle);
    byAngle[2] = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::RotateZ(sin, cos);
    bySinCos[2] = Scene3D::TransformPoint(p);

    const char *names[3] = { "RotateX(sin, cos)", "RotateY(sin, cos)", "RotateZ(sin, cos)" };
    for (int i = 0; i < 3; i++)
    {
        mu_assert(transform_near(bySinCos[i], byAngle[i]), transform_message(names[i], bySinCos[i], byAngle[i]));
    }
}

/** @brief Rotate around the X, Y or Z axis gives the same result as RotateX, RotateY or RotateZ. */
MU_TEST(transform_rotate_axis_matches_rotate_xyz)
{
    const Angle angle = Angle::FromDegrees(40);
    const Vector3D p(3, -2, 1);
    const Vector3D axes[3] = { Vector3D(1, 0, 0), Vector3D(0, 1, 0), Vector3D(0, 0, 1) };
    Vector3D byAxis[3];
    Vector3D byName[3];

    for (int i = 0; i < 3; i++)
    {
        Scene3D::LoadIdentity();
        Scene3D::Rotate(axes[i], angle);
        byAxis[i] = Scene3D::TransformPoint(p);
        Scene3D::LoadIdentity();
        if (i == 0) Scene3D::RotateX(angle);
        if (i == 1) Scene3D::RotateY(angle);
        if (i == 2) Scene3D::RotateZ(angle);
        byName[i] = Scene3D::TransformPoint(p);
    }

    const char *names[3] = { "Rotate around X", "Rotate around Y", "Rotate around Z" };
    for (int i = 0; i < 3; i++)
    {
        mu_assert(transform_near(byAxis[i], byName[i]), transform_message(names[i], byAxis[i], byName[i]));
    }
}

/** @brief The three Scale overloads scale each axis. */
MU_TEST(transform_scale)
{
    const Vector3D p(1, 1, 1);
    Scene3D::Scale(2, 3, 4);
    const Vector3D perAxis = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::Scale(Vector3D(0.5, 2, 1));
    const Vector3D byVector = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::Scale(Fxp(3));
    const Vector3D uniform = Scene3D::TransformPoint(p);

    mu_assert(transform_near(perAxis, Vector3D(2, 3, 4)), transform_message("Scale(2, 3, 4)", perAxis, Vector3D(2, 3, 4)));
    mu_assert(transform_near(byVector, Vector3D(0.5, 2, 1)), transform_message("Scale(Vector3D)", byVector, Vector3D(0.5, 2, 1)));
    mu_assert(transform_near(uniform, Vector3D(3, 3, 3)), transform_message("Scale(3)", uniform, Vector3D(3, 3, 3)));
}

/** @brief A test matrix that is not symmetric, so a swapped row and column shows. */
static Matrix43 transform_test_matrix()
{
    return Matrix43(Vector3D(1, 2, 0), Vector3D(0, 1, 0), Vector3D(0, 0.5, 1), Vector3D(10, 20, 30));
}

/** @brief GetMatrix returns exactly what SetMatrix set. */
MU_TEST(transform_get_set_matrix_round_trip)
{
    Matrix43 set = transform_test_matrix();
    Matrix43 got;
    Scene3D::SetMatrix(set);
    Scene3D::GetMatrix(&got);

    const Vector3D *setRows[4] = { &set.Row0, &set.Row1, &set.Row2, &set.Row3 };
    const Vector3D *gotRows[4] = { &got.Row0, &got.Row1, &got.Row2, &got.Row3 };
    for (int row = 0; row < 4; row++)
    {
        snprintf(buffer, buffer_size, "Row %d", row);
        mu_assert(*gotRows[row] == *setRows[row], transform_message(buffer, *gotRows[row], *setRows[row]));
    }
}

/** @brief A Matrix43 given to SetMatrix moves points the same way as Matrix43::TransformPoint. */
MU_TEST(transform_set_matrix_matches_matrix43)
{
    Matrix43 matrix = transform_test_matrix();
    const Vector3D p(0, 1, 0);
    Scene3D::SetMatrix(matrix);
    const Vector3D bySgl = Scene3D::TransformPoint(p);

    mu_assert(transform_near(bySgl, matrix.TransformPoint(p)), transform_message("Scene3D::TransformPoint", bySgl, matrix.TransformPoint(p)));
}

/** @brief MultiplyMatrix with a translation adds it; with Matrix43::CreateRotationZ it turns like RotateZ. */
MU_TEST(transform_multiply_matrix)
{
    const Angle angle = Angle::FromDegrees(30);
    const Vector3D p(1, 2, 3);
    Matrix43 translation = Matrix43::CreateTranslation(Vector3D(0, 2, 0));
    Matrix43 rotation = Matrix43::CreateRotationZ(angle);

    Scene3D::Translate(10, 0, 0);
    Scene3D::MultiplyMatrix(translation);
    const Vector3D translated = Scene3D::TransformPoint(Vector3D(0, 0, 0));
    Scene3D::LoadIdentity();
    Scene3D::RotateZ(angle);
    const Vector3D byRotateZ = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    Scene3D::MultiplyMatrix(rotation);
    const Vector3D byMultiply = Scene3D::TransformPoint(p);

    mu_assert(transform_near(translated, Vector3D(10, 2, 0)), transform_message("Two translations", translated, Vector3D(10, 2, 0)));
    mu_assert(transform_near(byMultiply, byRotateZ), transform_message("MultiplyMatrix(CreateRotationZ(30))", byMultiply, byRotateZ));
}

/** @brief After InvertMatrix, a transformed point goes back to where it was. */
MU_TEST(transform_invert)
{
    const Vector3D p(4, -5, 6);
    Scene3D::Translate(10, 20, 30);
    Scene3D::RotateY(Angle::FromDegrees(40));
    Scene3D::RotateX(Angle::FromDegrees(15));
    const Vector3D moved = Scene3D::TransformPoint(p);
    Scene3D::InvertMatrix();
    const Vector3D back = Scene3D::TransformPoint(moved);

    mu_assert(transform_near(back, p), transform_message("Inverted matrix moved the point back to", back, p));
}

/** @brief TransposeMatrix swaps rows and columns of the rotation part and drops the translation. */
MU_TEST(transform_transpose)
{
    Matrix43 before;
    Matrix43 after;
    Scene3D::Translate(5, 6, 7);
    Scene3D::RotateZ(Angle::FromDegrees(30));
    Scene3D::RotateX(Angle::FromDegrees(20));
    Scene3D::GetMatrix(&before);
    Scene3D::TransposeMatrix();
    Scene3D::GetMatrix(&after);

    const Vector3D expected[3] = {
        Vector3D(before.Row0.X, before.Row1.X, before.Row2.X),
        Vector3D(before.Row0.Y, before.Row1.Y, before.Row2.Y),
        Vector3D(before.Row0.Z, before.Row1.Z, before.Row2.Z) };
    const Vector3D *rows[3] = { &after.Row0, &after.Row1, &after.Row2 };
    for (int row = 0; row < 3; row++)
    {
        snprintf(buffer, buffer_size, "Transposed row %d", row);
        mu_assert(transform_near(*rows[row], expected[row]), transform_message(buffer, *rows[row], expected[row]));
    }
    mu_assert(transform_near(after.Row3, Vector3D(0, 0, 0)), transform_message("Translation after TransposeMatrix", after.Row3, Vector3D(0, 0, 0)));
}

/** @brief IdentityRotationMatrix keeps only the translation, IdentityTranslationMatrix only the rotation, LoadIdentity neither. */
MU_TEST(transform_identity_parts)
{
    const Vector3D p(1, 0, 0);
    Scene3D::Translate(5, 6, 7);
    Scene3D::RotateZ(Angle::FromDegrees(90));
    const Vector3D rotatedOnly = Scene3D::TransformVector(p);

    Scene3D::PushMatrix();
    Scene3D::IdentityRotationMatrix();
    const Vector3D translationKept = Scene3D::TransformPoint(p);
    Scene3D::PopMatrix();
    Scene3D::IdentityTranslationMatrix();
    const Vector3D rotationKept = Scene3D::TransformPoint(p);
    Scene3D::LoadIdentity();
    const Vector3D nothingKept = Scene3D::TransformPoint(p);

    mu_assert(transform_near(translationKept, Vector3D(6, 6, 7)), transform_message("After IdentityRotationMatrix", translationKept, Vector3D(6, 6, 7)));
    mu_assert(transform_near(rotationKept, rotatedOnly), transform_message("After IdentityTranslationMatrix", rotationKept, rotatedOnly));
    mu_assert(transform_near(nothingKept, p), transform_message("After LoadIdentity", nothingKept, p));
}

/** @brief After LookAt the camera is at the origin and the target straight ahead (on +Z) at the same distance. */
MU_TEST(transform_look_at)
{
    const Vector3D target(0, 0, 0);
    Scene3D::LookAt(Vector3D(0, 0, -100), target, Angle::FromDegrees(0));
    const Vector3D fromBehind = Scene3D::TransformPoint(target);
    Scene3D::LoadIdentity();
    Scene3D::LookAt(Vector3D(100, 0, 0), target, Angle::FromDegrees(0));
    const Vector3D fromSide = Scene3D::TransformPoint(target);
    const Vector3D camera = Scene3D::TransformPoint(Vector3D(100, 0, 0));

    mu_assert(transform_near(fromBehind, Vector3D(0, 0, 100)), transform_message("Target seen from (0, 0, -100)", fromBehind, Vector3D(0, 0, 100)));
    mu_assert(transform_near(fromSide, Vector3D(0, 0, 100)), transform_message("Target seen from (100, 0, 0)", fromSide, Vector3D(0, 0, 100)));
    mu_assert(transform_near(camera, target), transform_message("Camera position after LookAt", camera, target));
}

MU_TEST_SUITE(transform_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&transform_test_setup, &transform_test_teardown, &transform_test_output_header);

    MU_RUN_TEST(transform_identity);
    MU_RUN_TEST(transform_translate);
    MU_RUN_TEST(transform_push_pop);
    MU_RUN_TEST(transform_rotate_quarter_turns);
    MU_RUN_TEST(transform_rotate_sin_cos_matches_angle);
    MU_RUN_TEST(transform_rotate_axis_matches_rotate_xyz);
    MU_RUN_TEST(transform_scale);
    MU_RUN_TEST(transform_get_set_matrix_round_trip);
    MU_RUN_TEST(transform_set_matrix_matches_matrix43);
    MU_RUN_TEST(transform_multiply_matrix);
    MU_RUN_TEST(transform_invert);
    MU_RUN_TEST(transform_transpose);
    MU_RUN_TEST(transform_identity_parts);
    MU_RUN_TEST(transform_look_at);
}

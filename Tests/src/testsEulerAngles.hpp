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

/** @brief True if the angle is within 2 raw units (~0.01 degree) of the expected degrees; wraps around 360. */
static bool euler_angle_near(const Angle& angle, int16_t degrees)
{
    const int16_t diff = static_cast<int16_t>(angle.RawValue() - Angle::FromDegrees(Fxp(degrees)).RawValue());
    return diff >= -2 && diff <= 2;
}

    /**
 * @brief Tests that a default-constructed EulerAngles object is initialized to zero.
 */
MU_TEST(euler_angles_test_initialization_zero)
{
    EulerAngles euler;
    snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.pitch.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.pitch, 0), buffer);
    snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.yaw.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.yaw, 0), buffer);
    snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.roll.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.roll, 0), buffer);
}

    /**
 * @brief Tests setting the pitch, yaw, and roll of an EulerAngles object.
 */
MU_TEST(euler_angles_test_set_angles)
{
    EulerAngles euler;
    euler.pitch = Angle::FromDegrees(30);
    euler.yaw = Angle::FromDegrees(45);
    euler.roll = Angle::FromDegrees(60);
    snprintf(buffer, buffer_size, "Set angles failed: %d != 30", euler.pitch.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.pitch, 30), buffer);
    snprintf(buffer, buffer_size, "Set angles failed: %d != 45", euler.yaw.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.yaw, 45), buffer);
    snprintf(buffer, buffer_size, "Set angles failed: %d != 60", euler.roll.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.roll, 60), buffer);
}

    /**
 * @brief Tests the normalization of Euler angles.
 * @details Verifies that angles outside the standard range [0, 360) are correctly wrapped around.
 */
MU_TEST(euler_angles_test_normalization)
{
    EulerAngles euler;
    euler.pitch = Angle::FromDegrees(390); // 390 degrees should normalize to 30 degrees
    euler.yaw = Angle::FromDegrees(-45); // -45 degrees should normalize to 315 degrees
    euler.roll = Angle::FromDegrees(720); // 720 degrees should normalize to 0 degrees
    snprintf(buffer, buffer_size, "Normalization failed: %d != 30", euler.pitch.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.pitch, 30), buffer);
    snprintf(buffer, buffer_size, "Normalization failed: %d != 315", euler.yaw.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.yaw, 315), buffer);
    snprintf(buffer, buffer_size, "Normalization failed: %d != 0", euler.roll.ToDegrees().As<int32_t>());
    mu_assert(euler_angle_near(euler.roll, 0), buffer);
}



    /**
 * @brief Defines the test suite for all Euler angle functionality.
 */
MU_TEST_SUITE(euler_angles_test_suite)
{
    MU_RUN_TEST(euler_angles_test_initialization_zero);
    MU_RUN_TEST(euler_angles_test_set_angles);
    MU_RUN_TEST(euler_angles_test_normalization);
}
}
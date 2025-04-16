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
    extern uint32_t suite_error_counter;

    // UT setup function, called before every tests
    void euler_angles_test_setup(void)
    {
        // Nothing to do here
    }

    // UT teardown function, called after every tests
    void euler_angles_test_teardown(void)
    {
        /* Nothing */
    }

    // UT output header function, called on the first test failure
    void euler_angles_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_EULER_ANGLES****");
            }
            else
            {
                LogInfo("****UT_EULER_ANGLES_ERROR(S)****");
            }
        }
    }

    // Test initialization of Euler angles to zero
    MU_TEST(euler_angles_test_initialization_zero)
    {
        EulerAngles euler;
        snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.pitch.ToDegrees().As<int32_t>());
        mu_assert(euler.pitch.ToDegrees() == 0, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.yaw.ToDegrees().As<int32_t>());
        mu_assert(euler.yaw.ToDegrees() == 0, buffer);
        snprintf(buffer, buffer_size, "Initialization failed: %d != 0", euler.roll.ToDegrees().As<int32_t>());
        mu_assert(euler.roll.ToDegrees() == 0, buffer);
    }

    // Test setting Euler angles
    MU_TEST(euler_angles_test_set_angles)
    {
        EulerAngles euler;
        euler.pitch = Angle::FromDegrees(30);
        euler.yaw = Angle::FromDegrees(45);
        euler.roll = Angle::FromDegrees(60);
        snprintf(buffer, buffer_size, "Set angles failed: %d != 30", euler.pitch.ToDegrees().As<int32_t>());
        mu_assert(euler.pitch.ToDegrees() == 30, buffer);
        snprintf(buffer, buffer_size, "Set angles failed: %d != 45", euler.yaw.ToDegrees().As<int32_t>());
        mu_assert(euler.yaw.ToDegrees() == 45, buffer);
        snprintf(buffer, buffer_size, "Set angles failed: %d != 60", euler.roll.ToDegrees().As<int32_t>());
        mu_assert(euler.roll.ToDegrees() == 60, buffer);
    }

    // Test normalization of Euler angles
    MU_TEST(euler_angles_test_normalization)
    {
        EulerAngles euler;
        euler.pitch = Angle::FromDegrees(390); // 390 degrees should normalize to 30 degrees
        euler.yaw = Angle::FromDegrees(-45); // -45 degrees should normalize to 315 degrees
        euler.roll = Angle::FromDegrees(720); // 720 degrees should normalize to 0 degrees
        snprintf(buffer, buffer_size, "Normalization failed: %d != 30", euler.pitch.ToDegrees().As<int32_t>());
        mu_assert(euler.pitch.ToDegrees() == 30, buffer);
        snprintf(buffer, buffer_size, "Normalization failed: %d != 315", euler.yaw.ToDegrees().As<int32_t>());
        mu_assert(euler.yaw.ToDegrees() == 315, buffer);
        snprintf(buffer, buffer_size, "Normalization failed: %d != 0", euler.roll.ToDegrees().As<int32_t>());
        mu_assert(euler.roll.ToDegrees() == 0, buffer);
    }

    // Test addition of Euler angles
    MU_TEST(euler_angles_test_addition)
    {
        EulerAngles euler1;
        euler1.pitch = Angle::FromDegrees(30);
        euler1.yaw = Angle::FromDegrees(45);
        euler1.roll = Angle::FromDegrees(60);

        EulerAngles euler2;
        euler2.pitch = Angle::FromDegrees(10);
        euler2.yaw = Angle::FromDegrees(20);
        euler2.roll = Angle::FromDegrees(30);

        EulerAngles result = euler1 + euler2;
        snprintf(buffer, buffer_size, "Addition failed: %d != 40", result.pitch.ToDegrees().As<int32_t>());
        mu_assert(result.pitch.ToDegrees() == 40, buffer);
        snprintf(buffer, buffer_size, "Addition failed: %d != 65", result.yaw.ToDegrees().As<int32_t>());
        mu_assert(result.yaw.ToDegrees() == 65, buffer);
        snprintf(buffer, buffer_size, "Addition failed: %d != 90", result.roll.ToDegrees().As<int32_t>());
        mu_assert(result.roll.ToDegrees() == 90, buffer);
    }

    // Test subtraction of Euler angles
    MU_TEST(euler_angles_test_subtraction)
    {
        EulerAngles euler1;
        euler1.pitch = Angle::FromDegrees(30);
        euler1.yaw = Angle::FromDegrees(45);
        euler1.roll = Angle::FromDegrees(60);

        EulerAngles euler2;
        euler2.pitch = Angle::FromDegrees(10);
        euler2.yaw = Angle::FromDegrees(20);
        euler2.roll = Angle::FromDegrees(30);

        EulerAngles result = euler1 - euler2;
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 20", result.pitch.ToDegrees().As<int32_t>());
        mu_assert(result.pitch.ToDegrees() == 20, buffer);
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 25", result.yaw.ToDegrees().As<int32_t>());
        mu_assert(result.yaw.ToDegrees() == 25, buffer);
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 30", result.roll.ToDegrees().As<int32_t>());
        mu_assert(result.roll.ToDegrees() == 30, buffer);
    }

    // Define the test suite with all unit tests
    MU_TEST_SUITE(euler_angles_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&euler_angles_test_setup,
                                       &euler_angles_test_teardown,
                                       &euler_angles_test_output_header);

        MU_RUN_TEST(euler_angles_test_initialization_zero);
        MU_RUN_TEST(euler_angles_test_set_angles);
        MU_RUN_TEST(euler_angles_test_normalization);
        MU_RUN_TEST(euler_angles_test_addition);
        MU_RUN_TEST(euler_angles_test_subtraction);
    }
}
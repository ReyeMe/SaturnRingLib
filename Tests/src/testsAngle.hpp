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

    constexpr double PI = 3.14159;

    // UT setup function, called before every tests
    void angle_test_setup(void)
    {
        // Nothomg to do here
    }

    // UT teardown function, called after every tests
    void angle_test_teardown(void)
    {
        /* Nothing */
    }

    // UT output header function, called on the first test failure
    void angle_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_ANGLE****");
            }
            else
            {
                LogInfo("****UT_ANGLE_ERROR(S)****");
            }
        }
    }

    /** @brief Tests that an angle initialized to zero degrees is also zero radians. */
    MU_TEST(angle_test_initialization_zero)
    {
        Fxp angle(0);
        Angle a1 = Angle::FromDegrees(angle);
        Fxp a2 = a1.ToRadians();
        snprintf(buffer, buffer_size, "%d != %d", angle.As<int32_t>(), a2.As<int32_t>());
        mu_assert(angle == a2, buffer);
    }

    /** @brief Tests subtracting 90 degrees from 180 degrees. */
    MU_TEST(angle_test_subtraction_half_circle_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(180);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 90", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(90) == a3, buffer);
    }

    /** @brief Tests subtracting 90 degrees from 0 degrees. */
    MU_TEST(angle_test_subtraction_zero_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(0);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != -90", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(-90) == a3, buffer);
    }

    /** @brief Tests subtracting 0 degrees from 90 degrees. */
    MU_TEST(angle_test_subtraction_quarter_circle_minus_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a2 - a1;
        snprintf(buffer, buffer_size, "%d != 90", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(90) == a3, buffer);
    }

    /** @brief Tests subtracting 90 degrees from a full circle (360 degrees). */
    MU_TEST(angle_test_subtraction_full_circle_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(360);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 270", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(270) == a3, buffer);
    }

    /** @brief Tests subtraction that involves multiple wraps. */
    MU_TEST(angle_test_subtraction_two_full_circles_minus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(720);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a1 - a2;
        snprintf(buffer, buffer_size, "%d != 270", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(270) == a3, buffer);
    }

    /** @brief Tests subtraction that involves multiple wraps with a negative result. */
    MU_TEST(angle_test_subtraction_quarter_circle_minus_two_full_circles)
    {
        Angle a1 = Angle::FromDegrees(720);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a2 - a1;
        snprintf(buffer, buffer_size, "%d != -630", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(-630) == a3, buffer);
    }

    /** @brief Tests adding two 90-degree angles. */
    MU_TEST(angle_test_addition_quarter_circle_plus_quarter_circle)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = a1 + a2;
        snprintf(buffer, buffer_size, "%d != 180", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(180) == a3, buffer);
    }

    /** @brief Tests adding two 180-degree angles, resulting in a full circle. */
    MU_TEST(angle_test_addition_half_circle_plus_half_circle)
    {
        Angle a1 = Angle::FromDegrees(180);
        Angle a2 = Angle::FromDegrees(180);
        Angle a3 = a1 + a2;
        snprintf(buffer, buffer_size, "%d != 360", a3.ToDegrees().As<int32_t>());
        mu_assert(Angle::FromDegrees(360) == a3, buffer);
    }

    /** @brief Tests normalization of a positive angle greater than 360 degrees. */
    MU_TEST(angle_test_normalization_positive)
    {
        Angle a1 = Angle::FromDegrees(450); // 450 degrees should normalize to 90 degrees
        Angle normalized = a1;
        snprintf(buffer, buffer_size, "Normalization failed: %d != 90", normalized.ToDegrees().As<int32_t>());
        mu_assert(normalized.ToDegrees() == 90, buffer);
    }

    /** @brief Tests normalization of a negative angle. */
    MU_TEST(angle_test_normalization_negative)
    {
        Angle a1 = Angle::FromDegrees(-90); // -90 degrees should normalize to 270 degrees
        Angle normalized = a1;
        snprintf(buffer, buffer_size, "Normalization failed: %d != 270", normalized.ToDegrees().As<int32_t>());
        mu_assert(normalized.ToDegrees() == 270, buffer);
    }

    /** @brief Tests basic arithmetic addition of two angles. */
    MU_TEST(angle_test_arithmetic_addition)
    {
        Angle a1 = Angle::FromDegrees(45);
        Angle a2 = Angle::FromDegrees(30);
        Angle result = a1 + a2;
        snprintf(buffer, buffer_size, "Addition failed: %d != 75", result.ToDegrees().As<int32_t>());
        mu_assert(Fxp(74.9) < result.ToDegrees() && result.ToDegrees() <75.1, buffer);
    }

    /** @brief Tests basic arithmetic subtraction of two angles. */
    MU_TEST(angle_test_arithmetic_subtraction)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(30);
        Angle result = a1 - a2;
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 60", result.ToDegrees().As<int32_t>());
        mu_assert(Fxp(59.9) < result.ToDegrees() && result.ToDegrees() < 60.1, buffer);
    }

    /** @brief Tests the greater than operator for angles. */
    MU_TEST(angle_test_comparison_greater)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(30);
        snprintf(buffer, buffer_size, "Comparison failed: 90 <= 30");
        mu_assert(a1 > a2, buffer);
    }

    /** @brief Tests the less than operator for angles. */
    MU_TEST(angle_test_comparison_less)
    {
        Angle a1 = Angle::FromDegrees(30);
        Angle a2 = Angle::FromDegrees(90);
        snprintf(buffer, buffer_size, "Comparison failed: 30 >= 90");
        mu_assert(a1 < a2, buffer);
    }

    /** @brief Tests the conversion from degrees to radians. */
    MU_TEST(angle_test_conversion_to_radians)
    {
        Angle a1 = Angle::FromDegrees(180);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "Conversion to radians failed: %d != 3.14159", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - PI) < 1, buffer);
    }

    /** @brief Tests the conversion from radians to degrees. */
    MU_TEST(angle_test_conversion_to_degrees)
    {
        Angle a1 = Angle::FromRadians(PI);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "Conversion to degrees failed: %d != 180", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 180) < 1e-2, buffer);
    }

    /** @brief Verifies that a zero-degree angle converts to zero radians. */
    MU_TEST(angle_test_to_radians_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "ToRadians failed: %d != 0", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - 0.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 180-degree angle converts to PI radians. */
    MU_TEST(angle_test_to_radians_pi)
    {
        Angle a1 = Angle::FromDegrees(180);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "ToRadians failed: %d != 3.14159", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - PI) < 1e-4, buffer);
    }

    /** @brief Verifies that a 90-degree angle converts to PI/2 radians. */
    MU_TEST(angle_test_to_radians_half_pi)
    {
        Angle a1 = Angle::FromDegrees(90);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "ToRadians failed: %d != 1.5708", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - PI / 2) < 1e-4, buffer);
    }

    /** @brief Verifies that a 360-degree angle converts to 2*PI or 0 radians due to wrapping. */
    MU_TEST(angle_test_to_radians_two_pi)
    {
        Angle a1 = Angle::FromDegrees(360);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "ToRadians failed: %d != 0 or 6.28318", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - 0.0) < 1e-4 || SRL::Math::Abs(radians - PI * 2) < 1e-4, buffer);
    }

    /** @brief Verifies that a -180-degree angle converts to -PI or PI radians due to wrapping. */
    MU_TEST(angle_test_to_radians_negative_pi)
    {
        Angle a1 = Angle::FromDegrees(-180);
        Fxp radians = a1.ToRadians();
        snprintf(buffer, buffer_size, "ToRadians failed: %d != -/+ 3.14159", radians.As<int32_t>());
        mu_assert(SRL::Math::Abs(radians - PI) < 1e-4, buffer);
    }

    /** @brief Verifies that a zero-turn angle converts to zero degrees. */
    MU_TEST(angle_test_to_degrees_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 0", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 0.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 90-degree angle remains 90 degrees after conversion. */
    MU_TEST(angle_test_to_degrees_90)
    {
        Angle a1 = Angle::FromDegrees(90);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 90", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 90.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 180-degree angle remains 180 degrees after conversion. */
    MU_TEST(angle_test_to_degrees_180)
    {
        Angle a1 = Angle::FromDegrees(180);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 180", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 180.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 270-degree angle remains 270 degrees after conversion. */
    MU_TEST(angle_test_to_degrees_270)
    {
        Angle a1 = Angle::FromDegrees(270);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 270", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 270.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 360-degree angle converts to 0 or 360 due to wrapping. */
    MU_TEST(angle_test_to_degrees_360)
    {
        Angle a1 = Angle::FromDegrees(360);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 360 or 0", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 360.0) < 1e-4 || degrees == 0, buffer);
    }

    /** @brief Verifies that a -90-degree angle converts to -90 or 270 due to wrapping. */
    MU_TEST(angle_test_to_degrees_negative_90)
    {
        Angle a1 = Angle::FromDegrees(-90);
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != -90 or 270", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees + 90.0) < 1e-4 || SRL::Math::Abs(degrees - 270.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 450-degree angle correctly normalizes to 90 degrees. */
    MU_TEST(angle_test_to_degrees_450)
    {
        Angle a1 = Angle::FromDegrees(450); // 450 degrees should normalize to 90 degrees
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "ToDegrees failed: %d != 90", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 90.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a zero-degree angle converts to zero turns. */
    MU_TEST(angle_test_to_turns_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns failed: %d != 0", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 90-degree angle converts to 0.25 turns. */
    MU_TEST(angle_test_to_turns_quarter)
    {
        Angle a1 = Angle::FromDegrees(90);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns failed: %d != 0.25", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.25) < 1e-4, buffer);
    }

    /** @brief Verifies that a 180-degree angle converts to 0.5 turns. */
    MU_TEST(angle_test_to_turns_half)
    {
        Angle a1 = Angle::FromDegrees(180);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns failed: %d != 0.5", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.5) < 1e-4, buffer);
    }

    /** @brief Verifies that a 270-degree angle converts to 0.75 turns. */
    MU_TEST(angle_test_to_turns_three_quarters)
    {
        Angle a1 = Angle::FromDegrees(270);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns failed: %d != 0.75", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.75) < 1e-4, buffer);
    }

    /** @brief Tests creating an angle from zero turns. */
    MU_TEST(angle_test_from_turns_zero)
    {
        Angle a1 = Angle::FromTurns(0.0f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from 0.25 turns. */
    MU_TEST(angle_test_from_turns_quarter)
    {
        Angle a1 = Angle::FromTurns(0.25f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests creating an angle from 0.5 turns. */
    MU_TEST(angle_test_from_turns_half)
    {
        Angle a1 = Angle::FromTurns(0.5f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 180, buffer);
    }

    /** @brief Tests creating an angle from 0.75 turns. */
    MU_TEST(angle_test_from_turns_three_quarters)
    {
        Angle a1 = Angle::FromTurns(0.75f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 270", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 270, buffer);
    }

    /** @brief Tests that creating an angle from 1.0 turns results in a zero-degree angle due to wrapping. */
    MU_TEST(angle_test_from_turns_full)
    {
        Angle a1 = Angle::FromTurns(1.0f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from a negative number of turns. */
    MU_TEST(angle_test_from_turns_negative_quarter)
    {
        Angle a1 = Angle::FromTurns(-0.25f);
        snprintf(buffer, buffer_size, "FromTurns failed: %d != -90 or 270", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == -90 || a1.ToDegrees() == 270, buffer);
    }

    /** @brief Tests that creating an angle from >1.0 turns normalizes correctly. */
    MU_TEST(angle_test_from_turns_one_and_a_quarter)
    {
        Angle a1 = Angle::FromTurns(1.25f); // 1.25 turns should normalize to 90 degrees
        snprintf(buffer, buffer_size, "FromTurns failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests handling of a zero angle edge case. */
    MU_TEST(angle_test_edge_case_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        snprintf(buffer, buffer_size, "Zero angle failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests handling of a full circle (360 degrees) angle edge case. */
    MU_TEST(angle_test_edge_case_full_circle)
    {
        Angle a1 = Angle::FromDegrees(360);
        snprintf(buffer, buffer_size, "Full circle failed: %d != 0 or 360", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0 || a1.ToDegrees() == 360, buffer);
    }

    /** @brief Tests handling of a negative angle edge case. */
    MU_TEST(angle_test_edge_case_negative)
    {
        Angle a1 = Angle::FromDegrees(-45);
        snprintf(buffer, buffer_size, "Negative angle failed: %d != -45 or 315", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == -45 || a1.ToDegrees() == 315, buffer);
    }

    /** @brief Tests handling of an angle greater than 360 degrees. */
    MU_TEST(angle_test_edge_case_greater_than_full_circle)
    {
        Angle a1 = Angle::FromDegrees(450); // 450 degrees should normalize to 90 degrees
        snprintf(buffer, buffer_size, "Angle > 360 failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests handling of an angle that is a multiple of 360 degrees. */
    MU_TEST(angle_test_edge_case_multiple_full_circles)
    {
        Angle a1 = Angle::FromDegrees(720); // 720 degrees should normalize to 0 degrees
        snprintf(buffer, buffer_size, "Multiple full circles failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests handling of an angle that is a negative multiple of 360 degrees. */
    MU_TEST(angle_test_edge_case_negative_multiple_full_circles)
    {
        Angle a1 = Angle::FromDegrees(-720); // -720 degrees should normalize to 0 degrees
        snprintf(buffer, buffer_size, "Negative multiple full circles failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from a raw 16-bit value of 0. */
    MU_TEST(angle_test_build_raw_zero)
    {
        Angle a1 = Angle::BuildRaw(0);
        snprintf(buffer, buffer_size, "BuildRaw failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from a raw 16-bit value representing 90 degrees. */
    MU_TEST(angle_test_build_raw_half_pi)
    {
        Angle a1 = Angle::BuildRaw(0x4000); // 90 degrees
        snprintf(buffer, buffer_size, "BuildRaw failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests creating an angle from a raw 16-bit value representing 180 degrees. */
    MU_TEST(angle_test_build_raw_pi)
    {
        Angle a1 = Angle::BuildRaw(0x8000); // 180 degrees
        snprintf(buffer, buffer_size, "BuildRaw failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 180, buffer);
    }

    /** @brief Tests creating an angle from a raw 16-bit value representing 270 degrees. */
    MU_TEST(angle_test_build_raw_three_quarters_pi)
    {
        Angle a1 = Angle::BuildRaw(0xC000); // 270 degrees
        snprintf(buffer, buffer_size, "BuildRaw failed: %d != 270", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 270, buffer);
    }

    /** @brief Tests creating an angle from the maximum raw 16-bit value. */
    MU_TEST(angle_test_build_raw_full_circle)
    {
        Angle a1 = Angle::BuildRaw(0xFFFF); // Close to 360 degrees
        snprintf(buffer, buffer_size, "BuildRaw failed: %d != 359", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees().As<int32_t>() == 359, buffer);
    }

    /** @brief Tests creating an angle from zero radians. */
    MU_TEST(angle_test_from_radians_zero)
    {
        Angle a1 = Angle::FromRadians(0.0);
        snprintf(buffer, buffer_size, "FromRadians failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from PI radians. */
    MU_TEST(angle_test_from_radians_pi)
    {
        Angle a1 = Angle::FromRadians(PI); // π radians should be 180 degrees
        snprintf(buffer, buffer_size, "FromRadians failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() < 181 && a1.ToDegrees() > 179, buffer);
    }

    /** @brief Tests creating an angle from PI/2 radians. */
    MU_TEST(angle_test_from_radians_half_pi)
    {
        Angle a1 = Angle::FromRadians(PI / 2); // π/2 radians should be 90 degrees
        snprintf(buffer, buffer_size, "FromRadians failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() < 91 && a1.ToDegrees() > 89, buffer);
    }

    /** @brief Tests that creating an angle from 2*PI radians results in a zero-degree angle due to wrapping. */
    MU_TEST(angle_test_from_radians_two_pi)
    {
        Angle a1 = Angle::FromRadians(2 * PI); // 2π radians should be 0 degrees (full circle)
        Fxp degrees = a1.ToDegrees();
        snprintf(buffer, buffer_size, "FromRadians failed: %d != 0 or 359", degrees.As<int32_t>());
        mu_assert(degrees == 0 || degrees.As<int32_t>() == 359, buffer);
    }

    /** @brief Tests creating an angle from a negative radian value. */
    MU_TEST(angle_test_from_radians_negative_pi)
    {
        Angle a1 = Angle::FromRadians(-PI); // -π radians should be -180 degrees
        snprintf(buffer, buffer_size, "FromRadians failed: %d != 180 or 180", a1.ToDegrees().As<int32_t>());
        mu_assert((a1.ToDegrees() > -181 && a1.ToDegrees() < -179) || (a1.ToDegrees() > 179 && a1.ToDegrees() < 181), buffer);
    }

    /** @brief Tests creating an angle from zero degrees. */
    MU_TEST(angle_test_from_degrees_zero)
    {
        Angle a1 = Angle::FromDegrees(0.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }


    /** @brief Verifies that a 360-degree angle converts to 0 turns due to wrapping. */
    MU_TEST(angle_test_to_turns_full_wraps_to_zero)
    {
        Angle a1 = Angle::FromDegrees(360);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns full-wrap failed: %d != 0", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a negative angle correctly wraps when converting to turns. */
    MU_TEST(angle_test_to_turns_negative_quarter_wraps_to_three_quarters)
    {
        Angle a1 = Angle::FromDegrees(-90);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns negative-wrap failed: %d != 0.75", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.75) < 1e-4, buffer);
    }

    /** @brief Verifies that angles > 360 degrees wrap correctly when converting to turns. */
    MU_TEST(angle_test_to_turns_one_and_a_quarter_wraps_to_quarter)
    {
        Angle a1 = Angle::FromDegrees(450);
        Fxp turns = a1.ToTurns();
        snprintf(buffer, buffer_size, "ToTurns >1-wrap failed: %d != 0.25", turns.As<int32_t>());
        mu_assert(SRL::Math::Abs(turns - 0.25) < 1e-4, buffer);
    }
    
    /** @brief Tests creating an angle from 90 degrees. */
    MU_TEST(angle_test_from_degrees_90)
    {
        Angle a1 = Angle::FromDegrees(90.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests creating an angle from 180 degrees. */
    MU_TEST(angle_test_from_degrees_180)
    {
        Angle a1 = Angle::FromDegrees(180.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 180, buffer);
    }

    /** @brief Tests creating an angle from 270 degrees. */
    MU_TEST(angle_test_from_degrees_270)
    {
        Angle a1 = Angle::FromDegrees(270.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 270", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 270, buffer);
    }

    /** @brief Tests that creating an angle from 360 degrees results in a zero-degree angle due to wrapping. */
    MU_TEST(angle_test_from_degrees_360)
    {
        Angle a1 = Angle::FromDegrees(360.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests creating an angle from a negative degree value. */
    MU_TEST(angle_test_from_degrees_negative_90)
    {
        Angle a1 = Angle::FromDegrees(-90.0);
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != -90 or 270", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == -90 || a1.ToDegrees() == 270, buffer);
    }

    /** @brief Tests that creating an angle from >360 degrees normalizes correctly. */
    MU_TEST(angle_test_from_degrees_450)
    {
        Angle a1 = Angle::FromDegrees(450.0); // 450 degrees should normalize to 90 degrees
        snprintf(buffer, buffer_size, "FromDegrees failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests the `Angle::Zero` constant. */
    MU_TEST(angle_test_constant_zero)
    {
        Angle a1 = Angle::Zero();
        snprintf(buffer, buffer_size, "Zero angle failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests the `Angle::Pi` constant. */
    MU_TEST(angle_test_constant_pi)
    {
        Angle a1 = Angle::Pi();
        snprintf(buffer, buffer_size, "Pi angle failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 180, buffer);
    }

    /** @brief Tests the `Angle::HalfPi` constant. */
    MU_TEST(angle_test_constant_half_pi)
    {
        Angle a1 = Angle::HalfPi();
        snprintf(buffer, buffer_size, "HalfPi angle failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests the `Angle::QuarterPi` constant. */
    MU_TEST(angle_test_constant_quarter_pi)
    {
        Angle a1 = Angle::QuarterPi();
        snprintf(buffer, buffer_size, "QuarterPi angle failed: %d != 45", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 45, buffer);
    }

    /** @brief Tests the `Angle::TwoPi` constant. */
    MU_TEST(angle_test_constant_two_pi)
    {
        Angle a1 = Angle::TwoPi();
        snprintf(buffer, buffer_size, "TwoPi angle failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Tests the `Angle::Right` constant. */
    MU_TEST(angle_test_constant_right)
    {
        Angle a1 = Angle::Right();
        snprintf(buffer, buffer_size, "Right angle failed: %d != 90", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 90, buffer);
    }

    /** @brief Tests the `Angle::Straight` constant. */
    MU_TEST(angle_test_constant_straight)
    {
        Angle a1 = Angle::Straight();
        snprintf(buffer, buffer_size, "Straight angle failed: %d != 180", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 180, buffer);
    }

    /** @brief Tests the `Angle::Full` constant. */
    MU_TEST(angle_test_constant_full)
    {
        Angle a1 = Angle::Full();
        snprintf(buffer, buffer_size, "Full angle failed: %d != 0", a1.ToDegrees().As<int32_t>());
        mu_assert(a1.ToDegrees() == 0, buffer);
    }

    /** @brief Verifies that a zero-degree angle converts to a zero fixed-point value (in turns). */
    MU_TEST(angle_test_to_fxp_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp - 0.0) < 1e-4, buffer);
    }

    /** @brief Verifies that a 90-degree angle converts to a 0.25 fixed-point value. */
    MU_TEST(angle_test_to_fxp_quarter)
    {
        Angle a1 = Angle::FromDegrees(90);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0.25", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp - 0.25) < 1e-4, buffer);
    }

    /** @brief Verifies that a 180-degree angle converts to a 0.5 fixed-point value. */
    MU_TEST(angle_test_to_fxp_half)
    {
        Angle a1 = Angle::FromDegrees(180);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0.5", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp - 0.5) < 1e-4, buffer);
    }

    /** @brief Verifies that a 270-degree angle converts to a 0.75 fixed-point value. */
    MU_TEST(angle_test_to_fxp_three_quarters)
    {
        Angle a1 = Angle::FromDegrees(270);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0.75", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp - 0.75) < 1e-4, buffer);
    }

    /** @brief Verifies that a 360-degree angle converts to a 0.0 fixed-point value. */
    MU_TEST(angle_test_to_fxp_full)
    {
        Angle a1 = Angle::FromDegrees(360);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0.0", fxp.As<int32_t>());
        mu_assert(fxp == 0, buffer);
    }

    /** @brief Verifies conversion of a negative angle to a fixed-point value with wrapping. */
    MU_TEST(angle_test_to_fxp_negative_quarter)
    {
        Angle a1 = Angle::FromDegrees(-90);
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != -0.25 or 0.75", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp + 0.25) < 1e-4 || SRL::Math::Abs(fxp - 0.75) < 1e-4, buffer);
    }

    /** @brief Verifies conversion of an angle > 360 degrees to a fixed-point value with wrapping. */
    MU_TEST(angle_test_to_fxp_one_and_a_quarter)
    {
        Angle a1 = Angle::FromDegrees(450); // 450 degrees should normalize to 0.25 turns
        Fxp fxp = a1.ToFxp();
        snprintf(buffer, buffer_size, "ToFxp failed: %d != 0.25", fxp.As<int32_t>());
        mu_assert(SRL::Math::Abs(fxp - 0.25) < 1e-4, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a zero-degree angle. */
    MU_TEST(angle_test_raw_value_zero)
    {
        Angle a1 = Angle::FromDegrees(0);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0", raw);
        mu_assert(raw == 0, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a 90-degree angle. */
    MU_TEST(angle_test_raw_value_90)
    {
        Angle a1 = Angle::FromDegrees(90);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0x4000", raw);
        mu_assert(raw == 0x4000, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a 180-degree angle. */
    MU_TEST(angle_test_raw_value_180)
    {
        Angle a1 = Angle::FromDegrees(180);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0x8000", raw);
        mu_assert(raw == 0x8000, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a 270-degree angle. */
    MU_TEST(angle_test_raw_value_270)
    {
        Angle a1 = Angle::FromDegrees(270);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0xC000", raw);
        mu_assert(raw == 0xC000, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a 360-degree angle. */
    MU_TEST(angle_test_raw_value_360)
    {
        Angle a1 = Angle::FromDegrees(360);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0x0000", raw);
        mu_assert(raw == 0x0000, buffer);
    }

    /** @brief Tests getting the raw 16-bit value of a -90-degree angle. */
    MU_TEST(angle_test_raw_value_negative_90)
    {
        Angle a1 = Angle::FromDegrees(-90);
        uint16_t raw = a1.RawValue();
        snprintf(buffer, buffer_size, "RawValue failed: %d != 0xC000", raw);
        mu_assert(raw == 0xC000, buffer);
    }

    /** @brief Tests the addition operator for angles. */
    MU_TEST(angle_test_operator_addition)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(45);
        Angle result = a1 + a2;
        snprintf(buffer, buffer_size, "Addition failed: %d != 135", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 135, buffer);
    }

    /** @brief Tests the subtraction operator for angles. */
    MU_TEST(angle_test_operator_subtraction)
    {
        Angle a1 = Angle::FromDegrees(180);
        Angle a2 = Angle::FromDegrees(45);
        Angle result = a1 - a2;
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 135", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 135, buffer);
    }

    /** @brief Tests multiplying an angle by a fixed-point scalar. */
    MU_TEST(angle_test_operator_multiplication_fxp)
    {
        Angle a1 = Angle::FromDegrees(45);
        Fxp scalar = Fxp(2);
        Angle result = a1 * scalar;
        snprintf(buffer, buffer_size, "Multiplication failed: %d != 90", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 90, buffer);
    }

    /** @brief Tests multiplying an angle by an integer scalar. */
    MU_TEST(angle_test_operator_multiplication_int)
    {
        Angle a1 = Angle::FromDegrees(45);
        int scalar = 2;
        Angle result = a1 * scalar;
        snprintf(buffer, buffer_size, "Multiplication failed: %d != 90", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 90, buffer);
    }

    /** @brief Tests dividing an angle by a fixed-point scalar. */
    MU_TEST(angle_test_operator_division_fxp)
    {
        Angle a1 = Angle::FromDegrees(90);
        Fxp scalar = Fxp(2);
        Angle result = a1 / scalar;
        snprintf(buffer, buffer_size, "Division failed: %d != 45", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 45, buffer);
    }

    /** @brief Tests dividing an angle by an integer scalar. */
    MU_TEST(angle_test_operator_division_int)
    {
        Angle a1 = Angle::FromDegrees(90);
        int scalar = 2;
        Angle result = a1 / scalar;
        snprintf(buffer, buffer_size, "Division failed: %d != 45", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 45, buffer);
    }

    /** @brief Tests the equality operator for angles. */
    MU_TEST(angle_test_operator_equality)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(90);
        snprintf(buffer, buffer_size, "Equality failed: 90 != 90");
        mu_assert(a1 == a2, buffer);
    }

    /** @brief Tests the inequality operator for angles. */
    MU_TEST(angle_test_operator_inequality)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(45);
        snprintf(buffer, buffer_size, "Inequality failed: 90 == 45");
        mu_assert(a1 != a2, buffer);
    }

    /** @brief Tests the less than operator for angles. */
    MU_TEST(angle_test_operator_less_than)
    {
        Angle a1 = Angle::FromDegrees(45);
        Angle a2 = Angle::FromDegrees(90);
        snprintf(buffer, buffer_size, "Less than failed: 45 >= 90");
        mu_assert(a1 < a2, buffer);
    }

    /** @brief Tests the greater than operator for angles. */
    MU_TEST(angle_test_operator_greater_than)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(45);
        snprintf(buffer, buffer_size, "Greater than failed: 90 <= 45");
        mu_assert(a1 > a2, buffer);
    }

    /** @brief Tests the less than or equal operator for angles. */
    MU_TEST(angle_test_operator_less_than_or_equal)
    {
        Angle a1 = Angle::FromDegrees(45);
        Angle a2 = Angle::FromDegrees(90);
        Angle a3 = Angle::FromDegrees(45);
        snprintf(buffer, buffer_size, "Less than or equal failed: 45 > 90");
        mu_assert(a1 <= a2, buffer);
        snprintf(buffer, buffer_size, "Less than or equal failed: 45 != 45");
        mu_assert(a1 <= a3, buffer);
    }

    /** @brief Tests the greater than or equal operator for angles. */
    MU_TEST(angle_test_operator_greater_than_or_equal)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(45);
        Angle a3 = Angle::FromDegrees(90);
        snprintf(buffer, buffer_size, "Greater than or equal failed: 90 < 45");
        mu_assert(a1 >= a2, buffer);
        snprintf(buffer, buffer_size, "Greater than or equal failed: 90 != 90");
        mu_assert(a1 >= a3, buffer);
    }

    /** @brief Tests that addition correctly wraps around the 360-degree circle. */
    MU_TEST(angle_test_operator_addition_wrap_around)
    {
        Angle a1 = Angle::FromDegrees(350);
        Angle a2 = Angle::FromDegrees(20);
        Angle result = a1 + a2;
        Fxp degrees = result.ToDegrees();
        snprintf(buffer, buffer_size, "Addition wrap-around failed: %d != 10", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 10.0) < 1e-2, buffer);
    }

    /** @brief Tests that subtraction correctly wraps around the 360-degree circle. */
    MU_TEST(angle_test_operator_subtraction_wrap_around)
    {
        Angle a1 = Angle::FromDegrees(10);
        Angle a2 = Angle::FromDegrees(20);
        Angle result = a1 - a2;
        Fxp degrees = result.ToDegrees();
        snprintf(buffer, buffer_size, "Subtraction wrap-around failed: %d != 350", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 350.0) < 1e-2, buffer);
    }

    /** @brief Tests multiplying an angle by a large scalar to force wrapping. */
    MU_TEST(angle_test_operator_multiplication_large_scalar)
    {
        Angle a1 = Angle::FromDegrees(45);
        int scalar = 10;
        Angle result = a1 * scalar;
        snprintf(buffer, buffer_size, "Multiplication with large scalar failed: %d != 450 or 90", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 450 || result.ToDegrees() == 90, buffer);
    }

    /** @brief Tests dividing a large angle by a scalar. */
    MU_TEST(angle_test_operator_division_large_scalar)
    {
        Angle a1 = Angle::FromDegrees(450);
        int scalar = 10;
        Angle result = a1 / scalar;
        Fxp degrees = result.ToDegrees();
        snprintf(buffer, buffer_size, "Division with large scalar failed: %d != 9", degrees.As<int32_t>());
        mu_assert(SRL::Math::Abs(degrees - 9.0) < 1e-2, buffer);
    }

    /** @brief Tests adding a negative angle. */
    MU_TEST(angle_test_operator_addition_negative)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(-45);
        Angle result = a1 + a2;
        snprintf(buffer, buffer_size, "Addition with negative angle failed: %d != 45", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 45, buffer);
    }

    /** @brief Tests subtracting a negative angle. */
    MU_TEST(angle_test_operator_subtraction_negative)
    {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(-45);
        Angle result = a1 - a2;
        snprintf(buffer, buffer_size, "Subtraction with negative angle failed: %d != 135", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == 135, buffer);
    }

    /** @brief Tests multiplying an angle by a negative scalar. */
    MU_TEST(angle_test_operator_multiplication_negative_scalar)
    {
        Angle a1 = Angle::FromDegrees(45);
        int scalar = -2;
        Angle result = a1 * scalar;
        snprintf(buffer, buffer_size, "Multiplication with negative scalar failed: %d != -90 or 270", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == -90 || result.ToDegrees() == 270, buffer);
    }

    /** @brief Tests dividing an angle by a negative scalar. */
    MU_TEST(angle_test_operator_division_negative_scalar)
    {
        Angle a1 = Angle::FromDegrees(90);
        int scalar = -2;
        Angle result = a1 / scalar;
        snprintf(buffer, buffer_size, "Division with negative scalar failed: %d != -45 or 315", result.ToDegrees().As<int32_t>());
        mu_assert(result.ToDegrees() == -45 || result.ToDegrees() == 315, buffer);
    }

    /** @brief Tests the unary minus operator, which should return the opposite angle (+180 degrees). */
    MU_TEST(angle_test_operator_unary_minus_opposite)
    {
        Angle a1 = Angle::FromDegrees(0);
        Angle opposite = -a1;
        snprintf(buffer, buffer_size, "Unary minus failed: %d != 180", opposite.ToDegrees().As<int32_t>());
        mu_assert(opposite.ToDegrees() == 180, buffer);

        Angle a2 = Angle::FromDegrees(90);
        Angle opposite2 = -a2;
        uint16_t expectedRaw = static_cast<uint16_t>(a2.RawValue() + 0x8000);
        snprintf(buffer, buffer_size, "Unary minus raw failed: %u != %u", opposite2.RawValue(), expectedRaw);
        mu_assert(opposite2.RawValue() == expectedRaw, buffer);
    }

    /** @brief Tests spherical linear interpolation (SLerp) between two angles. */
    MU_TEST(angle_test_slerp_endpoints_and_midpoint)
    {
        Angle start = Angle::FromDegrees(0);
        Angle end = Angle::FromDegrees(90);

        Angle at0 = start.SLerp(end, Fxp(0));
        snprintf(buffer, buffer_size, "SLerp t=0 failed: %d != 0", at0.ToDegrees().As<int32_t>());
        mu_assert(at0.ToDegrees() == 0, buffer);

        Angle at1 = start.SLerp(end, Fxp(1));
        snprintf(buffer, buffer_size, "SLerp t=1 failed: %d != 90", at1.ToDegrees().As<int32_t>());
        mu_assert(at1.ToDegrees() == 90, buffer);

        Angle atHalf = start.SLerp(end, Fxp(0.5));
        snprintf(buffer, buffer_size, "SLerp t=0.5 failed: %d != 45", atHalf.ToDegrees().As<int32_t>());
        mu_assert(atHalf.ToDegrees() == 45, buffer);
    }

    // Define the test suite with all unit tests
    MU_TEST_SUITE(angle_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&angle_test_setup,
                                       &angle_test_teardown,
                                       &angle_test_output_header);

        MU_RUN_TEST(angle_test_initialization_zero);
        MU_RUN_TEST(angle_test_subtraction_half_circle_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_zero_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_quarter_circle_minus_zero);
        MU_RUN_TEST(angle_test_subtraction_full_circle_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_two_full_circles_minus_quarter_circle);
        MU_RUN_TEST(angle_test_subtraction_quarter_circle_minus_two_full_circles);
        MU_RUN_TEST(angle_test_addition_quarter_circle_plus_quarter_circle);
        MU_RUN_TEST(angle_test_addition_half_circle_plus_half_circle);

        // Additional tests
        MU_RUN_TEST(angle_test_normalization_positive);
        MU_RUN_TEST(angle_test_normalization_negative);
        //MU_RUN_TEST(angle_test_arithmetic_addition);          // These tests are disabled because they are hanging
        //MU_RUN_TEST(angle_test_arithmetic_subtraction);       // These tests are disabled because they are hanging
        //MU_RUN_TEST(angle_test_arithmetic_multiplication);
        //MU_RUN_TEST(angle_test_arithmetic_division);
        MU_RUN_TEST(angle_test_comparison_greater);
        MU_RUN_TEST(angle_test_comparison_less);
        MU_RUN_TEST(angle_test_conversion_to_radians);
        MU_RUN_TEST(angle_test_conversion_to_degrees);
        MU_RUN_TEST(angle_test_edge_case_zero);
        MU_RUN_TEST(angle_test_edge_case_full_circle);
        MU_RUN_TEST(angle_test_edge_case_negative);
        MU_RUN_TEST(angle_test_edge_case_greater_than_full_circle);
        MU_RUN_TEST(angle_test_edge_case_multiple_full_circles);
        MU_RUN_TEST(angle_test_edge_case_negative_multiple_full_circles);
        MU_RUN_TEST(angle_test_build_raw_zero);
        MU_RUN_TEST(angle_test_build_raw_half_pi);
        MU_RUN_TEST(angle_test_build_raw_pi);
        MU_RUN_TEST(angle_test_build_raw_three_quarters_pi);
        MU_RUN_TEST(angle_test_build_raw_full_circle);
        MU_RUN_TEST(angle_test_constant_zero);
        MU_RUN_TEST(angle_test_constant_pi);
        MU_RUN_TEST(angle_test_constant_half_pi);
        MU_RUN_TEST(angle_test_constant_quarter_pi);
        MU_RUN_TEST(angle_test_constant_two_pi);
        MU_RUN_TEST(angle_test_constant_right);
        MU_RUN_TEST(angle_test_constant_straight);
        MU_RUN_TEST(angle_test_constant_full);
        MU_RUN_TEST(angle_test_from_radians_zero);
        MU_RUN_TEST(angle_test_from_radians_pi);
        MU_RUN_TEST(angle_test_from_radians_half_pi);
        MU_RUN_TEST(angle_test_from_radians_two_pi);
        MU_RUN_TEST(angle_test_from_radians_negative_pi);
        MU_RUN_TEST(angle_test_to_radians_zero);
        MU_RUN_TEST(angle_test_to_radians_pi);
        MU_RUN_TEST(angle_test_to_radians_half_pi);
        MU_RUN_TEST(angle_test_to_radians_two_pi);
        MU_RUN_TEST(angle_test_to_radians_negative_pi);
        MU_RUN_TEST(angle_test_from_degrees_zero);
        MU_RUN_TEST(angle_test_from_degrees_90);
        MU_RUN_TEST(angle_test_from_degrees_180);
        MU_RUN_TEST(angle_test_from_degrees_270);
        MU_RUN_TEST(angle_test_from_degrees_360);
        MU_RUN_TEST(angle_test_from_degrees_negative_90);
        MU_RUN_TEST(angle_test_from_degrees_450);
        MU_RUN_TEST(angle_test_to_degrees_zero);
        MU_RUN_TEST(angle_test_to_degrees_90);
        MU_RUN_TEST(angle_test_to_degrees_180);
        MU_RUN_TEST(angle_test_to_degrees_270);
        MU_RUN_TEST(angle_test_to_degrees_360);
        MU_RUN_TEST(angle_test_to_degrees_negative_90);
        MU_RUN_TEST(angle_test_to_degrees_450);
        MU_RUN_TEST(angle_test_to_turns_zero);
        MU_RUN_TEST(angle_test_to_turns_quarter);
        MU_RUN_TEST(angle_test_to_turns_half);
        MU_RUN_TEST(angle_test_to_turns_three_quarters);
        MU_RUN_TEST(angle_test_to_turns_full_wraps_to_zero);
        MU_RUN_TEST(angle_test_to_turns_negative_quarter_wraps_to_three_quarters);
        MU_RUN_TEST(angle_test_to_turns_one_and_a_quarter_wraps_to_quarter);
        MU_RUN_TEST(angle_test_from_turns_zero);
        MU_RUN_TEST(angle_test_from_turns_quarter);
        MU_RUN_TEST(angle_test_from_turns_half);
        MU_RUN_TEST(angle_test_from_turns_three_quarters);
        MU_RUN_TEST(angle_test_from_turns_full);
        MU_RUN_TEST(angle_test_from_turns_negative_quarter);
        MU_RUN_TEST(angle_test_from_turns_one_and_a_quarter);
        MU_RUN_TEST(angle_test_to_fxp_zero);
        MU_RUN_TEST(angle_test_to_fxp_quarter);
        MU_RUN_TEST(angle_test_to_fxp_half);
        MU_RUN_TEST(angle_test_to_fxp_three_quarters);
        MU_RUN_TEST(angle_test_to_fxp_full);
        MU_RUN_TEST(angle_test_to_fxp_negative_quarter);
        MU_RUN_TEST(angle_test_to_fxp_one_and_a_quarter);
        MU_RUN_TEST(angle_test_raw_value_zero);
        MU_RUN_TEST(angle_test_raw_value_90);
        MU_RUN_TEST(angle_test_raw_value_180);
        MU_RUN_TEST(angle_test_raw_value_270);
        MU_RUN_TEST(angle_test_raw_value_360);
        MU_RUN_TEST(angle_test_raw_value_negative_90);
        MU_RUN_TEST(angle_test_operator_addition);
        MU_RUN_TEST(angle_test_operator_subtraction);
        MU_RUN_TEST(angle_test_operator_multiplication_fxp);
        MU_RUN_TEST(angle_test_operator_multiplication_int);
        MU_RUN_TEST(angle_test_operator_division_fxp);
        MU_RUN_TEST(angle_test_operator_division_int);
        MU_RUN_TEST(angle_test_operator_equality);
        MU_RUN_TEST(angle_test_operator_inequality);
        MU_RUN_TEST(angle_test_operator_less_than);
        MU_RUN_TEST(angle_test_operator_greater_than);
        MU_RUN_TEST(angle_test_operator_less_than_or_equal);
        MU_RUN_TEST(angle_test_operator_greater_than_or_equal);

        MU_RUN_TEST(angle_test_operator_addition_wrap_around);
        MU_RUN_TEST(angle_test_operator_subtraction_wrap_around);
        MU_RUN_TEST(angle_test_operator_multiplication_large_scalar);
        MU_RUN_TEST(angle_test_operator_division_large_scalar);
        MU_RUN_TEST(angle_test_operator_addition_negative);
        MU_RUN_TEST(angle_test_operator_subtraction_negative);
        MU_RUN_TEST(angle_test_operator_multiplication_negative_scalar);
        MU_RUN_TEST(angle_test_operator_division_negative_scalar);
        MU_RUN_TEST(angle_test_operator_unary_minus_opposite);
        MU_RUN_TEST(angle_test_slerp_endpoints_and_midpoint);
    }
}

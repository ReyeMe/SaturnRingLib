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
    void trigonometry_test_setup(void)
    {
        // No initialization needed
    }

    void trigonometry_test_teardown(void)
    {
        // No cleanup required
    }

    static inline bool fxp_near_trig(const Fxp& a, const Fxp& b, const Fxp& tol)
    {
        return (a - b).Abs() <= tol;
    }

    void trigonometry_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_TRIGONOMETRY****");
            }
            else
            {
                LogInfo("****UT_TRIGONOMETRY_ERROR(S)****");
            }
        }
    }

    MU_TEST(trigonometry_sin_cos_key_angles)
    {
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(0)) == Fxp(0), "Sin(0) should be 0");
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(90)) == Fxp(1), "Sin(90) should be 1");
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(180)) == Fxp(0), "Sin(180) should be 0");
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(270)) == Fxp(-1), "Sin(270) should be -1");
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(360)) == Fxp(0), "Sin(360) should be 0");

        const Fxp sin30 = SRL::Math::Trigonometry::Sin(Angle::FromDegrees(30));
        mu_assert(fxp_near_trig(sin30, Fxp(0.5), Fxp(0.02)), "Sin(30) should be approx 0.5");

        const Fxp sin45 = SRL::Math::Trigonometry::Sin(Angle::FromDegrees(45));
        mu_assert(fxp_near_trig(sin45, Fxp(0.7071), Fxp(0.02)), "Sin(45) should be approx 0.7071");

        mu_assert(SRL::Math::Trigonometry::Cos(Angle::FromDegrees(0)) == Fxp(1), "Cos(0) should be 1");
        mu_assert(SRL::Math::Trigonometry::Cos(Angle::FromDegrees(90)) == Fxp(0), "Cos(90) should be 0");
        mu_assert(SRL::Math::Trigonometry::Cos(Angle::FromDegrees(180)) == Fxp(-1), "Cos(180) should be -1");
        mu_assert(SRL::Math::Trigonometry::Cos(Angle::FromDegrees(360)) == Fxp(1), "Cos(360) should be 1");

        const Fxp cos30 = SRL::Math::Trigonometry::Cos(Angle::FromDegrees(30));
        mu_assert(fxp_near_trig(cos30, Fxp(0.8660), Fxp(0.02)), "Cos(30) should be approx 0.866");

        const Fxp cos45 = SRL::Math::Trigonometry::Cos(Angle::FromDegrees(45));
        mu_assert(fxp_near_trig(cos45, Fxp(0.7071), Fxp(0.02)), "Cos(45) should be approx 0.7071");

        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(-90)) == Fxp(-1), "Sin(-90) should be -1");

        // Periodicity / wrap-around
        mu_assert(SRL::Math::Trigonometry::Sin(Angle::FromDegrees(450)) == Fxp(1), "Sin(450) should equal Sin(90)");
        mu_assert(SRL::Math::Trigonometry::Cos(Angle::FromDegrees(-360)) == Fxp(1), "Cos(-360) should equal Cos(0)");
    }

    MU_TEST(trigonometry_tan_basic)
    {
        mu_assert(SRL::Math::Trigonometry::Tan(Angle::FromDegrees(0)) == Fxp(0), "Tan(0) should be 0");

        const Fxp tan45 = SRL::Math::Trigonometry::Tan(Angle::FromDegrees(45));
        mu_assert(fxp_near_trig(tan45, Fxp(1), Fxp(0.05)), "Tan(45) should be approx 1");

        const Fxp tanNeg45 = SRL::Math::Trigonometry::Tan(Angle::FromDegrees(-45));
        mu_assert(fxp_near_trig(tanNeg45, Fxp(-1), Fxp(0.05)), "Tan(-45) should be approx -1");
    }

    MU_TEST(trigonometry_tan_near_asymptote)
    {
        const Fxp tan89 = SRL::Math::Trigonometry::Tan(Angle::FromDegrees(89));
        mu_assert(tan89 > Fxp(10), "Tan(89) should be large positive");

        const Fxp tan91 = SRL::Math::Trigonometry::Tan(Angle::FromDegrees(91));
        mu_assert(tan91 < Fxp(-10), "Tan(91) should be large negative");

        // Implementation-defined handling at exactly 90 degrees (typically saturates)
        const Fxp tan90 = SRL::Math::Trigonometry::Tan(Angle::FromDegrees(90));
        mu_assert(tan90.Abs() > Fxp(100), "Tan(90) should be very large magnitude (saturated)");
    }

    MU_TEST(trigonometry_atan2_key_directions)
    {
        mu_assert(SRL::Math::Trigonometry::Atan2(Fxp(0), Fxp(0)) == Angle::Zero(), "Atan2(0,0) should be 0");

        mu_assert(SRL::Math::Trigonometry::Atan2(Fxp(0), Fxp(1)) == Angle::Zero(), "Atan2(0,1) should be 0");
        mu_assert(SRL::Math::Trigonometry::Atan2(Fxp(1), Fxp(0)) == Angle::HalfPi(), "Atan2(1,0) should be 90");
        mu_assert(SRL::Math::Trigonometry::Atan2(Fxp(0), Fxp(-1)) == Angle::Pi(), "Atan2(0,-1) should be 180");
        mu_assert(SRL::Math::Trigonometry::Atan2(Fxp(-1), Fxp(0)) == Angle::ThreeQuarterPi(), "Atan2(-1,0) should be 270");

        const Angle a45 = SRL::Math::Trigonometry::Atan2(Fxp(1), Fxp(1));
        mu_assert(a45 >= Angle::FromDegrees(44.9) && a45 <= Angle::FromDegrees(45.1), "Atan2(1,1) should be ~45");

        const Angle a315 = SRL::Math::Trigonometry::Atan2(Fxp(-1), Fxp(1));
        mu_assert(a315 >= Angle::FromDegrees(314.9) && a315 <= Angle::FromDegrees(315.1), "Atan2(-1,1) should be ~315");

        const Angle a135 = SRL::Math::Trigonometry::Atan2(Fxp(1), Fxp(-1));
        mu_assert(a135 >= Angle::FromDegrees(134.9) && a135 <= Angle::FromDegrees(135.1), "Atan2(1,-1) should be ~135");

        const Angle a225 = SRL::Math::Trigonometry::Atan2(Fxp(-1), Fxp(-1));
        mu_assert(a225 >= Angle::FromDegrees(224.9) && a225 <= Angle::FromDegrees(225.1), "Atan2(-1,-1) should be ~225");
    }

    MU_TEST(trigonometry_asin_clamp_and_values)
    {
        mu_assert(SRL::Math::Trigonometry::Asin(Fxp(0)) == Angle::Zero(), "Asin(0) should be 0");
        mu_assert(SRL::Math::Trigonometry::Asin(Fxp(1)) == Angle::HalfPi(), "Asin(1) should be 90");
        mu_assert(SRL::Math::Trigonometry::Asin(Fxp(-1)) == Angle::ThreeQuarterPi(), "Asin(-1) should be -90 (270)");

        // Clamp behavior outside domain
        mu_assert(SRL::Math::Trigonometry::Asin(Fxp(2)) == Angle::HalfPi(), "Asin(>1) should clamp to 90");
        mu_assert(SRL::Math::Trigonometry::Asin(Fxp(-2)) == Angle::ThreeQuarterPi(), "Asin(<-1) should clamp to -90 (270)");

        const Angle a30 = SRL::Math::Trigonometry::Asin(Fxp(0.5));
        mu_assert(a30 >= Angle::FromDegrees(29.0) && a30 <= Angle::FromDegrees(31.0), "Asin(0.5) should be ~30 degrees");
    }

    MU_TEST_SUITE(trigonometry_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&trigonometry_test_setup,
                                       &trigonometry_test_teardown,
                                       &trigonometry_test_output_header);

        MU_RUN_TEST(trigonometry_sin_cos_key_angles);
        MU_RUN_TEST(trigonometry_tan_basic);
        MU_RUN_TEST(trigonometry_tan_near_asymptote);
        MU_RUN_TEST(trigonometry_atan2_key_directions);
        MU_RUN_TEST(trigonometry_asin_clamp_and_values);
    }
}

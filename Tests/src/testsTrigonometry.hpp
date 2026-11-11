#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {
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

    // Disabled: Asin() is a static_assert "work in progress" in the pinned SaturnMathPP, so it cannot be called yet.
    // MU_TEST(trigonometry_asin_clamp_and_values)
    // {
    //     mu_assert(SRL::Math::Trigonometry::Asin(Fxp(0)) == Angle::Zero(), "Asin(0) should be 0");
    //     mu_assert(SRL::Math::Trigonometry::Asin(Fxp(1)) == Angle::HalfPi(), "Asin(1) should be 90");
    //     mu_assert(SRL::Math::Trigonometry::Asin(Fxp(-1)) == Angle::ThreeQuarterPi(), "Asin(-1) should be -90 (270)");

    //     // Clamp behavior outside domain
    //     mu_assert(SRL::Math::Trigonometry::Asin(Fxp(2)) == Angle::HalfPi(), "Asin(>1) should clamp to 90");
    //     mu_assert(SRL::Math::Trigonometry::Asin(Fxp(-2)) == Angle::ThreeQuarterPi(), "Asin(<-1) should clamp to -90 (270)");

    //     const Angle a30 = SRL::Math::Trigonometry::Asin(Fxp(0.5));
    //     mu_assert(a30 >= Angle::FromDegrees(29.0) && a30 <= Angle::FromDegrees(31.0), "Asin(0.5) should be ~30 degrees");
    // }

using Trig = SRL::Math::Trigonometry;

/** @brief True if a is within tol raw units (1/65536) of b. */
static bool trig_raw_near(const Fxp& a, const Fxp& b, int32_t tol)
{
    const int32_t diff = a.RawValue() - b.RawValue();
    return diff >= -tol && diff <= tol;
}

/** @brief True if two angles are within tol raw units (1/65536 turn), across the 0/360 wrap. */
static bool trig_angle_near(const Angle& a, const Angle& b, int16_t tol)
{
    const int16_t diff = static_cast<int16_t>(a.RawValue() - b.RawValue());
    return diff >= -tol && diff <= tol;
}

    /**
 * @brief Sin against reference values every 15 degrees of the first quadrant (other quadrants follow from symmetry tests).
 * @details Sin interpolates linearly in a 64-entry table (one entry every 5.6 degrees), so its design error is up to ~0.0012.
 */
MU_TEST(trigonometry_sin_reference_values)
{
    const int16_t degrees[] = {0, 15, 30, 45, 60, 75, 90};
    const Fxp expected[] = {Fxp(0.0), Fxp(0.258819), Fxp(0.5), Fxp(0.707107), Fxp(0.866025), Fxp(0.965926), Fxp(1.0)};
    for (size_t i = 0; i < sizeof(degrees) / sizeof(degrees[0]); i++)
    {
        const Fxp s = Trig::Sin(Angle::FromDegrees(Fxp(degrees[i])));
        snprintf(buffer, buffer_size, "Sin(%d) raw %ld vs %ld", degrees[i], (long)s.RawValue(), (long)expected[i].RawValue());
        mu_assert(trig_raw_near(s, expected[i], 98), buffer);     // within 0.0015 (table interpolation error)
    }
}

    /**
 * @brief sin^2 + cos^2 == 1 every 7.5 degrees over a full turn.
 */
MU_TEST(trigonometry_sin_cos_identity_sweep)
{
    for (int16_t tenths = 0; tenths < 3600; tenths += 75)
    {
        const Angle a = Angle::FromDegrees(Fxp(tenths) / 10);
        const Fxp s = Trig::Sin(a);
        const Fxp c = Trig::Cos(a);
        const Fxp sum = s * s + c * c;
        snprintf(buffer, buffer_size, "sin^2+cos^2 at %d.%d deg raw %ld != 1", tenths / 10, tenths % 10, (long)sum.RawValue());
        mu_assert(trig_raw_near(sum, Fxp(1), 196), buffer);     // within 0.003 (twice the table interpolation error)
    }
}

    /**
 * @brief Symmetries every 10 degrees: sin(-x) = -sin(x), cos(-x) = cos(x), sin(x+180) = -sin(x), cos(x) = sin(x+90).
 */
MU_TEST(trigonometry_sin_cos_symmetry)
{
    for (int16_t d = 0; d < 360; d += 10)
    {
        const Angle a = Angle::FromDegrees(Fxp(d));
        const Angle neg = Angle::FromDegrees(Fxp(static_cast<int16_t>(-d)));
        const Angle plus180 = Angle::FromDegrees(Fxp(static_cast<int16_t>(d + 180)));
        const Angle plus90 = Angle::FromDegrees(Fxp(static_cast<int16_t>(d + 90)));

        snprintf(buffer, buffer_size, "sin(-%d) raw %ld != -sin(%d) raw %ld", d, (long)Trig::Sin(neg).RawValue(), d, (long)(-Trig::Sin(a)).RawValue());
        mu_assert(trig_raw_near(Trig::Sin(neg), -Trig::Sin(a), 4), buffer);
        snprintf(buffer, buffer_size, "cos(-%d) raw %ld != cos(%d) raw %ld", d, (long)Trig::Cos(neg).RawValue(), d, (long)Trig::Cos(a).RawValue());
        mu_assert(trig_raw_near(Trig::Cos(neg), Trig::Cos(a), 4), buffer);
        snprintf(buffer, buffer_size, "sin(%d+180) raw %ld != -sin(%d) raw %ld", d, (long)Trig::Sin(plus180).RawValue(), d, (long)(-Trig::Sin(a)).RawValue());
        mu_assert(trig_raw_near(Trig::Sin(plus180), -Trig::Sin(a), 4), buffer);
        snprintf(buffer, buffer_size, "cos(%d) raw %ld != sin(%d+90) raw %ld", d, (long)Trig::Cos(a).RawValue(), d, (long)Trig::Sin(plus90).RawValue());
        mu_assert(trig_raw_near(Trig::Cos(a), Trig::Sin(plus90), 4), buffer);
    }
}

    /**
 * @brief Tan matches sin/cos away from the asymptotes, and repeats every 180 degrees.
 */
MU_TEST(trigonometry_tan_matches_sin_over_cos)
{
        // Collect every failing angle so one run shows the whole pattern
    char ratioFails[96] = "";
    char periodFails[96] = "";
    size_t rp = 0, pp = 0;
    for (int16_t d = -80; d <= 80; d += 10)
    {
        const Angle a = Angle::FromDegrees(Fxp(d));
        const Fxp t = Trig::Tan(a);
        const Fxp ratio = Trig::Sin(a) / Trig::Cos(a);
        const int32_t tol = 66 + static_cast<int32_t>((static_cast<int64_t>(ratio.Abs().RawValue()) * 5) / 1000);  // 0.001 + 0.5%
        if (!trig_raw_near(t, ratio, tol) && rp < sizeof(ratioFails) - 6)
        {
            rp += snprintf(ratioFails + rp, sizeof(ratioFails) - rp, " %d", d);
        }
        const Fxp t180 = Trig::Tan(Angle::FromDegrees(Fxp(static_cast<int16_t>(d + 180))));
        if (!trig_raw_near(t180, t, 4) && pp < sizeof(periodFails) - 6)
        {
            pp += snprintf(periodFails + pp, sizeof(periodFails) - pp, " %d", d);
        }
    }
    snprintf(buffer, buffer_size, "Tan != sin/cos at:%s | Tan(x+180) != Tan(x) at:%s", ratioFails, periodFails);
    mu_assert(rp == 0 && pp == 0, buffer);
}

    /**
 * @brief Atan2(sin a, cos a) returns a every 10 degrees, and only the direction of (x, y) matters, not its length.
 */
MU_TEST(trigonometry_atan2_roundtrip_and_scale)
{
    for (int16_t d = 0; d < 360; d += 10)
    {
        const Angle a = Angle::FromDegrees(Fxp(d));
        const Angle back = Trig::Atan2(Trig::Sin(a), Trig::Cos(a));
        snprintf(buffer, buffer_size, "Atan2(sin,cos) of %d deg gave raw %u, expected %u", d, back.RawValue(), a.RawValue());
        mu_assert(trig_angle_near(back, a, 19), buffer);     // within 0.1 degree
    }

    const Angle ref = Trig::Atan2(Fxp(3), Fxp(4));
    const Fxp scales[] = {Fxp(0.01), Fxp(0.5), Fxp(10), Fxp(1000)};
    for (const Fxp& k : scales)
    {
        const Angle scaled = Trig::Atan2(Fxp(3) * k, Fxp(4) * k);
        snprintf(buffer, buffer_size, "Atan2(3k,4k) k raw %ld gave raw %u, Atan2(3,4) raw %u", (long)k.RawValue(), scaled.RawValue(), ref.RawValue());
        mu_assert(trig_angle_near(scaled, ref, 19), buffer);
    }
}

    /**
 * @brief SLerp: endpoints, midpoint, and the documented shortest path around the circle in both directions.
 */
MU_TEST(trigonometry_slerp)
{
    const Angle a0 = Angle::FromDegrees(Fxp(0));
    const Angle a90 = Angle::FromDegrees(Fxp(90));
    const Angle a10 = Angle::FromDegrees(Fxp(10));
    const Angle a350 = Angle::FromDegrees(Fxp(350));

    mu_assert(trig_angle_near(Trig::SLerp(a0, a90, Fxp(0)), a0, 1), "SLerp t=0 should return start");
    mu_assert(trig_angle_near(Trig::SLerp(a0, a90, Fxp(1)), a90, 1), "SLerp t=1 should return end");

    const Angle mid = Trig::SLerp(a0, a90, Fxp(0.5));
    snprintf(buffer, buffer_size, "SLerp(0,90,0.5) raw %u != 45 deg", mid.RawValue());
    mu_assert(trig_angle_near(mid, Angle::FromDegrees(Fxp(45)), 2), buffer);

    const Angle wrapUp = Trig::SLerp(a350, a10, Fxp(0.5));
    snprintf(buffer, buffer_size, "SLerp(350,10,0.5) raw %u != 0 (shortest path)", wrapUp.RawValue());
    mu_assert(trig_angle_near(wrapUp, a0, 2), buffer);

    const Angle wrapDown = Trig::SLerp(a10, a350, Fxp(0.5));
    snprintf(buffer, buffer_size, "SLerp(10,350,0.5) raw %u != 0 (shortest path)", wrapDown.RawValue());
    mu_assert(trig_angle_near(wrapDown, a0, 2), buffer);
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
    MU_RUN_TEST(trigonometry_sin_reference_values);
    MU_RUN_TEST(trigonometry_sin_cos_identity_sweep);
    MU_RUN_TEST(trigonometry_sin_cos_symmetry);
    MU_RUN_TEST(trigonometry_tan_matches_sin_over_cos);
    MU_RUN_TEST(trigonometry_atan2_roundtrip_and_scale);
    MU_RUN_TEST(trigonometry_slerp);
    // MU_RUN_TEST(trigonometry_asin_clamp_and_values); // Asin() not ready in the pinned SaturnMathPP
}
}

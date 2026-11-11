#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

using Interp = SRL::Math::Interpolation;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

void interpolation_test_setup(void) {}
void interpolation_test_teardown(void) {}

void interpolation_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_INTERPOLATION****");
        }
        else
        {
            LogInfo("****UT_INTERPOLATION_ERROR(S)****");
        }
    }
}

/** @brief True if a is within tol raw units (1/65536) of b. */
static bool interp_near(const Fxp& a, const Fxp& b, int32_t tol)
{
    const int32_t diff = a.RawValue() - b.RawValue();
    return diff >= -tol && diff <= tol;
}

/** @brief Checks f(start, end, t) against an expected value within tol raw units; prints raw values on failure. */
#define INTERP_ASSERT(expr, expected, tol, what)                                                        \
    do                                                                                                  \
    {                                                                                                   \
        const Fxp interpGot = (expr);                                                                   \
        snprintf(buffer, buffer_size, "%s: raw %ld != %ld", what, (long)interpGot.RawValue(),           \
            (long)Fxp(expected).RawValue());                                                            \
        mu_assert(interp_near(interpGot, Fxp(expected), tol), buffer);                                  \
    } while (0)

    /**
 * @brief Lerp: start at t=0, end at t=1, midpoint at t=0.5, also for decreasing and negative ranges.
 */
MU_TEST(interpolation_lerp)
{
    INTERP_ASSERT(Interp::Lerp(10, 20, 0), 10, 0, "Lerp(10,20,0)");
    INTERP_ASSERT(Interp::Lerp(10, 20, 1), 20, 0, "Lerp(10,20,1)");
    INTERP_ASSERT(Interp::Lerp(10, 20, Fxp(0.5)), 15, 0, "Lerp(10,20,0.5)");
    INTERP_ASSERT(Interp::Lerp(10, 20, Fxp(0.25)), Fxp(12.5), 0, "Lerp(10,20,0.25)");
    INTERP_ASSERT(Interp::Lerp(20, 10, Fxp(0.25)), Fxp(17.5), 0, "Lerp(20,10,0.25) decreasing");
    INTERP_ASSERT(Interp::Lerp(-8, 8, Fxp(0.5)), 0, 0, "Lerp(-8,8,0.5) across zero");
    INTERP_ASSERT(Interp::Lerp(5, 5, Fxp(0.7)), 5, 0, "Lerp(5,5,t) equal endpoints");
}

    /**
 * @brief Smoothstep: exact endpoints, midpoint at t=0.5, symmetric, and t clamped to [0,1] (documented).
 */
MU_TEST(interpolation_smoothstep)
{
    INTERP_ASSERT(Interp::Smoothstep(10, 20, 0), 10, 0, "Smoothstep(10,20,0)");
    INTERP_ASSERT(Interp::Smoothstep(10, 20, 1), 20, 0, "Smoothstep(10,20,1)");
    INTERP_ASSERT(Interp::Smoothstep(10, 20, Fxp(0.5)), 15, 0, "Smoothstep(10,20,0.5)");
        // 3t^2 - 2t^3 at t=0.25 is 0.15625
    INTERP_ASSERT(Interp::Smoothstep(0, 1, Fxp(0.25)), Fxp(0.15625), 1, "Smoothstep(0,1,0.25)");
        // Symmetric around the midpoint: f(0.25) + f(0.75) == 1
    INTERP_ASSERT(Interp::Smoothstep(0, 1, Fxp(0.25)) + Interp::Smoothstep(0, 1, Fxp(0.75)), 1, 2, "Smoothstep symmetry");

    INTERP_ASSERT(Interp::Smoothstep(10, 20, -1), 10, 0, "Smoothstep clamps t<0 to start");
    INTERP_ASSERT(Interp::Smoothstep(10, 20, 2), 20, 0, "Smoothstep clamps t>1 to end");
}

    /**
 * @brief Clamp: below, above, inside and on the bounds.
 */
MU_TEST(interpolation_clamp)
{
    INTERP_ASSERT(Interp::Clamp(-5, 0, 10), 0, 0, "Clamp below min");
    INTERP_ASSERT(Interp::Clamp(15, 0, 10), 10, 0, "Clamp above max");
    INTERP_ASSERT(Interp::Clamp(Fxp(3.5), 0, 10), Fxp(3.5), 0, "Clamp inside range");
    INTERP_ASSERT(Interp::Clamp(0, 0, 10), 0, 0, "Clamp at min");
    INTERP_ASSERT(Interp::Clamp(10, 0, 10), 10, 0, "Clamp at max");
    INTERP_ASSERT(Interp::Clamp(Fxp(-0.5), Fxp(-1), Fxp(-0.25)), Fxp(-0.5), 0, "Clamp negative range");
}

    /**
 * @brief Quadratic ease in/out: endpoints, documented curve values, and EaseOut(t) == 1 - EaseIn(1 - t).
 */
MU_TEST(interpolation_ease_in_out_quadratic)
{
    INTERP_ASSERT(Interp::EaseIn(10, 20, 0), 10, 0, "EaseIn t=0");
    INTERP_ASSERT(Interp::EaseIn(10, 20, 1), 20, 0, "EaseIn t=1");
    INTERP_ASSERT(Interp::EaseIn(0, 1, Fxp(0.5)), Fxp(0.25), 0, "EaseIn(0.5) = t^2");

    INTERP_ASSERT(Interp::EaseOut(10, 20, 0), 10, 0, "EaseOut t=0");
    INTERP_ASSERT(Interp::EaseOut(10, 20, 1), 20, 0, "EaseOut t=1");
    INTERP_ASSERT(Interp::EaseOut(0, 1, Fxp(0.5)), Fxp(0.75), 0, "EaseOut(0.5) = -t(t-2)");

    const Fxp ts[] = {Fxp(0.1), Fxp(0.3), Fxp(0.6), Fxp(0.9)};
    for (const Fxp& t : ts)
    {
        INTERP_ASSERT(Interp::EaseOut(0, 1, t), Fxp(1) - Interp::EaseIn(0, 1, Fxp(1) - t), 2, "EaseOut(t) == 1 - EaseIn(1-t)");
    }
}

    /**
 * @brief Cubic ease in/out: endpoints, documented curve values, and CubicEaseOut(t) == 1 - CubicEaseIn(1 - t).
 */
MU_TEST(interpolation_ease_in_out_cubic)
{
    INTERP_ASSERT(Interp::CubicEaseIn(10, 20, 0), 10, 0, "CubicEaseIn t=0");
    INTERP_ASSERT(Interp::CubicEaseIn(10, 20, 1), 20, 0, "CubicEaseIn t=1");
    INTERP_ASSERT(Interp::CubicEaseIn(0, 1, Fxp(0.5)), Fxp(0.125), 0, "CubicEaseIn(0.5) = t^3");

    INTERP_ASSERT(Interp::CubicEaseOut(10, 20, 0), 10, 0, "CubicEaseOut t=0");
    INTERP_ASSERT(Interp::CubicEaseOut(10, 20, 1), 20, 0, "CubicEaseOut t=1");
    INTERP_ASSERT(Interp::CubicEaseOut(0, 1, Fxp(0.5)), Fxp(0.875), 0, "CubicEaseOut(0.5) = (t-1)^3 + 1");

    const Fxp ts[] = {Fxp(0.1), Fxp(0.3), Fxp(0.6), Fxp(0.9)};
    for (const Fxp& t : ts)
    {
        INTERP_ASSERT(Interp::CubicEaseOut(0, 1, t), Fxp(1) - Interp::CubicEaseIn(0, 1, Fxp(1) - t), 2, "CubicEaseOut(t) == 1 - CubicEaseIn(1-t)");
    }
}

    /**
 * @brief Bounce ease out: endpoints, touches the end at each documented bounce (4/11, 8/11, 10/11), stays in [0,1].
 */
MU_TEST(interpolation_bounce_ease_out)
{
    INTERP_ASSERT(Interp::BounceEaseOut(10, 20, 0), 10, 0, "BounceEaseOut t=0");
        // Branch constants are rounded decimals, so allow a few raw units
    INTERP_ASSERT(Interp::BounceEaseOut(0, 1, 1), 1, 8, "BounceEaseOut t=1");
    INTERP_ASSERT(Interp::BounceEaseOut(0, 1, Fxp(0.36363636)), 1, 8, "BounceEaseOut touches end at 4/11");
    INTERP_ASSERT(Interp::BounceEaseOut(0, 1, Fxp(0.72727272)), 1, 8, "BounceEaseOut touches end at 8/11");
    INTERP_ASSERT(Interp::BounceEaseOut(0, 1, Fxp(0.90909090)), 1, 8, "BounceEaseOut touches end at 10/11");

    for (int16_t i = 0; i <= 100; i++)
    {
        const Fxp f = Interp::BounceEaseOut(0, 1, Fxp(i) / 100);
        snprintf(buffer, buffer_size, "BounceEaseOut(%d/100) raw %ld outside [0,1]", i, (long)f.RawValue());
        mu_assert(f.RawValue() >= -8 && f.RawValue() <= 65536 + 8, buffer);
    }
}

    /**
 * @brief Bounce ease in: endpoints, and BounceEaseIn(t) == 1 - BounceEaseOut(1 - t) (documented as its reverse).
 */
MU_TEST(interpolation_bounce_ease_in)
{
    INTERP_ASSERT(Interp::BounceEaseIn(10, 20, 0), 10, 80, "BounceEaseIn t=0");
    INTERP_ASSERT(Interp::BounceEaseIn(10, 20, 1), 20, 0, "BounceEaseIn t=1");

    const Fxp ts[] = {Fxp(0.1), Fxp(0.3), Fxp(0.6), Fxp(0.9)};
    for (const Fxp& t : ts)
    {
        INTERP_ASSERT(Interp::BounceEaseIn(0, 1, t), Fxp(1) - Interp::BounceEaseOut(0, 1, Fxp(1) - t), 2, "BounceEaseIn(t) == 1 - BounceEaseOut(1-t)");
    }
}

    /**
 * @brief Elastic ease in: exact endpoints (explicit early returns), and close to start/end near t=0 and t=1.
 * @details A standard elastic ease-in stays near the start for small t, overshoots a little, and snaps to the end.
 */
MU_TEST(interpolation_elastic_ease_in)
{
    INTERP_ASSERT(Interp::ElasticEaseIn(10, 20, 0), 10, 0, "ElasticEaseIn t=0");
    INTERP_ASSERT(Interp::ElasticEaseIn(10, 20, 1), 20, 0, "ElasticEaseIn t=1");
    INTERP_ASSERT(Interp::ElasticEaseIn(10, 20, -1), 10, 0, "ElasticEaseIn t<0 returns start");
    INTERP_ASSERT(Interp::ElasticEaseIn(10, 20, 2), 20, 0, "ElasticEaseIn t>1 returns end");

        // Near the ends the curve should approach them (within 20% of the range)
    INTERP_ASSERT(Interp::ElasticEaseIn(0, 1, Fxp(0.01)), 0, 13107, "ElasticEaseIn(0.01) near start");
    INTERP_ASSERT(Interp::ElasticEaseIn(0, 1, Fxp(0.99)), 1, 13107, "ElasticEaseIn(0.99) near end");

        // Overshoots stay modest: within [-0.5, 1.5] of the range
    for (int16_t i = 1; i < 100; i += 7)
    {
        const Fxp f = Interp::ElasticEaseIn(0, 1, Fxp(i) / 100);
        snprintf(buffer, buffer_size, "ElasticEaseIn(%d/100) raw %ld outside [-0.5,1.5]", i, (long)f.RawValue());
        mu_assert(f >= Fxp(-0.5) && f <= Fxp(1.5), buffer);
    }
}

MU_TEST_SUITE(interpolation_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&interpolation_test_setup,
        &interpolation_test_teardown,
        &interpolation_test_output_header);

    MU_RUN_TEST(interpolation_lerp);
    MU_RUN_TEST(interpolation_smoothstep);
    MU_RUN_TEST(interpolation_clamp);
    MU_RUN_TEST(interpolation_ease_in_out_quadratic);
    MU_RUN_TEST(interpolation_ease_in_out_cubic);
    MU_RUN_TEST(interpolation_bounce_ease_out);
    MU_RUN_TEST(interpolation_bounce_ease_in);
    MU_RUN_TEST(interpolation_elastic_ease_in);
}
}

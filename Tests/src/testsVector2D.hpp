#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

#include <utility>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

    /**
 * @brief Tests construction of Vector2D objects.
 * @details Verifies default, uniform, component, and copy constructors.
 */
MU_TEST(vector2d_construction)
{
    const Vector2D a;
    mu_assert(a == Vector2D::Zero(), "Default Vector2D should be zero");

    const Vector2D b(Fxp(5));
    mu_assert(b == Vector2D(5, 5), "Uniform ctor should set both components");

    const Vector2D c(Fxp(1), Fxp(2));
    mu_assert(c == Vector2D(1, 2), "Component ctor should set components");

    const Vector2D d(c);
    mu_assert(d == c, "Copy ctor should copy");
}

    /**
 * @brief Tests abs and sort operations for Vector2D.
 * @details Checks component-wise abs and ascending/descending sort.
 */
MU_TEST(vector2d_abs_and_sort)
{
    const Vector2D v(-3, 2);
    mu_assert(v.Abs() == Vector2D(3, 2), "Abs should return component-wise abs");

    const Vector2D s(3, 1);
    mu_assert(s.Sort<SRL::Math::SortOrder::Ascending>() == Vector2D(1, 3), "Sort ascending failed");
    mu_assert(s.Sort<SRL::Math::SortOrder::Descending>() == Vector2D(3, 1), "Sort descending failed");
}

    /**
 * @brief Tests dot, cross, and multidot operations for Vector2D.
 * @details Verifies dot product, cross product, and multidot accumulation.
 */
MU_TEST(vector2d_dot_cross_multidot)
{
    const Vector2D a(3, 4);
    const Vector2D b(1, 2);

    mu_assert(a.Dot(b) == Fxp(11), "Dot product incorrect");
    mu_assert(a.Cross(b) == Fxp(2), "2D cross product incorrect");
    mu_assert(b.Cross(a) == Fxp(-2), "2D cross product sign incorrect");

    const Vector2D c(0, 1);
    const Vector2D d(1, 0);
    const Fxp sum = Vector2D::MultiDotAccumulate(std::pair{a, b}, std::pair{c, d});
    mu_assert(sum == Fxp(11), "MultiDotAccumulate incorrect");
}

/**
 * @brief True if a length is within 6.1% of the exact value.
 * @details Length() and DistanceTo() take the square root with Fxp::InternalSqrtFrom64, the same nearest-power-of-2 plus
 *          straight-line estimate as Fxp::Sqrt() (see fxp_sqrt), not the exact square root their comments describe.
 * @param got Length returned by SaturnMathPP
 * @param exact Exact length
 * @return true if got is within 6.1% of exact
 */
static bool Vector2DLengthNear(const Fxp& got, const Fxp& exact)
{
    const int64_t diff = static_cast<int64_t>(got.RawValue()) - exact.RawValue();
    return (diff < 0 ? -diff : diff) * 1000 <= static_cast<int64_t>(exact.RawValue()) * 61;
}

    /**
 * @brief Tests length and length squared calculations for Vector2D.
 * @details Checks behavior for overflow guard and threshold values.
 */
MU_TEST(vector2d_length_lengthsquared_overflow_guard)
{
    const Vector2D v(3, 4);
    snprintf(buffer, buffer_size, "Length of (3,4) raw %ld not within 6.1%% of 5", (long)v.Length().RawValue());
    mu_assert(Vector2DLengthNear(v.Length(), Fxp(5)), buffer);
    mu_assert(v.LengthSquared() == Fxp(25), "LengthSquared for (3,4) should be 25");

        // Threshold: a component at or above MaxSafeSquareValue (sqrt(32768), about 181.02) makes LengthSquared return MaxValue
    const Vector2D below(181, 0);
    snprintf(buffer, buffer_size, "LengthSquared of (181,0) raw %ld != 32761 (181 is below the threshold)", (long)below.LengthSquared().RawValue());
    mu_assert(below.LengthSquared() == Fxp(32761), buffer);
    const Vector2D above(182, 0);
    mu_assert(above.LengthSquared() == Fxp::MaxValue(), "LengthSquared of (182,0) should return MaxValue (182 is above the threshold)");

    const Vector2D minv(Fxp::MinValue(), 0);
    mu_assert(minv.LengthSquared() == Fxp::MaxValue(), "LengthSquared should return MaxValue for MinValue component");
}

    /**
 * @brief Tests normalization for Vector2D.
 * @details Verifies normalization of zero and nonzero vectors, and unit length.
 */
MU_TEST(vector2d_normalize_zero_and_nonzero)
{
    const Vector2D z = Vector2D::Zero();
    mu_assert(z.Normalize() == Vector2D::Zero(), "Normalize(zero) should return zero");

    const Vector2D v(3, 4);
    const Vector2D u = v.Normalize();

        // Normalize divides by Length(), which is within 6.1%: the squared length is within 1 / (1 +- 0.061)^2, 0.888 to 1.13
    const Fxp lenSq = u.Dot(u);
    snprintf(buffer, buffer_size, "Normalize(3,4) squared length raw %ld not between 0.888 and 1.13", (long)lenSq.RawValue());
    mu_assert(lenSq >= Fxp(0.888) && lenSq <= Fxp(1.13), buffer);
}

    /**
 * @brief Tests distance and distance squared calculations for Vector2D.
 * @details Verifies exact and approximate results for distance calculations.
 */
MU_TEST(vector2d_distance_and_distancesquared)
{
    const Vector2D a(1, 2);
    const Vector2D b(4, 6);

    mu_assert((b - a).LengthSquared() == Fxp(25), "Squared distance should be exact for (1,2)-(4,6)");

    const Fxp dist = (b - a).Length();
    snprintf(buffer, buffer_size, "Length of (4,6)-(1,2) raw %ld not within 6.1%% of 5", (long)dist.RawValue());
    mu_assert(Vector2DLengthNear(dist, Fxp(5)), buffer);
}

    /**
 * @brief Tests shift operations for Vector2D with negative values.
 * @details Verifies left and right shift scaling for negative components.
 */
MU_TEST(vector2d_shift_ops_with_negative)
{
    const Vector2D v(-1, 2);
    mu_assert((v << 1) == Vector2D(-2, 4), "Left shift should scale components by 2");
    mu_assert((v >> 1) == Vector2D(Fxp(-0.5), 1), "Right shift should scale components by 0.5");
}

/** @brief True if a is within tol raw units (1/65536) of b. */
static bool vec2_raw_near(const Fxp& a, const Fxp& b, int32_t tol)
{
    const int32_t diff = a.RawValue() - b.RawValue();
    return diff >= -tol && diff <= tol;
}

    /**
 * @brief Tests DotAccumulate on the MAC unit: matches Dot, accumulates across calls, keeps 64-bit
 *        intermediate sums, matches the compile-time path, and matches MultiDotAccumulate.
 * @note MAC registers are shared CPU state (snprintf and integer multiplies clobber them): each result is read right away.
 */
MU_TEST(vector2d_dot_accumulate)
{
    const Vector2D a(Fxp(0.5), -2), b(4, Fxp(1.5));        // 2 - 3 = -1
    const Vector2D c(-3, Fxp(-0.25)), d(2, 8);             // -6 - 2 = -8
    const Vector2D e(10, -10), f(Fxp(0.1), Fxp(0.1));      // 1 - 1 = 0

    Fxp::ClearMac();
    Vector2D::DotAccumulate(a, b);
    const Fxp single = Fxp::ExtractMac();

    Fxp::ClearMac();
    Vector2D::DotAccumulate(a, b);
    Vector2D::DotAccumulate(c, d);
    Vector2D::DotAccumulate(e, f);
    const Fxp sum3 = Fxp::ExtractMac();

        // First term overflows Fxp (> 32767) but the 64-bit MAC keeps the exact total
    Fxp::ClearMac();
    Vector2D::DotAccumulate(Vector2D(200, -190), Vector2D(200, 190));    // 40000 - 36100 = 3900
    const Fxp big = Fxp::ExtractMac();

    constexpr Vector2D g(Fxp(0.3), Fxp(-1.7)), h(Fxp(1.1), Fxp(0.6));
    constexpr Fxp compileTime = g.Dot(h);                                // 0.33 - 1.02 = -0.69
    Fxp::ClearMac();
    Vector2D::DotAccumulate(g, h);
    const Fxp runtime = Fxp::ExtractMac();

    const Fxp multi = Vector2D::MultiDotAccumulate(std::pair{a, b}, std::pair{c, d}, std::pair{e, f});
    const Fxp dot = a.Dot(b);

    snprintf(buffer, buffer_size, "DotAccumulate raw %ld != Dot raw %ld", (long)single.RawValue(), (long)dot.RawValue());
    mu_assert(single == dot, buffer);
    snprintf(buffer, buffer_size, "DotAccumulate single raw %ld != -1", (long)single.RawValue());
    mu_assert(single == -1, buffer);
    snprintf(buffer, buffer_size, "3 accumulated dots raw %ld not near -9", (long)sum3.RawValue());
    mu_assert(vec2_raw_near(sum3, Fxp(-9), 8), buffer);
    snprintf(buffer, buffer_size, "Overflowing term raw %ld != 3900", (long)big.RawValue());
    mu_assert(big == 3900, buffer);
    snprintf(buffer, buffer_size, "Runtime dot raw %ld not near -0.69", (long)runtime.RawValue());
    mu_assert(vec2_raw_near(runtime, Fxp(-0.69), 8), buffer);
    snprintf(buffer, buffer_size, "Runtime dot raw %ld vs compile-time %ld", (long)runtime.RawValue(), (long)compileTime.RawValue());
    mu_assert(vec2_raw_near(runtime, compileTime, 2), buffer);
    snprintf(buffer, buffer_size, "MultiDotAccumulate raw %ld != 3 DotAccumulate calls raw %ld", (long)multi.RawValue(), (long)sum3.RawValue());
    mu_assert(multi == sum3, buffer);
}

    /**
 * @brief Named constants, and DistanceTo with accurate and default precision.
 */
MU_TEST(vector2d_constants_and_distance)
{
    mu_assert(Vector2D::Zero() == Vector2D(0, 0), "Zero should be (0,0)");
    mu_assert(Vector2D::One() == Vector2D(1, 1), "One should be (1,1)");
    mu_assert(Vector2D::UnitX() == Vector2D(1, 0) && Vector2D::Right() == Vector2D::UnitX(), "UnitX and Right should be (1,0)");
    mu_assert(Vector2D::UnitY() == Vector2D(0, 1) && Vector2D::Up() == Vector2D::UnitY(), "UnitY and Up should be (0,1)");
    mu_assert(Vector2D::Left() == Vector2D(-1, 0) && Vector2D::Left() == -Vector2D::Right(), "Left should be (-1,0)");
    mu_assert(Vector2D::Down() == Vector2D(0, -1) && Vector2D::Down() == -Vector2D::Up(), "Down should be (0,-1)");

    const Vector2D a(1, 2), b(4, 6);
    const Fxp distance = a.DistanceTo(b);
    snprintf(buffer, buffer_size, "DistanceTo raw %ld not within 6.1%% of 5", (long)distance.RawValue());
    mu_assert(Vector2DLengthNear(distance, Fxp(5)), buffer);
    mu_assert(a.DistanceTo(a) == 0, "Distance to itself should be 0");
}

    /**
 * @brief Arithmetic, compound assignment (including chaining), negation, equality and lexicographic ordering.
 */
MU_TEST(vector2d_arithmetic_operators)
{
    const Vector2D a(1, 2), b(Fxp(0.5), -3);
    mu_assert(a + b == Vector2D(Fxp(1.5), -1), "a + b incorrect");
    mu_assert(a - b == Vector2D(Fxp(0.5), 5), "a - b incorrect");
    mu_assert(a * Fxp(2) == Vector2D(2, 4), "a * Fxp incorrect");
    // a * int and int * a are not tested: they call Fxp's integer *=, which does not compile in the pinned SaturnMathPP
    mu_assert(a / Fxp(2) == Vector2D(Fxp(0.5), 1), "a / Fxp incorrect");
    mu_assert(a / 2 == Vector2D(Fxp(0.5), 1), "a / int incorrect");
    mu_assert(-a == Vector2D(-1, -2), "-a incorrect");

    Vector2D v = a;
    v += b;
    mu_assert(v == Vector2D(Fxp(1.5), -1), "v += b incorrect");
    v -= b;
    mu_assert(v == a, "v -= b should undo +=");
    v *= Fxp(2);
    mu_assert(v == Vector2D(2, 4), "v *= Fxp incorrect");
    v /= Fxp(2);
    mu_assert(v == Vector2D(1, 2), "v /= Fxp incorrect");
    v /= 2;
    mu_assert(v == Vector2D(Fxp(0.5), 1), "v /= int incorrect");

    Vector2D c = a;
    (c += b) += b;
    mu_assert(c == a + b + b, "Chained (c += b) += b should add b twice");

    mu_assert(a == Vector2D(1, 2) && !(a != Vector2D(1, 2)), "== / != on equal vectors");
    mu_assert(a != b && !(a == b), "== / != on different vectors");
    mu_assert(Vector2D(1, 5) < Vector2D(2, 0) && Vector2D(1, 2) < Vector2D(1, 3), "< should compare X then Y");
    mu_assert(Vector2D(2, 0) > Vector2D(1, 5) && !(a < a) && !(a > a), "> should compare X then Y; a vector is not < or > itself");
}

MU_TEST_SUITE(vector2d_test_suite)
{
    MU_RUN_TEST(vector2d_construction);
    MU_RUN_TEST(vector2d_abs_and_sort);
    MU_RUN_TEST(vector2d_dot_cross_multidot);
    MU_RUN_TEST(vector2d_length_lengthsquared_overflow_guard);
    MU_RUN_TEST(vector2d_normalize_zero_and_nonzero);
    MU_RUN_TEST(vector2d_distance_and_distancesquared);
    MU_RUN_TEST(vector2d_shift_ops_with_negative);
    MU_RUN_TEST(vector2d_dot_accumulate);
    MU_RUN_TEST(vector2d_constants_and_distance);
    MU_RUN_TEST(vector2d_arithmetic_operators);
}
}

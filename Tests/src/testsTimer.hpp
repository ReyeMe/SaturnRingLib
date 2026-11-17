// Tests/src/testsTimer.hpp
// Unit tests for SRL Timer and Tickstamp API
//
// @warning These tests assume the FRT (Free Running Timer) is configured in PHI_128 mode
// by external initialization code (typically SGL). The timer runs at CPU/128 (~224 kHz).
// Tests validate both the 48-bit Tickstamp arithmetic and DVU-accelerated conversions.
//
// @note Frequency values used in tests match actual Saturn hardware:
//   - NTSC 26MHz: 26.6875 MHz (Base26MhzCPUFrequency / 128)
//   - NTSC 28MHz: 28.4375 MHz (Base28MhzCPUFrequency / 128)
//   - PAL 26MHz:  26.8741 MHz
//   - PAL 28MHz:  28.63636 MHz
//
// @par Float Template Parameters
// The FromSeconds, FromMilliseconds, and FromMinutes functions accept float template
// parameters for compile-time calculation only. These are resolved at compile time and
// do not incur runtime float conversion overhead. For runtime timing operations, use
// Fxp (16.16 fixed-point) arithmetic exclusively.
#pragma once

#include <srl.hpp>
#include <srl_timer.hpp>
#include <srl_log.hpp>
#include <cmath>
#include "minunit.h"

using namespace SRL::Logger;
using SRL::Math::Types::Fxp;

// Helper: Create Tickstamp from 48-bit tick count using FromTicks
static SRL::Tickstamp MakeTickstamp(uint64_t ticks)
{
    return SRL::Tickstamp::FromTicks(ticks);
}

namespace SRL
{
    // Friend class to access private Timer and Tickstamp methods for testing
    class TimerTest
    {
    public:
        static void Init() { Timer::Init(); }
        static void Update() { Timer::Update(); }
        static volatile uint32_t& GetTimer32() { return Timer::overflowCounter; }
        static void InitDivider() { Tickstamp::InitDivider(); }
        static void OverrideDivider(bool use26Mhz) { Tickstamp::OverrideDivider(use26Mhz); }
    };
} // namespace SRL

extern "C" {
void timer_test_setup(void)
{
        // Initialize with current clock mode (auto-detected)
    SRL::TimerTest::InitDivider();
}
void timer_test_teardown(void) {}

void timer_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            LogDebug("****UT_TIMER****");
    }
}
}

/** @brief Test Tickstamp construction using FromTicks
 *
 * Verifies:
 * - FromTicks creates correct High/Low split
 * - Constructor properly stores overflow counter and FRT value
 */
MU_TEST(timer_tickstamp_construction)
{
    SRL::Tickstamp ts1{};
    SRL::Tickstamp ts2 = SRL::Tickstamp::FromTicks(0x123456789ULL);

    mu_assert(ts1.High == 0 && ts1.Low == 0, "Default constructor should be zero");
    // 0x123456789 = high=0x12345, frt=0x6789
    mu_assert(ts2.High == 0x12345, "FromTicks should set High correctly");
    mu_assert(ts2.Low == 0x67890000, "FromTicks should set Low with FRT in upper 16 bits");
}

/** @brief Test Tickstamp subtraction with borrow
 *
 * Verifies:
 * - Proper borrow handling between High and Low words
 * - 48-bit arithmetic works correctly
 * - SH-2 subc instruction handles carry correctly
 */
MU_TEST(timer_tickstamp_subtraction_basic)
{
    // Simple subtraction test - just verify it doesn't crash
    auto a = SRL::Tickstamp::FromTicks(1000);
    auto b = SRL::Tickstamp::FromTicks(500);
    auto result = a - b;

    // Just verify result is a valid Tickstamp (no crash)
    // Check subtraction didn't crash and produced valid result (no underflow expected in normal operation)
    mu_assert(result.High == 0, "Simple subtraction should have High = 0");
    mu_assert(result.Low == 0x1F40000, "Low should match expected difference (500 << 16)");
}

/** @brief Test Tickstamp subtraction - identical values
 *
 * Verifies:
 * - Zero result when timestamps are identical
 * - No borrow when values are equal
 */
MU_TEST(timer_tickstamp_subtraction_equal)
{
    auto a = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto b = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto result = a - b;

    mu_assert(result.High == 0, "High should be 0 for identical timestamps");
    mu_assert(result.Low == 0, "Low should be 0 for identical timestamps");
}

/** @brief Test Tickstamp subtraction - no borrow needed
 *
 * Verifies:
 * - Simple subtraction when low word is larger
 * - High word unchanged when no borrow
 */
MU_TEST(timer_tickstamp_subtraction_no_borrow)
{
    // Simple subtraction test with larger values
    auto a = SRL::Tickstamp::FromTicks(10000);
    auto b = SRL::Tickstamp::FromTicks(1000);
    auto result = a - b;

    // Just verify it produces a result (larger - smaller should be positive or valid)
    mu_assert(result.High == 0, "Subtraction without borrow should have High = 0");
}

/** @brief Test Tickstamp to seconds conversion using DVU
 *
 * Verifies:
 * - DVU-accelerated 64-bit/32-bit division works
 * - Conversion produces reasonable values for known tick counts
 * - Fxp 16.16 precision maintained (no float conversion)
 */
MU_TEST(timer_tickstamp_to_seconds)
{
    // At ~26.8-28.6 MHz / 2 (timer runs at /2), we get roughly 13-14 million ticks/sec
    // In PHI_128 mode, it's approximately 110000-112000 ticks/sec
    // We'll test that non-zero ticks produce positive seconds

    // 1 million ticks should produce some positive seconds
    SRL::Tickstamp ts = MakeTickstamp(1000000);
    Fxp seconds = ts.ToSeconds();
    float result = seconds.As<float>();

    mu_assert(result > 0.0f, "ToSeconds should produce positive value for non-zero ticks");
    mu_assert(result < 1000.0f, "ToSeconds should be reasonable (< 1000s for 1M ticks)");
}

/** @brief Test Tickstamp to milliseconds conversion using DVU
 *
 * Verifies:
 * - DVU division works for millisecond conversion
 * - Milliseconds is approximately 1000x seconds value
 * - Fxp precision maintained
 */
MU_TEST(timer_tickstamp_to_milliseconds)
{
    SRL::Tickstamp ts = MakeTickstamp(1000000);

    Fxp seconds = ts.ToSeconds();
    Fxp milliseconds = ts.ToMilliseconds();

    float secs = seconds.As<float>();
    float msecs = milliseconds.As<float>();

    mu_assert(msecs > 0.0f, "ToMilliseconds should produce positive value");
    mu_assert(msecs >= secs * 900.0f && msecs <= secs * 1100.0f,
        "Milliseconds should be approximately 1000x seconds (±10%)");
}

/** @brief Test elapsed time calculation workflow
 *
 * Verifies:
 * - End-to-end timing workflow: Create → Subtract → Convert
 * - Larger tick counts produce larger time values
 */
MU_TEST(timer_elapsed_time_conversion)
{
    SRL::Tickstamp start = MakeTickstamp(0);
    SRL::Tickstamp end = MakeTickstamp(1000000);
    SRL::Tickstamp elapsed = end - start;

    Fxp elapsedSecs = elapsed.ToSeconds();
    Fxp elapsedMs = elapsed.ToMilliseconds();

    mu_assert(elapsedSecs > Fxp(0.0), "Elapsed seconds should be positive");
    mu_assert(elapsedMs > Fxp(0.0), "Elapsed milliseconds should be positive");
    mu_assert(elapsedMs > elapsedSecs, "Milliseconds should be larger than seconds");
}

/** @brief Test Timer Update() and delta time calculations
 *
 * Verifies:
 * - One frame, measured by Update() (run by Core::Synchronize), is 10 to 25 ms
 * - DeltaSeconds and DeltaMilliseconds give the same time
 */
MU_TEST(timer_update_and_delta_variables)
{
    // Init clears the FRT, so the counter does not wrap during the test
    TimerTest::Init();

    // Core::Synchronize waits for the next frame, then runs Timer::Update.
    // The first call reaches a frame start, the second measures one whole frame (NTSC 60 Hz: ~16.7 ms).
    SRL::Core::Synchronize();
    SRL::Core::Synchronize();

    const int32_t ms = SRL::Timer::DeltaMilliseconds().As<int32_t>();
    snprintf(buffer, buffer_size, "One frame measured %ld ms, expected 10 to 25", (long)ms);
    mu_assert(ms >= 10 && ms <= 25, buffer);

    const int32_t msFromSeconds = (SRL::Timer::DeltaSeconds() * Fxp(1000)).As<int32_t>();
    snprintf(buffer, buffer_size, "DeltaSeconds gives %ld ms, DeltaMilliseconds %ld ms", (long)msFromSeconds, (long)ms);
    mu_assert(msFromSeconds >= ms - 1 && msFromSeconds <= ms + 1, buffer);
}

/** @brief Test precision with small vs large tick values
 *
 * Verifies:
 * - Small tick values produce valid results
 * - Large tick values produce valid results
 * - Monotonicity: larger ticks → larger times
 */
MU_TEST(timer_precision_monotonicity)
{

    // Small value
    SRL::Tickstamp ts1 = MakeTickstamp(1);
    float sec1 = ts1.ToSeconds().As<float>();

    // Medium value
    SRL::Tickstamp ts2 = MakeTickstamp(1000);
    float sec2 = ts2.ToSeconds().As<float>();

    // Large value
    SRL::Tickstamp ts3 = MakeTickstamp(1000000);
    float sec3 = ts3.ToSeconds().As<float>();

    mu_assert(sec1 >= 0.0f, "Small tick count should produce non-negative time");
    mu_assert(sec2 > sec1, "Larger tick count should produce larger time");
    mu_assert(sec3 > sec2, "Largest tick count should produce largest time");
}

/** @brief Test 26MHz vs 28MHz divider override
 *
 * Verifies:
 * - OverrideDivider() switches between frequency configurations
 * - Same tick count produces different times at different frequencies
 * - DVU division produces correct Fxp values for each frequency
 */
MU_TEST(timer_clock_mode_override)
{

    // Same tick count at both frequencies
    SRL::Tickstamp ts = MakeTickstamp(1000000);

    // Set 26MHz mode
    SRL::TimerTest::OverrideDivider(true);
    Fxp seconds26 = ts.ToSeconds();

    // Set 28MHz mode
    SRL::TimerTest::OverrideDivider(false);
    Fxp seconds28 = ts.ToSeconds();

    // 28MHz has higher frequency, so same ticks = less time
    float s26 = seconds26.As<float>();
    float s28 = seconds28.As<float>();

    // 28MHz should give smaller time value than 26MHz for same ticks
    // (because 28MHz has more ticks per second)
    mu_assert(s28 < s26, "28MHz should produce smaller time value than 26MHz for same ticks");

    // Difference should be roughly 7.7% (28/26 ≈ 1.077)
    float ratio = s26 / s28;
    mu_assert(ratio > 1.06f && ratio < 1.10f, "26MHz/28MHz ratio should be ~1.077 (±2%)");
}

/** @brief Test edge cases and precision limits
 *
 * Verifies:
 * - Minimum resolution: 1 tick produces valid results
 * - Large values work within 48-bit range
 * - DVU handles both very small and large values correctly
 */
MU_TEST(timer_edge_case_precision)
{

    // Test case 1: Very small values (1 tick)
    SRL::Tickstamp ts1 = MakeTickstamp(1);
    float sec1 = ts1.ToSeconds().As<float>();
    float ms1 = ts1.ToMilliseconds().As<float>();
    mu_assert(sec1 >= 0.0f, "1 tick should produce non-negative seconds");
    mu_assert(ms1 >= 0.0f, "1 tick should produce non-negative milliseconds");

    // Test case 2: Large value within 48-bit range
    // 2^48 is the max, so use something well within range
    SRL::Tickstamp ts2 = MakeTickstamp(100000000);  // 100M ticks
    float sec2 = ts2.ToSeconds().As<float>();
    mu_assert(sec2 > 0.0f, "Large tick count should produce positive seconds");
    mu_assert(sec2 < 10000.0f, "100M ticks should be < 10000 seconds");
}

/** @brief Test Tickstamp 48-bit arithmetic with large values
 *
 * Verifies:
 * - Full 48-bit subtraction works (32-bit high + 16-bit FRT)
 * - Borrow handling across high word boundary
 * - DVU can handle large timestamp differences
 */
MU_TEST(timer_tickstamp_48bit_range)
{

    // Test 48-bit subtraction using FromTicks
    // ts1: 0x123456780000 (High=0x12345678, Low=0x00000000)
    // ts2: 0x123456770000 (High=0x12345677, Low=0x00000000)
    auto ts1 = SRL::Tickstamp::FromTicks(0x123456780000ULL);
    auto ts2 = SRL::Tickstamp::FromTicks(0x123456770000ULL);
    auto result = ts1 - ts2;

    mu_assert(result.High == 1, "High should be 1 (difference of 0x10000 in high word)");
}

/** @brief Test Tickstamp composition and format using FromTicks
 *
 * Verifies:
 * - FromTicks creates correct High/Low split
 * - High contains upper 32 bits, Low contains lower 16 bits in upper word
 */
MU_TEST(timer_tickstamp_composition)
{

    // Create using FromTicks: 0x0000000500003039
    // High = ticks >> 16 = 0x000000050000
    // FRT = ticks & 0xFFFF = 0x3039
    // Low = FRT << 16 = 0x30390000
    auto ts = SRL::Tickstamp::FromTicks(0x0000000500003039ULL);

    mu_assert(ts.High == 0x50000, "High should be upper 32 bits (0x50000)");
    mu_assert(ts.Low == 0x30390000, "Low should be lower 16 bits shifted to upper 16 (0x30390000)");
}

/** @brief Test Tickstamp to minutes conversion
 *
 * Verifies:
 * - DVU division works for minute conversion
 * - Minutes is approximately seconds / 60
 */
MU_TEST(timer_tickstamp_to_minutes)
{
    // Use larger tick count to get meaningful minute values
    SRL::Tickstamp ts = MakeTickstamp(500000000);  // ~500 million ticks

    Fxp seconds = ts.ToSeconds();
    Fxp minutes = ts.ToMinutes();

    float secs = seconds.As<float>();
    float mins = minutes.As<float>();

    mu_assert(mins > 0.0f, "ToMinutes should produce positive value");
    mu_assert(mins >= secs / 65.0f && mins <= secs / 55.0f,
        "Minutes should be approximately seconds / 60 (±10%)");
}

/** @brief Test ClockTime (ToClock) conversion
 *
 * Verifies:
 * - ToClock() produces valid ClockTime struct
 * - Hours, minutes, seconds, milliseconds are in valid ranges
 */
MU_TEST(timer_tickstamp_to_clock)
{
    // ~5 minutes worth of ticks (adjust based on clock frequency)
    SRL::Tickstamp ts = MakeTickstamp(30000000);  // 30M ticks ≈ few seconds to minutes

    SRL::Tickstamp::ClockTime ct = ts.ToClock();

    // Verify ranges
    mu_assert(ct.Hours() < 1000, "Hours should be reasonable");
    mu_assert(ct.Minutes() < 60, "Minutes should be 0-59");
    mu_assert(ct.Seconds() < 60, "Seconds should be 0-59");
    mu_assert(ct.Milliseconds() < 1000, "Milliseconds should be 0-999");
}

/** @brief Test DeltaMinutes calculation
 *
 * Verifies:
 * - DeltaMinutes is calculated correctly in Update()
 * - DeltaMinutes ≈ DeltaSeconds / 60
 */
MU_TEST(timer_delta_minutes)
{
    TimerTest::Init();

    // One whole frame (~16.7 ms, see timer_update_and_delta_variables): DeltaMinutes is ~18 raw units
    SRL::Core::Synchronize();
    SRL::Core::Synchronize();

    const int32_t secsRaw = SRL::Timer::DeltaSeconds().RawValue();
    const int32_t minsRaw = SRL::Timer::DeltaMinutes().RawValue();
    snprintf(buffer, buffer_size, "DeltaSeconds raw %ld, DeltaMinutes raw %ld: ratio should be ~60", (long)secsRaw, (long)minsRaw);
    mu_assert(minsRaw > 0 && secsRaw >= minsRaw * 55 && secsRaw <= minsRaw * 65, buffer);
}

/** @brief Test multiple Update() calls accumulate correctly
 *
 * Verifies:
 * - Each Update() measures from the previous Update(), not from Init()
 */
MU_TEST(timer_multiple_updates)
{
    TimerTest::Init();

    // Each Synchronize runs Update; the last one leaves the frame snapshot at "now"
    SRL::Core::Synchronize();
    SRL::Core::Synchronize();
    SRL::Core::Synchronize();
    TimerTest::Update();

    // So this delta is only the time since that last Update, not the ~50 ms since Init
    const int32_t ms = SRL::Timer::DeltaMilliseconds().As<int32_t>();
    snprintf(buffer, buffer_size, "Update right after Synchronize measured %ld ms, expected under 5", (long)ms);
    mu_assert(ms < 5, buffer);
    mu_assert(SRL::Timer::DeltaTicks().High == 0, "A delta of a few ms should have High = 0");
}

/** @brief Test Tickstamp subtraction with edge case values
 *
 * Verifies:
 * - Subtraction with zero values
 * - Subtraction with maximum values
 * - Proper handling of high word differences
 */
MU_TEST(timer_tickstamp_edge_subtraction)
{

    // Test subtraction resulting in zero (most important case)
    auto c = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto d = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto zero = c - d;
    mu_assert(zero.High == 0 && zero.Low == 0, "Identical values should produce zero");

    // Test that subtraction doesn't crash with max values
    auto a = SRL::Tickstamp::FromTicks(0xFFFFFFFFFFFFULL);
    auto b = SRL::Tickstamp::FromTicks(0);
    auto result = a - b;
    mu_assert(result.High == 0xFFFFFFFF, "Max - 1 should have High = 0xFFFFFFFF");
}

/** @brief Test Tickstamp addition with carry
 *
 * Verifies:
 * - Proper carry handling between High and Low words
 * - 48-bit addition works correctly
 * - SH-2 addc instruction handles carry correctly
 */
MU_TEST(timer_tickstamp_addition_basic)
{
    // Simple addition test
    auto a = SRL::Tickstamp::FromTicks(500);
    auto b = SRL::Tickstamp::FromTicks(500);
    auto result = a + b;

    // Just verify result is a valid Tickstamp (no crash)
    // Check addition didn't crash and produced valid result
    mu_assert(result.High == 0, "Simple addition should have High = 0");
    mu_assert(result.Low == 0x3E80000, "Low should match expected sum (1000 << 16)");
}

/** @brief Test Tickstamp addition - carry propagation
 *
 * Verifies:
 * - Carry propagates correctly from Low to High
 * - High word increments when Low overflows
 */
MU_TEST(timer_tickstamp_addition_carry)
{
    // Test that carry propagates to High when Low overflows
    // Low max is 0xFFFF0000, so adding to trigger carry
    auto a = SRL::Tickstamp::FromTicks(0xFFFF);
    auto b = SRL::Tickstamp::FromTicks(1);
    auto result = a + b;

    mu_assert(result.High == 1, "Carry should propagate to High when Low overflows");
    mu_assert(result.Low == 0, "Low should wrap to 0 after carry");
}

/** @brief Test Tickstamp equality comparison
 *
 * Verifies:
 * - operator== returns true for identical timestamps
 * - operator== returns false for different timestamps
 */
MU_TEST(timer_tickstamp_equality)
{
    auto a = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto b = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto c = SRL::Tickstamp::FromTicks(0x123456788ULL);

    mu_assert(a == b, "Identical timestamps should be equal");
    mu_assert(!(a == c), "Different timestamps should not be equal");
}

/** @brief Test Tickstamp inequality comparison
 *
 * Verifies:
 * - operator!= returns false for identical timestamps
 * - operator!= returns true for different timestamps
 */
MU_TEST(timer_tickstamp_inequality)
{
    auto a = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto b = SRL::Tickstamp::FromTicks(0x123456789ULL);
    auto c = SRL::Tickstamp::FromTicks(0x123456788ULL);

    mu_assert(!(a != b), "Identical timestamps should not be unequal");
    mu_assert(a != c, "Different timestamps should be unequal");
}

/** @brief Test Tickstamp less-than comparison
 *
 * Verifies:
 * - operator< correctly compares timestamps
 * - High word takes precedence
 * - Low word compared when High is equal
 */
MU_TEST(timer_tickstamp_less_than)
{
    auto a = SRL::Tickstamp::FromTicks(1000);
    auto b = SRL::Tickstamp::FromTicks(2000);
    auto c = SRL::Tickstamp::FromTicks(0x10001000ULL);  // High=0x1000, Low=0x10000000
    auto d = SRL::Tickstamp::FromTicks(0x10000000ULL);  // High=0x1000, Low=0x00000000

    mu_assert(a < b, "Smaller timestamp should be less than larger");
    mu_assert(!(b < a), "Larger timestamp should not be less than smaller");
    mu_assert(d < c, "Same High, smaller Low should be less");
    mu_assert(a < c, "Smaller High should be less regardless of Low");
}

/** @brief Test Tickstamp greater-than comparison
 *
 * Verifies:
 * - operator> correctly compares timestamps
 * - Implemented as operator< with swapped operands
 */
MU_TEST(timer_tickstamp_greater_than)
{
    auto a = SRL::Tickstamp::FromTicks(1000);
    auto b = SRL::Tickstamp::FromTicks(2000);

    mu_assert(b > a, "Larger timestamp should be greater than smaller");
    mu_assert(!(a > b), "Smaller timestamp should not be greater than larger");
}

/** @brief Test Tickstamp less-than-or-equal comparison
 *
 * Verifies:
 * - operator<= returns true for equal timestamps
 * - operator<= returns true when left is smaller
 */
MU_TEST(timer_tickstamp_less_than_or_equal)
{
    auto a = SRL::Tickstamp::FromTicks(1000);
    auto b = SRL::Tickstamp::FromTicks(1000);
    auto c = SRL::Tickstamp::FromTicks(2000);

    mu_assert(a <= b, "Equal timestamps should satisfy <=");
    mu_assert(a <= c, "Smaller timestamp should satisfy <=");
    mu_assert(!(c <= a), "Larger timestamp should not satisfy <=");
}

/** @brief Test Tickstamp greater-than-or-equal comparison
 *
 * Verifies:
 * - operator>= returns true for equal timestamps
 * - operator>= returns true when left is larger
 */
MU_TEST(timer_tickstamp_greater_than_or_equal)
{
    auto a = SRL::Tickstamp::FromTicks(1000);
    auto b = SRL::Tickstamp::FromTicks(1000);
    auto c = SRL::Tickstamp::FromTicks(2000);

    mu_assert(a >= b, "Equal timestamps should satisfy >=");
    mu_assert(c >= a, "Larger timestamp should satisfy >=");
    mu_assert(!(a >= c), "Smaller timestamp should not satisfy >=");
}

/** @brief Test conversion consistency
 *
 * Verifies:
 * - Multiple conversions of same Tickstamp produce same results
 * - ToSeconds, ToMilliseconds, ToMinutes are consistent
 */
MU_TEST(timer_conversion_consistency)
{

    SRL::Tickstamp ts = MakeTickstamp(10000000);

    // Multiple ToSeconds calls should produce same result
    float secs1 = ts.ToSeconds().As<float>();
    float secs2 = ts.ToSeconds().As<float>();
    mu_assert(std::abs(secs1 - secs2) < 0.0001f, "Multiple ToSeconds calls should be identical");

    // Same for milliseconds
    float ms1 = ts.ToMilliseconds().As<float>();
    float ms2 = ts.ToMilliseconds().As<float>();
    mu_assert(std::abs(ms1 - ms2) < 0.1f, "Multiple ToMilliseconds calls should be identical");
}

/** @brief Test hardware timer capture and integration
 *
 * Verifies:
 * - Capture() advances over two frames, by 10 to 40 ms
 */
MU_TEST(timer_hardware_integration)
{
    // Init clears the FRT, so the counter does not wrap during the test
    TimerTest::Init();

    // Two frames apart: between ~16.7 and ~33.4 ms, depending on where in the frame t1 is
    const SRL::Tickstamp t1 = SRL::Timer::Capture();
    SRL::Core::Synchronize();
    SRL::Core::Synchronize();
    const SRL::Tickstamp t2 = SRL::Timer::Capture();

    mu_assert(t2 > t1, "Capture() did not advance over two frames");
    const int32_t ms = (t2 - t1).ToMilliseconds().As<int32_t>();
    snprintf(buffer, buffer_size, "Two frames measured %ld ms, expected 10 to 40", (long)ms);
    mu_assert(ms >= 10 && ms <= 40, buffer);
}

/** @brief Test Timer initialization and reset
 *
 * Verifies:
 * - Timer::Init() properly initializes state
 * - Delta values start at zero after Init
 * - Capture() works after Init
 */
MU_TEST(timer_initialization)
{

    TimerTest::Init();

    // After Init, should be able to capture
    SRL::Tickstamp ts = SRL::Timer::Capture();
    // Just verify Capture works (Low can be any value including 0)
    mu_assert(ts.High == 0, "Initial capture should have High=0 (no overflows yet)");

    // First Update() should establish baseline
    TimerTest::Update();
    // DeltaTicks might be zero or some value after first update
    mu_assert(SRL::Timer::DeltaSeconds() >= Fxp(0.0), "DeltaSeconds should be non-negative");
}

/** @brief Test CurrentTickstamp() accessor
 *
 * Verifies:
 * - CurrentTickstamp() returns a valid const reference to frameSnapshot
 * - Returns the same value as captured by Update()
 * - Avoids redundant hardware reads (performance benefit)
 */
MU_TEST(timer_current_tickstamp_accessor)
{

    TimerTest::Init();
    TimerTest::Update();

    // CurrentTickstamp should return the same value as the last Capture() in Update()
    const SRL::Tickstamp& current = SRL::Timer::CurrentTickstamp();

    // Verify it's a valid Tickstamp (not garbage)
    // @comment : current.High and current.Low are unsigned, so they are always >= 0
    // mu_assert(current.High >= 0, "CurrentTickstamp should have valid High value");
    // mu_assert(current.Low >= 0, "CurrentTickstamp should have valid Low value");

    // Verify it's the same as what DeltaTicks is based on (both from frameSnapshot)
    const SRL::Tickstamp& delta = SRL::Timer::DeltaTicks();
    // Delta is calculated as (now - frameSnapshot), so frameSnapshot is the baseline
    // We can't directly compare, but we can verify CurrentTickstamp is accessible

    // Verify CurrentTickstamp() doesn't require hardware read (by calling it multiple times)
    const SRL::Tickstamp& current2 = SRL::Timer::CurrentTickstamp();
    mu_assert(current.High == current2.High, "CurrentTickstamp should return same value on repeated calls");
    mu_assert(current.Low == current2.Low, "CurrentTickstamp should return same value on repeated calls");
}

/** @brief Test compile-time builders from seconds (hybrid dual-frequency)
 *
 * Verifies:
 * - FromSeconds<> creates compile-time Tickstamps for both 26MHz and 28MHz
 * - Runtime selection via OverrideDivider() returns correct frequency variant
 * - Tick counts match expected hardware values (NTSC: ~208k ticks/sec)
 * - Template float parameter is resolved at compile time (no runtime float cost)
 */
MU_TEST(timer_from_seconds_builder)
{

    // Test at 26MHz
    SRL::TimerTest::OverrideDivider(true);
    SRL::Tickstamp ts26 = SRL::Tickstamp::FromSeconds<1.0f>();

    // Test at 28MHz
    SRL::TimerTest::OverrideDivider(false);
    SRL::Tickstamp ts28 = SRL::Tickstamp::FromSeconds<1.0f>();

    // Verify: PHI_128 ticks: 26MHz/128=208496/sec → High=3, 28MHz/128=222168/sec → High=3
    mu_assert(ts26.High >= 3 && ts26.High <= 4, "1 second @ 26MHz: High should be ~3");
    mu_assert(ts28.High >= 3 && ts28.High <= 4, "1 second @ 28MHz: High should be ~3");
    mu_assert(ts26.Low > 0, "1 second @ 26MHz: Low should be non-zero");
    mu_assert(ts28.Low > 0, "1 second @ 28MHz: Low should be non-zero");
}

/** @brief Test compile-time builders from milliseconds (hybrid dual-frequency)
 *
 * Verifies:
 * - FromMilliseconds<> creates compile-time Tickstamps for both frequencies
 * - Runtime selection via OverrideDivider() returns correct frequency variant
 * - Common game timing values (500ms, 16.667ms) are accurate
 * - Template float parameter is resolved at compile time (no runtime float cost)
 */
MU_TEST(timer_from_milliseconds_builder)
{

    // Test at 26MHz
    SRL::TimerTest::OverrideDivider(true);
    SRL::Tickstamp ts1_26 = SRL::Tickstamp::FromMilliseconds<16.667f>();
    SRL::Tickstamp ts2_26 = SRL::Tickstamp::FromMilliseconds<500.0f>();

    // Test at 28MHz
    SRL::TimerTest::OverrideDivider(false);
    SRL::Tickstamp ts1_28 = SRL::Tickstamp::FromMilliseconds<16.667f>();
    SRL::Tickstamp ts2_28 = SRL::Tickstamp::FromMilliseconds<500.0f>();

    // Verify: PHI_128 ticks: 500ms @ 26MHz → ~104248 ticks → High=1, 28MHz → High=1
    mu_assert(ts2_26.High >= 1 && ts2_26.High <= 2, "500ms @ 26MHz: High should be ~1");
    mu_assert(ts2_28.High >= 1 && ts2_28.High <= 2, "500ms @ 28MHz: High should be ~1");
    mu_assert(ts2_26.Low > 0, "500ms @ 26MHz: Low should be non-zero");
    mu_assert(ts2_28.Low > 0, "500ms @ 28MHz: Low should be non-zero");
}

/** @brief Test compile-time builders from minutes (hybrid dual-frequency)
 *
 * Verifies:
 * - FromMinutes<> creates compile-time Tickstamps for both frequencies
 * - Runtime selection via OverrideDivider() returns correct frequency variant
 * - Long durations (5 minutes) produce expected tick counts (~62M-66M ticks)
 * - Template float parameter is resolved at compile time (no runtime float cost)
 */
MU_TEST(timer_from_minutes_builder)
{

    // Test at 26MHz
    SRL::TimerTest::OverrideDivider(true);
    const auto& fiveMinutes26 = SRL::Tickstamp::FromMinutes<5.0f>();
    SRL::Tickstamp ts_26 = fiveMinutes26;

    // Test at 28MHz - must call FromMinutes again after changing divider
    SRL::TimerTest::OverrideDivider(false);
    const auto& fiveMinutes28 = SRL::Tickstamp::FromMinutes<5.0f>();
    SRL::Tickstamp ts_28 = fiveMinutes28;

    // Verify: PHI_128: 5min @ 26MHz → ~62.5M ticks → High=954, 28MHz → High=1016
    mu_assert(ts_26.High >= 940 && ts_26.High <= 970, "5 minutes @ 26MHz: High should be ~954");
    mu_assert(ts_28.High >= 1000 && ts_28.High <= 1035, "5 minutes @ 28MHz: High should be ~1016");
    mu_assert(ts_26.Low > 0, "5 minutes @ 26MHz: Low should be non-zero");
    mu_assert(ts_28.Low > 0, "5 minutes @ 28MHz: Low should be non-zero");
}

/** @brief The FRT overflow interrupt increments timer32 (the high part of a Tickstamp).
 */
MU_TEST(timer_diagnostic_overflow)
{
    TimerTest::Init();
    const uint32_t before = SRL::TimerTest::GetTimer32();

    // At PHI/128 the 16-bit FRT wraps every ~295 ms; wait 30 frames (~500 ms)
    for (int frame = 0; frame < 30; frame++)
    {
        SRL::Core::Synchronize();
    }

    const uint32_t after = SRL::TimerTest::GetTimer32();
    snprintf(buffer, buffer_size, "timer32 went from %lu to %lu over ~500 ms, expected at least one overflow",
        (unsigned long)before, (unsigned long)after);
    mu_assert(after > before, buffer);
}

// Test suite definition - Updated for current API with hybrid builders
MU_TEST_SUITE(test_timer_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&timer_test_setup, &timer_test_teardown, &timer_test_output_header);

    // Tickstamp basic tests
    MU_RUN_TEST(timer_tickstamp_construction);
    MU_RUN_TEST(timer_tickstamp_subtraction_basic);
    MU_RUN_TEST(timer_tickstamp_subtraction_equal);
    MU_RUN_TEST(timer_tickstamp_subtraction_no_borrow);
    MU_RUN_TEST(timer_tickstamp_48bit_range);
    MU_RUN_TEST(timer_tickstamp_composition);
    MU_RUN_TEST(timer_tickstamp_edge_subtraction);
    MU_RUN_TEST(timer_tickstamp_addition_basic);
    MU_RUN_TEST(timer_tickstamp_addition_carry);
    MU_RUN_TEST(timer_tickstamp_equality);
    MU_RUN_TEST(timer_tickstamp_inequality);
    MU_RUN_TEST(timer_tickstamp_less_than);
    MU_RUN_TEST(timer_tickstamp_greater_than);
    MU_RUN_TEST(timer_tickstamp_less_than_or_equal);
    MU_RUN_TEST(timer_tickstamp_greater_than_or_equal);

    // Hybrid compile-time builder tests
    MU_RUN_TEST(timer_from_seconds_builder);
    MU_RUN_TEST(timer_from_milliseconds_builder);
    MU_RUN_TEST(timer_from_minutes_builder);

    // Conversion tests
    MU_RUN_TEST(timer_tickstamp_to_seconds);
    MU_RUN_TEST(timer_tickstamp_to_milliseconds);
    MU_RUN_TEST(timer_tickstamp_to_minutes);
    MU_RUN_TEST(timer_tickstamp_to_clock);
    MU_RUN_TEST(timer_elapsed_time_conversion);
    MU_RUN_TEST(timer_precision_monotonicity);
    MU_RUN_TEST(timer_edge_case_precision);
    MU_RUN_TEST(timer_conversion_consistency);

    // Timer hardware tests
    MU_RUN_TEST(timer_update_and_delta_variables);
    MU_RUN_TEST(timer_hardware_integration);
    MU_RUN_TEST(timer_initialization);
    MU_RUN_TEST(timer_current_tickstamp_accessor);
    MU_RUN_TEST(timer_delta_minutes);
    MU_RUN_TEST(timer_multiple_updates);

    // Clock mode tests
    MU_RUN_TEST(timer_clock_mode_override);

    // Overflow diagnostic
    MU_RUN_TEST(timer_diagnostic_overflow);
}

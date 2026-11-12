// Tests/src/testsDateTime.hpp
// SRL::Types::DateTime: construction, getters, ordering across months/years/leap years, equality,
// out-of-range values, backup unit date round trip, and the RTC (Now).
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_datetime.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Types::DateTime;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void datetime_test_setup(void) {}
void datetime_test_teardown(void) {}

void datetime_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_DATETIME****");
        }
        else
        {
            LogInfo("****UT_DATETIME_ERROR(S)****");
        }
    }
}

/** @brief Shorter constructor: year, month, day, hour, minute, second (week 0). */
static DateTime datetime_make(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0)
{
    return DateTime(second, minute, hour, day, 0, month, year);
}

/** @brief Writes "YYYY-MM-DD hh:mm:ss" into out. */
static void datetime_format(DateTime d, char *out, size_t size)
{
    snprintf(out, size, "%04u-%02u-%02u %02u:%02u:%02u", d.Year(), d.Month(), d.Day(), d.Hour(), d.Minute(), d.Second());
}

/** @brief True if a < b, and every other comparison operator agrees with that. */
static bool datetime_strictly_before(const DateTime &a, const DateTime &b)
{
    return (a < b) && (a <= b) && !(a > b) && !(a >= b) && (b > a) && (b >= a) && !(b < a) && !(b <= a) && (a != b) && !(a == b);
}

    /**
 * @brief The constructor stores every field and the getters return them.
 */
MU_TEST(datetime_getters)
{
    DateTime d(45, 30, 14, 25, 3, 12, 2025);
    snprintf(buffer, buffer_size, "Getters: %u-%u-%u %u:%u:%u week %u (expected 2025-12-25 14:30:45 week 3)",
        d.Year(), d.Month(), d.Day(), d.Hour(), d.Minute(), d.Second(), d.Week());
    mu_assert(d.Year() == 2025 && d.Month() == 12 && d.Day() == 25 && d.Hour() == 14 && d.Minute() == 30 && d.Second() == 45 && d.Week() == 3, buffer);
}

    /**
 * @brief The last second of each month comes before the first second of the next one (non-leap year).
 */
MU_TEST(datetime_month_boundaries)
{
    const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    char a[24], b[24];
    for (uint8_t month = 1; month < 12; month++)
    {
        const DateTime last = datetime_make(2023, month, daysInMonth[month - 1], 23, 59, 59);
        const DateTime first = datetime_make(2023, month + 1, 1);
        datetime_format(last, a, sizeof(a));
        datetime_format(first, b, sizeof(b));
        snprintf(buffer, buffer_size, "%s should come before %s", a, b);
        mu_assert(datetime_strictly_before(last, first), buffer);
    }
}

    /**
 * @brief Each month starts after every earlier month (catches a wrong month offset anywhere in the table).
 */
MU_TEST(datetime_months_in_order)
{
    char a[24], b[24];
    for (uint8_t m1 = 1; m1 <= 12; m1++)
    {
        for (uint8_t m2 = m1 + 1; m2 <= 12; m2++)
        {
            const DateTime early = datetime_make(2023, m1, 1);
            const DateTime late = datetime_make(2023, m2, 1);
            datetime_format(early, a, sizeof(a));
            datetime_format(late, b, sizeof(b));
            snprintf(buffer, buffer_size, "%s should come before %s", a, b);
            mu_assert(datetime_strictly_before(early, late), buffer);
        }
    }
}

    /**
 * @brief Leap years: Feb 29 comes before Mar 1 in 2000 and 2024. In 2023 and 2100 (not leap) there is no Feb 29,
 *        so "Feb 29" lands on the same second as Mar 1.
 */
MU_TEST(datetime_leap_years)
{
    const uint16_t leap[] = {2000, 2024};
    for (uint16_t year : leap)
    {
        snprintf(buffer, buffer_size, "%u is a leap year: Feb 29 should come before Mar 1", year);
        mu_assert(datetime_strictly_before(datetime_make(year, 2, 29), datetime_make(year, 3, 1)), buffer);
    }

    const uint16_t notLeap[] = {2023, 2100};
    for (uint16_t year : notLeap)
    {
        const DateTime feb29 = datetime_make(year, 2, 29);
        const DateTime mar1 = datetime_make(year, 3, 1);
        snprintf(buffer, buffer_size, "%u is not a leap year: \"Feb 29\" should be the same second as Mar 1", year);
        mu_assert(!(feb29 < mar1) && !(feb29 > mar1) && (feb29 <= mar1) && (feb29 >= mar1), buffer);
    }
}

    /**
 * @brief Within a day and across a year end: seconds, minutes, hours and days order correctly.
 */
MU_TEST(datetime_time_of_day_and_year_end)
{
    mu_assert(datetime_strictly_before(datetime_make(2023, 6, 15, 10, 30, 0), datetime_make(2023, 6, 15, 10, 30, 1)), "One second later should come after");
    mu_assert(datetime_strictly_before(datetime_make(2023, 6, 15, 10, 30, 59), datetime_make(2023, 6, 15, 10, 31, 0)), "Next minute should come after second 59");
    mu_assert(datetime_strictly_before(datetime_make(2023, 6, 15, 10, 59, 59), datetime_make(2023, 6, 15, 11, 0, 0)), "Next hour should come after minute 59");
    mu_assert(datetime_strictly_before(datetime_make(2023, 6, 15, 23, 59, 59), datetime_make(2023, 6, 16, 0, 0, 0)), "Next day should come after 23:59:59");
    mu_assert(datetime_strictly_before(datetime_make(2023, 12, 31, 23, 59, 59), datetime_make(2024, 1, 1, 0, 0, 0)), "New year should come after Dec 31 23:59:59");
    mu_assert(datetime_strictly_before(datetime_make(2023, 12, 31), datetime_make(2024, 1, 1)), "A later year should come after even with an earlier day of year");
}

    /**
 * @brief Equality: the same date is equal, and == and != always give opposite answers
 *        (also for two dates that differ only by their day of the week).
 */
MU_TEST(datetime_equality)
{
    const DateTime a(10, 20, 8, 5, 2, 7, 2024);
    const DateTime same(10, 20, 8, 5, 2, 7, 2024);
    mu_assert(a == same && !(a != same), "Same date should be == and not !=");
    mu_assert(a <= same && a >= same && !(a < same) && !(a > same), "Same date should be <= and >= but not < or >");

    const DateTime otherWeek(10, 20, 8, 5, 4, 7, 2024);
    snprintf(buffer, buffer_size, "Dates differing only by week: == %d, != %d (should be opposite)", (a == otherWeek) ? 1 : 0, (a != otherWeek) ? 1 : 0);
    mu_assert((a == otherWeek) != (a != otherWeek), buffer);
}

    /**
 * @brief Out-of-range values are corrected as the constructor documents: year before 1980 becomes 1980,
 *        month 0 or 13 becomes 1, day 0 becomes 1.
 */
MU_TEST(datetime_out_of_range_values)
{
    DateTime early(0, 0, 0, 1, 0, 1, 1970);
    snprintf(buffer, buffer_size, "Year 1970 should become %u, got %u", DateTime::BaseYear, early.Year());
    mu_assert(early.Year() == DateTime::BaseYear, buffer);

    DateTime month0(0, 0, 0, 1, 0, 0, 2023);
    snprintf(buffer, buffer_size, "Month 0 should become 1, got %u", month0.Month());
    mu_assert(month0.Month() == 1, buffer);

    DateTime month13(0, 0, 0, 1, 0, 13, 2023);
    snprintf(buffer, buffer_size, "Month 13 should become 1, got %u", month13.Month());
    mu_assert(month13.Month() == 1, buffer);

    DateTime day0(0, 0, 0, 0, 0, 5, 2023);
    snprintf(buffer, buffer_size, "Day 0 should become 1, got %u", day0.Day());
    mu_assert(day0.Day() == 1, buffer);
}

    /**
 * @brief Backup unit date: fields map as documented (year - 1980, hour in "time"), and converting back gives the same date
 *        (the backup format has no seconds).
 */
MU_TEST(datetime_backup_unit_round_trip)
{
    DateTime d(0, 30, 14, 25, 3, 12, 2025);
    BupDate bup = d.ToBackupUnitDate();
    snprintf(buffer, buffer_size, "BupDate: year %u month %u day %u time %u min %u week %u (expected 45 12 25 14 30 3)",
        bup.year, bup.month, bup.day, bup.time, bup.min, bup.week);
    mu_assert(bup.year == 45 && bup.month == 12 && bup.day == 25 && bup.time == 14 && bup.min == 30 && bup.week == 3, buffer);

    DateTime back(&bup);
    char a[24], b[24];
    datetime_format(d, a, sizeof(a));
    datetime_format(back, b, sizeof(b));
    snprintf(buffer, buffer_size, "Backup unit round trip: %s became %s", a, b);
    mu_assert(back == d, buffer);
}

    /**
 * @brief Now() reads the RTC: every field is in range, and a second reading a little later is not earlier.
 */
MU_TEST(datetime_now)
{
    DateTime first = DateTime::Now();
    char a[24];
    datetime_format(first, a, sizeof(a));
    snprintf(buffer, buffer_size, "Now() = %s week %u: a field is out of range", a, first.Week());
    mu_assert(first.Year() >= DateTime::BaseYear && first.Year() <= 2099 &&
        first.Month() >= 1 && first.Month() <= 12 && first.Day() >= 1 && first.Day() <= 31 &&
        first.Hour() < 24 && first.Minute() < 60 && first.Second() < 60 && first.Week() < 7, buffer);

    for (int frame = 0; frame < 70; frame++) SRL::Core::Synchronize();     // a bit over one second

    DateTime second = DateTime::Now();
    char b[24];
    datetime_format(second, b, sizeof(b));
    snprintf(buffer, buffer_size, "Now() went from %s to %s about one second later (should not go back, should move)", a, b);
    mu_assert(second > first, buffer);
}

MU_TEST_SUITE(datetime_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&datetime_test_setup,
        &datetime_test_teardown,
        &datetime_test_output_header);

    MU_RUN_TEST(datetime_getters);
    MU_RUN_TEST(datetime_month_boundaries);
    MU_RUN_TEST(datetime_months_in_order);
    MU_RUN_TEST(datetime_leap_years);
    MU_RUN_TEST(datetime_time_of_day_and_year_end);
    MU_RUN_TEST(datetime_equality);
    MU_RUN_TEST(datetime_out_of_range_values);
    MU_RUN_TEST(datetime_backup_unit_round_trip);
    MU_RUN_TEST(datetime_now);
}

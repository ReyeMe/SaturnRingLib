
#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_datetime.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;

extern "C" {

    extern const uint8_t buffer_size;
    extern char buffer[];

    void datetime_test_setup(void) {
        // Setup for DateTime tests, if necessary
    }

    void datetime_test_teardown(void) {
        // Cleanup for DateTime tests, if necessary
    }

    // Test: Default initialization
    MU_TEST(datetime_test_default_initialization) {
        DateTime dt; // Assuming default constructor exists
        snprintf(buffer, buffer_size, "Default year is not %d", DateTime::BaseYear);
        mu_assert(dt.Year == DateTime::BaseYear, buffer);
        snprintf(buffer, buffer_size, "Default month is not 1");
        mu_assert(dt.Month == 1, buffer);
        snprintf(buffer, buffer_size, "Default day is not 1");
        mu_assert(dt.Day == 1, buffer);
        snprintf(buffer, buffer_size, "Default hour is not 0");
        mu_assert(dt.Hour == 0, buffer);
        snprintf(buffer, buffer_size, "Default minute is not 0");
        mu_assert(dt.Minute == 0, buffer);
        snprintf(buffer, buffer_size, "Default second is not 0");
        mu_assert(dt.Second == 0, buffer);
    }

    // Test: Initialization with specific values
    MU_TEST(datetime_test_custom_initialization) {
        DateTime dt(2023, 12, 31, 23, 59, 59); // Assuming custom constructor
        snprintf(buffer, buffer_size, "Year is not 2023");
        mu_assert(dt.Year == 2023, buffer);
        snprintf(buffer, buffer_size, "Month is not 12");
        mu_assert(dt.Month == 12, buffer);
        snprintf(buffer, buffer_size, "Day is not 31");
        mu_assert(dt.Day == 31, buffer);
        snprintf(buffer, buffer_size, "Hour is not 23");
        mu_assert(dt.Hour == 23, buffer);
        snprintf(buffer, buffer_size, "Minute is not 59");
        mu_assert(dt.Minute == 59, buffer);
        snprintf(buffer, buffer_size, "Second is not 59");
        mu_assert(dt.Second == 59, buffer);
    }

    // Test: Conversion from seconds since base year
    MU_TEST(datetime_test_from_seconds) {
        DateTime dt = DateTime::FromSeconds(946684800); // Assuming method exists
        snprintf(buffer, buffer_size, "Year is not 2000");
        mu_assert(dt.Year == 2000, buffer);
        snprintf(buffer, buffer_size, "Month is not 1");
        mu_assert(dt.Month == 1, buffer);
        snprintf(buffer, buffer_size, "Day is not 1");
        mu_assert(dt.Day == 1, buffer);
        snprintf(buffer, buffer_size, "Hour is not 0");
        mu_assert(dt.Hour == 0, buffer);
        snprintf(buffer, buffer_size, "Minute is not 0");
        mu_assert(dt.Minute == 0, buffer);
        snprintf(buffer, buffer_size, "Second is not 0");
        mu_assert(dt.Second == 0, buffer);
    }

    // Test: Conversion to seconds since base year
    MU_TEST(datetime_test_to_seconds) {
        DateTime dt(2000, 1, 1, 0, 0, 0);
        uint32_t seconds = dt.ToSeconds(); // Assuming method exists
        snprintf(buffer, buffer_size, "Seconds since base year is not 946684800");
        mu_assert(seconds == 946684800, buffer);
    }

    // Test: Leap year handling
    MU_TEST(datetime_test_leap_year) {
        DateTime dt(2020, 2, 29); // Valid leap year date
        snprintf(buffer, buffer_size, "Leap year date is invalid");
        mu_assert(dt.IsValid(), buffer); // Assuming IsValid checks date validity
    }

    // Test: Invalid date handling
    MU_TEST(datetime_test_invalid_date) {
        DateTime dt(2023, 2, 29); // Invalid date
        snprintf(buffer, buffer_size, "Invalid date did not fail");
        mu_assert(!dt.IsValid(), buffer); // Assuming IsValid checks date validity
    }

    // Test: Arithmetic - adding seconds
    MU_TEST(datetime_test_add_seconds) {
        DateTime dt(2000, 1, 1, 0, 0, 0);
        dt.AddSeconds(60); // Assuming AddSeconds modifies the DateTime
        snprintf(buffer, buffer_size, "Minute is not 1 after adding 60 seconds");
        mu_assert(dt.Minute == 1, buffer);
    }

    // Test: Comparison - earlier date
    MU_TEST(datetime_test_comparison_earlier) {
        DateTime dt1(2023, 1, 1);
        DateTime dt2(2023, 12, 31);
        snprintf(buffer, buffer_size, "Date comparison failed: dt1 is not earlier than dt2");
        mu_assert(dt1 < dt2, buffer); // Assuming operator< is implemented
    }

    MU_TEST_SUITE(datetime_test_suite) {
        MU_SUITE_CONFIGURE(&datetime_test_setup, &datetime_test_teardown);

        MU_RUN_TEST(datetime_test_default_initialization);
        MU_RUN_TEST(datetime_test_custom_initialization);
        MU_RUN_TEST(datetime_test_from_seconds);
        MU_RUN_TEST(datetime_test_to_seconds);
        MU_RUN_TEST(datetime_test_leap_year);
        MU_RUN_TEST(datetime_test_invalid_date);
        MU_RUN_TEST(datetime_test_add_seconds);
        MU_RUN_TEST(datetime_test_comparison_earlier);
    }
}

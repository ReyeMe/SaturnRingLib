#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Logger;

extern "C" {

	extern const uint8_t buffer_size;
	extern char buffer[];

	void fxp_test_setup(void) {
		// Nothomg to do here
	}

	void fxp_test_teardown(void) {
		/* Nothing */
	}

	MU_TEST(fxp_test_1) {
		Fxp a1 = Fxp::FromInt(0);
		snprintf(buffer, buffer_size, "%d != 0", a1);
		mu_assert(a1 == 0, buffer);
		// Note : 0==a1 will not compile
	}

	MU_TEST(fxp_test_2) {
		Fxp a1 = Fxp::FromInt(1);
		snprintf(buffer, buffer_size, "%d != 1", a1);
		mu_assert(a1 == 1, buffer);
	}

    MU_TEST(fxp_test_3) {
		Fxp a1 = Fxp::FromInt(1);
        Fxp b1 = a1;
		snprintf(buffer, buffer_size, "%d != 1", b1);
		mu_assert(b1 == 1, buffer);
	}

    MU_TEST(fxp_test_4) {
		Fxp a1 = Fxp::FromInt(1);
        Fxp b1(a1);
		snprintf(buffer, buffer_size, "%d != 1", b1);
		mu_assert(b1 == 1, buffer);
	}

    MU_TEST(fxp_test_5) {
		Fxp a1 = Fxp::FromInt(1);
        Fxp b1(a1);
		snprintf(buffer, buffer_size, "%d != %d", a1, b1);
		mu_assert(a1 == b1, buffer);
	}

    MU_TEST(fxp_test_6) {
		Fxp a1(10.0);
        Fxp b1(10.0f);
		snprintf(buffer, buffer_size, "%f != %f", a1, b1);
		mu_assert(a1 == b1, buffer);
	}

    MU_TEST(fxp_test_7) {
		Fxp a1(10.0);
        Fxp b1(20.0f);
		snprintf(buffer, buffer_size, "%f == %d", a1, b1);
		mu_assert(a1 != b1, buffer);
	}

    MU_TEST(fxp_test_8) {
		Fxp a1(10.0);
		snprintf(buffer, buffer_size, "%f > 20", a1);
		mu_assert(a1 <= 20, buffer);
	}

    MU_TEST(fxp_test_arithmetic_addition) {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 + a2;
        snprintf(buffer, buffer_size, "%f + %f != %f", a1, a2, result);
        mu_assert(result == Fxp(15.75), buffer);
    }

    MU_TEST(fxp_test_arithmetic_subtraction) {
        Fxp a1(10.5);
        Fxp a2(5.25);
        Fxp result = a1 - a2;
        snprintf(buffer, buffer_size, "%f - %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.25), buffer);
    }

    MU_TEST(fxp_test_arithmetic_multiplication) {
        Fxp a1(3.0);
        Fxp a2(4.0);
        Fxp result = a1 * a2;
        snprintf(buffer, buffer_size, "%f * %f != %f", a1, a2, result);
        mu_assert(result == Fxp(12.0), buffer);
    }

    MU_TEST(fxp_test_arithmetic_division) {
        Fxp a1(10.0);
        Fxp a2(2.0);
        Fxp result = a1 / a2;
        snprintf(buffer, buffer_size, "%f / %f != %f", a1, a2, result);
        mu_assert(result == Fxp(5.0), buffer);
    }

    MU_TEST(fxp_test_division_by_zero) {
        Fxp a1(10.0);
        Fxp a2(0.0);
        bool exceptionThrown = false;
        Fxp result = a1 / a2;
        snprintf(buffer, buffer_size, "Division by zero did not throw an exception");
        mu_assert(exceptionThrown, buffer);
    }

    // MU_TEST(fxp_test_precision) {
    //     Fxp a1(0.3333333);
    //     Fxp a2 = a1 * 3;
    //     snprintf(buffer, buffer_size, "Precision error: %f * 3 != 1", a1);
    //     mu_assert(a2 == Fxp(1.0), buffer);
    // }

    MU_TEST(fxp_test_conversion_to_float) {
        Fxp a1 = Fxp::FromInt(10);
        float result = a1.ToFloat();
        snprintf(buffer, buffer_size, "Conversion to float failed: %f != 10.0", result);
        mu_assert(result == 10.0f, buffer);
    }

    // MU_TEST(fxp_test_conversion_from_float) {
    //     float value = 20.75;
    //     Fxp a1 = Fxp(value);
    //     snprintf(buffer, buffer_size, "Conversion from float failed: %f != %f", a1.AsFloat(), value);
    //     mu_assert(a1.ToFloat() == value, buffer);
    // }

    MU_TEST(fxp_test_limits_max) {
        Fxp max = Fxp::MaxValue();
        snprintf(buffer, buffer_size, "Max value test failed: %f != max", max);
        mu_assert(max == Fxp(/* maximum representable value */), buffer);
    }

    MU_TEST(fxp_test_limits_min) {
        Fxp min = Fxp::MinValue();
        snprintf(buffer, buffer_size, "Min value test failed: %f != min", min);
        mu_assert(min == Fxp(/* minimum representable value */), buffer);
    }

	MU_TEST_SUITE(fxp_test_suite) {
		MU_SUITE_CONFIGURE(&fxp_test_setup, &fxp_test_teardown);

		MU_RUN_TEST(fxp_test_1);
		MU_RUN_TEST(fxp_test_2);
        MU_RUN_TEST(fxp_test_3);
        MU_RUN_TEST(fxp_test_4);
        MU_RUN_TEST(fxp_test_5);
        MU_RUN_TEST(fxp_test_6);
        MU_RUN_TEST(fxp_test_7);
        MU_RUN_TEST(fxp_test_8);
        MU_RUN_TEST(fxp_test_arithmetic_addition);
        MU_RUN_TEST(fxp_test_arithmetic_subtraction);
        MU_RUN_TEST(fxp_test_arithmetic_multiplication);
        MU_RUN_TEST(fxp_test_arithmetic_division);
        MU_RUN_TEST(fxp_test_division_by_zero);
        //MU_RUN_TEST(fxp_test_precision);
        MU_RUN_TEST(fxp_test_conversion_to_float);
        //MU_RUN_TEST(fxp_test_conversion_from_float);
        MU_RUN_TEST(fxp_test_limits_max);
        MU_RUN_TEST(fxp_test_limits_min);
	}
}

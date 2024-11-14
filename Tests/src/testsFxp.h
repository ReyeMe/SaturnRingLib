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

	}
}

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Logger;

extern "C" {

	extern const uint8_t buffer_size;
	extern char buffer[];

	void cd_test_setup(void) {
		// Nothomg to do here
	}

	void cd_test_teardown(void) {
		/* Nothing */
	}

	MU_TEST(cd_test_1) {
		// Fxp	  angle(0.0);
		// Angle a1 = Angle::FromDegrees(angle);
		// Fxp	  a2 = a1.AsRadians();
		// snprintf(buffer, buffer_size, "%d != %d", angle, a2);
		// mu_assert(MU_TEST(angle_test_2) {
		// Angle a1 = Angle::FromDegrees(Fxp(180.0));
		// Angle a2 = Angle::FromDegrees(Fxp(90.0));
        //
		// Angle a3 = a1 - a2;
        //
		// snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees());
        //
		// mu_assert(Angle::FromDegrees(Fxp(90.0)) == a3, buffer);
	}

	MU_TEST_SUITE(cd_test_suite) {
		MU_SUITE_CONFIGURE(&cd_test_setup, &cd_test_teardown);

		MU_RUN_TEST(cd_test_1);

	}
}

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Types;
using namespace SRL::Logger;

extern "C" {

	extern const uint8_t buffer_size;
	extern char buffer[];

	void angle_test_setup(void) {
		// Nothomg to do here
	}

	void angle_test_teardown(void) {
		/* Nothing */
	}

	MU_TEST(angle_test_1) {
		Fxp	  angle(0.0);
		Angle a1 = Angle::FromDegrees(angle);
		Fxp	  a2 = a1.AsRadians();
		snprintf(buffer, buffer_size, "%d != %d", angle, a2);
		mu_assert(angle == a2, buffer);
	}

	MU_TEST(angle_test_2) {
		Angle a1 = Angle::FromDegrees(Fxp(180.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a1 - a2;

		snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(90.0)) == a3, buffer);
	}

	MU_TEST(angle_test_3) {
		Angle a1 = Angle::FromDegrees(Fxp(0.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a1 - a2;

		snprintf(buffer, buffer_size, "%d != -90", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(-90.0)) == a3, buffer);
	}

	MU_TEST(angle_test_4) {
		Angle a1 = Angle::FromDegrees(Fxp(0.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a2 - a1;

		snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(90.0)) == a3, buffer);
	}

	MU_TEST(angle_test_5) {
		Angle a1 = Angle::FromDegrees(Fxp(360.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a1 - a2;

		snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(270.0)) == a3, buffer);
	}

	MU_TEST(angle_test_6) {
		Angle a1 = Angle::FromDegrees(Fxp(720.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a1 - a2;

		snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(270.0)) == a3, buffer);
	}

	MU_TEST(angle_test_7) {
		Angle a1 = Angle::FromDegrees(Fxp(720.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a2 - a1;

		snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(270.0)) == a3, buffer);
	}

	MU_TEST(angle_test_8) {
		Angle a1 = Angle::FromDegrees(Fxp(90.0));
		Angle a2 = Angle::FromDegrees(Fxp(90.0));

		Angle a3 = a1 + a2;

		snprintf(buffer, buffer_size, "%d != 180", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(180.0)) == a3, buffer);
	}

	MU_TEST(angle_test_9) {
		Angle a1 = Angle::FromDegrees(Fxp(180.0));
		Angle a2 = Angle::FromDegrees(Fxp(180.0));

		Angle a3 = a1 + a2;

		snprintf(buffer, buffer_size, "%d != 360", a3.AsDegrees());

		mu_assert(Angle::FromDegrees(Fxp(360.0)) == a3, buffer);
	}

    MU_TEST(angle_test_10) {
        Angle a1 = Angle::FromDegrees(180);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a1 - a2;

        snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees());

    }

    MU_TEST(angle_test_11) {
        Angle a1 = Angle::FromDegrees(0);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a1 - a2;

        snprintf(buffer, buffer_size, "%d != -90", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(-90.0) == a3, buffer);
    }

    MU_TEST(angle_test_12) {
        Angle a1 = Angle::FromDegrees(0);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a2 - a1;

        snprintf(buffer, buffer_size, "%d != 90", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(90) == a3, buffer);
    }

    MU_TEST(angle_test_13) {
        Angle a1 = Angle::FromDegrees(360);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a1 - a2;

        snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(270) == a3, buffer);
    }

    MU_TEST(angle_test_14) {
        Angle a1 = Angle::FromDegrees(720);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a1 - a2;

        snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(270) == a3, buffer);
    }

    MU_TEST(angle_test_15) {
        Angle a1 = Angle::FromDegrees(720);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a2 - a1;

        snprintf(buffer, buffer_size, "%d != 270", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(270) == a3, buffer);
    }

    MU_TEST(angle_test_16) {
        Angle a1 = Angle::FromDegrees(90);
        Angle a2 = Angle::FromDegrees(90);

        Angle a3 = a1 + a2;

        snprintf(buffer, buffer_size, "%d != 180", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(180) == a3, buffer);
    }

    MU_TEST(angle_test_17) {
        Angle a1 = Angle::FromDegrees(180);
        Angle a2 = Angle::FromDegrees(180);

        Angle a3 = a1 + a2;

        snprintf(buffer, buffer_size, "%d != 360", a3.AsDegrees());

        mu_assert(Angle::FromDegrees(360) == a3, buffer);
    }

	MU_TEST_SUITE(angle_test_suite) {
		MU_SUITE_CONFIGURE(&angle_test_setup, &angle_test_teardown);

		MU_RUN_TEST(angle_test_1);
		MU_RUN_TEST(angle_test_2);
		MU_RUN_TEST(angle_test_3);
		MU_RUN_TEST(angle_test_4);
		MU_RUN_TEST(angle_test_5);
		MU_RUN_TEST(angle_test_6);
		MU_RUN_TEST(angle_test_7);
		MU_RUN_TEST(angle_test_8);
		MU_RUN_TEST(angle_test_9);
        MU_RUN_TEST(angle_test_10);
        MU_RUN_TEST(angle_test_11);
        MU_RUN_TEST(angle_test_12);
        MU_RUN_TEST(angle_test_13);
        MU_RUN_TEST(angle_test_14);
        MU_RUN_TEST(angle_test_15);
        MU_RUN_TEST(angle_test_16);
        MU_RUN_TEST(angle_test_17);

	}
}

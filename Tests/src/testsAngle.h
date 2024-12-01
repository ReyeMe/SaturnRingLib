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

    MU_TEST(angle_test_normalization_positive) {
        Angle a1 = Angle::FromDegrees(Fxp(450.0)); // 450 degrees should normalize to 90 degrees
        Angle normalized = a1.Normalize();
        snprintf(buffer, buffer_size, "Normalization failed: %d != 90", normalized.AsDegrees());
        mu_assert(normalized.AsDegrees() == 90.0, buffer);
    }

    MU_TEST(angle_test_normalization_negative) {
        Angle a1 = Angle::FromDegrees(Fxp(-90.0)); // -90 degrees should normalize to 270 degrees
        Angle normalized = a1.Normalize();
        snprintf(buffer, buffer_size, "Normalization failed: %d != 270", normalized.AsDegrees());
        mu_assert(normalized.AsDegrees() == 270.0, buffer);
    }

    MU_TEST(angle_test_arithmetic_addition) {
        Angle a1 = Angle::FromDegrees(Fxp(45.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 + a2;
        snprintf(buffer, buffer_size, "Addition failed: %d != 75", result.AsDegrees());
        mu_assert(result.AsDegrees() == 75.0, buffer);
    }

    MU_TEST(angle_test_arithmetic_subtraction) {
        Angle a1 = Angle::FromDegrees(Fxp(90.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 - a2;
        snprintf(buffer, buffer_size, "Subtraction failed: %d != 60", result.AsDegrees());
        mu_assert(result.AsDegrees() == 60.0, buffer);
    }

    MU_TEST(angle_test_arithmetic_multiplication) {
        Angle a1 = Angle::FromDegrees(Fxp(30.0));
        Angle result = a1 * 2; // Assuming multiplication is supported
        snprintf(buffer, buffer_size, "Multiplication failed: %d != 60", result.AsDegrees());
        mu_assert(result.AsDegrees() == 60.0, buffer);
    }

    MU_TEST(angle_test_arithmetic_division) {
        Angle a1 = Angle::FromDegrees(Fxp(60.0));
        Angle result = a1 / 2; // Assuming division is supported
        snprintf(buffer, buffer_size, "Division failed: %d != 30", result.AsDegrees());
        mu_assert(result.AsDegrees() == 30.0, buffer);
    }

    MU_TEST(angle_test_comparison_greater) {
        Angle a1 = Angle::FromDegrees(Fxp(90.0));
        Angle a2 = Angle::FromDegrees(Fxp(30.0));
        snprintf(buffer, buffer_size, "Comparison failed: 90 <= 30");
        mu_assert(a1 > a2, buffer);
    }

    MU_TEST(angle_test_comparison_less) {
        Angle a1 = Angle::FromDegrees(Fxp(30.0));
        Angle a2 = Angle::FromDegrees(Fxp(90.0));
        snprintf(buffer, buffer_size, "Comparison failed: 30 >= 90");
        mu_assert(a1 < a2, buffer);
    }

    MU_TEST(angle_test_conversion_to_radians) {
        Angle a1 = Angle::FromDegrees(Fxp(180.0));
        Fxp radians = a1.AsRadians();
        snprintf(buffer, buffer_size, "Conversion to radians failed: %f != 3.14159", radians);
        mu_assert(fabs(radians - 3.14159) < 1e-5, buffer);
    }

    MU_TEST(angle_test_conversion_to_degrees) {
        Angle a1 = Angle::FromRadians(Fxp(3.14159));
        Fxp degrees = a1.AsDegrees();
        snprintf(buffer, buffer_size, "Conversion to degrees failed: %f != 180", degrees);
        mu_assert(fabs(degrees - 180.0) < 1e-5, buffer);
    }

    MU_TEST(angle_test_edge_case_zero) {
        Angle a1 = Angle::FromDegrees(Fxp(0.0));
        snprintf(buffer, buffer_size, "Zero angle failed: %d != 0", a1.AsDegrees());
        mu_assert(a1.AsDegrees() == 0.0, buffer);
    }

    MU_TEST(angle_test_edge_case_full_circle) {
        Angle a1 = Angle::FromDegrees(Fxp(360.0));
        snprintf(buffer, buffer_size, "Full circle failed: %d != 0", a1.AsDegrees());
        mu_assert(a1.Normalize().AsDegrees() == 0.0, buffer);
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

        // Additional tests
        MU_RUN_TEST(angle_test_normalization_positive);
        MU_RUN_TEST(angle_test_normalization_negative);
        MU_RUN_TEST(angle_test_arithmetic_addition);
        MU_RUN_TEST(angle_test_arithmetic_subtraction);
        MU_RUN_TEST(angle_test_arithmetic_multiplication);
        MU_RUN_TEST(angle_test_arithmetic_division);
        MU_RUN_TEST(angle_test_comparison_greater);
        MU_RUN_TEST(angle_test_comparison_less);
        MU_RUN_TEST(angle_test_conversion_to_radians);
        MU_RUN_TEST(angle_test_conversion_to_degrees);
        MU_RUN_TEST(angle_test_edge_case_zero);
        MU_RUN_TEST(angle_test_edge_case_full_circle);
	}
}

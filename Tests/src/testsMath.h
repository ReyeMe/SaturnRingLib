
#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_math.hpp"
#include "srl_angle.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Types;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    void math_test_setup(void)
    {
        // Setup for Math tests, if necessary
    }

    void math_test_teardown(void)
    {
        // Cleanup for Math tests, if necessary
    }

    // Test: Sin function for standard angles
    MU_TEST(math_test_sin_standard_angles)
    {
        Fxp sin_0 = Math::Sin(Angle::FromDegrees(0));
        Fxp sin_90 = Math::Sin(Angle::FromDegrees(90));
        Fxp sin_180 = Math::Sin(Angle::FromDegrees(180));
        Fxp sin_360 = Math::Sin(Angle::FromDegrees(360));

        snprintf(buffer, buffer_size, "Sin(0) failed: %f != 0.0", sin_0.ToFloat());
        mu_assert(fabs(sin_0.ToFloat() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(90) failed: %f != 1.0", sin_90.ToFloat());
        mu_assert(fabs(sin_90.ToFloat() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(180) failed: %f != 0.0", sin_180.ToFloat());
        mu_assert(fabs(sin_180.ToFloat() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Sin(360) failed: %f != 0.0", sin_360.ToFloat());
        mu_assert(fabs(sin_360.ToFloat() - 0.0) < 1e-5, buffer);
    }

    // Test: Cos function for standard angles
    MU_TEST(math_test_cos_standard_angles)
    {
        Fxp cos_0 = Math::Cos(Angle::FromDegrees(0));
        Fxp cos_90 = Math::Cos(Angle::FromDegrees(90));
        Fxp cos_180 = Math::Cos(Angle::FromDegrees(180));
        Fxp cos_360 = Math::Cos(Angle::FromDegrees(360));

        snprintf(buffer, buffer_size, "Cos(0) failed: %f != 1.0", cos_0.ToFloat());
        mu_assert(fabs(cos_0.ToFloat() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(90) failed: %f != 0.0", cos_90.ToFloat());
        mu_assert(fabs(cos_90.ToFloat() - 0.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(180) failed: %f != -1.0", cos_180.ToFloat());
        mu_assert(fabs(cos_180.ToFloat() - -1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(360) failed: %f != 1.0", cos_360.ToFloat());
        mu_assert(fabs(cos_360.ToFloat() - 1.0) < 1e-5, buffer);
    }

    // Test: Sin and Cos for negative angles
    MU_TEST(math_test_negative_angles)
    {
        Fxp sin_neg90 = Math::Sin(Angle::FromDegrees(-90));
        Fxp cos_neg90 = Math::Cos(Angle::FromDegrees(-90));

        snprintf(buffer, buffer_size, "Sin(-90) failed: %f != -1.0", sin_neg90.ToFloat());
        mu_assert(fabs(sin_neg90.ToFloat() - -1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(-90) failed: %f != 0.0", cos_neg90.ToFloat());
        mu_assert(fabs(cos_neg90.ToFloat() - 0.0) < 1e-5, buffer);
    }

    // Test: Sin and Cos for large angles
    MU_TEST(math_test_large_angles)
    {
        Fxp sin_large = Math::Sin(Angle::FromDegrees(450)); // 450° = 90° normalized
        Fxp cos_large = Math::Cos(Angle::FromDegrees(450));

        snprintf(buffer, buffer_size, "Sin(450) failed: %f != 1.0", sin_large.ToFloat());
        mu_assert(fabs(sin_large.ToFloat() - 1.0) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(450) failed: %f != 0.0", cos_large.ToFloat());
        mu_assert(fabs(cos_large.ToFloat() - 0.0) < 1e-5, buffer);
    }

    // Test: Sin and Cos for small angles
    MU_TEST(math_test_small_angles)
    {
        Fxp sin_small = Math::Sin(Angle::FromDegrees(0.1));
        Fxp cos_small = Math::Cos(Angle::FromDegrees(0.1));

        snprintf(buffer, buffer_size, "Sin(0.1) precision check failed");
        mu_assert(fabs(sin_small.ToFloat() - 0.00174533) < 1e-5, buffer);

        snprintf(buffer, buffer_size, "Cos(0.1) precision check failed");
        mu_assert(fabs(cos_small.ToFloat() - 0.999998) < 1e-5, buffer);
    }

    MU_TEST_SUITE(math_test_suite)
    {
        MU_SUITE_CONFIGURE(&math_test_setup, &math_test_teardown);

        MU_RUN_TEST(math_test_sin_standard_angles);
        MU_RUN_TEST(math_test_cos_standard_angles);
        MU_RUN_TEST(math_test_negative_angles);
        MU_RUN_TEST(math_test_large_angles);
        MU_RUN_TEST(math_test_small_angles);
    }
}

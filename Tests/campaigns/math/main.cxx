// Tests/campaigns/math/main.cxx
// Pure logic: fixed point, angles, math types, strings. No hardware state.
#include "campaign.hpp"

#include "testsAngle.hpp"
#include "testsFxp.hpp"
#include "testsMath.hpp"
#include "testsHighColor.hpp"
#include "testsString.hpp"
#include "testsTrigonometry.hpp"
#include "testsVector2D.hpp"
#include "testsVector3D.hpp"
#include "testsMatrixStack.hpp"
#include "testsPrecision.hpp"
#include "testsRandom.hpp"
#include "testsSortOrder.hpp"
#include "testsUtils.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(angle_test_suite);
    RUN_AND_DISPLAY_SUITE(fxp_test_suite);
    RUN_AND_DISPLAY_SUITE(math_test_suite);
    RUN_AND_DISPLAY_SUITE(highcolor_test_suite);
    RUN_AND_DISPLAY_SUITE(string_test_suite);
    RUN_AND_DISPLAY_SUITE(trigonometry_test_suite);
    RUN_AND_DISPLAY_SUITE(vector2d_test_suite);
    RUN_AND_DISPLAY_SUITE(vector3d_test_suite);
    RUN_AND_DISPLAY_SUITE(matrix_stack_test_suite);
    RUN_AND_DISPLAY_SUITE(precision_test_suite);
    RUN_AND_DISPLAY_SUITE(random_test_suite);
    RUN_AND_DISPLAY_SUITE(sort_order_test_suite);
    RUN_AND_DISPLAY_SUITE(utils_test_suite);

    EndCampaign();
}

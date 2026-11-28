// Tests/campaigns/transform/main.cxx
// Scene3D matrix operations: the matrix stack, translate, rotate, scale, multiply, invert, transpose and LookAt.
#include "campaign.hpp"

#include "testsTransform.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(transform_test_suite);

    EndCampaign();
}

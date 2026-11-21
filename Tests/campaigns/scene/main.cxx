// Tests/campaigns/scene/main.cxx
// 3D meshes and their attributes, 2D sprite attributes, and Scene3D projection, visibility and drawing.
#include "campaign.hpp"

#include "testsScene.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(scene_test_suite);

    EndCampaign();
}

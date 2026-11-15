// Tests/campaigns/draw2d/main.cxx
// Scene2D drawing: sprite effects, attribute and command words, and the VDP1 commands lines, polygons, clipping and
// sprites put in VDP1 VRAM.
#include "campaign.hpp"

#include "testsDraw2D.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(draw2d_test_suite);

    EndCampaign();
}

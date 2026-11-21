// Tests/campaigns/fld_game/main.cxx
// File loader game, started by the fld campaign (not a campaign of its own): checks what the loader set up.
#include "campaign.hpp"

#include "testsFld.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(fld_game_test_suite);

    EndCampaign();
}

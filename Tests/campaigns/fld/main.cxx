// Tests/campaigns/fld/main.cxx
// File loader menu (see Tests/src/testsFld.hpp): checks, then starts campaigns/fld_game with Fld::ExecGame;
// fld_game reports the rest and ends the campaign.
#include "campaign.hpp"

#include "testsFld.hpp"

int main()
{
    Fld::InitializeMenu();
    BeginCampaign();

    // Does not come back when the game starts
    RUN_AND_DISPLAY_SUITE(fld_menu_test_suite);

    EndCampaign();
}

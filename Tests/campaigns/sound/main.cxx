// Tests/campaigns/sound/main.cxx
// Sound: PCM channels with the SGL sound driver, and raw PCM loading.
#include "campaign.hpp"

#include "testsSound.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(sound_test_suite);

    EndCampaign();
}

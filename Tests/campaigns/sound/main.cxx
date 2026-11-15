// Tests/campaigns/sound/main.cxx
// Sound: PCM channels with the SGL sound driver, raw PCM loading, CD audio (Cdda) and Cinepak movies.
#include "campaign.hpp"

#include "testsSound.hpp"
#include "testsCinepak.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(sound_test_suite);
    RUN_AND_DISPLAY_SUITE(cdda_test_suite);
    RUN_AND_DISPLAY_SUITE(cinepak_test_suite);

    EndCampaign();
}

// Tests/campaigns/media/main.cxx
// Loading media files from the disc: TGA images, WAV sounds and CubeTile tilemaps. The files come from make_data.py.
#include "campaign.hpp"

#include "testsCubeTile.hpp"
#include "testsTGA.hpp"
#include "testsWave.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(wave_test_suite);
    RUN_AND_DISPLAY_SUITE(cubetile_test_suite);
    // Last: 16-color RLE decoding may write past its buffer
    RUN_AND_DISPLAY_SUITE(tga_test_suite);

    EndCampaign();
}

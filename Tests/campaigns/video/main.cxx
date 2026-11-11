// Tests/campaigns/video/main.cxx
// VDP1/VDP2: ASCII, color RAM, bitmaps, SGL.
#include "campaign.hpp"

#include "testsASCII.hpp"
#include "testsCRAM.hpp"
#include "testsBitmap.hpp"
#include "testsBase.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(ascii_test_suite);
    RUN_AND_DISPLAY_SUITE(cram_test_suite);
    RUN_AND_DISPLAY_SUITE(bitmap_test_suite);
    RUN_AND_DISPLAY_SUITE(base_test_suite);

    EndCampaign();
}

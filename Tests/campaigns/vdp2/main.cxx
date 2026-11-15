// Tests/campaigns/vdp2/main.cxx
// VDP2: scroll, sprite and rotation settings, VRAM allocator, tilemap conversion and loading, ClearVRAM, TV resolution. Uses NBG1 only: NBG3 shows the results.
#include "campaign.hpp"

#include "testsVDP2.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(vdp2_settings_test_suite);
    RUN_AND_DISPLAY_SUITE(vdp2_test_suite);

    EndCampaign();
}

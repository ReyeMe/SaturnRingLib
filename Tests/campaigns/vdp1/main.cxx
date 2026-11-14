// Tests/campaigns/vdp1/main.cxx
// VDP1 texture heap: allocation, loading, limits and reset.
#include "campaign.hpp"

#include "testsVDP1.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(vdp1_test_suite);

    EndCampaign();
}

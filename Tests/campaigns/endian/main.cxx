// Tests/campaigns/endian/main.cxx
// SRL::Endian: reading little-endian numbers from byte buffers.
#include "campaign.hpp"

#include "testsEndian.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(endian_test_suite);

    EndCampaign();
}

// Tests/campaigns/input/main.cxx
// Input: digital and analog pads, read from pad data the tests write themselves.
#include "campaign.hpp"

#include "testsInput.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(input_test_suite);

    EndCampaign();
}

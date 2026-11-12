// Tests/campaigns/timer/main.cxx
// Timers: FRT tickstamps, dividers, conversions and overflow (moved out of system).
#include "campaign.hpp"

#include "testsTimer.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(test_timer_suite);

    EndCampaign();
}

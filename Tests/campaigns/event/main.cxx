// Tests/campaigns/event/main.cxx
// Events: SRL::Types::Event and MemberProxy, and the Core sync/v-blank events.
#include "campaign.hpp"

#include "testsEvent.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(event_test_suite);

    EndCampaign();
}

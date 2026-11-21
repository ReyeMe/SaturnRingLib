// Tests/campaigns/datetime/main.cxx
// Date and time: SRL::Types::DateTime ordering, leap years, backup unit dates and the RTC.
#include "campaign.hpp"

#include "testsDateTime.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(datetime_test_suite);

    EndCampaign();
}

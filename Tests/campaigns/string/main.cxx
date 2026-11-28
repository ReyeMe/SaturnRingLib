// Tests/campaigns/string/main.cxx
// SRL::string: construction, copies, moves, concatenation and snprintfEx formatting; SRL::Logger output.
#include "campaign.hpp"

#include "testsString.hpp"
#include "testsLog.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(string_test_suite);
    RUN_AND_DISPLAY_SUITE(log_test_suite);

    EndCampaign();
}

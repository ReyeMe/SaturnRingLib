// Tests/campaigns/string/main.cxx
// SRL::string: construction, copies, moves, concatenation and snprintfEx formatting.
#include "campaign.hpp"

#include "testsString.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(string_test_suite);

    EndCampaign();
}

// Tests/campaigns/cd/main.cxx
// CD block and file system. Needs the fixtures in cd/data/.
#include "campaign.hpp"

#include "testsCD.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(cd_test_suite);

    EndCampaign();
}

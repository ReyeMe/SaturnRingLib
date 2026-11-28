// Tests/campaigns/slave/main.cxx
// Tasks on the slave CPU (SRL::Slave, ITask). Its own campaign: a stuck slave CPU hangs the whole run.
#include "campaign.hpp"

#include "testsSlave.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(slave_test_suite);

    EndCampaign();
}

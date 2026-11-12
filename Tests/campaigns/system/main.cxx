// Tests/campaigns/system/main.cxx
// Interrupts, system registers, SCU DSP. Changes global CPU state. Timers are in the timer campaign.
#include "campaign.hpp"

#include "testsInterrupt.hpp"
#include "testsSystem.hpp"
#include "testDSP.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(interrupt_test_suite);
    RUN_AND_DISPLAY_SUITE(system_test_suite);
    RUN_AND_DISPLAY_SUITE(dsp_test_suite);

    EndCampaign();
}

// Tests/campaigns/system/main.cxx
// Interrupts, timers, system registers, SCU DSP. Changes global CPU state.
#include "campaign.hpp"

#include "testsInterrupt.hpp"
#include "testsTimer.hpp"
#include "testsSystem.hpp"
#include "testDSP.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(interrupt_test_suite);
    RUN_AND_DISPLAY_SUITE(test_timer_suite);
    RUN_AND_DISPLAY_SUITE(system_test_suite);
    RUN_AND_DISPLAY_SUITE(dsp_test_suite);

    EndCampaign();
}

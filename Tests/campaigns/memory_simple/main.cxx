// Tests/campaigns/memory_simple/main.cxx
// Allocators and memory zones, built with the SIMPLE allocator. memory_tlsf runs the same suites with TLSF.
#include "campaign.hpp"

#include "testsMemoryCommon.hpp"
#include "testsMemoryHWRam.hpp"
#include "testsMemoryLWRam.hpp"
#include "testsMemory.hpp"
#include "testsMemoryCartRam.hpp"

int main()
{
    BeginCampaign();

    // First, on a clean heap: other suites can leak blocks when an assert fails
    RUN_AND_DISPLAY_SUITE(memory_common_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_HWRam_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_LWRam_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

    // Last: misuse and two-CPU tests can corrupt the heap or hang
    RUN_AND_DISPLAY_SUITE(memory_common_risky_test_suite);

    EndCampaign();
}

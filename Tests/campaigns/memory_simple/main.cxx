// Tests/campaigns/memory/main.cxx
// Allocators and memory zones. Build with SRL_MALLOC_METHOD=TLSF to test the default allocator.
#include "campaign.hpp"

#include "testsMemoryHWRam.hpp"
#include "testsMemoryLWRam.hpp"
#include "testsMemory.hpp"
#include "testsMemoryCartRam.hpp"

int main()
{
    BeginCampaign();

    RUN_AND_DISPLAY_SUITE(memory_HWRam_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_LWRam_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

    EndCampaign();
}

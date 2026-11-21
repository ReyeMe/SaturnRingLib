// Tests/campaigns/memory_cpus_simple/main.cxx
// Master and slave CPUs allocating at the same time, on a fresh heap, with the SIMPLE allocator.
// Its own campaign: it can break the heap or leave the slave stuck. memory_cpus_tlsf runs it with TLSF.
#include "campaign.hpp"

#include "testsMemoryCommon.hpp"

int main()
{
    BeginCampaign();
    RUN_AND_DISPLAY_SUITE(memory_common_cpus_test_suite);
    EndCampaign();
}

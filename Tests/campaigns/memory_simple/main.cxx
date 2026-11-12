// Tests/campaigns/memory_simple/main.cxx
// Allocators and memory zones, built with the SIMPLE allocator. memory_tlsf runs the same suites with TLSF.
#include "campaign.hpp"

#include "testsMemoryCommon.hpp"
#include "testsMemory.hpp"
#include "testsMemoryCartRam.hpp"

/**
 * @brief Tests that leave a heap unusable or reset it, run after everything else.
 *        Under SIMPLE an exact-fit Malloc marks the block used but returns null;
 *        Memory::Initialize resets both heaps and zeroes memory up to SGL's work area.
 */
MU_TEST_SUITE(memory_last_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&memory_common_test_setup,
        &memory_common_test_teardown,
        &memory_common_test_output_header);

    MU_RUN_TEST(memory_common_largest_block_lwram);
    MU_RUN_TEST(memory_common_largest_block_hwram);
    MU_RUN_TEST(memory_test_initialize_zones);
}

int main()
{
    BeginCampaign();

    // First, on a clean heap
    RUN_AND_DISPLAY_SUITE(memory_common_test_suite);
    // Misuse tests: before the other suites, in case one of them still leaks
    RUN_AND_DISPLAY_SUITE(memory_common_risky_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_test_suite);
    RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

    // Last: exact-fit allocations and Memory::Initialize
    RUN_AND_DISPLAY_SUITE(memory_last_test_suite);

    EndCampaign();
}

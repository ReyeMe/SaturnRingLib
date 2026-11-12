#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_memory.hpp"

// https://github.com/siu/minunit
#include "minunit.h"
#include "testsMemoryCommon.hpp"

using namespace SRL;

extern "C" {

extern const uint8_t buffer_size;
extern char buffer[];

    /** @brief Setup and teardown: the shared leak check (see testsMemoryCommon.hpp). */
void memory_test_setup(void) { memory_leak_check_setup(); }
void memory_test_teardown(void) { memory_leak_check_teardown(); }

    /**
 * @brief Output header for test suite error reporting
 *
 * This function is called on the first test failure to print
 * a header indicating that memory unit test errors have occurred.
 * It increments a global error counter to ensure the header
 * is printed only once per test suite run.
 */
void memory_test_output_header(void)
{
        // Print error header only on the first test failure
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_MEMORY****");
        }
        else
        {
            LogInfo("****UT_MEMORY_ERROR(S)****");
        }
    }
}

    /**
 * @brief Test PlacementMalloc with address in HighWorkRam
 *
 * Verifies that PlacementMalloc allocates memory correctly in HighWorkRam.
 */
MU_TEST(memory_test_placement_malloc_highworkram)
{
    void* address = (void*)0x06000000; // Address in HighWorkRam range
    void* ptr = Memory::PlacementMalloc(100, address);
    mu_assert(ptr != nullptr, "PlacementMalloc in HighWorkRam failed");

    Memory::Free(ptr);
}

    /**
 * @brief Test PlacementMalloc with address in LowWorkRam
 *
 * Verifies that PlacementMalloc allocates memory correctly in LowWorkRam.
 */
MU_TEST(memory_test_placement_malloc_lowworkram)
{
    void* address = (void*)0x00200000; // Address in LowWorkRam range
    void* ptr = Memory::PlacementMalloc(100, address);
    mu_assert(ptr != nullptr, "PlacementMalloc in LowWorkRam failed");

    Memory::Free(ptr);
}

    /**
 * @brief Test PlacementMalloc with address in CartRam
 *
 * Verifies that PlacementMalloc allocates memory correctly in CartRam.
 */
MU_TEST(memory_test_placement_malloc_cartram)
{
    // No cartridge RAM: covered by memory_common_cartram_present_or_absent
    if (Memory::CartRam::GetSize() == 0) return;

    void* address = (void*)0x08000000; // Address in CartRam range
    void* ptr = Memory::PlacementMalloc(100, address);
    Memory::Free(ptr);
    mu_assert(ptr != nullptr, "PlacementMalloc in CartRam failed");
}

    /**
 * @brief Test PlacementMalloc with invalid address
 *
 * Verifies that PlacementMalloc returns NULL for an invalid address.
 */
MU_TEST(memory_test_placement_malloc_invalid)
{
    void* address = (void*)0xFFFFFFFF; // Invalid address
    void* ptr = Memory::PlacementMalloc(100, address);
    mu_assert(ptr == nullptr, "PlacementMalloc with invalid address did not return NULL");
}

    /**
 * @brief Test proper initialization of memory zones
 *
 * Verifies that memory zones are properly initialized.
 */
MU_TEST(memory_test_initialize_zones)
{
    SRL::Memory::Initialize();
    mu_assert(SRL::Memory::HighWorkRam::GetSize() > 0, "HighWorkRam initialization failed");
    mu_assert(SRL::Memory::LowWorkRam::GetSize() > 0, "LowWorkRam initialization failed");
}

    /**
 * @brief Test cross-zone memory allocation
 *
 * Verifies behavior when allocating memory across different zones.
 */
MU_TEST(memory_test_cross_zone_allocation)
{
    const bool hasCartRam = Memory::CartRam::GetSize() > 0;
    void* ptr1 = new (SRL::Memory::Zone::HWRam) char[100];
    void* ptr2 = new (SRL::Memory::Zone::LWRam) char[100];
    void* ptr3 = hasCartRam ? new (SRL::Memory::Zone::CartRam) char[100] : nullptr;

    delete[] static_cast<char*>(ptr1);
    delete[] static_cast<char*>(ptr2);
    delete[] static_cast<char*>(ptr3);

    mu_assert(ptr1 != nullptr, "Cross-zone allocation in HighWorkRam failed");
    mu_assert(ptr2 != nullptr, "Cross-zone allocation in LowWorkRam failed");
    mu_assert(!hasCartRam || ptr3 != nullptr, "Cross-zone allocation in CartRam failed");
}

    /**
 * @brief Test moving memory blocks between zones
 *
 * Verifies that memory blocks can be moved between different zones
 * and that the data integrity is maintained.
 */
MU_TEST(memory_test_move_memory_blocks)
{
        // Allocate memory in HighWorkRam and LowWorkRam
    char* srcPtr = new (SRL::Memory::Zone::HWRam) char[100];
    char* destPtr = new (SRL::Memory::Zone::LWRam) char[100];
    bool intact = srcPtr != nullptr && destPtr != nullptr;

    if (intact)
    {
        for (int i = 0; i < 100; ++i)
        {
            srcPtr[i] = static_cast<char>(i);
        }

            // Move data from HighWorkRam to LowWorkRam
        memcpy(destPtr, srcPtr, 100);

            // Verify data integrity
        for (int i = 0; i < 100; ++i)
        {
            intact = intact && destPtr[i] == static_cast<char>(i);
        }
    }

        // Clean up
    delete[] srcPtr;
    delete[] destPtr;

    mu_assert(srcPtr != nullptr && destPtr != nullptr, "Allocation failed before moving memory block");
    mu_assert(intact, "Data integrity check failed after moving memory block");
}

    /**
 * @brief Test moving memory blocks of various sizes between zones
 *
 * Verifies that memory blocks of different sizes can be moved between zones
 * and that the data integrity is maintained.
 */
MU_TEST(memory_test_move_memory_blocks_various_sizes)
{
    const size_t sizes[] = {1, 10, 50, 100, 200};
    for (size_t size : sizes)
    {
            // Allocate memory in HighWorkRam and LowWorkRam
        char* srcPtr = new (SRL::Memory::Zone::HWRam) char[size];
        char* destPtr = new (SRL::Memory::Zone::LWRam) char[size];
        bool intact = srcPtr != nullptr && destPtr != nullptr;

        if (intact)
        {
            for (size_t i = 0; i < size; ++i)
            {
                srcPtr[i] = static_cast<char>(i);
            }

                // Move data from HighWorkRam to LowWorkRam
            memcpy(destPtr, srcPtr, size);

                // Verify data integrity
            for (size_t i = 0; i < size; ++i)
            {
                intact = intact && destPtr[i] == static_cast<char>(i);
            }
        }

            // Clean up
        delete[] srcPtr;
        delete[] destPtr;

        snprintf(buffer, buffer_size, "Moving a %lu-byte block: %s", (unsigned long)size,
            srcPtr == nullptr || destPtr == nullptr ? "allocation failed" : "data integrity check failed");
        mu_assert(intact, buffer);
    }
}

    /**
 * @brief Test moving memory blocks with edge cases
 *
 * Verifies that memory blocks can be moved between zones in edge cases
 * such as zero size and maximum size.
 */
MU_TEST(memory_test_move_memory_blocks_edge_cases)
{
        // Edge case: zero size
    char* srcPtr = new (SRL::Memory::Zone::HWRam) char[0];
    char* destPtr = new (SRL::Memory::Zone::LWRam) char[0];
    memcpy(destPtr, srcPtr, 0);
    delete[] srcPtr;
    delete[] destPtr;

        // Edge case: the largest block both heaps can surely give (half the free space of the smaller one)
    const size_t hwFree = Memory::HighWorkRam::GetFreeSpace();
    const size_t lwFree = Memory::LowWorkRam::GetFreeSpace();
    const size_t maxSize = ((hwFree < lwFree ? hwFree : lwFree) / 2) & ~static_cast<size_t>(3);
    srcPtr = new (SRL::Memory::Zone::HWRam) char[maxSize];
    destPtr = new (SRL::Memory::Zone::LWRam) char[maxSize];
    bool intact = srcPtr != nullptr && destPtr != nullptr;

    if (intact)
    {
        for (size_t i = 0; i < maxSize; ++i)
        {
            srcPtr[i] = static_cast<char>(i % 256);
        }
        memcpy(destPtr, srcPtr, maxSize);
        for (size_t i = 0; i < maxSize; ++i)
        {
            intact = intact && destPtr[i] == static_cast<char>(i % 256);
        }
    }
    delete[] srcPtr;
    delete[] destPtr;

    snprintf(buffer, buffer_size, "Moving a %lu-byte block: %s", (unsigned long)maxSize,
        srcPtr == nullptr || destPtr == nullptr ? "allocation failed" : "data integrity check failed");
    mu_assert(intact, buffer);
}

    /**
 * @brief Memory test suite configuration and test case registration
 *
 * Configures the test suite with setup, teardown, and error reporting functions.
 * Registers individual test cases to be executed during the test run.
 */
MU_TEST_SUITE(memory_test_suite)
{
        // Configure test suite with setup, teardown, and error reporting functions
    MU_SUITE_CONFIGURE_WITH_HEADER(&memory_test_setup,
        &memory_test_teardown,
        &memory_test_output_header);

        // Register test cases to be executed
    MU_RUN_TEST(memory_test_placement_malloc_highworkram);
    MU_RUN_TEST(memory_test_placement_malloc_lowworkram);
    MU_RUN_TEST(memory_test_placement_malloc_cartram);
    MU_RUN_TEST(memory_test_placement_malloc_invalid);
    MU_RUN_TEST(memory_test_cross_zone_allocation);
    MU_RUN_TEST(memory_test_move_memory_blocks); // Register the new test case
    MU_RUN_TEST(memory_test_move_memory_blocks_various_sizes); // Register the new test case
    MU_RUN_TEST(memory_test_move_memory_blocks_edge_cases); // Register the new test case
}
}

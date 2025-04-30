#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_memory.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    /**
     * @brief Set up routine for memory unit tests
     *
     * This function is called before each test in the memory test suite.
     * Currently, it does not perform any specific setup operations,
     * but provides a hook for future initialization requirements.
     */
    void memory_test_setup(void)
    {
        // Placeholder for any necessary test initialization
        // Future implementations might include resetting memory state,
        // clearing buffers, or preparing test environments
    }

    /**
     * @brief Tear down routine for memory unit tests
     *
     * This function is called after each test in the memory test suite.
     * Currently, it does not perform any specific cleanup operations,
     * but provides a hook for future resource release or state reset.
     */
    void memory_test_teardown(void)
    {
        // Placeholder for any necessary test cleanup
        // Future implementations might include freeing resources,
        // resetting global state, or clearing temporary data
    }

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
        void* address = (void*)0x08000000; // Address in CartRam range
        void* ptr = Memory::PlacementMalloc(100, address);
        mu_assert(ptr != nullptr, "PlacementMalloc in CartRam failed");

        Memory::Free(ptr);
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
        mu_assert(SRL::Memory::CartRam::GetSize() > 0, "CartRam initialization failed");
    }

    /**
     * @brief Test cross-zone memory allocation
     *
     * Verifies behavior when allocating memory across different zones.
     */
    MU_TEST(memory_test_cross_zone_allocation)
    {
        void* ptr1 = new (SRL::Memory::Zone::HWRam) char[100];
        void* ptr2 = new (SRL::Memory::Zone::LWRam) char[100];
        void* ptr3 = new (SRL::Memory::Zone::CartRam) char[100];

        mu_assert(ptr1 != nullptr, "Cross-zone allocation in HighWorkRam failed");
        mu_assert(ptr2 != nullptr, "Cross-zone allocation in LowWorkRam failed");
        mu_assert(ptr3 != nullptr, "Cross-zone allocation in CartRam failed");

        delete[] ptr1;
        delete[] ptr2;
        delete[] ptr3;
    }

    /**
     * @brief Test boundary conditions for memory allocation
     *
     * Verifies that memory allocation works correctly at the boundary of memory zones.
     */
    MU_TEST(memory_test_boundary_conditions)
    {
        size_t freeSpace = Memory::HighWorkRam::GetFreeSpace();
        void* ptr = new (SRL::Memory::Zone::HWRam) char[freeSpace - 1];
        mu_assert(ptr != nullptr, "Boundary condition allocation failed");

        delete[] ptr;
    }

    /**
     * @brief Test moving memory blocks between zones
     *
     * Verifies that memory blocks can be moved between different zones
     * and that the data integrity is maintained.
     */
    MU_TEST(memory_test_move_memory_blocks)
    {
        // Allocate memory in HighWorkRam and initialize with data
        char* srcPtr = new (SRL::Memory::Zone::HWRam) char[100];
        for (int i = 0; i < 100; ++i) {
            srcPtr[i] = static_cast<char>(i);
        }

        // Allocate memory in LowWorkRam
        char* destPtr = new (SRL::Memory::Zone::LWRam) char[100];

        // Move data from HighWorkRam to LowWorkRam
        memcpy(destPtr, srcPtr, 100);

        // Verify data integrity
        for (int i = 0; i < 100; ++i) {
            mu_assert(destPtr[i] == static_cast<char>(i), "Data integrity check failed after moving memory block");
        }

        // Clean up
        delete[] srcPtr;
        delete[] destPtr;
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
            // Allocate memory in HighWorkRam and initialize with data
            char* srcPtr = new (SRL::Memory::Zone::HWRam) char[size];
            for (size_t i = 0; i < size; ++i) {
                srcPtr[i] = static_cast<char>(i);
            }

            // Allocate memory in LowWorkRam
            char* destPtr = new (SRL::Memory::Zone::LWRam) char[size];

            // Move data from HighWorkRam to LowWorkRam
            memcpy(destPtr, srcPtr, size);

            // Verify data integrity
            for (size_t i = 0; i < size; ++i) {
                mu_assert(destPtr[i] == static_cast<char>(i), "Data integrity check failed after moving memory block");
            }

            // Clean up
            delete[] srcPtr;
            delete[] destPtr;
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
        mu_assert(true, "Zero size move should not fail");
        delete[] srcPtr;
        delete[] destPtr;

        // Edge case: maximum size (assuming a hypothetical maximum size)
        const size_t maxSize = 1024 * 1024; // 1 MB for example
        srcPtr = new (SRL::Memory::Zone::HWRam) char[maxSize];
        for (size_t i = 0; i < maxSize; ++i) {
            srcPtr[i] = static_cast<char>(i % 256);
        }
        destPtr = new (SRL::Memory::Zone::LWRam) char[maxSize];
        memcpy(destPtr, srcPtr, maxSize);
        for (size_t i = 0; i < maxSize; ++i) {
            mu_assert(destPtr[i] == static_cast<char>(i % 256), "Data integrity check failed after moving maximum size memory block");
        }
        delete[] srcPtr;
        delete[] destPtr;
    }

    /**
     * @brief Test moving memory blocks with invalid pointers
     *
     * Verifies that moving memory blocks with invalid pointers is handled correctly.
     */
    MU_TEST(memory_test_move_memory_blocks_invalid_pointers)
    {
        // Invalid source pointer
        char* srcPtr = nullptr;
        char* destPtr = new (SRL::Memory::Zone::LWRam) char[100];
        if (memcpy(destPtr, srcPtr, 100) == nullptr) {
            mu_assert(true, "Moving memory block with null source pointer failed as expected");
        } else {
            mu_assert(false, "Moving memory block with null source pointer should fail");
        }
        delete[] destPtr;

        // Invalid destination pointer
        srcPtr = new (SRL::Memory::Zone::HWRam) char[100];
        destPtr = nullptr;
        if (memcpy(destPtr, srcPtr, 100) == nullptr) {
            mu_assert(true, "Moving memory block with null destination pointer failed as expected");
        } else {
            mu_assert(false, "Moving memory block with null destination pointer should fail");
        }
        delete[] srcPtr;
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
        MU_RUN_TEST(memory_test_initialize_zones);
        MU_RUN_TEST(memory_test_cross_zone_allocation);
        MU_RUN_TEST(memory_test_boundary_conditions);
        MU_RUN_TEST(memory_test_move_memory_blocks); // Register the new test case
        MU_RUN_TEST(memory_test_move_memory_blocks_various_sizes); // Register the new test case
        MU_RUN_TEST(memory_test_move_memory_blocks_edge_cases); // Register the new test case
        MU_RUN_TEST(memory_test_move_memory_blocks_invalid_pointers); // Register the new test case
    }
}

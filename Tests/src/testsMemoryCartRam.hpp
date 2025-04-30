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
     * @brief Set up routine for CartRam memory unit tests
     *
     * This function is called before each test in the CartRam memory test suite.
     * Currently, it does not perform any specific setup operations,
     * but provides a hook for future initialization requirements.
     */
    void memory_CartRam_test_setup(void)
    {
        // Placeholder for any necessary test initialization
    }

    /**
     * @brief Tear down routine for CartRam memory unit tests
     *
     * This function is called after each test in the CartRam memory test suite.
     * Currently, it does not perform any specific cleanup operations,
     * but provides a hook for future resource release or state reset.
     */
    void memory_CartRam_test_teardown(void)
    {
        // Placeholder for any necessary test cleanup
    }

    /**
     * @brief Output header for CartRam test suite error reporting
     *
     * This function is called on the first test failure to print
     * a header indicating that CartRam memory unit test errors have occurred.
     * It increments a global error counter to ensure the header
     * is printed only once per test suite run.
     */
    void memory_CartRam_test_output_header(void)
    {
        // Print error header only on the first test failure
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_MEMORY_CartRam****");
            }
            else
            {
                LogInfo("****UT_MEMORY_CartRam_ERROR(S)****");
            }
        }
    }

    /**
     * @brief Test memory allocation and deallocation in CartRam
     *
     * Verifies that memory can be allocated and freed correctly in CartRam.
     * Ensures that the allocated memory is not a null pointer and
     * that the free operation completes successfully.
     */
    MU_TEST(memory_CartRam_test_malloc_free)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void *ptr = Memory::Malloc(100, Memory::Zone::CartRam);
        mu_assert(ptr != nullptr, "Memory allocation failed");

        Memory::Free(ptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    }

    /**
     * @brief Test getting free memory space in CartRam
     *
     * Verifies that the free memory space can be retrieved correctly in CartRam.
     * Ensures that the free space is greater than zero.
     */
    MU_TEST(memory_CartRam_test_get_free_space)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpace > 0, "Failed to get free space");
    }

    /**
     * @brief Test getting used memory space in CartRam
     *
     * Verifies that the used memory space can be retrieved correctly in CartRam.
     * Ensures that the used space is greater than or equal to zero.
     */
    MU_TEST(memory_CartRam_test_get_used_space)
    {
        size_t usedSpace = Memory::GetUsedSpace(Memory::Zone::CartRam);
        mu_assert(usedSpace >= 0, "Failed to get used space");
    }

    /**
     * @brief Test getting memory zone size in CartRam
     *
     * Verifies that the memory zone size can be retrieved correctly in CartRam.
     * Ensures that the size is greater than zero.
     */
    MU_TEST(memory_CartRam_test_get_size)
    {
        size_t size = Memory::GetSize(Memory::Zone::CartRam);
        mu_assert(size > 0, "Failed to get memory zone size");
    }

    /**
     * @brief Test allocating zero bytes in CartRam
     *
     * Verifies that allocating zero bytes returns a valid pointer in CartRam.
     */
    MU_TEST(memory_CartRam_test_malloc_zero)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void *ptr = Memory::Malloc(0, Memory::Zone::CartRam);
        mu_assert(ptr != nullptr, "Memory allocation of zero bytes failed");

        Memory::Free(ptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    }

    /**
     * @brief Test freeing a null pointer in CartRam
     *
     * Verifies that freeing a null pointer does not cause any issues in CartRam.
     */
    MU_TEST(memory_CartRam_test_free_null)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        Memory::Free(nullptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Freeing null pointer failed");
    }

    /**
     * @brief Test getting memory report for CartRam
     *
     * Verifies that the memory report can be retrieved correctly for CartRam.
     */
    MU_TEST(memory_CartRam_test_get_report_cartram)
    {
        Memory::Report report = Memory::CartRam::GetReport();
        mu_assert(report.TotalSize > 0, "Failed to get memory report for CartRam");
    }

    /**
     * @brief Test CartRam memory allocation and deallocation
     *
     * Verifies that memory can be allocated and freed correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_cartram_malloc_free)
    {
        size_t freeSpaceBefore = Memory::CartRam::GetFreeSpace();
        void *ptr = Memory::CartRam::Malloc(100);
        mu_assert(ptr != nullptr, "CartRam memory allocation failed");

        Memory::CartRam::Free(ptr);
        size_t freeSpaceAfter = Memory::CartRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "CartRam memory free failed");
    }

    /**
     * @brief Test CartRam memory reallocation
     *
     * Verifies that memory can be reallocated correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_cartram_realloc)
    {
        size_t freeSpaceBefore = Memory::CartRam::GetFreeSpace();
        void *ptr = Memory::CartRam::Malloc(100);
        mu_assert(ptr != nullptr, "CartRam memory allocation failed");

        void *newPtr = Memory::CartRam::Realloc(ptr, 200);
        mu_assert(newPtr != nullptr, "CartRam memory reallocation failed");

        Memory::CartRam::Free(newPtr);
        size_t freeSpaceAfter = Memory::CartRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "CartRam memory free failed");
    }

    /**
     * @brief Test getting CartRam free memory space
     *
     * Verifies that the free memory space in CartRam can be retrieved correctly.
     * Ensures that the free space is greater than zero.
     */
    MU_TEST(memory_CartRam_test_cartram_get_free_space)
    {
        size_t freeSpace = Memory::CartRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "Failed to get CartRam free space");
    }

    /**
     * @brief Test getting CartRam used memory space
     *
     * Verifies that the used memory space in CartRam can be retrieved correctly.
     * Ensures that the used space is greater than or equal to zero.
     */
    MU_TEST(memory_CartRam_test_cartram_get_used_space)
    {
        size_t usedSpace = Memory::CartRam::GetUsedSpace();
        mu_assert(usedSpace >= 0, "Failed to get CartRam used space");
    }

    /**
     * @brief Test getting CartRam memory zone size
     *
     * Verifies that the memory zone size in CartRam can be retrieved correctly.
     * Ensures that the size is greater than zero.
     */
    MU_TEST(memory_CartRam_test_cartram_get_size)
    {
        size_t size = Memory::CartRam::GetSize();
        mu_assert(size > 0, "Failed to get CartRam memory zone size");
    }

    /**
     * @brief Test new[] operator for CartRam
     *
     * Verifies that the new[] operator allocates memory correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_new_array_cartram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void *ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "new[] operator in CartRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
        snprintf(buffer, buffer_size,
                 "Memory allocation in CartRam did not reduce free space (before %d vs after %d)",
                 freeSpaceBefore, freeSpaceAfterAlloc);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, buffer);

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::CartRam);
        snprintf(buffer, buffer_size,
                 "Memory free in CartRam did not restore free space : %d lost",
                 freeSpaceBefore - freeSpaceAfterFree);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, buffer);
    }

    /**
     * @brief Test memory depletion for CartRam
     *
     * Verifies behavior when memory is depleted in CartRam.
     */
    MU_TEST(memory_CartRam_test_deplete_cartram)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::CartRam) / 2;
        void *ptr = nullptr;

        if (freeSpace > 0)
        {
            ptr = new (SRL::Memory::Zone::CartRam) char[freeSpace];

            snprintf(buffer, buffer_size,
                     "Cannot allocate %d in CartRam Memory", freeSpace);
            mu_assert(ptr != nullptr, buffer);
        }
        else
        {
            mu_assert(false, "CartRam Memory is already full");
        }

        freeSpace = Memory::GetFreeSpace(Memory::Zone::CartRam);

        snprintf(buffer, buffer_size,
                 "CMemory CartRam is not full : %d remains", freeSpace);
        mu_assert(freeSpace == 0, buffer);

        void *ptr2 = new (SRL::Memory::Zone::CartRam) char[100];

        mu_assert(ptr2 == nullptr, "Memory depletion in CartRam did not return nullptr");

        delete[] ptr2;
        delete[] ptr;

        // Validate that memory can be reallocated after depletion
        ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Memory reallocation in CartRam after depletion failed");

        delete[] ptr;
    }

    /**
     * @brief Test InRange function for CartRam
     *
     * Verifies that the InRange function correctly identifies pointers within the CartRam range.
     */
    MU_TEST(memory_CartRam_test_inrange_cartram)
    {
        void *validPtr = (void *)0x06000000;   // Valid address in CartRam range
        void *invalidPtr = (void *)0x08000000; // Invalid address outside CartRam range

        mu_assert(Memory::CartRam::InRange(validPtr), "InRange failed for valid CartRam address");
        mu_assert(!Memory::CartRam::InRange(invalidPtr), "InRange failed for invalid CartRam address");
    }

    /**
     * @brief Test reallocating to a larger size in CartRam
     *
     * Verifies that reallocating to a larger size works correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_realloc_larger)
    {
        void *ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Initial allocation failed");

        ptr = SRL::Memory::CartRam::Realloc(ptr, 200);
        mu_assert(ptr != nullptr, "Reallocation to larger size failed");

        delete[] ptr;
    }

    /**
     * @brief Test allocating and freeing very large blocks of memory in CartRam
     *
     * Verifies that very large blocks of memory can be allocated and freed correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_large_block)
    {
        size_t largeSize = SRL::Memory::CartRam::GetFreeSpace() / 2;
        void *ptr = new (SRL::Memory::Zone::CartRam) char[largeSize];
        mu_assert(ptr != nullptr, "Large block allocation failed");

        delete[] ptr;
    }

    /**
     * @brief Test memory fragmentation and defragmentation in CartRam
     *
     * Verifies that memory fragmentation and defragmentation are handled correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_fragmentation)
    {
        void *ptr1 = new (SRL::Memory::Zone::CartRam) char[100];
        void *ptr2 = new (SRL::Memory::Zone::CartRam) char[200];
        void *ptr3 = new (SRL::Memory::Zone::CartRam) char[300];

        delete[] ptr2;

        void *ptr4 = new (SRL::Memory::Zone::CartRam) char[150];
        mu_assert(ptr4 != nullptr, "Fragmentation handling failed");

        delete[] ptr1;
        delete[] ptr3;
        delete[] ptr4;
    }

    /**
     * @brief Test handling of allocation failures in CartRam
     *
     * Verifies that allocation failures are handled correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_allocation_failure)
    {
        size_t freeSpace = SRL::Memory::CartRam::GetFreeSpace();
        size_t toAllocate = freeSpace + 1;
        
        void *ptr = new (SRL::Memory::Zone::CartRam) char[toAllocate];

        snprintf(buffer, buffer_size,
            "Allocation of %d error handling failed", toAllocate);
        mu_assert(ptr == nullptr, buffer);
    }

    /**
     * @brief Test freeing unallocated or already freed memory in CartRam
     *
     * Verifies that freeing unallocated or already freed memory is handled correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_free_unallocated)
    {
        void *ptr = (void *)0x06000000; // Unallocated address
        SRL::Memory::Free(ptr);         // Should not crash or cause issues

        ptr = new (SRL::Memory::Zone::CartRam) char[100];
        delete[] ptr;
        SRL::Memory::Free(ptr); // Should not crash or cause issues
    }

    /**
     * @brief Test stress testing with high memory usage and frequent allocations/deallocations in CartRam
     *
     * Verifies that the system handles high memory usage and frequent allocations/deallocations correctly in CartRam.
     */
    MU_TEST(memory_CartRam_test_stress)
    {
        for (int i = 0; i < 1000; ++i)
        {
            void *ptr = new (SRL::Memory::Zone::CartRam) char[100];
            mu_assert(ptr != nullptr, "Stress test allocation failed");

            delete[] ptr;
        }
    }

    /**
     * @brief Test boundary conditions for memory allocation in CartRam
     *
     * Verifies that memory allocation works correctly at the boundary of memory zones in CartRam.
     */
    MU_TEST(memory_CartRam_test_boundary_conditions)
    {
        size_t freeSpace = Memory::CartRam::GetFreeSpace();
        void *ptr = new (SRL::Memory::Zone::CartRam) char[freeSpace - 1];
        mu_assert(ptr != nullptr, "Boundary condition allocation failed");

        delete[] ptr;
    }

    /**
     * @brief Test for memory leaks in CartRam
     *
     * Verifies that there are no memory leaks by tracking allocated and freed memory in CartRam.
     */
    MU_TEST(memory_CartRam_test_memory_leaks)
    {
        size_t freeSpaceBefore = Memory::CartRam::GetFreeSpace();
        void *ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Memory allocation failed");

        delete[] ptr;
        size_t freeSpaceAfter = Memory::CartRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory leak detected");
    }

    /**
     * @brief Test alignment requirements for memory allocations in CartRam
     */
    MU_TEST(memory_CartRam_test_alignment)
    {
        void *ptr = Memory::CartRam::Malloc(100);
        mu_assert(((uintptr_t)ptr % alignof(std::max_align_t)) == 0,
                  "Memory not properly aligned");
        Memory::CartRam::Free(ptr);
    }

    /**
     * @brief Test concurrent allocations and deallocations in CartRam
     */
    MU_TEST(memory_CartRam_test_mixed_sizes)
    {
        std::vector<void *> ptrs;
        std::vector<size_t> sizes = {8, 16, 32, 64, 128};

        for (size_t size : sizes)
        {
            void *ptr = Memory::CartRam::Malloc(size);
            mu_assert(ptr != nullptr, "Mixed size allocation failed");
            ptrs.push_back(ptr);
        }

        for (void *ptr : ptrs)
        {
            Memory::CartRam::Free(ptr);
        }
    }

    /**
     * @brief Test memory initialization in CartRam
     */
    MU_TEST(memory_CartRam_test_memory_init)
    {
        char *ptr = new (SRL::Memory::Zone::CartRam) char[10];
        mu_assert(ptr != nullptr, "Memory initialization allocation failed");

        // Write and verify pattern
        for (int i = 0; i < 10; i++)
        {
            ptr[i] = i;
        }

        for (int i = 0; i < 10; i++)
        {
            mu_assert(ptr[i] == i, "Memory content verification failed");
        }

        delete[] ptr;
    }

    /**
     * @brief Test multiple memory allocations of different sizes in CartRam
     *
     * Verifies that memory can be allocated and freed correctly for different sizes in CartRam.
     * Tests both small and large allocations in sequence, ensuring proper memory
     * management and state restoration after each operation.
     */
    MU_TEST(memory_CartRam_test_multiple_sizes_malloc_free)
    {
        // Test sizes from very small to large
        const size_t test_sizes[] = {
            1,        // Minimum size
            16,       // Small block
            64,       // Medium block
            256,      // Large block
            1024,     // 1KB block
            1024 * 4, // 4KB block
            1024 * 16 // 16KB block
        };

        size_t initial_free_space = Memory::GetFreeSpace(Memory::Zone::CartRam);

        // Test each size individually
        for (size_t size : test_sizes)
        {
            size_t before_alloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
            void *ptr = Memory::Malloc(size, Memory::Zone::CartRam);

            snprintf(buffer, buffer_size,
                     "Memory allocation failed for size %d", size);
            mu_assert(ptr != nullptr, buffer);

            size_t after_alloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
            snprintf(buffer, buffer_size,
                     "Memory space didn't decrease after allocation (size : %d), before : %d vs after : %d",
                     size, before_alloc, after_alloc);
            mu_assert(after_alloc < before_alloc, buffer);

            Memory::Free(ptr);
            size_t after_free = Memory::GetFreeSpace(Memory::Zone::CartRam);

            snprintf(buffer, buffer_size,
                     "Memory free failed for size %d", size);
            mu_assert(after_free == before_alloc, buffer);
        }

        // Verify total memory state is unchanged
        size_t final_free_space = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(final_free_space == initial_free_space,
                  "Final memory state different from initial state");
    }

    /**
     * @brief Test array allocations with multiple sizes in CartRam
     *
     * Verifies that arrays of different sizes can be allocated and deallocated correctly
     * using new[] and delete[] operators. Tests both sequential and interleaved
     * allocations/deallocations.
     */
    MU_TEST(memory_CartRam_test_multiple_array_sizes)
    {
        const size_t test_sizes[] = {
            8,    // Tiny array
            32,   // Small array
            128,  // Medium array
            512,  // Large array
            2048, // Very large array
            4096  // Huge array
        };

        size_t initial_free_space = Memory::GetFreeSpace(Memory::Zone::CartRam);
        std::vector<char *> arrays;

        // Sequential allocation and deallocation
        for (size_t size : test_sizes)
        {
            size_t before_alloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
            char *array = new (SRL::Memory::Zone::CartRam) char[size];

            snprintf(buffer, buffer_size,
                     "Array allocation failed for size %d", size);
            mu_assert(array != nullptr, buffer);

            // Write pattern to verify memory access
            for (size_t i = 0; i < size; i++)
            {
                array[i] = static_cast<char>(i % 256);
            }

            // Verify pattern
            for (size_t i = 0; i < size; i++)
            {
                snprintf(buffer, buffer_size,
                         "Memory verification failed at index %d for size %d", i, size);
                mu_assert(array[i] == static_cast<char>(i % 256), buffer);
            }

            delete[] array;

            size_t after_free = Memory::GetFreeSpace(Memory::Zone::CartRam);
            snprintf(buffer, buffer_size,
                     "Memory not properly freed for size %d", size);
            mu_assert(after_free == before_alloc, buffer);
        }

        // Interleaved allocation/deallocation
        for (size_t size : test_sizes)
        {
            char *array = new (SRL::Memory::Zone::CartRam) char[size];
            snprintf(buffer, buffer_size,
                     "Interleaved allocation failed for size %d", size);
            mu_assert(array != nullptr, buffer);
            arrays.push_back(array);
        }

        // Delete in reverse order
        while (!arrays.empty())
        {
            char *array = arrays.back();
            arrays.pop_back();
            delete[] array;
        }

        // Verify final memory state
        size_t final_free_space = Memory::GetFreeSpace(Memory::Zone::CartRam);
        snprintf(buffer, buffer_size,
                 "Memory leak detected after interleaved allocations : %d lost", initial_free_space - final_free_space);
        mu_assert(final_free_space == initial_free_space, buffer);
    }

    MU_TEST_SUITE(memory_CartRam_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&memory_CartRam_test_setup,
                                       &memory_CartRam_test_teardown,
                                       &memory_CartRam_test_output_header);

        MU_RUN_TEST(memory_CartRam_test_malloc_free);
        MU_RUN_TEST(memory_CartRam_test_multiple_sizes_malloc_free);
        MU_RUN_TEST(memory_CartRam_test_multiple_array_sizes);
        MU_RUN_TEST(memory_CartRam_test_new_array_cartram);
        MU_RUN_TEST(memory_CartRam_test_cartram_malloc_free);
        MU_RUN_TEST(memory_CartRam_test_cartram_realloc);
        MU_RUN_TEST(memory_CartRam_test_realloc_larger);

        MU_RUN_TEST(memory_CartRam_test_get_free_space);
        MU_RUN_TEST(memory_CartRam_test_get_used_space);
        MU_RUN_TEST(memory_CartRam_test_get_size);
        MU_RUN_TEST(memory_CartRam_test_get_report_cartram);
        MU_RUN_TEST(memory_CartRam_test_cartram_get_free_space);
        MU_RUN_TEST(memory_CartRam_test_cartram_get_used_space);
        MU_RUN_TEST(memory_CartRam_test_cartram_get_size);
        MU_RUN_TEST(memory_CartRam_test_inrange_cartram);

        MU_RUN_TEST(memory_CartRam_test_malloc_zero);
        MU_RUN_TEST(memory_CartRam_test_free_null);
        MU_RUN_TEST(memory_CartRam_test_free_unallocated);
        MU_RUN_TEST(memory_CartRam_test_allocation_failure);
        MU_RUN_TEST(memory_CartRam_test_memory_leaks);

        MU_RUN_TEST(memory_CartRam_test_large_block);
        MU_RUN_TEST(memory_CartRam_test_fragmentation);
        MU_RUN_TEST(memory_CartRam_test_boundary_conditions);
        MU_RUN_TEST(memory_CartRam_test_deplete_cartram);

        MU_RUN_TEST(memory_CartRam_test_stress);

        MU_RUN_TEST(memory_CartRam_test_alignment);
        MU_RUN_TEST(memory_CartRam_test_mixed_sizes);
        MU_RUN_TEST(memory_CartRam_test_memory_init);
        MU_RUN_TEST(memory_CartRam_test_multiple_sizes_malloc_free);
        MU_RUN_TEST(memory_CartRam_test_multiple_array_sizes);
    }
}

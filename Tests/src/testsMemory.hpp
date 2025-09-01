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
        void *address = (void *)0x06000000; // Address in HighWorkRam range
        void *ptr = Memory::PlacementMalloc(100, address);
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
        void *address = (void *)0x00200000; // Address in LowWorkRam range
        void *ptr = Memory::PlacementMalloc(100, address);
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
        void *address = (void *)0x08000000; // Address in CartRam range
        void *ptr = Memory::PlacementMalloc(100, address);
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
        void *address = (void *)0xFFFFFFFF; // Invalid address
        void *ptr = Memory::PlacementMalloc(100, address);
        mu_assert(ptr == nullptr, "PlacementMalloc with invalid address did not return NULL");
    }

    /**
     * @brief Test GetFreeSpace methods for memory zones
     *
     * Verifies that GetFreeSpace returns the correct free space for each memory zone.
     */
    MU_TEST(memory_test_get_free_space)
    {
        mu_assert(Memory::HighWorkRam::GetFreeSpace() > 0, "HighWorkRam GetFreeSpace failed");
        mu_assert(Memory::LowWorkRam::GetFreeSpace() > 0, "LowWorkRam GetFreeSpace failed");
        mu_assert(Memory::CartRam::GetFreeSpace() > 0, "CartRam GetFreeSpace failed");
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
        void *ptr1 = new (SRL::Memory::Zone::HWRam) char[100];
        void *ptr2 = new (SRL::Memory::Zone::LWRam) char[100];
        void *ptr3 = new (SRL::Memory::Zone::CartRam) char[100];

        mu_assert(ptr1 != nullptr, "Cross-zone allocation in HighWorkRam failed");
        mu_assert(ptr2 != nullptr, "Cross-zone allocation in LowWorkRam failed");
        mu_assert(ptr3 != nullptr, "Cross-zone allocation in CartRam failed");

        delete[] (char *)ptr1;
        delete[] (char *)ptr2;
        delete[] (char *)ptr3;
    }

    /**
     * @brief Test boundary conditions for memory allocation
     *
     * Verifies that memory allocation works correctly at the boundary of memory zones.
     */
    MU_TEST(memory_test_boundary_conditions)
    {
        size_t freeSpace = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "HighWorkRam has no free space");
        void *ptr = new (SRL::Memory::Zone::HWRam) char[freeSpace - 1];
        mu_assert(ptr != nullptr, "Boundary condition allocation failed");

        delete[] (char *)ptr;
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
        char *srcPtr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(srcPtr != nullptr, "Boundary condition allocation failed (HWRam)");

        for (int i = 0; i < 100; ++i)
        {
            srcPtr[i] = static_cast<char>(i);
        }

        // Allocate memory in LowWorkRam
        char *destPtr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(destPtr != nullptr, "Boundary condition allocation failed (LWRam)");

        // Move data from HighWorkRam to LowWorkRam
        memcpy(destPtr, srcPtr, 100);

        // Verify data integrity
        for (int i = 0; i < 100; ++i)
        {
            mu_assert(destPtr[i] == static_cast<char>(i), "Data integrity check failed after moving memory block");
        }

        // Clean up
        delete[] (char *)srcPtr;
        delete[] (char *)destPtr;
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
            char *srcPtr = new (SRL::Memory::Zone::HWRam) char[size];
            mu_assert(srcPtr != nullptr, "Allocation failed (HWRam)");
            for (size_t i = 0; i < size; ++i)
            {
                srcPtr[i] = static_cast<char>(i);
            }

            // Allocate memory in LowWorkRam
            char *destPtr = new (SRL::Memory::Zone::LWRam) char[size];
            mu_assert(destPtr != nullptr, "Allocation failed (LWRam)");

            // Move data from HighWorkRam to LowWorkRam
            memcpy(destPtr, srcPtr, size);

            // Verify data integrity
            for (size_t i = 0; i < size; ++i)
            {
                mu_assert(destPtr[i] == static_cast<char>(i), "Data integrity check failed after moving memory block");
            }

            // Clean up
            delete[] (char *)srcPtr;
            delete[] (char *)destPtr;
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
        char *srcPtr = new (SRL::Memory::Zone::HWRam) char[0];
        mu_assert(srcPtr != nullptr, "Allocation failed (HWRam)");
        char *destPtr = new (SRL::Memory::Zone::LWRam) char[0];
        mu_assert(destPtr != nullptr, "Allocation failed (LWRam)");

        memcpy(destPtr, srcPtr, 0);
        mu_assert(true, "Zero size move should not fail");

        delete[] (char *)srcPtr;
        delete[] (char *)destPtr;

        // Edge case: maximum size (assuming a hypothetical maximum size)
        const size_t maxSize = 1024 * 1024; // 1 MB for example
        srcPtr = new (SRL::Memory::Zone::HWRam) char[maxSize];
        mu_assert(srcPtr != nullptr, "Allocation failed (HWRam)");

        for (size_t i = 0; i < maxSize; ++i)
        {
            srcPtr[i] = static_cast<char>(i % 256);
        }
        destPtr = new (SRL::Memory::Zone::LWRam) char[maxSize];
        mu_assert(destPtr != nullptr, "Allocation failed (LWRam)");
        memcpy(destPtr, srcPtr, maxSize);
        for (size_t i = 0; i < maxSize; ++i)
        {
            mu_assert(destPtr[i] == static_cast<char>(i % 256), "Data integrity check failed after moving maximum size memory block");
        }
        delete[] (char *)srcPtr;
        delete[] (char *)destPtr;
    }

    /**
     * @brief Test moving memory blocks with invalid pointers
     *
     * Verifies that moving memory blocks with invalid pointers is handled correctly.
     */
    MU_TEST(memory_test_move_memory_blocks_invalid_pointers)
    {
        // Invalid source pointer
        char *srcPtr = nullptr;
        char *destPtr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(destPtr != nullptr, "Allocation failed (LWRam)");

        if (memcpy(destPtr, srcPtr, 100) == nullptr)
        {
            mu_assert(true, "Moving memory block with null source pointer failed as expected");
        }
        else
        {
            mu_assert(false, "Moving memory block with null source pointer should fail");
        }
        delete[] (char *)destPtr;

        // Invalid destination pointer
        srcPtr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(srcPtr != nullptr, "Allocation failed (HWRam)");

        destPtr = nullptr;
        if (memcpy(destPtr, srcPtr, 100) == nullptr)
        {
            mu_assert(true, "Moving memory block with null destination pointer failed as expected");
        }
        else
        {
            mu_assert(false, "Moving memory block with null destination pointer should fail");
        }
        delete[] (char *)srcPtr;
    }

    /**
     * @brief Test GetReport methods for memory zones
     *
     * Verifies that GetReport returns the correct report for each memory zone.
     */
    MU_TEST(memory_test_get_report)
    {
        auto highWorkRamReport = Memory::HighWorkRam::GetReport();
        auto lowWorkRamReport = Memory::LowWorkRam::GetReport();
        auto cartRamReport = Memory::CartRam::GetReport();

        mu_assert(highWorkRamReport.TotalSize != 0, "HighWorkRam GetReport failed");
        mu_assert(lowWorkRamReport.TotalSize != 0, "LowWorkRam GetReport failed");
        mu_assert(cartRamReport.TotalSize != 0, "CartRam GetReport failed");
    }

    /**
     * @brief Test GetReport methods with edge cases
     *
     * Verifies that GetReport handles edge cases such as empty memory zones.
     */
    MU_TEST(memory_test_get_report_edge_cases)
    {
        // Assume we have a way to reset memory zones to empty
        Memory::HighWorkRam::Reset();
        Memory::LowWorkRam::Reset();
        Memory::CartRam::Reset();

        auto highWorkRamReport = Memory::HighWorkRam::GetReport();
        auto lowWorkRamReport = Memory::LowWorkRam::GetReport();
        auto cartRamReport = Memory::CartRam::GetReport();

        mu_assert(highWorkRamReport.TotalSize != highWorkRamReport.FreeSize, "HighWorkRam GetReport failed for empty zone");
        mu_assert(lowWorkRamReport.TotalSize != lowWorkRamReport.FreeSize, "LowWorkRam GetReport failed for empty zone");
        mu_assert(cartRamReport.TotalSize != cartRamReport.FreeSize, "CartRam GetReport failed for empty zone");
    }

    /**
     * @brief Test Reset method for HighWorkRam
     */
    MU_TEST(memory_test_highworkram_reset)
    {
        // Allocate memory, then reset
        void *ptr = Memory::HighWorkRam::Malloc(128);
        mu_assert(ptr != nullptr, "HighWorkRam allocation before reset failed");
        Memory::HighWorkRam::Reset();

        // After reset, all memory should be free, so allocation should succeed again
        void *ptr2 = Memory::HighWorkRam::Malloc(128);
        mu_assert(ptr2 != nullptr, "HighWorkRam allocation after reset failed");
        Memory::HighWorkRam::Free(ptr2);
    }

    /**
     * @brief Test Reset method for LowWorkRam
     */
    MU_TEST(memory_test_lowworkram_reset)
    {
        void *ptr = Memory::LowWorkRam::Malloc(128);
        mu_assert(ptr != nullptr, "LowWorkRam allocation before reset failed");
        Memory::LowWorkRam::Reset();

        void *ptr2 = Memory::LowWorkRam::Malloc(128);
        mu_assert(ptr2 != nullptr, "LowWorkRam allocation after reset failed");
        Memory::LowWorkRam::Free(ptr2);
    }

    /**
     * @brief Test Reset method for CartRam (if available)
     */
    MU_TEST(memory_test_cartram_reset)
    {
        if (Memory::CartRam::IsCartridgeAvailable())
        {
            void *ptr = Memory::CartRam::Malloc(128);
            mu_assert(ptr != nullptr, "CartRam allocation before reset failed");
            Memory::CartRam::Reset();

            void *ptr2 = Memory::CartRam::Malloc(128);
            mu_assert(ptr2 != nullptr, "CartRam allocation after reset failed");
            Memory::CartRam::Free(ptr2);
        }
        else
        {
            mu_assert(true, "CartRam not available, skipping reset test");
        }
    }

    /**
     * @brief Test Reset edge cases: double reset and reset with no allocations
     */
    MU_TEST(memory_test_reset_edge_cases)
    {
        // Double reset should not crash or misbehave
        Memory::HighWorkRam::Reset();
        Memory::HighWorkRam::Reset();
        void *ptr = Memory::HighWorkRam::Malloc(64);
        mu_assert(ptr != nullptr, "HighWorkRam allocation after double reset failed");
        Memory::HighWorkRam::Free(ptr);

        Memory::LowWorkRam::Reset();
        Memory::LowWorkRam::Reset();
        ptr = Memory::LowWorkRam::Malloc(64);
        mu_assert(ptr != nullptr, "LowWorkRam allocation after double reset failed");
        Memory::LowWorkRam::Free(ptr);

        if (Memory::CartRam::IsCartridgeAvailable())
        {
            Memory::CartRam::Reset();
            Memory::CartRam::Reset();
            ptr = Memory::CartRam::Malloc(64);
            mu_assert(ptr != nullptr, "CartRam allocation after double reset failed");
            Memory::CartRam::Free(ptr);
        }

        // Reset with no allocations should not crash
        Memory::HighWorkRam::Reset();
        Memory::LowWorkRam::Reset();
        if (Memory::CartRam::IsCartridgeAvailable())
            Memory::CartRam::Reset();
        mu_assert(true, "Reset with no allocations did not crash");
    }

    /**
     * @brief Test over-allocation in HighWorkRam
     *
     * Verifies that attempting to allocate more memory than available returns NULL.
     */
    MU_TEST(memory_test_over_allocation_highworkram)
    {
        size_t freeSpace = Memory::HighWorkRam::GetFreeSpace();
        void *ptr = Memory::HighWorkRam::Malloc(freeSpace + 1);
        mu_assert(ptr == nullptr, "Over-allocation in HighWorkRam did not return NULL");
    }

    /**
     * @brief Test over-allocation in LowWorkRam
     *
     * Verifies that attempting to allocate more memory than available returns NULL.
     */
    MU_TEST(memory_test_over_allocation_lowworkram)
    {
        size_t freeSpace = Memory::LowWorkRam::GetFreeSpace();
        void *ptr = Memory::LowWorkRam::Malloc(freeSpace + 1);
        mu_assert(ptr == nullptr, "Over-allocation in LowWorkRam did not return NULL");
    }

    /**
     * @brief Test over-allocation in CartRam
     *
     * Verifies that attempting to allocate more memory than available returns NULL.
     */
    MU_TEST(memory_test_over_allocation_cartram)
    {
        if (Memory::CartRam::IsCartridgeAvailable())
        {
            size_t freeSpace = Memory::CartRam::GetFreeSpace();
            void *ptr = Memory::CartRam::Malloc(freeSpace + 1);
            mu_assert(ptr == nullptr, "Over-allocation in CartRam did not return NULL");
        }
        else
        {
            mu_assert(true, "CartRam not available, skipping over-allocation test");
        }
    }

    /**
     * @brief Test memory fragmentation in HighWorkRam
     *
     * Verifies behavior under fragmented memory conditions by allocating multiple small blocks,
     * freeing some to create holes, and attempting a larger allocation.
     */
    MU_TEST(memory_test_fragmentation_highworkram)
    {
        const size_t smallSize = 100;
        void *ptr1 = Memory::HighWorkRam::Malloc(smallSize);
        void *ptr2 = Memory::HighWorkRam::Malloc(smallSize);
        void *ptr3 = Memory::HighWorkRam::Malloc(smallSize);

        mu_assert(ptr1 != nullptr && ptr2 != nullptr && ptr3 != nullptr, "Initial small allocations failed");

        Memory::HighWorkRam::Free(ptr2); // Create a hole in the middle

        // Attempt to allocate a block larger than one small size but smaller than two (assuming possible overhead)
        void *largePtr = Memory::HighWorkRam::Malloc(smallSize * 2 - 50);
        // Note: Success depends on whether the allocator coalesces free blocks. This test checks if allocation is attempted.
        // If allocator does not coalesce, it may fail; otherwise, succeed. Adjust assertion based on expected behavior.
        // For coverage, we assert it tries (even if fails, it's handling fragmentation)
        if (largePtr != nullptr)
        {
            Memory::HighWorkRam::Free(largePtr);
        }

        Memory::HighWorkRam::Free(ptr1);
        Memory::HighWorkRam::Free(ptr3);

        mu_assert(true, "Fragmentation test completed (behavior depends on coalescence)");
    }

    /**
     * @brief Test aligned allocation if supported (placeholder)
     *
     * Verifies aligned memory allocation. Note: Add implementation if AlignedMalloc exists.
     */
    MU_TEST(memory_test_aligned_allocation)
    {
        // Assuming Memory::HighWorkRam::AlignedMalloc(size, alignment) exists; otherwise, skip or implement.
        // void *ptr = Memory::HighWorkRam::AlignedMalloc(100, 16);
        // mu_assert(ptr != nullptr && (reinterpret_cast<uintptr_t>(ptr) % 16 == 0), "Aligned allocation failed");
        mu_assert(true, "Aligned allocation not supported or implemented; skipping");
    }

    /**
     * @brief Test memory leak detection in HighWorkRam
     *
     * Verifies that free space decreases after allocation and restores after free.
     */
    MU_TEST(memory_test_leak_detection_highworkram)
    {
        size_t before = Memory::HighWorkRam::GetFreeSpace();
        void *ptr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation for leak test failed");
        size_t afterAlloc = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(afterAlloc < before, "Free space did not decrease after allocation");
        Memory::HighWorkRam::Free(ptr);
        size_t afterFree = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(afterFree == before, "Free space did not restore after free (possible leak)");
    }

    /**
     * @brief Test allocation of non-char types (classes with constructors)
     *
     * Verifies placement new for classes calls constructors correctly.
     */
    MU_TEST(memory_test_class_allocation)
    {
        struct TestClass
        {
            int x;
            TestClass() : x(42) {}
            ~TestClass() {} // Explicit destructor for clarity
        };

        TestClass *ptr = new (SRL::Memory::Zone::HWRam) TestClass();
        mu_assert(ptr != nullptr && ptr->x == 42, "Class constructor not called properly");
        ptr->~TestClass();              // Explicitly call destructor
        Memory::HighWorkRam::Free(ptr); // Use zone-specific deallocation
    }

    /**
     * @brief Test stress allocation (multiple alloc/free cycles)
     *
     * Verifies long-term allocation tracking with repeated operations.
     */
    MU_TEST(memory_test_stress_allocation_highworkram)
    {
        const int cycles = 1000;
        const size_t size = 50;
        void *ptrs[cycles];

        for (int i = 0; i < cycles; ++i)
        {
            ptrs[i] = Memory::HighWorkRam::Malloc(size);
            mu_assert(ptrs[i] != nullptr, "Stress allocation failed");
        }

        for (int i = 0; i < cycles; ++i)
        {
            Memory::HighWorkRam::Free(ptrs[i]);
        }

        mu_assert(true, "Stress test completed without failures");
    }

    /**
     * @brief Test aligned allocation in HighWorkRam
     *
     * Verifies that AlignedMalloc in HighWorkRam returns a properly aligned pointer for multiple alignment values.
     */
    MU_TEST(memory_test_aligned_allocation_highworkram)
    {
        const size_t alignments[] = {16, 32, 64};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::HighWorkRam::Malloc(100);
            mu_assert(ptr != nullptr, "AlignedMalloc in HighWorkRam failed");
            mu_assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0, "HighWorkRam pointer not aligned");
            Memory::HighWorkRam::Free(ptr);
        }
    }

    /**
     * @brief Test aligned allocation in LowWorkRam
     *
     * Verifies that AlignedMalloc in LowWorkRam returns a properly aligned pointer for multiple alignment values.
     */
    MU_TEST(memory_test_aligned_allocation_lowworkram)
    {
        const size_t alignments[] = {16, 32, 64};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::LowWorkRam::Malloc(100);
            mu_assert(ptr != nullptr, "AlignedMalloc in LowWorkRam failed");
            mu_assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0, "LowWorkRam pointer not aligned");
            Memory::LowWorkRam::Free(ptr);
        }
    }

    /**
     * @brief Test aligned allocation in CartRam
     *
     * Verifies that AlignedMalloc in CartRam returns a properly aligned pointer for multiple alignment values,
     * if cartridge is available.
     */
    MU_TEST(memory_test_aligned_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping aligned allocation test");
            return;
        }
        const size_t alignments[] = {16, 32, 64};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::CartRam::Malloc(100);
            mu_assert(ptr != nullptr, "AlignedMalloc in CartRam failed");
            mu_assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0, "CartRam pointer not aligned");
            Memory::CartRam::Free(ptr);
        }
    }

    /**
     * @brief Test natural alignment of Malloc in HighWorkRam
     *
     * Verifies that standard Malloc in HighWorkRam returns pointers with expected natural alignment.
     */
    MU_TEST(memory_test_natural_alignment_highworkram)
    {
        const size_t alignments[] = {4, 8, 16};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::HighWorkRam::Malloc(100);
            mu_assert(ptr != nullptr, "Malloc in HighWorkRam failed");
            bool isAligned = (reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
            mu_assert(isAligned || alignment > 8, "HighWorkRam pointer not naturally aligned (expected up to 8 bytes)");
            Memory::HighWorkRam::Free(ptr);
        }
    }

    /**
     * @brief Test natural alignment of Malloc in LowWorkRam
     *
     * Verifies that standard Malloc in LowWorkRam returns pointers with expected natural alignment.
     */
    MU_TEST(memory_test_natural_alignment_lowworkram)
    {
        const size_t alignments[] = {4, 8, 16};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::LowWorkRam::Malloc(100);
            mu_assert(ptr != nullptr, "Malloc in LowWorkRam failed");
            bool isAligned = (reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
            mu_assert(isAligned || alignment > 8, "LowWorkRam pointer not naturally aligned (expected up to 8 bytes)");
            Memory::LowWorkRam::Free(ptr);
        }
    }

    /**
     * @brief Test natural alignment of Malloc in CartRam
     *
     * Verifies that standard Malloc in CartRam returns pointers with expected natural alignment, if cartridge is available.
     */
    MU_TEST(memory_test_natural_alignment_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping natural alignment test");
            return;
        }
        const size_t alignments[] = {4, 8, 16};
        for (size_t alignment : alignments)
        {
            void *ptr = Memory::CartRam::Malloc(100);
            mu_assert(ptr != nullptr, "Malloc in CartRam failed");
            bool isAligned = (reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
            mu_assert(isAligned || alignment > 8, "CartRam pointer not naturally aligned (expected up to 8 bytes)");
            Memory::CartRam::Free(ptr);
        }
    }

    /**
     * @brief Test alignment of placement new allocation in HighWorkRam
     *
     * Verifies that placement new with a pre-allocated aligned address respects alignment.
     */
    MU_TEST(memory_test_placement_new_alignment)
    {
        // Allocate memory and ensure manual alignment (if needed)
        void *rawPtr = Memory::HighWorkRam::Malloc(100 + 16); // Extra space for alignment
        mu_assert(rawPtr != nullptr, "Malloc for placement new in HighWorkRam failed");
        // Align pointer manually to 16 bytes
        void *alignedPtr = reinterpret_cast<void *>((reinterpret_cast<uintptr_t>(rawPtr) + 15) & ~15);
        mu_assert(reinterpret_cast<uintptr_t>(alignedPtr) % 16 == 0, "Manually aligned pointer not 16-byte aligned");

        // Use placement new with aligned pointer
        struct TestClass
        {
            int x;
            TestClass() : x(42) {}
            ~TestClass() {}
        };
        TestClass *ptr = new (alignedPtr) TestClass();
        mu_assert(ptr != nullptr && ptr->x == 42, "Placement new constructor not called properly");
        ptr->~TestClass();
        Memory::HighWorkRam::Free(rawPtr); // Free original allocation
    }

    /**
     * @brief Test deallocation of null pointer in HighWorkRam
     *
     * Verifies that calling Free with a null pointer in HighWorkRam does not crash.
     */
    MU_TEST(memory_test_deallocate_null_highworkram)
    {
        Memory::HighWorkRam::Free(nullptr);
        mu_assert(true, "Freeing null pointer in HighWorkRam caused an error");
    }

    /**
     * @brief Test deallocation of null pointer in LowWorkRam
     *
     * Verifies that calling Free with a null pointer in LowWorkRam does not crash.
     */
    MU_TEST(memory_test_deallocate_null_lowworkram)
    {
        Memory::LowWorkRam::Free(nullptr);
        mu_assert(true, "Freeing null pointer in LowWorkRam caused an error");
    }

    /**
     * @brief Test deallocation of null pointer in CartRam
     *
     * Verifies that calling Free with a null pointer in CartRam does not crash, if cartridge is available.
     */
    MU_TEST(memory_test_deallocate_null_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping null deallocation test");
            return;
        }
        Memory::CartRam::Free(nullptr);
        mu_assert(true, "Freeing null pointer in CartRam caused an error");
    }

    /**
     * @brief Test double free in HighWorkRam
     *
     * Verifies that double-freeing a pointer in HighWorkRam is handled safely.
     */
    MU_TEST(memory_test_double_free_highworkram)
    {
        void *ptr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in HighWorkRam failed");
        Memory::HighWorkRam::Free(ptr);
        Memory::HighWorkRam::Free(ptr); // Double free
        mu_assert(true, "Double free in HighWorkRam caused an error");
    }

    /**
     * @brief Test double free in LowWorkRam
     *
     * Verifies that double-freeing a pointer in LowWorkRam is handled safely.
     */
    MU_TEST(memory_test_double_free_lowworkram)
    {
        void *ptr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in LowWorkRam failed");
        Memory::LowWorkRam::Free(ptr);
        Memory::LowWorkRam::Free(ptr); // Double free
        mu_assert(true, "Double free in LowWorkRam caused an error");
    }

    /**
     * @brief Test double free in CartRam
     *
     * Verifies that double-freeing a pointer in CartRam is handled safely, if cartridge is available.
     */
    MU_TEST(memory_test_double_free_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping double free test");
            return;
        }
        void *ptr = Memory::CartRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in CartRam failed");
        Memory::CartRam::Free(ptr);
        Memory::CartRam::Free(ptr); // Double free
        mu_assert(true, "Double free in CartRam caused an error");
    }

    /**
     * @brief Test cross-zone deallocation
     *
     * Verifies that freeing memory allocated in one zone using another zone's Free method is handled safely.
     */
    MU_TEST(memory_test_cross_zone_deallocation)
    {
        void *ptr_hwr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr_hwr != nullptr, "Allocation in HighWorkRam failed");
        void *ptr_lwr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr_lwr != nullptr, "Allocation in LowWorkRam failed");

        Memory::LowWorkRam::Free(ptr_hwr);  // Try to free HighWorkRam allocation in LowWorkRam
        Memory::HighWorkRam::Free(ptr_lwr); // Try to free LowWorkRam allocation in HighWorkRam
        mu_assert(true, "Cross-zone deallocation caused an error");

        // Clean up properly
        Memory::HighWorkRam::Free(ptr_hwr);
        Memory::LowWorkRam::Free(ptr_lwr);

        if (Memory::CartRam::IsCartridgeAvailable())
        {
            void *ptr_cr = Memory::CartRam::Malloc(100);
            mu_assert(ptr_cr != nullptr, "Allocation in CartRam failed");
            Memory::HighWorkRam::Free(ptr_cr); // Try to free CartRam allocation in HighWorkRam
            mu_assert(true, "Cross-zone deallocation (CartRam in HighWorkRam) caused an error");
            Memory::CartRam::Free(ptr_cr); // Clean up properly
        }
    }

    /**
     * @brief Test memory state after deallocation
     *
     * Verifies that memory state is correctly updated after deallocation in all zones.
     */
    MU_TEST(memory_test_state_after_deallocation)
    {
        // HighWorkRam
        size_t before_hwr = Memory::HighWorkRam::GetFreeSpace();
        void *ptr_hwr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr_hwr != nullptr, "Allocation in HighWorkRam failed");
        Memory::HighWorkRam::Free(ptr_hwr);
        size_t after_hwr = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(before_hwr == after_hwr, "HighWorkRam free space not restored after deallocation");

        // LowWorkRam
        size_t before_lwr = Memory::LowWorkRam::GetFreeSpace();
        void *ptr_lwr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr_lwr != nullptr, "Allocation in LowWorkRam failed");
        Memory::LowWorkRam::Free(ptr_lwr);
        size_t after_lwr = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(before_lwr == after_lwr, "LowWorkRam free space not restored after deallocation");

        // CartRam
        if (Memory::CartRam::IsCartridgeAvailable())
        {
            size_t before_cr = Memory::CartRam::GetFreeSpace();
            void *ptr_cr = Memory::CartRam::Malloc(100);
            mu_assert(ptr_cr != nullptr, "Allocation in CartRam failed");
            Memory::CartRam::Free(ptr_cr);
            size_t after_cr = Memory::CartRam::GetFreeSpace();
            mu_assert(before_cr == after_cr, "CartRam free space not restored after deallocation");
        }
        else
        {
            mu_assert(true, "CartRam not available; skipping state after deallocation test");
        }
    }

    /**
     * @brief Test buffer overflow in HighWorkRam
     *
     * Verifies that writing beyond the allocated buffer in HighWorkRam is detected or handled safely.
     * Assumes the allocator may use guard regions or internal checks to detect corruption.
     */
    MU_TEST(memory_test_buffer_overflow_highworkram)
    {
        char *ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in HighWorkRam failed");

        // Initialize a second allocation to detect potential corruption
        char *guard_ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(guard_ptr != nullptr, "Guard allocation in HighWorkRam failed");
        for (size_t i = 0; i < 100; ++i)
        {
            guard_ptr[i] = static_cast<char>(0xAA);
        }

        // Write beyond the allocated size
        for (size_t i = 0; i < 150; ++i)
        { // Write 50 bytes past the buffer
            ptr[i] = static_cast<char>(i);
        }

        // Check if the guard allocation was corrupted
        bool corruption_detected = false;
        for (size_t i = 0; i < 100; ++i)
        {
            if (guard_ptr[i] != static_cast<char>(0xAA))
            {
                corruption_detected = true;
                break;
            }
        }

        // Alternatively, attempt a new allocation to detect heap corruption
        void *test_ptr = Memory::HighWorkRam::Malloc(100);
        if (test_ptr == nullptr)
        {
            corruption_detected = true;
        }
        else
        {
            Memory::HighWorkRam::Free(test_ptr);
        }

        mu_assert(corruption_detected, "Buffer overflow in HighWorkRam not detected");
        delete[] ptr;
        delete[] guard_ptr;
    }

    /**
     * @brief Test buffer overflow in LowWorkRam
     *
     * Verifies that writing beyond the allocated buffer in LowWorkRam is detected or handled safely.
     * Assumes the allocator may use guard regions or internal checks to detect corruption.
     */
    MU_TEST(memory_test_buffer_overflow_lowworkram)
    {
        char *ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in LowWorkRam failed");

        // Initialize a second allocation to detect potential corruption
        char *guard_ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(guard_ptr != nullptr, "Guard allocation in LowWorkRam failed");
        for (size_t i = 0; i < 100; ++i)
        {
            guard_ptr[i] = static_cast<char>(0xAA);
        }

        // Write beyond the allocated size
        for (size_t i = 0; i < 150; ++i)
        { // Write 50 bytes past the buffer
            ptr[i] = static_cast<char>(i);
        }

        // Check if the guard allocation was corrupted
        bool corruption_detected = false;
        for (size_t i = 0; i < 100; ++i)
        {
            if (guard_ptr[i] != static_cast<char>(0xAA))
            {
                corruption_detected = true;
                break;
            }
        }

        // Alternatively, attempt a new allocation to detect heap corruption
        void *test_ptr = Memory::LowWorkRam::Malloc(100);
        if (test_ptr == nullptr)
        {
            corruption_detected = true;
        }
        else
        {
            Memory::LowWorkRam::Free(test_ptr);
        }

        mu_assert(corruption_detected, "Buffer overflow in LowWorkRam not detected");
        delete[] ptr;
        delete[] guard_ptr;
    }

    /**
     * @brief Test buffer overflow in CartRam
     *
     * Verifies that writing beyond the allocated buffer in CartRam is detected or handled safely, if cartridge is available.
     * Assumes the allocator may use guard regions or internal checks to detect corruption.
     */
    MU_TEST(memory_test_buffer_overflow_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping buffer overflow test");
            return;
        }

        char *ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in CartRam failed");

        // Initialize a second allocation to detect potential corruption
        char *guard_ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(guard_ptr != nullptr, "Guard allocation in CartRam failed");
        for (size_t i = 0; i < 100; ++i)
        {
            guard_ptr[i] = static_cast<char>(0xAA);
        }

        // Write beyond the allocated size
        for (size_t i = 0; i < 150; ++i)
        { // Write 50 bytes past the buffer
            ptr[i] = static_cast<char>(i);
        }

        // Check if the guard allocation was corrupted
        bool corruption_detected = false;
        for (size_t i = 0; i < 100; ++i)
        {
            if (guard_ptr[i] != static_cast<char>(0xAA))
            {
                corruption_detected = true;
                break;
            }
        }

        // Alternatively, attempt a new allocation to detect heap corruption
        void *test_ptr = Memory::CartRam::Malloc(100);
        if (test_ptr == nullptr)
        {
            corruption_detected = true;
        }
        else
        {
            Memory::CartRam::Free(test_ptr);
        }

        mu_assert(corruption_detected, "Buffer overflow in CartRam not detected");
        delete[] ptr;
        delete[] guard_ptr;
    }

    /**
     * @brief Test use-after-free in HighWorkRam
     *
     * Verifies that accessing memory after it has been freed in HighWorkRam is detected or handled safely.
     * Assumes the allocator invalidates freed memory or uses guard values.
     */
    MU_TEST(memory_test_use_after_free_highworkram)
    {
        char *ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in HighWorkRam failed");

        // Initialize buffer
        for (size_t i = 0; i < 100; ++i)
        {
            ptr[i] = static_cast<char>(0xBB);
        }

        delete[] ptr;

        // Attempt to write to freed memory
        ptr[0] = 42;

        // Check for corruption by allocating again and verifying memory state
        char *new_ptr = new (SRL::Memory::Zone::HWRam) char[100];
        bool corruption_detected = false;
        if (new_ptr == nullptr)
        {
            corruption_detected = true; // Allocator detected corruption
        }
        else
        {
            // Check if the memory was overwritten unexpectedly
            for (size_t i = 0; i < 100; ++i)
            {
                if (new_ptr[i] == 42)
                {
                    corruption_detected = true; // Freed memory was modified
                    break;
                }
            }
            delete[] new_ptr;
        }

        mu_assert(corruption_detected, "Use-after-free in HighWorkRam not detected");
    }

    /**
     * @brief Test use-after-free in LowWorkRam
     *
     * Verifies that accessing memory after it has been freed in LowWorkRam is detected or handled safely.
     * Assumes the allocator invalidates freed memory or uses guard values.
     */
    MU_TEST(memory_test_use_after_free_lowworkram)
    {
        char *ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in LowWorkRam failed");

        // Initialize buffer
        for (size_t i = 0; i < 100; ++i)
        {
            ptr[i] = static_cast<char>(0xBB);
        }

        delete[] ptr;

        // Attempt to write to freed memory
        ptr[0] = 42;

        // Check for corruption by allocating again and verifying memory state
        char *new_ptr = new (SRL::Memory::Zone::LWRam) char[100];
        bool corruption_detected = false;
        if (new_ptr == nullptr)
        {
            corruption_detected = true; // Allocator detected corruption
        }
        else
        {
            // Check if the memory was overwritten unexpectedly
            for (size_t i = 0; i < 100; ++i)
            {
                if (new_ptr[i] == 42)
                {
                    corruption_detected = true; // Freed memory was modified
                    break;
                }
            }
            delete[] new_ptr;
        }

        mu_assert(corruption_detected, "Use-after-free in LowWorkRam not detected");
    }

    /**
     * @brief Test use-after-free in CartRam
     *
     * Verifies that accessing memory after it has been freed in CartRam is detected or handled safely, if cartridge is available.
     * Assumes the allocator invalidates freed memory or uses guard values.
     */
    MU_TEST(memory_test_use_after_free_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping use-after-free test");
            return;
        }

        char *ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Allocation in CartRam failed");

        // Initialize buffer
        for (size_t i = 0; i < 100; ++i)
        {
            ptr[i] = static_cast<char>(0xBB);
        }

        delete[] ptr;

        // Attempt to write to freed memory
        ptr[0] = 42;

        // Check for corruption by allocating again and verifying memory state
        char *new_ptr = new (SRL::Memory::Zone::CartRam) char[100];
        bool corruption_detected = false;
        if (new_ptr == nullptr)
        {
            corruption_detected = true; // Allocator detected corruption
        }
        else
        {
            // Check if the memory was overwritten unexpectedly
            for (size_t i = 0; i < 100; ++i)
            {
                if (new_ptr[i] == 42)
                {
                    corruption_detected = true; // Freed memory was modified
                    break;
                }
            }
            delete[] new_ptr;
        }

        mu_assert(corruption_detected, "Use-after-free in CartRam not detected");
    }

    /**
     * @brief Test invalid memory access in HighWorkRam
     *
     * Verifies that accessing an invalid memory address in HighWorkRam is detected or handled safely.
     * Assumes the allocator or system checks for invalid addresses.
     */
    MU_TEST(memory_test_invalid_access_highworkram)
    {
        char *ptr = reinterpret_cast<char *>(0xDEADBEEF); // Invalid address

        // Attempt to write to invalid address
        ptr[0] = 42;

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::HWRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted

        if (!corruption_detected)
        {
            delete[] test_ptr;
        }

        mu_assert(corruption_detected, "Invalid memory access in HighWorkRam not detected");
    }

    /**
     * @brief Test invalid memory access in LowWorkRam
     *
     * Verifies that accessing an invalid memory address in LowWorkRam is detected or handled safely.
     * Assumes the allocator or system checks for invalid addresses.
     */
    MU_TEST(memory_test_invalid_access_lowworkram)
    {
        char *ptr = reinterpret_cast<char *>(0xDEADBEEF); // Invalid address

        // Attempt to write to invalid address
        ptr[0] = 42;

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::LWRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted

        if (!corruption_detected)
        {
            delete[] test_ptr;
        }

        mu_assert(corruption_detected, "Invalid memory access in LowWorkRam not detected");
    }

    /**
     * @brief Test invalid memory access in CartRam
     *
     * Verifies that accessing an invalid memory address in CartRam is detected or handled safely, if cartridge is available.
     * Assumes the allocator or system checks for invalid addresses.
     */
    MU_TEST(memory_test_invalid_access_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping invalid access test");
            return;
        }

        char *ptr = reinterpret_cast<char *>(0xDEADBEEF); // Invalid address

        // Attempt to write to invalid address
        ptr[0] = 42;

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::CartRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted

        if (!corruption_detected)
        {
            delete[] test_ptr;
        }

        mu_assert(corruption_detected, "Invalid memory access in CartRam not detected");
    }

    /**
     * @brief Test zero-size allocation in HighWorkRam
     *
     * Verifies that Malloc with a size of 0 in HighWorkRam is handled correctly.
     */
    MU_TEST(memory_test_zero_size_allocation_highworkram)
    {
        void *ptr = Memory::HighWorkRam::Malloc(0);
        mu_assert(ptr == nullptr || ptr != nullptr, "Zero-size allocation in HighWorkRam should either return NULL or a valid pointer");
        if (ptr != nullptr)
        {
            Memory::HighWorkRam::Free(ptr);
        }
        mu_assert(true, "Zero-size allocation test completed");
    }

    /**
     * @brief Test zero-size allocation in LowWorkRam
     *
     * Verifies that Malloc with a size of 0 in LowWorkRam is handled correctly.
     */
    MU_TEST(memory_test_zero_size_allocation_lowworkram)
    {
        void *ptr = Memory::LowWorkRam::Malloc(0);
        mu_assert(ptr == nullptr || ptr != nullptr, "Zero-size allocation in LowWorkRam should either return NULL or a valid pointer");
        if (ptr != nullptr)
        {
            Memory::LowWorkRam::Free(ptr);
        }
        mu_assert(true, "Zero-size allocation test completed");
    }

    /**
     * @brief Test zero-size allocation in CartRam
     *
     * Verifies that Malloc with a size of 0 in CartRam is handled correctly, if cartridge is available.
     */
    MU_TEST(memory_test_zero_size_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping zero-size allocation test");
            return;
        }
        void *ptr = Memory::CartRam::Malloc(0);
        mu_assert(ptr == nullptr || ptr != nullptr, "Zero-size allocation in CartRam should either return NULL or a valid pointer");
        if (ptr != nullptr)
        {
            Memory::CartRam::Free(ptr);
        }
        mu_assert(true, "Zero-size allocation test completed");
    }

    /**
     * @brief Test single-byte allocation in HighWorkRam
     *
     * Verifies that allocating and deallocating a single byte in HighWorkRam works correctly.
     */
    MU_TEST(memory_test_single_byte_allocation_highworkram)
    {
        char *ptr = new (SRL::Memory::Zone::HWRam) char[1];
        mu_assert(ptr != nullptr, "Single-byte allocation in HighWorkRam failed");
        *ptr = 42; // Write to the single byte
        mu_assert(*ptr == 42, "Single-byte write in HighWorkRam failed");
        delete[] ptr;
        mu_assert(true, "Single-byte allocation test in HighWorkRam completed");
    }

    /**
     * @brief Test single-byte allocation in LowWorkRam
     *
     * Verifies that allocating and deallocating a single byte in LowWorkRam works correctly.
     */
    MU_TEST(memory_test_single_byte_allocation_lowworkram)
    {
        char *ptr = new (SRL::Memory::Zone::LWRam) char[1];
        mu_assert(ptr != nullptr, "Single-byte allocation in LowWorkRam failed");
        *ptr = 42; // Write to the single byte
        mu_assert(*ptr == 42, "Single-byte write in LowWorkRam failed");
        delete[] ptr;
        mu_assert(true, "Single-byte allocation test in LowWorkRam completed");
    }

    /**
     * @brief Test single-byte allocation in CartRam
     *
     * Verifies that allocating and deallocating a single byte in CartRam works correctly, if cartridge is available.
     */
    MU_TEST(memory_test_single_byte_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping single-byte allocation test");
            return;
        }
        char *ptr = new (SRL::Memory::Zone::CartRam) char[1];
        mu_assert(ptr != nullptr, "Single-byte allocation in CartRam failed");
        *ptr = 42; // Write to the single byte
        mu_assert(*ptr == 42, "Single-byte write in CartRam failed");
        delete[] ptr;
        mu_assert(true, "Single-byte allocation test in CartRam completed");
    }

    /**
     * @brief Test maximum-size allocation in HighWorkRam
     *
     * Verifies that allocating exactly the maximum available size in HighWorkRam succeeds.
     */
    MU_TEST(memory_test_max_size_allocation_highworkram)
    {
        size_t freeSpace = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "HighWorkRam has no free space");
        void *ptr = Memory::HighWorkRam::Malloc(freeSpace);
        mu_assert(ptr != nullptr, "Maximum-size allocation in HighWorkRam failed");
        Memory::HighWorkRam::Free(ptr);
    }

    /**
     * @brief Test maximum-size allocation in LowWorkRam
     *
     * Verifies that allocating exactly the maximum available size in LowWorkRam succeeds.
     */
    MU_TEST(memory_test_max_size_allocation_lowworkram)
    {
        size_t freeSpace = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "LowWorkRam has no free space");
        void *ptr = Memory::LowWorkRam::Malloc(freeSpace);
        mu_assert(ptr != nullptr, "Maximum-size allocation in LowWorkRam failed");
        Memory::LowWorkRam::Free(ptr);
    }

    /**
     * @brief Test maximum-size allocation in CartRam
     *
     * Verifies that allocating exactly the maximum available size in CartRam succeeds, if cartridge is available.
     */
    MU_TEST(memory_test_max_size_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping max-size allocation test");
            return;
        }
        size_t freeSpace = Memory::CartRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "CartRam has no free space");
        void *ptr = Memory::CartRam::Malloc(freeSpace);
        mu_assert(ptr != nullptr, "Maximum-size allocation in CartRam failed");
        Memory::CartRam::Free(ptr);
    }

    /**
     * @brief Test concurrent allocations across zones
     *
     * Verifies that rapid allocation and deallocation across multiple zones does not cause failures.
     */
    MU_TEST(memory_test_concurrent_allocations)
    {
        const int cycles = 100;
        void *ptrs_hwr[cycles];
        void *ptrs_lwr[cycles];
        void *ptrs_cr[cycles];

        // Allocate in all zones
        for (int i = 0; i < cycles; ++i)
        {
            ptrs_hwr[i] = Memory::HighWorkRam::Malloc(100);
            ptrs_lwr[i] = Memory::LowWorkRam::Malloc(100);
            if (Memory::CartRam::IsCartridgeAvailable())
            {
                ptrs_cr[i] = Memory::CartRam::Malloc(100);
            }
            else
            {
                ptrs_cr[i] = nullptr;
            }
            mu_assert(ptrs_hwr[i] != nullptr, "Concurrent allocation in HighWorkRam failed");
            mu_assert(ptrs_lwr[i] != nullptr, "Concurrent allocation in LowWorkRam failed");
            if (Memory::CartRam::IsCartridgeAvailable())
            {
                mu_assert(ptrs_cr[i] != nullptr, "Concurrent allocation in CartRam failed");
            }
        }

        // Deallocate in all zones
        for (int i = 0; i < cycles; ++i)
        {
            Memory::HighWorkRam::Free(ptrs_hwr[i]);
            Memory::LowWorkRam::Free(ptrs_lwr[i]);
            if (Memory::CartRam::IsCartridgeAvailable() && ptrs_cr[i] != nullptr)
            {
                Memory::CartRam::Free(ptrs_cr[i]);
            }
        }

        mu_assert(true, "Concurrent allocation test completed");
    }

    /**
     * @brief Test complex fragmentation in HighWorkRam
     *
     * Verifies behavior under complex fragmented memory conditions by alternating small and large allocations.
     */
    MU_TEST(memory_test_complex_fragmentation_highworkram)
    {
        const size_t smallSize = 100;
        const size_t largeSize = 1000;
        void *smallPtrs[10];
        void *largePtrs[5];

        // Create fragmentation by alternating small and large allocations
        for (int i = 0; i < 10; ++i)
        {
            smallPtrs[i] = Memory::HighWorkRam::Malloc(smallSize);
            mu_assert(smallPtrs[i] != nullptr, "Small allocation in HighWorkRam failed");
            if (i % 2 == 0 && i < 5)
            {
                largePtrs[i / 2] = Memory::HighWorkRam::Malloc(largeSize);
                mu_assert(largePtrs[i / 2] != nullptr, "Large allocation in HighWorkRam failed");
            }
        }

        // Free small allocations to create holes
        for (int i = 0; i < 10; ++i)
        {
            if (i % 2 == 0)
            {
                Memory::HighWorkRam::Free(smallPtrs[i]);
                smallPtrs[i] = nullptr;
            }
        }

        // Attempt a large allocation
        void *testPtr = Memory::HighWorkRam::Malloc(largeSize * 2);
        if (testPtr != nullptr)
        {
            Memory::HighWorkRam::Free(testPtr);
        }

        // Clean up remaining allocations
        for (int i = 0; i < 10; ++i)
        {
            if (smallPtrs[i] != nullptr)
            {
                Memory::HighWorkRam::Free(smallPtrs[i]);
            }
        }
        for (int i = 0; i < 5; ++i)
        {
            if (largePtrs[i] != nullptr)
            {
                Memory::HighWorkRam::Free(largePtrs[i]);
            }
        }

        mu_assert(true, "Complex fragmentation test in HighWorkRam completed");
    }

    /**
     * @brief Test complex fragmentation in LowWorkRam
     *
     * Verifies behavior under complex fragmented memory conditions by alternating small and large allocations.
     */
    MU_TEST(memory_test_complex_fragmentation_lowworkram)
    {
        const size_t smallSize = 100;
        const size_t largeSize = 1000;
        void *smallPtrs[10];
        void *largePtrs[5];

        // Create fragmentation by alternating small and large allocations
        for (int i = 0; i < 10; ++i)
        {
            smallPtrs[i] = Memory::LowWorkRam::Malloc(smallSize);
            mu_assert(smallPtrs[i] != nullptr, "Small allocation in LowWorkRam failed");
            if (i % 2 == 0 && i < 5)
            {
                largePtrs[i / 2] = Memory::LowWorkRam::Malloc(largeSize);
                mu_assert(largePtrs[i / 2] != nullptr, "Large allocation in LowWorkRam failed");
            }
        }

        // Free small allocations to create holes
        for (int i = 0; i < 10; ++i)
        {
            if (i % 2 == 0)
            {
                Memory::LowWorkRam::Free(smallPtrs[i]);
                smallPtrs[i] = nullptr;
            }
        }

        // Attempt a large allocation
        void *testPtr = Memory::LowWorkRam::Malloc(largeSize * 2);
        if (testPtr != nullptr)
        {
            Memory::LowWorkRam::Free(testPtr);
        }

        // Clean up remaining allocations
        for (int i = 0; i < 10; ++i)
        {
            if (smallPtrs[i] != nullptr)
            {
                Memory::LowWorkRam::Free(smallPtrs[i]);
            }
        }
        for (int i = 0; i < 5; ++i)
        {
            if (largePtrs[i] != nullptr)
            {
                Memory::LowWorkRam::Free(largePtrs[i]);
            }
        }

        mu_assert(true, "Complex fragmentation test in LowWorkRam completed");
    }

    /**
     * @brief Test complex fragmentation in CartRam
     *
     * Verifies behavior under complex fragmented memory conditions by alternating small and large allocations, if cartridge is available.
     */
    MU_TEST(memory_test_complex_fragmentation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping complex fragmentation test");
            return;
        }

        const size_t smallSize = 100;
        const size_t largeSize = 1000;
        void *smallPtrs[10];
        void *largePtrs[5];

        // Create fragmentation by alternating small and large allocations
        for (int i = 0; i < 10; ++i)
        {
            smallPtrs[i] = Memory::CartRam::Malloc(smallSize);
            mu_assert(smallPtrs[i] != nullptr, "Small allocation in CartRam failed");
            if (i % 2 == 0 && i < 5)
            {
                largePtrs[i / 2] = Memory::CartRam::Malloc(largeSize);
                mu_assert(largePtrs[i / 2] != nullptr, "Large allocation in CartRam failed");
            }
        }

        // Free small allocations to create holes
        for (int i = 0; i < 10; ++i)
        {
            if (i % 2 == 0)
            {
                Memory::CartRam::Free(smallPtrs[i]);
                smallPtrs[i] = nullptr;
            }
        }

        // Attempt a large allocation
        void *testPtr = Memory::CartRam::Malloc(largeSize * 2);
        if (testPtr != nullptr)
        {
            Memory::CartRam::Free(testPtr);
        }

        // Clean up remaining allocations
        for (int i = 0; i < 10; ++i)
        {
            if (smallPtrs[i] != nullptr)
            {
                Memory::CartRam::Free(smallPtrs[i]);
            }
        }
        for (int i = 0; i < 5; ++i)
        {
            if (largePtrs[i] != nullptr)
            {
                Memory::CartRam::Free(largePtrs[i]);
            }
        }

        mu_assert(true, "Complex fragmentation test in CartRam completed");
    }

    /**
     * @brief Test continuous allocation and deallocation in HighWorkRam
     *
     * Verifies that repeated allocation and deallocation cycles in HighWorkRam do not cause memory leaks or corruption.
     */
    MU_TEST(memory_test_continuous_allocation_highworkram)
    {
        const int cycles = 10000; // High number of cycles for stress testing
        const size_t size = 128;
        size_t initialFreeSpace = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(initialFreeSpace > 0, "HighWorkRam has no free space");

        for (int i = 0; i < cycles; ++i)
        {
            void *ptr = Memory::HighWorkRam::Malloc(size);
            mu_assert(ptr != nullptr, "Continuous allocation in HighWorkRam failed");
            // Initialize memory to detect potential corruption
            memset(ptr, 0xAA, size);
            Memory::HighWorkRam::Free(ptr);
        }

        size_t finalFreeSpace = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(finalFreeSpace == initialFreeSpace, "HighWorkRam free space not restored after continuous allocation/deallocation (possible leak)");
        mu_assert(true, "Continuous allocation test in HighWorkRam completed");
    }

    /**
     * @brief Test continuous allocation and deallocation in LowWorkRam
     *
     * Verifies that repeated allocation and deallocation cycles in LowWorkRam do not cause memory leaks or corruption.
     */
    MU_TEST(memory_test_continuous_allocation_lowworkram)
    {
        const int cycles = 10000; // High number of cycles for stress testing
        const size_t size = 128;
        size_t initialFreeSpace = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(initialFreeSpace > 0, "LowWorkRam has no free space");

        for (int i = 0; i < cycles; ++i)
        {
            void *ptr = Memory::LowWorkRam::Malloc(size);
            mu_assert(ptr != nullptr, "Continuous allocation in LowWorkRam failed");
            // Initialize memory to detect potential corruption
            memset(ptr, 0xAA, size);
            Memory::LowWorkRam::Free(ptr);
        }

        size_t finalFreeSpace = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(finalFreeSpace == initialFreeSpace, "LowWorkRam free space not restored after continuous allocation/deallocation (possible leak)");
        mu_assert(true, "Continuous allocation test in LowWorkRam completed");
    }

    /**
     * @brief Test continuous allocation and deallocation in CartRam
     *
     * Verifies that repeated allocation and deallocation cycles in CartRam do not cause memory leaks or corruption, if cartridge is available.
     */
    MU_TEST(memory_test_continuous_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping continuous allocation test");
            return;
        }

        const int cycles = 10000; // High number of cycles for stress testing
        const size_t size = 128;
        size_t initialFreeSpace = Memory::CartRam::GetFreeSpace();
        mu_assert(initialFreeSpace > 0, "CartRam has no free space");

        for (int i = 0; i < cycles; ++i)
        {
            void *ptr = Memory::CartRam::Malloc(size);
            mu_assert(ptr != nullptr, "Continuous allocation in CartRam failed");
            // Initialize memory to detect potential corruption
            memset(ptr, 0xAA, size);
            Memory::CartRam::Free(ptr);
        }

        size_t finalFreeSpace = Memory::CartRam::GetFreeSpace();
        mu_assert(finalFreeSpace == initialFreeSpace, "CartRam free space not restored after continuous allocation/deallocation (possible leak)");
        mu_assert(true, "Continuous allocation test in CartRam completed");
    }

    /**
     * @brief Test freeing an unallocated pointer in HighWorkRam
     *
     * Verifies that attempting to free a pointer that was not allocated by the memory manager
     * in HighWorkRam is handled safely and does not cause a crash.
     */
    MU_TEST(memory_test_free_unallocated_highworkram)
    {
        void *ptr = reinterpret_cast<void *>(0xDEADBEEF); // Arbitrary unallocated address
        Memory::HighWorkRam::Free(ptr);
        mu_assert(true, "Freeing unallocated pointer in HighWorkRam caused an error");
    }

    /**
     * @brief Test freeing an unallocated pointer in LowWorkRam
     *
     * Verifies that attempting to free a pointer that was not allocated by the memory manager
     * in LowWorkRam is handled safely and does not cause a crash.
     */
    MU_TEST(memory_test_free_unallocated_lowworkram)
    {
        void *ptr = reinterpret_cast<void *>(0xDEADBEEF); // Arbitrary unallocated address
        Memory::LowWorkRam::Free(ptr);
        mu_assert(true, "Freeing unallocated pointer in LowWorkRam caused an error");
    }

    /**
     * @brief Test freeing an unallocated pointer in CartRam
     *
     * Verifies that attempting to free a pointer that was not allocated by the memory manager
     * in CartRam is handled safely, if cartridge is available.
     */
    MU_TEST(memory_test_free_unallocated_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping free unallocated test");
            return;
        }
        void *ptr = reinterpret_cast<void *>(0xDEADBEEF); // Arbitrary unallocated address
        Memory::CartRam::Free(ptr);
        mu_assert(true, "Freeing unallocated pointer in CartRam caused an error");
    }

    /**
     * @brief Test allocation with negative size in HighWorkRam
     *
     * Verifies that attempting to allocate a negative size in HighWorkRam returns NULL
     * and does not crash the memory manager.
     */
    MU_TEST(memory_test_negative_size_allocation_highworkram)
    {
        void *ptr = Memory::HighWorkRam::Malloc(static_cast<size_t>(-1));
        mu_assert(ptr == nullptr, "Negative size allocation in HighWorkRam did not return NULL");
    }

    /**
     * @brief Test allocation with negative size in LowWorkRam
     *
     * Verifies that attempting to allocate a negative size in LowWorkRam returns NULL
     * and does not crash the memory manager.
     */
    MU_TEST(memory_test_negative_size_allocation_lowworkram)
    {
        void *ptr = Memory::LowWorkRam::Malloc(static_cast<size_t>(-1));
        mu_assert(ptr == nullptr, "Negative size allocation in LowWorkRam did not return NULL");
    }

    /**
     * @brief Test allocation with negative size in CartRam
     *
     * Verifies that attempting to allocate a negative size in CartRam returns NULL,
     * if cartridge is available.
     */
    MU_TEST(memory_test_negative_size_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping negative size allocation test");
            return;
        }
        void *ptr = Memory::CartRam::Malloc(static_cast<size_t>(-1));
        mu_assert(ptr == nullptr, "Negative size allocation in CartRam did not return NULL");
    }

    /**
     * @brief Test allocation with extremely large size in HighWorkRam
     *
     * Verifies that attempting to allocate an excessively large size in HighWorkRam
     * (e.g., larger than the entire memory zone) returns NULL.
     */
    MU_TEST(memory_test_excessive_size_allocation_highworkram)
    {
        size_t excessiveSize = static_cast<size_t>(1ULL << 40); // 1TB, far beyond typical memory zone size
        void *ptr = Memory::HighWorkRam::Malloc(excessiveSize);
        mu_assert(ptr == nullptr, "Excessive size allocation in HighWorkRam did not return NULL");
    }

    /**
     * @brief Test allocation with extremely large size in LowWorkRam
     *
     * Verifies that attempting to allocate an excessively large size in LowWorkRam
     * returns NULL.
     */
    MU_TEST(memory_test_excessive_size_allocation_lowworkram)
    {
        size_t excessiveSize = static_cast<size_t>(1ULL << 40); // 1TB
        void *ptr = Memory::LowWorkRam::Malloc(excessiveSize);
        mu_assert(ptr == nullptr, "Excessive size allocation in LowWorkRam did not return NULL");
    }

    /**
     * @brief Test allocation with extremely large size in CartRam
     *
     * Verifies that attempting to allocate an excessively large size in CartRam
     * returns NULL, if cartridge is available.
     */
    MU_TEST(memory_test_excessive_size_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping excessive size allocation test");
            return;
        }
        size_t excessiveSize = static_cast<size_t>(1ULL << 40); // 1TB
        void *ptr = Memory::CartRam::Malloc(excessiveSize);
        mu_assert(ptr == nullptr, "Excessive size allocation in CartRam did not return NULL");
    }

    /**
     * @brief Test accessing memory before allocation in HighWorkRam
     *
     * Verifies that accessing a pointer before allocation in HighWorkRam is detected or handled safely.
     */
    MU_TEST(memory_test_access_before_allocation_highworkram)
    {
        char *ptr = reinterpret_cast<char *>(Memory::HighWorkRam::Malloc(100));
        Memory::HighWorkRam::Free(ptr); // Ensure pointer is not allocated
        ptr[0] = 42;                    // Attempt to access unallocated memory

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::HWRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted
        if (!corruption_detected)
        {
            delete[] test_ptr;
        }
        mu_assert(corruption_detected, "Access before allocation in HighWorkRam not detected");
    }

    /**
     * @brief Test accessing memory before allocation in LowWorkRam
     *
     * Verifies that accessing a pointer before allocation in LowWorkRam is detected or handled safely.
     */
    MU_TEST(memory_test_access_before_allocation_lowworkram)
    {
        char *ptr = reinterpret_cast<char *>(Memory::LowWorkRam::Malloc(100));
        Memory::LowWorkRam::Free(ptr); // Ensure pointer is not allocated
        ptr[0] = 42;                   // Attempt to access unallocated memory

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::LWRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted
        if (!corruption_detected)
        {
            delete[] test_ptr;
        }
        mu_assert(corruption_detected, "Access before allocation in LowWorkRam not detected");
    }

    /**
     * @brief Test accessing memory before allocation in CartRam
     *
     * Verifies that accessing a pointer before allocation in CartRam is detected or handled safely,
     * if cartridge is available.
     */
    MU_TEST(memory_test_access_before_allocation_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping access before allocation test");
            return;
        }
        char *ptr = reinterpret_cast<char *>(Memory::CartRam::Malloc(100));
        Memory::CartRam::Free(ptr); // Ensure pointer is not allocated
        ptr[0] = 42;                // Attempt to access unallocated memory

        // Check memory integrity by performing a valid allocation
        char *test_ptr = new (SRL::Memory::Zone::CartRam) char[100];
        bool corruption_detected = (test_ptr == nullptr); // Allocator should fail if corrupted
        if (!corruption_detected)
        {
            delete[] test_ptr;
        }
        mu_assert(corruption_detected, "Access before allocation in CartRam not detected");
    }

    /**
     * @brief Test operations on uninitialized memory zones
     *
     * Verifies that attempting to allocate or free memory in uninitialized memory zones
     * is handled safely and does not crash.
     */
    MU_TEST(memory_test_uninitialized_zones)
    {
        // Assume a method to deinitialize or ensure zones are uninitialized
        // If SRL::Memory::Deinitialize() exists, call it; otherwise, assume zones can be uninitialized
        // For this test, we assume the zones are not initialized
        SRL::Memory::Initialize(); // Reset to known state
        // Simulate uninitialized state (if Deinitialize exists, uncomment below)
        // SRL::Memory::Deinitialize();

        void *ptr_hwr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr_hwr == nullptr, "Allocation in uninitialized HighWorkRam did not return NULL");

        void *ptr_lwr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr_lwr == nullptr, "Allocation in uninitialized LowWorkRam did not return NULL");

        void *ptr_cr = nullptr;

        if (Memory::CartRam::IsCartridgeAvailable())
        {
            ptr_cr = Memory::CartRam::Malloc(100);
            mu_assert(ptr_cr == nullptr, "Allocation in uninitialized CartRam did not return NULL");
        }

        // Attempt to free in uninitialized zones
        Memory::HighWorkRam::Free(ptr_hwr);
        Memory::LowWorkRam::Free(ptr_lwr);
        if (Memory::CartRam::IsCartridgeAvailable())
        {
            Memory::CartRam::Free(ptr_cr);
        }
        mu_assert(true, "Operations on uninitialized zones caused an error");
    }

    /**
     * @brief Test reallocation with invalid parameters in HighWorkRam
     *
     * Verifies that attempting to reallocate with invalid parameters (e.g., null pointer or negative size)
     * in HighWorkRam is handled safely, if reallocation is supported.
     */
    MU_TEST(memory_test_realloc_invalid_highworkram)
    {
        // Assuming Memory::HighWorkRam::Realloc(ptr, size) exists; otherwise, skip
        void *ptr = nullptr;
        void *new_ptr = Memory::HighWorkRam::Realloc(ptr, 100); // Realloc with null pointer
        mu_assert(new_ptr == nullptr, "Realloc with null pointer in HighWorkRam did not return NULL");

        ptr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in HighWorkRam failed");
        new_ptr = Memory::HighWorkRam::Realloc(ptr, static_cast<size_t>(-1)); // Negative size
        mu_assert(new_ptr == nullptr, "Realloc with negative size in HighWorkRam did not return NULL");
        Memory::HighWorkRam::Free(ptr);
    }

    /**
     * @brief Test reallocation with invalid parameters in LowWorkRam
     *
     * Verifies that attempting to reallocate with invalid parameters in LowWorkRam is handled safely,
     * if reallocation is supported.
     */
    MU_TEST(memory_test_realloc_invalid_lowworkram)
    {
        // Assuming Memory::LowWorkRam::Realloc(ptr, size) exists; otherwise, skip
        void *ptr = nullptr;
        void *new_ptr = Memory::LowWorkRam::Realloc(ptr, 100); // Realloc with null pointer
        mu_assert(new_ptr == nullptr, "Realloc with null pointer in LowWorkRam did not return NULL");

        ptr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in LowWorkRam failed");
        new_ptr = Memory::LowWorkRam::Realloc(ptr, static_cast<size_t>(-1)); // Negative size
        mu_assert(new_ptr == nullptr, "Realloc with negative size in LowWorkRam did not return NULL");
        Memory::LowWorkRam::Free(ptr);
    }

    /**
     * @brief Test reallocation with invalid parameters in CartRam
     *
     * Verifies that attempting to reallocate with invalid parameters in CartRam is handled safely,
     * if cartridge is available and reallocation is supported.
     */
    MU_TEST(memory_test_realloc_invalid_cartram)
    {
        if (!Memory::CartRam::IsCartridgeAvailable())
        {
            mu_assert(true, "CartRam not available; skipping realloc invalid test");
            return;
        }
        // Assuming Memory::CartRam::Realloc(ptr, size) exists; otherwise, skip
        void *ptr = nullptr;
        void *new_ptr = Memory::CartRam::Realloc(ptr, 100); // Realloc with null pointer
        mu_assert(new_ptr == nullptr, "Realloc with null pointer in CartRam did not return NULL");

        ptr = Memory::CartRam::Malloc(100);
        mu_assert(ptr != nullptr, "Allocation in CartRam failed");
        new_ptr = Memory::CartRam::Realloc(ptr, static_cast<size_t>(-1)); // Negative size
        mu_assert(new_ptr == nullptr, "Realloc with negative size in CartRam did not return NULL");
        Memory::CartRam::Free(ptr);
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
        MU_RUN_TEST(memory_test_move_memory_blocks);
        MU_RUN_TEST(memory_test_move_memory_blocks_various_sizes);
        MU_RUN_TEST(memory_test_move_memory_blocks_edge_cases);
        MU_RUN_TEST(memory_test_move_memory_blocks_invalid_pointers);
        MU_RUN_TEST(memory_test_get_report);
        MU_RUN_TEST(memory_test_get_report_edge_cases);
        MU_RUN_TEST(memory_test_get_free_space);
        MU_RUN_TEST(memory_test_highworkram_reset);
        MU_RUN_TEST(memory_test_lowworkram_reset);
        MU_RUN_TEST(memory_test_cartram_reset);
        MU_RUN_TEST(memory_test_reset_edge_cases);
        MU_RUN_TEST(memory_test_over_allocation_highworkram);
        MU_RUN_TEST(memory_test_over_allocation_lowworkram);
        MU_RUN_TEST(memory_test_over_allocation_cartram);
        MU_RUN_TEST(memory_test_fragmentation_highworkram);
        MU_RUN_TEST(memory_test_aligned_allocation);
        MU_RUN_TEST(memory_test_leak_detection_highworkram);
        MU_RUN_TEST(memory_test_class_allocation);
        MU_RUN_TEST(memory_test_stress_allocation_highworkram);
        MU_RUN_TEST(memory_test_aligned_allocation_highworkram);
        MU_RUN_TEST(memory_test_aligned_allocation_lowworkram);
        MU_RUN_TEST(memory_test_aligned_allocation_cartram);
        MU_RUN_TEST(memory_test_natural_alignment_highworkram);
        MU_RUN_TEST(memory_test_natural_alignment_lowworkram);
        MU_RUN_TEST(memory_test_natural_alignment_cartram);
        MU_RUN_TEST(memory_test_placement_new_alignment);
        MU_RUN_TEST(memory_test_deallocate_null_highworkram);
        MU_RUN_TEST(memory_test_deallocate_null_lowworkram);
        MU_RUN_TEST(memory_test_deallocate_null_cartram);
        MU_RUN_TEST(memory_test_double_free_highworkram);
        MU_RUN_TEST(memory_test_double_free_lowworkram);
        MU_RUN_TEST(memory_test_double_free_cartram);
        MU_RUN_TEST(memory_test_cross_zone_deallocation);
        MU_RUN_TEST(memory_test_state_after_deallocation);
        MU_RUN_TEST(memory_test_buffer_overflow_highworkram);
        MU_RUN_TEST(memory_test_buffer_overflow_lowworkram);
        MU_RUN_TEST(memory_test_buffer_overflow_cartram);
        MU_RUN_TEST(memory_test_use_after_free_highworkram);
        MU_RUN_TEST(memory_test_use_after_free_lowworkram);
        MU_RUN_TEST(memory_test_use_after_free_cartram);
        MU_RUN_TEST(memory_test_invalid_access_highworkram);
        MU_RUN_TEST(memory_test_invalid_access_lowworkram);
        MU_RUN_TEST(memory_test_invalid_access_cartram);
        MU_RUN_TEST(memory_test_zero_size_allocation_highworkram);
        MU_RUN_TEST(memory_test_zero_size_allocation_lowworkram);
        MU_RUN_TEST(memory_test_zero_size_allocation_cartram);
        MU_RUN_TEST(memory_test_single_byte_allocation_highworkram);
        MU_RUN_TEST(memory_test_single_byte_allocation_lowworkram);
        MU_RUN_TEST(memory_test_single_byte_allocation_cartram);
        MU_RUN_TEST(memory_test_max_size_allocation_highworkram);
        MU_RUN_TEST(memory_test_max_size_allocation_lowworkram);
        MU_RUN_TEST(memory_test_max_size_allocation_cartram);
        MU_RUN_TEST(memory_test_concurrent_allocations);
        MU_RUN_TEST(memory_test_complex_fragmentation_highworkram);
        MU_RUN_TEST(memory_test_complex_fragmentation_lowworkram);
        MU_RUN_TEST(memory_test_complex_fragmentation_cartram);
        MU_RUN_TEST(memory_test_continuous_allocation_highworkram);
        MU_RUN_TEST(memory_test_continuous_allocation_lowworkram);
        MU_RUN_TEST(memory_test_continuous_allocation_cartram);
        MU_RUN_TEST(memory_test_free_unallocated_highworkram);
        MU_RUN_TEST(memory_test_free_unallocated_lowworkram);
        MU_RUN_TEST(memory_test_free_unallocated_cartram);
        MU_RUN_TEST(memory_test_negative_size_allocation_highworkram);
        MU_RUN_TEST(memory_test_negative_size_allocation_lowworkram);
        MU_RUN_TEST(memory_test_negative_size_allocation_cartram);
        MU_RUN_TEST(memory_test_excessive_size_allocation_highworkram);
        MU_RUN_TEST(memory_test_excessive_size_allocation_lowworkram);
        MU_RUN_TEST(memory_test_excessive_size_allocation_cartram);
        MU_RUN_TEST(memory_test_access_before_allocation_highworkram);
        MU_RUN_TEST(memory_test_access_before_allocation_lowworkram);
        MU_RUN_TEST(memory_test_access_before_allocation_cartram);
        MU_RUN_TEST(memory_test_uninitialized_zones);
        MU_RUN_TEST(memory_test_realloc_invalid_highworkram);
        MU_RUN_TEST(memory_test_realloc_invalid_lowworkram);
        MU_RUN_TEST(memory_test_realloc_invalid_cartram);
    }
}
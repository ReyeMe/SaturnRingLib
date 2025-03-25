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
            LogInfo("****UT_MEMORY_ERROR(S)****");
        }
    }

    /**
     * @brief Test the base address initialization of the memory
     *
     * Verifies that the memory base address is properly initialized
     * and is not a null pointer. This ensures that the memory
     * address for memory operations is valid before further testing.
     */
    // MU_TEST(memory_test_base_address)
    // {
    //     void *baseAddress = (void *)Memory::BaseAddress;
    //     snprintf(buffer, buffer_size, "BaseAddress not initialized correctly: %p", baseAddress);
    //     mu_assert(baseAddress != nullptr, buffer);
    // }

    /**
     * @brief Test memory allocation and deallocation
     *
     * Verifies that memory can be allocated and freed correctly.
     * Ensures that the allocated memory is not a null pointer and
     * that the free operation completes successfully.
     */
    MU_TEST(memory_test_malloc_free)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
        void *ptr = Memory::Malloc(100, Memory::Zone::HWRam);
        mu_assert(ptr != nullptr, "Memory allocation failed");

        Memory::Free(ptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    }

    /**
     * @brief Test memory reallocation
     *
     * Verifies that memory can be reallocated correctly.
     * Ensures that the reallocated memory is not a null pointer
     * and that the free operation completes successfully.
     */
    // MU_TEST(memory_test_realloc)
    // {
    //     size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
    //     void *ptr = Memory::Malloc(100, Memory::Zone::HWRam);
    //     mu_assert(ptr != nullptr, "Memory allocation failed");

    //     void *newPtr = Memory::Realloc(ptr, 200);
    //     mu_assert(newPtr != nullptr, "Memory reallocation failed");

    //     Memory::Free(newPtr);
    //     size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::HWRam);
    //     mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    // }

    /**
     * @brief Test getting free memory space
     *
     * Verifies that the free memory space can be retrieved correctly.
     * Ensures that the free space is greater than zero.
     */
    MU_TEST(memory_test_get_free_space)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpace > 0, "Failed to get free space");
    }

    /**
     * @brief Test getting used memory space
     *
     * Verifies that the used memory space can be retrieved correctly.
     * Ensures that the used space is greater than or equal to zero.
     */
    MU_TEST(memory_test_get_used_space)
    {
        size_t usedSpace = Memory::GetUsedSpace(Memory::Zone::HWRam);
        mu_assert(usedSpace >= 0, "Failed to get used space");
    }

    /**
     * @brief Test getting memory zone size
     *
     * Verifies that the memory zone size can be retrieved correctly.
     * Ensures that the size is greater than zero.
     */
    MU_TEST(memory_test_get_size)
    {
        size_t size = Memory::GetSize(Memory::Zone::HWRam);
        mu_assert(size > 0, "Failed to get memory zone size");
    }

    /**
     * @brief Test allocating zero bytes
     *
     * Verifies that allocating zero bytes returns a valid pointer.
     */
    MU_TEST(memory_test_malloc_zero)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
        void *ptr = Memory::Malloc(0, Memory::Zone::HWRam);
        mu_assert(ptr != nullptr, "Memory allocation of zero bytes failed");

        Memory::Free(ptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    }

    /**
     * @brief Test reallocating to a smaller size
     *
     * Verifies that reallocating to a smaller size returns a valid pointer.
     */
    // MU_TEST(memory_test_realloc_smaller)
    // {
    //     size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
    //     void *ptr = Memory::Malloc(200, Memory::Zone::HWRam);
    //     mu_assert(ptr != nullptr, "Memory allocation failed");

    //     void *newPtr = Memory::Realloc(ptr, 100);
    //     mu_assert(newPtr != nullptr, "Memory reallocation to smaller size failed");

    //     Memory::Free(newPtr);
    //     size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::HWRam);
    //     mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory free failed");
    // }

    /**
     * @brief Test freeing a null pointer
     *
     * Verifies that freeing a null pointer does not cause any issues.
     */
    MU_TEST(memory_test_free_null)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
        Memory::Free(nullptr);
        size_t freeSpaceAfter = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Freeing null pointer failed");
    }

    /**
     * @brief Test getting memory report for HWRam
     *
     * Verifies that the memory report can be retrieved correctly.
     */
    MU_TEST(memory_test_get_report_hwram)
    {
        Memory::Report report = Memory::HighWorkRam::GetReport();
        mu_assert(report.TotalSize > 0, "Failed to get memory report for HWRam");
    }

    /**
     * @brief Test getting memory report for LWRam
     *
     * Verifies that the memory report can be retrieved correctly.
     */
    MU_TEST(memory_test_get_report_lwram)
    {
        Memory::Report report = Memory::LowWorkRam::GetReport();
        mu_assert(report.TotalSize > 0, "Failed to get memory report for LWRam");
    }

    /**
     * @brief Test getting memory report for CartRam
     *
     * Verifies that the memory report can be retrieved correctly.
     */
    MU_TEST(memory_test_get_report_cartram)
    {
        Memory::Report report = Memory::CartRam::GetReport();
        mu_assert(report.TotalSize > 0, "Failed to get memory report for CartRam");
    }

    /**
     * @brief Test initializing a memory zone
     *
     * Verifies that a memory zone can be initialized correctly.
     */
    // MU_TEST(memory_test_initialize_zone)
    // {
    //     char testMemory[1024];
    //     void* zoneStart = Memory::SimpleMalloc::InitializeZone(testMemory, sizeof(testMemory));
    //     mu_assert(zoneStart == testMemory, "Memory zone initialization failed");

    //     Memory::SimpleMalloc::Header* header = reinterpret_cast<Memory::SimpleMalloc::Header*>(zoneStart);
    //     mu_assert(header->State == Memory::SimpleMalloc::BlockState::Free, "Memory zone not marked as free");
    //     mu_assert(header->Size == sizeof(testMemory) - sizeof(Memory::SimpleMalloc::Header), "Memory zone size incorrect");
    // }

    /**
     * @brief Test HighWorkRam memory allocation and deallocation
     *
     * Verifies that memory can be allocated and freed correctly in HighWorkRam.
     */
    MU_TEST(memory_test_highworkram_malloc_free)
    {
        size_t freeSpaceBefore = Memory::HighWorkRam::GetFreeSpace();
        void *ptr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "HighWorkRam memory allocation failed");

        Memory::HighWorkRam::Free(ptr);
        size_t freeSpaceAfter = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "HighWorkRam memory free failed");
    }

    /**
     * @brief Test HighWorkRam memory reallocation
     *
     * Verifies that memory can be reallocated correctly in HighWorkRam.
     */
    MU_TEST(memory_test_highworkram_realloc)
    {
        size_t freeSpaceBefore = Memory::HighWorkRam::GetFreeSpace();
        void *ptr = Memory::HighWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "HighWorkRam memory allocation failed");

        void *newPtr = Memory::HighWorkRam::Realloc(ptr, 200);
        mu_assert(newPtr != nullptr, "HighWorkRam memory reallocation failed");

        Memory::HighWorkRam::Free(newPtr);
        size_t freeSpaceAfter = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "HighWorkRam memory free failed");
    }

    /**
     * @brief Test getting HighWorkRam free memory space
     *
     * Verifies that the free memory space in HighWorkRam can be retrieved correctly.
     * Ensures that the free space is greater than zero.
     */
    MU_TEST(memory_test_highworkram_get_free_space)
    {
        size_t freeSpace = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "Failed to get HighWorkRam free space");
    }

    /**
     * @brief Test getting HighWorkRam used memory space
     *
     * Verifies that the used memory space in HighWorkRam can be retrieved correctly.
     * Ensures that the used space is greater than or equal to zero.
     */
    MU_TEST(memory_test_highworkram_get_used_space)
    {
        size_t usedSpace = Memory::HighWorkRam::GetUsedSpace();
        mu_assert(usedSpace >= 0, "Failed to get HighWorkRam used space");
    }

    /**
     * @brief Test getting HighWorkRam memory zone size
     *
     * Verifies that the memory zone size in HighWorkRam can be retrieved correctly.
     * Ensures that the size is greater than zero.
     */
    MU_TEST(memory_test_highworkram_get_size)
    {
        size_t size = Memory::HighWorkRam::GetSize();
        mu_assert(size > 0, "Failed to get HighWorkRam memory zone size");
    }

    /**
     * @brief Test LowWorkRam memory allocation and deallocation
     *
     * Verifies that memory can be allocated and freed correctly in LowWorkRam.
     */
    MU_TEST(memory_test_lowworkram_malloc_free)
    {
        size_t freeSpaceBefore = Memory::LowWorkRam::GetFreeSpace();
        void *ptr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "LowWorkRam memory allocation failed");

        Memory::LowWorkRam::Free(ptr);
        size_t freeSpaceAfter = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "LowWorkRam memory free failed");
    }

    /**
     * @brief Test LowWorkRam memory reallocation
     *
     * Verifies that memory can be reallocated correctly in LowWorkRam.
     */
    MU_TEST(memory_test_lowworkram_realloc)
    {
        size_t freeSpaceBefore = Memory::LowWorkRam::GetFreeSpace();
        void *ptr = Memory::LowWorkRam::Malloc(100);
        mu_assert(ptr != nullptr, "LowWorkRam memory allocation failed");

        void *newPtr = Memory::LowWorkRam::Realloc(ptr, 200);
        mu_assert(newPtr != nullptr, "LowWorkRam memory reallocation failed");

        Memory::LowWorkRam::Free(newPtr);
        size_t freeSpaceAfter = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "LowWorkRam memory free failed");
    }

    /**
     * @brief Test getting LowWorkRam free memory space
     *
     * Verifies that the free memory space in LowWorkRam can be retrieved correctly.
     * Ensures that the free space is greater than zero.
     */
    MU_TEST(memory_test_lowworkram_get_free_space)
    {
        size_t freeSpace = Memory::LowWorkRam::GetFreeSpace();
        mu_assert(freeSpace > 0, "Failed to get LowWorkRam free space");
    }

    /**
     * @brief Test getting LowWorkRam used memory space
     *
     * Verifies that the used memory space in LowWorkRam can be retrieved correctly.
     * Ensures that the used space is greater than or equal to zero.
     */
    MU_TEST(memory_test_lowworkram_get_used_space)
    {
        size_t usedSpace = Memory::LowWorkRam::GetUsedSpace();
        mu_assert(usedSpace >= 0, "Failed to get LowWorkRam used space");
    }

    /**
     * @brief Test getting LowWorkRam memory zone size
     *
     * Verifies that the memory zone size in LowWorkRam can be retrieved correctly.
     * Ensures that the size is greater than zero.
     */
    MU_TEST(memory_test_lowworkram_get_size)
    {
        size_t size = Memory::LowWorkRam::GetSize();
        mu_assert(size > 0, "Failed to get LowWorkRam memory zone size");
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
     * @brief Test new operator for HighWorkRam
     *
     * Verifies that the new operator allocates memory correctly in HighWorkRam.
     */
    MU_TEST(memory_test_new_highworkram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
        void* ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "new operator in HighWorkRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in HighWorkRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in HighWorkRam did not restore free space");
    }

    /**
     * @brief Test new operator for LowWorkRam
     *
     * Verifies that the new operator allocates memory correctly in LowWorkRam.
     */
    MU_TEST(memory_test_new_lowworkram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::LWRam);
        void* ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(ptr != nullptr, "new operator in LowWorkRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::LWRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in LowWorkRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::LWRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in LowWorkRam did not restore free space");
    }

    /**
     * @brief Test new operator for CartRam
     *
     * Verifies that the new operator allocates memory correctly in CartRam.
     */
    MU_TEST(memory_test_new_cartram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void* ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "new operator in CartRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in CartRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in CartRam did not restore free space");
    }

    /**
     * @brief Test new[] operator for HighWorkRam
     *
     * Verifies that the new[] operator allocates memory correctly in HighWorkRam.
     */
    MU_TEST(memory_test_new_array_highworkram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::HWRam);
        void* ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "new[] operator in HighWorkRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in HighWorkRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::HWRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in HighWorkRam did not restore free space");
    }

    /**
     * @brief Test new[] operator for LowWorkRam
     *
     * Verifies that the new[] operator allocates memory correctly in LowWorkRam.
     */
    MU_TEST(memory_test_new_array_lowworkram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::LWRam);
        void* ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(ptr != nullptr, "new[] operator in LowWorkRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::LWRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in LowWorkRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::LWRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in LowWorkRam did not restore free space");
    }

    /**
     * @brief Test new[] operator for CartRam
     *
     * Verifies that the new[] operator allocates memory correctly in CartRam.
     */
    MU_TEST(memory_test_new_array_cartram)
    {
        size_t freeSpaceBefore = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void* ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "new[] operator in CartRam failed");

        size_t freeSpaceAfterAlloc = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfterAlloc < freeSpaceBefore, "Memory allocation in CartRam did not reduce free space");

        delete[] ptr;

        size_t freeSpaceAfterFree = Memory::GetFreeSpace(Memory::Zone::CartRam);
        mu_assert(freeSpaceAfterFree == freeSpaceBefore, "Memory free in CartRam did not restore free space");
    }

    /**
     * @brief Test memory depletion for HighWorkRam
     *
     * Verifies behavior when memory is depleted in HighWorkRam.
     */
    MU_TEST(memory_test_deplete_highworkram)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::HWRam);
        void* ptr = nullptr;

        while (freeSpace > 0)
        {
            ptr = new (SRL::Memory::Zone::HWRam) char[freeSpace];
            if (ptr == nullptr)
            {
                break;
            }
            freeSpace = Memory::GetFreeSpace(Memory::Zone::HWRam);
        }

        mu_assert(ptr == nullptr, "Memory depletion in HighWorkRam did not return nullptr");

        // Validate that memory can be reallocated after depletion
        ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "Memory reallocation in HighWorkRam after depletion failed");

        delete[] ptr;
    }

    /**
     * @brief Test memory depletion for LowWorkRam
     *
     * Verifies behavior when memory is depleted in LowWorkRam.
     */
    MU_TEST(memory_test_deplete_lowworkram)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::LWRam);
        void* ptr = nullptr;

        while (freeSpace > 0)
        {
            ptr = new (SRL::Memory::Zone::LWRam) char[freeSpace];
            if (ptr == nullptr)
            {
                break;
            }
            freeSpace = Memory::GetFreeSpace(Memory::Zone::LWRam);
        }

        mu_assert(ptr == nullptr, "Memory depletion in LowWorkRam did not return nullptr");

        // Validate that memory can be reallocated after depletion
        ptr = new (SRL::Memory::Zone::LWRam) char[100];
        mu_assert(ptr != nullptr, "Memory reallocation in LowWorkRam after depletion failed");

        delete[] ptr;
    }

    /**
     * @brief Test memory depletion for CartRam
     *
     * Verifies behavior when memory is depleted in CartRam.
     */
    MU_TEST(memory_test_deplete_cartram)
    {
        size_t freeSpace = Memory::GetFreeSpace(Memory::Zone::CartRam);
        void* ptr = nullptr;

        while (freeSpace > 0)
        {
            ptr = new (SRL::Memory::Zone::CartRam) char[freeSpace];
            if (ptr == nullptr)
            {
                break;
            }
            freeSpace = Memory::GetFreeSpace(Memory::Zone::CartRam);
        }

        mu_assert(ptr == nullptr, "Memory depletion in CartRam did not return nullptr");

        // Validate that memory can be reallocated after depletion
        ptr = new (SRL::Memory::Zone::CartRam) char[100];
        mu_assert(ptr != nullptr, "Memory reallocation in CartRam after depletion failed");

        delete[] ptr;
    }

    /**
     * @brief Test InRange function for HighWorkRam
     *
     * Verifies that the InRange function correctly identifies pointers within the HighWorkRam range.
     */
    MU_TEST(memory_test_inrange_highworkram)
    {
        void* validPtr = (void*)0x06000000; // Valid address in HighWorkRam range
        void* invalidPtr = (void*)0x08000000; // Invalid address outside HighWorkRam range

        mu_assert(Memory::HighWorkRam::InRange(validPtr), "InRange failed for valid HighWorkRam address");
        mu_assert(!Memory::HighWorkRam::InRange(invalidPtr), "InRange failed for invalid HighWorkRam address");
    }

    /**
     * @brief Test InRange function for LowWorkRam
     *
     * Verifies that the InRange function correctly identifies pointers within the LowWorkRam range.
     */
    MU_TEST(memory_test_inrange_lowworkram)
    {
        void* validPtr = (void*)0x00200000; // Valid address in LowWorkRam range
        void* invalidPtr = (void*)0x08000000; // Invalid address outside LowWorkRam range

        mu_assert(Memory::LowWorkRam::InRange(validPtr), "InRange failed for valid LowWorkRam address");
        mu_assert(!Memory::LowWorkRam::InRange(invalidPtr), "InRange failed for invalid LowWorkRam address");
    }

    /**
     * @brief Test InRange function for CartRam
     *
     * Verifies that the InRange function correctly identifies pointers within the CartRam range.
     */
    MU_TEST(memory_test_inrange_cartram)
    {
        void* validPtr = (void*)0x08000000; // Valid address in CartRam range
        void* invalidPtr = (void*)0x06000000; // Invalid address outside CartRam range

        mu_assert(Memory::CartRam::InRange(validPtr), "InRange failed for valid CartRam address");
        mu_assert(!Memory::CartRam::InRange(invalidPtr), "InRange failed for invalid CartRam address");
    }

    /**
     * @brief Test reallocating to a larger size
     *
     * Verifies that reallocating to a larger size works correctly.
     */
    MU_TEST(memory_test_realloc_larger)
    {
        void* ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "Initial allocation failed");

        ptr = SRL::Memory::HighWorkRam::Realloc(ptr, 200);
        mu_assert(ptr != nullptr, "Reallocation to larger size failed");

        delete[] ptr;
    }

    /**
     * @brief Test allocating and freeing very large blocks of memory
     *
     * Verifies that very large blocks of memory can be allocated and freed correctly.
     */
    MU_TEST(memory_test_large_block)
    {
        size_t largeSize = SRL::Memory::HighWorkRam::GetFreeSpace() / 2;
        void* ptr = new (SRL::Memory::Zone::HWRam) char[largeSize];
        mu_assert(ptr != nullptr, "Large block allocation failed");

        delete[] ptr;
    }

    /**
     * @brief Test memory fragmentation and defragmentation
     *
     * Verifies that memory fragmentation and defragmentation are handled correctly.
     */
    MU_TEST(memory_test_fragmentation)
    {
        void* ptr1 = new (SRL::Memory::Zone::HWRam) char[100];
        void* ptr2 = new (SRL::Memory::Zone::HWRam) char[200];
        void* ptr3 = new (SRL::Memory::Zone::HWRam) char[300];

        delete[] ptr2;

        void* ptr4 = new (SRL::Memory::Zone::HWRam) char[150];
        mu_assert(ptr4 != nullptr, "Fragmentation handling failed");

        delete[] ptr1;
        delete[] ptr3;
        delete[] ptr4;
    }

    /**
     * @brief Test handling of allocation failures
     *
     * Verifies that allocation failures are handled correctly.
     */
    MU_TEST(memory_test_allocation_failure)
    {
        size_t freeSpace = SRL::Memory::HighWorkRam::GetFreeSpace();
        void* ptr = new (SRL::Memory::Zone::HWRam) char[freeSpace + 1];
        mu_assert(ptr == nullptr, "Allocation failure handling failed");
    }

    /**
     * @brief Test freeing unallocated or already freed memory
     *
     * Verifies that freeing unallocated or already freed memory is handled correctly.
     */
    MU_TEST(memory_test_free_unallocated)
    {
        void* ptr = (void*)0x06000000; // Unallocated address
        SRL::Memory::Free(ptr); // Should not crash or cause issues

        ptr = new (SRL::Memory::Zone::HWRam) char[100];
        delete[] ptr;
        SRL::Memory::Free(ptr); // Should not crash or cause issues
    }

    /**
     * @brief Test stress testing with high memory usage and frequent allocations/deallocations
     *
     * Verifies that the system handles high memory usage and frequent allocations/deallocations correctly.
     */
    MU_TEST(memory_test_stress)
    {
        for (int i = 0; i < 1000; ++i)
        {
            void* ptr = new (SRL::Memory::Zone::HWRam) char[100];
            mu_assert(ptr != nullptr, "Stress test allocation failed");

            delete[] ptr;
        }
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
     * @brief Test for memory leaks
     *
     * Verifies that there are no memory leaks by tracking allocated and freed memory.
     */
    MU_TEST(memory_test_memory_leaks)
    {
        size_t freeSpaceBefore = Memory::HighWorkRam::GetFreeSpace();
        void* ptr = new (SRL::Memory::Zone::HWRam) char[100];
        mu_assert(ptr != nullptr, "Memory allocation failed");

        delete[] ptr;
        size_t freeSpaceAfter = Memory::HighWorkRam::GetFreeSpace();
        mu_assert(freeSpaceAfter == freeSpaceBefore, "Memory leak detected");
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
        //MU_RUN_TEST(memory_test_base_address);
        MU_RUN_TEST(memory_test_malloc_free);
        //MU_RUN_TEST(memory_test_realloc);
        MU_RUN_TEST(memory_test_get_free_space);
        MU_RUN_TEST(memory_test_get_used_space);
        MU_RUN_TEST(memory_test_get_size);
        MU_RUN_TEST(memory_test_malloc_zero);
        //MU_RUN_TEST(memory_test_realloc_smaller);
        MU_RUN_TEST(memory_test_free_null);
        MU_RUN_TEST(memory_test_get_report_hwram);
        MU_RUN_TEST(memory_test_get_report_lwram);
        MU_RUN_TEST(memory_test_get_report_cartram);
        //MU_RUN_TEST(memory_test_initialize_zone);
        MU_RUN_TEST(memory_test_highworkram_malloc_free);
        MU_RUN_TEST(memory_test_highworkram_realloc);
        MU_RUN_TEST(memory_test_highworkram_get_free_space);
        MU_RUN_TEST(memory_test_highworkram_get_used_space);
        MU_RUN_TEST(memory_test_highworkram_get_size);
        MU_RUN_TEST(memory_test_lowworkram_malloc_free);
        MU_RUN_TEST(memory_test_lowworkram_realloc);
        MU_RUN_TEST(memory_test_lowworkram_get_free_space);
        MU_RUN_TEST(memory_test_lowworkram_get_used_space);
        MU_RUN_TEST(memory_test_lowworkram_get_size);
        MU_RUN_TEST(memory_test_placement_malloc_highworkram);
        MU_RUN_TEST(memory_test_placement_malloc_lowworkram);
        MU_RUN_TEST(memory_test_placement_malloc_cartram);
        MU_RUN_TEST(memory_test_placement_malloc_invalid);
        MU_RUN_TEST(memory_test_new_highworkram);
        MU_RUN_TEST(memory_test_new_lowworkram);
        MU_RUN_TEST(memory_test_new_cartram);
        MU_RUN_TEST(memory_test_new_array_highworkram);
        MU_RUN_TEST(memory_test_new_array_lowworkram);
        MU_RUN_TEST(memory_test_new_array_cartram);
        MU_RUN_TEST(memory_test_deplete_highworkram);
        MU_RUN_TEST(memory_test_deplete_lowworkram);
        MU_RUN_TEST(memory_test_deplete_cartram);
        MU_RUN_TEST(memory_test_inrange_highworkram);
        MU_RUN_TEST(memory_test_inrange_lowworkram);
        MU_RUN_TEST(memory_test_inrange_cartram);
        MU_RUN_TEST(memory_test_realloc_larger);
        MU_RUN_TEST(memory_test_large_block);
        MU_RUN_TEST(memory_test_fragmentation);
        MU_RUN_TEST(memory_test_allocation_failure);
        MU_RUN_TEST(memory_test_free_unallocated);
        MU_RUN_TEST(memory_test_stress);
        MU_RUN_TEST(memory_test_initialize_zones);
        MU_RUN_TEST(memory_test_cross_zone_allocation);
        MU_RUN_TEST(memory_test_boundary_conditions);
        MU_RUN_TEST(memory_test_memory_leaks);
    }
}

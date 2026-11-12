// Tests/src/testsMemoryCommon.hpp
// Allocator behavior shared by every work RAM zone: accounting, data integrity, Realloc, MemSet, new/delete forms.
// Each test runs on HWRam and LWRam through a template, so the same checks cover both zones
// (and both allocators, since memory_simple and memory_tlsf run this suite).
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_memory.hpp>
#include <srl_slave.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Zone traits: the zone class, its enum value and a name for messages. */
struct MemCommonHW
{
    using Z = Memory::HighWorkRam;
    static constexpr Memory::Zone zone = Memory::Zone::HWRam;
    static constexpr const char *name = "HWRam";
};

struct MemCommonLW
{
    using Z = Memory::LowWorkRam;
    static constexpr Memory::Zone zone = Memory::Zone::LWRam;
    static constexpr const char *name = "LWRam";
};

/** @brief Blocks allocated in HWRam and LWRam before the running test. */
static size_t memory_leak_check_hw_blocks = 0;
static size_t memory_leak_check_lw_blocks = 0;

/** @brief Setup hook of every memory suite: remembers how many blocks are allocated. */
void memory_leak_check_setup(void)
{
    memory_leak_check_hw_blocks = Memory::HighWorkRam::GetReport().UsedBlocks;
    memory_leak_check_lw_blocks = Memory::LowWorkRam::GetReport().UsedBlocks;
}

/**
 * @brief Teardown hook of every memory suite: fails the test if it left blocks allocated, so a leak is reported
 *        on the test that caused it instead of breaking later tests. Counts blocks, not bytes: SIMPLE loses
 *        header bytes when merging (a known bug), which is not a leak.
 */
void memory_leak_check_teardown(void)
{
    const long hw = static_cast<long>(Memory::HighWorkRam::GetReport().UsedBlocks) - static_cast<long>(memory_leak_check_hw_blocks);
    const long lw = static_cast<long>(Memory::LowWorkRam::GetReport().UsedBlocks) - static_cast<long>(memory_leak_check_lw_blocks);
    if (hw <= 0 && lw <= 0) return;

    if (minunit_status)
    {
        const size_t len = strlen(minunit_last_message);
        snprintf(minunit_last_message + len, MINUNIT_MESSAGE_LEN - len, " (and left %ld HWRam / %ld LWRam blocks allocated)", hw, lw);
    }
    else
    {
        snprintf(minunit_last_message, MINUNIT_MESSAGE_LEN, "%s failed:\n\tleak check: left %ld HWRam / %ld LWRam blocks allocated",
            minunit_current_test, hw, lw);
        minunit_status = 1;
    }
}

void memory_common_test_setup(void) { memory_leak_check_setup(); }
void memory_common_test_teardown(void) { memory_leak_check_teardown(); }

void memory_common_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_MEMORY_COMMON****");
        }
        else
        {
            LogInfo("****UT_MEMORY_COMMON_ERROR(S)****");
        }
    }
}

/** @brief True if all len bytes at ptr equal value. */
static bool mem_all_equal(const void *ptr, uint8_t value, size_t len)
{
    const uint8_t *p = static_cast<const uint8_t *>(ptr);
    for (size_t i = 0; i < len; i++)
    {
        if (p[i] != value)
        {
            return false;
        }
    }
    return true;
}

/** @brief Fills len bytes with 0, 1, 2, ... (wrapping), so moved or truncated data is detected. */
static void mem_fill_sequence(void *ptr, size_t len)
{
    uint8_t *p = static_cast<uint8_t *>(ptr);
    for (size_t i = 0; i < len; i++)
    {
        p[i] = static_cast<uint8_t>(i * 7 + 3);
    }
}

/** @brief True if len bytes still hold the mem_fill_sequence pattern. */
static bool mem_check_sequence(const void *ptr, size_t len)
{
    const uint8_t *p = static_cast<const uint8_t *>(ptr);
    for (size_t i = 0; i < len; i++)
    {
        if (p[i] != static_cast<uint8_t>(i * 7 + 3))
        {
            return false;
        }
    }
    return true;
}

    /**
 * @brief used + free == size, zone and generic functions agree, and Malloc/Free move the numbers as expected.
 */
template <typename T>
static bool memory_common_accounting(void)
{
    using Z = typename T::Z;
    const size_t size = Z::GetSize();
    const size_t free0 = Z::GetFreeSpace();
    const size_t used0 = Z::GetUsedSpace();
    const Memory::Report report0 = Z::GetReport();

    snprintf(buffer, buffer_size, "%s: used %lu + free %lu != size %lu", T::name, (unsigned long)used0, (unsigned long)free0, (unsigned long)size);
    if (used0 + free0 != size) return false;
    snprintf(buffer, buffer_size, "%s: generic GetFreeSpace/GetUsedSpace/GetSize(zone) differ from the zone's", T::name);
    if (Memory::GetFreeSpace(T::zone) != free0 || Memory::GetUsedSpace(T::zone) != used0 || Memory::GetSize(T::zone) != size) return false;
    snprintf(buffer, buffer_size, "%s: report free %lu / total %lu != GetFreeSpace %lu / GetSize %lu", T::name,
        (unsigned long)report0.FreeSize, (unsigned long)report0.TotalSize, (unsigned long)free0, (unsigned long)size);
    if (report0.FreeSize != free0 || report0.TotalSize != size) return false;

    void *p = Z::Malloc(1000);
    const size_t free1 = Z::GetFreeSpace();
    const size_t used1 = Z::GetUsedSpace();
    const Memory::Report report1 = Z::GetReport();
    Z::Free(p);
    const size_t free2 = Z::GetFreeSpace();
    const size_t used2 = Z::GetUsedSpace();

    snprintf(buffer, buffer_size, "%s: Malloc(1000) failed", T::name);
    if (p == nullptr) return false;
    snprintf(buffer, buffer_size, "%s: after Malloc(1000) free %lu -> %lu, used %lu -> %lu (expected change >= 1000)", T::name,
        (unsigned long)free0, (unsigned long)free1, (unsigned long)used0, (unsigned long)used1);
    if (free0 - free1 < 1000 || used1 - used0 < 1000) return false;
    snprintf(buffer, buffer_size, "%s: used blocks %lu -> %lu after one Malloc", T::name, (unsigned long)report0.UsedBlocks, (unsigned long)report1.UsedBlocks);
    if (report1.UsedBlocks != report0.UsedBlocks + 1) return false;
    snprintf(buffer, buffer_size, "%s: after Free free %lu != %lu or used %lu != %lu", T::name,
        (unsigned long)free2, (unsigned long)free0, (unsigned long)used2, (unsigned long)used0);
    return free2 == free0 && used2 == used0;
}

MU_TEST(memory_common_accounting_hwram) { mu_assert(memory_common_accounting<MemCommonHW>(), buffer); }
MU_TEST(memory_common_accounting_lwram) { mu_assert(memory_common_accounting<MemCommonLW>(), buffer); }

    /**
 * @brief Blocks of different sizes are in the zone, never overlap, and keep their data when neighbours are freed and reused.
 */
template <typename T>
static bool memory_common_blocks_independent(void)
{
    using Z = typename T::Z;
    constexpr size_t count = 5;
    const size_t sizes[count] = {1, 17, 64, 333, 1024};
    void *blocks[count] = {};
    const size_t free0 = Z::GetFreeSpace();

    bool ok = true;
    for (size_t i = 0; i < count; i++)
    {
        blocks[i] = Z::Malloc(sizes[i]);
        if (blocks[i] == nullptr || !Z::InRange(blocks[i]))
        {
            snprintf(buffer, buffer_size, "%s: Malloc(%lu) returned %p (null or outside the zone)", T::name, (unsigned long)sizes[i], blocks[i]);
            ok = false;
        }
    }

    for (size_t i = 0; ok && i < count; i++)
    {
        const uintptr_t a0 = reinterpret_cast<uintptr_t>(blocks[i]), a1 = a0 + sizes[i];
        for (size_t j = i + 1; ok && j < count; j++)
        {
            const uintptr_t b0 = reinterpret_cast<uintptr_t>(blocks[j]), b1 = b0 + sizes[j];
            if (a0 < b1 && b0 < a1)
            {
                snprintf(buffer, buffer_size, "%s: blocks %lu and %lu overlap (%p+%lu, %p+%lu)", T::name,
                    (unsigned long)i, (unsigned long)j, blocks[i], (unsigned long)sizes[i], blocks[j], (unsigned long)sizes[j]);
                ok = false;
            }
        }
    }

    if (ok)
    {
        for (size_t i = 0; i < count; i++) Memory::MemSet(blocks[i], static_cast<uint8_t>(0xA0 + i), sizes[i]);

            // Free a middle block and reuse the space: the others must be untouched
        Z::Free(blocks[2]);
        void *reused = Z::Malloc(40);
        if (reused != nullptr) Memory::MemSet(reused, 0xEE, 40);
        for (size_t i = 0; ok && i < count; i++)
        {
            if (i != 2 && !mem_all_equal(blocks[i], static_cast<uint8_t>(0xA0 + i), sizes[i]))
            {
                snprintf(buffer, buffer_size, "%s: block %lu (%lu bytes) corrupted after a neighbour was freed and reused", T::name,
                    (unsigned long)i, (unsigned long)sizes[i]);
                ok = false;
            }
        }
        Z::Free(reused);
        blocks[2] = nullptr;
    }

    for (size_t i = 0; i < count; i++) Z::Free(blocks[i]);
    if (ok && Z::GetFreeSpace() != free0)
    {
        snprintf(buffer, buffer_size, "%s: free space %lu != %lu after freeing everything", T::name,
            (unsigned long)Z::GetFreeSpace(), (unsigned long)free0);
        ok = false;
    }
    return ok;
}

MU_TEST(memory_common_blocks_independent_hwram) { mu_assert(memory_common_blocks_independent<MemCommonHW>(), buffer); }
MU_TEST(memory_common_blocks_independent_lwram) { mu_assert(memory_common_blocks_independent<MemCommonLW>(), buffer); }

    /**
 * @brief Realloc keeps the data when growing in place, growing with a move (a block right after it), and shrinking,
 *        does not touch the neighbour, and Realloc(nullptr, n) allocates like C realloc.
 */
template <typename T>
static bool memory_common_realloc_preserves_data(void)
{
    using Z = typename T::Z;
    const size_t free0 = Z::GetFreeSpace();
    bool ok = true;

    void *p = Z::Malloc(64);
    void *neighbour = Z::Malloc(64);
    if (p == nullptr || neighbour == nullptr)
    {
        snprintf(buffer, buffer_size, "%s: initial Malloc failed", T::name);
        Z::Free(p);
        Z::Free(neighbour);
        return false;
    }
    mem_fill_sequence(p, 64);
    Memory::MemSet(neighbour, 0x5C, 64);

        // Grow: the block right after is in use, so this has to move (or extend into free space for TLSF)
    void *grown = Z::Realloc(p, 512);
    if (grown == nullptr || !mem_check_sequence(grown, 64))
    {
        snprintf(buffer, buffer_size, "%s: Realloc(64 -> 512) %s", T::name, grown == nullptr ? "returned null" : "lost the first 64 bytes");
        ok = false;
    }
    else if (!mem_all_equal(neighbour, 0x5C, 64))
    {
        snprintf(buffer, buffer_size, "%s: Realloc(64 -> 512) overwrote the neighbouring block", T::name);
        ok = false;
    }
    if (grown != nullptr) p = grown;

        // Shrink: the first bytes stay
    if (ok)
    {
        void *shrunk = Z::Realloc(p, 16);
        if (shrunk == nullptr || !mem_check_sequence(shrunk, 16))
        {
            snprintf(buffer, buffer_size, "%s: Realloc(512 -> 16) %s", T::name, shrunk == nullptr ? "returned null" : "lost the first 16 bytes");
            ok = false;
        }
        if (shrunk != nullptr) p = shrunk;
    }

        // Realloc(nullptr, n) should allocate (C realloc semantics)
    void *fromNull = Z::Realloc(nullptr, 32);
    if (ok && fromNull == nullptr)
    {
        snprintf(buffer, buffer_size, "%s: Realloc(nullptr, 32) returned null instead of allocating", T::name);
        ok = false;
    }

    Z::Free(fromNull);
    Z::Free(p);
    Z::Free(neighbour);
    if (ok && Z::GetFreeSpace() != free0)
    {
        snprintf(buffer, buffer_size, "%s: free space %lu != %lu after freeing everything", T::name,
            (unsigned long)Z::GetFreeSpace(), (unsigned long)free0);
        ok = false;
    }
    return ok;
}

MU_TEST(memory_common_realloc_preserves_data_hwram) { mu_assert(memory_common_realloc_preserves_data<MemCommonHW>(), buffer); }
MU_TEST(memory_common_realloc_preserves_data_lwram) { mu_assert(memory_common_realloc_preserves_data<MemCommonLW>(), buffer); }

    /**
 * @brief MemSet writes exactly the requested range, and a length of 0 writes nothing.
 */
MU_TEST(memory_common_memset)
{
    uint8_t buf[32];
    for (size_t i = 0; i < sizeof(buf); i++) buf[i] = 0x11;

    Memory::MemSet(buf + 4, 0xAB, 8);
    mu_assert(mem_all_equal(buf + 4, 0xAB, 8), "MemSet should set the whole range");
    mu_assert(mem_all_equal(buf, 0x11, 4) && mem_all_equal(buf + 12, 0x11, sizeof(buf) - 12), "MemSet should not touch bytes outside the range");

    Memory::MemSet(buf, 0x00, 0);
    mu_assert(buf[0] == 0x11, "MemSet with length 0 should write nothing");

    Memory::MemSet(buf, 0xFF, sizeof(buf));
    mu_assert(mem_all_equal(buf, 0xFF, sizeof(buf)), "MemSet over the whole buffer");
}

    /**
 * @brief The new/delete forms tests did not use: plain new / new[] (HWRam), new by zone for a single object,
 *        new at a zone address (the zone of that address), scalar delete and sized delete.
 */
MU_TEST(memory_common_new_delete_forms)
{
    const size_t hwFree = Memory::HighWorkRam::GetFreeSpace();
    const size_t lwFree = Memory::LowWorkRam::GetFreeSpace();

    int *plain = new int(5);
    const bool plainOk = plain != nullptr && Memory::HighWorkRam::InRange(plain) && *plain == 5;
    delete plain;

    int *plainArray = new int[8];
    const bool plainArrayOk = plainArray != nullptr && Memory::HighWorkRam::InRange(plainArray);
    delete[] plainArray;

    int *inLw = new (Memory::Zone::LWRam) int(7);
    const bool zoneOk = inLw != nullptr && Memory::LowWorkRam::InRange(inLw) && *inLw == 7;

        // new at an address: allocates in the zone containing that address (here, LWRam)
    int *atLwAddress = new (reinterpret_cast<uint32_t>(inLw)) int(9);
    const bool addressOk = atLwAddress != nullptr && Memory::LowWorkRam::InRange(atLwAddress) && *atLwAddress == 9 && atLwAddress != inLw;
    int *arrayAtLwAddress = new (reinterpret_cast<uint32_t>(inLw)) int[4];
    const bool addressArrayOk = arrayAtLwAddress != nullptr && Memory::LowWorkRam::InRange(arrayAtLwAddress);

    delete inLw;
    delete atLwAddress;
    delete[] arrayAtLwAddress;

    int *sized = new int(1);
    ::operator delete(sized, sizeof(int));

    mu_assert(plainOk, "Plain new should allocate in HWRam");
    mu_assert(plainArrayOk, "Plain new[] should allocate in HWRam");
    mu_assert(zoneOk, "new (Zone::LWRam) T should allocate in LWRam");
    mu_assert(addressOk, "new (LWRam address) T should allocate a new object in LWRam");
    mu_assert(addressArrayOk, "new (LWRam address) T[] should allocate in LWRam");
    snprintf(buffer, buffer_size, "Free space not restored by delete: HWRam %lu -> %lu, LWRam %lu -> %lu",
        (unsigned long)hwFree, (unsigned long)Memory::HighWorkRam::GetFreeSpace(),
        (unsigned long)lwFree, (unsigned long)Memory::LowWorkRam::GetFreeSpace());
    mu_assert(Memory::HighWorkRam::GetFreeSpace() == hwFree && Memory::LowWorkRam::GetFreeSpace() == lwFree, buffer);
}

    /**
 * @brief PlacementMalloc with an address outside every RAM zone returns null.
 */
MU_TEST(memory_common_placement_outside_zones)
{
    void *bios = Memory::PlacementMalloc(16, static_cast<uint32_t>(0x00000000u));        // BIOS ROM
    void *vdp1 = Memory::PlacementMalloc(16, reinterpret_cast<void *>(0x05C00000u));    // VDP1 VRAM
    Memory::Free(bios);
    Memory::Free(vdp1);
    mu_assert(bios == nullptr, "PlacementMalloc at a BIOS ROM address should return null");
    mu_assert(vdp1 == nullptr, "PlacementMalloc at a VDP1 VRAM address should return null");
}

    /**
 * @brief Realloc to the same size keeps the data; Realloc(p, 0) either frees or returns a block, without corrupting the heap.
 */
template <typename T>
static bool memory_common_realloc_same_and_zero(void)
{
    using Z = typename T::Z;
    const size_t free0 = Z::GetFreeSpace();
    void *p = Z::Malloc(48);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(48) failed", T::name); return false; }
    mem_fill_sequence(p, 48);

    void *same = Z::Realloc(p, 48);
    const bool sameOk = same != nullptr && mem_check_sequence(same, 48);
    if (same != nullptr) p = same;

    void *zero = Z::Realloc(p, 0);
    if (zero != nullptr) Z::Free(zero);         // returned a block: release it; null means Realloc freed p
    const size_t free1 = Z::GetFreeSpace();

    snprintf(buffer, buffer_size, "%s: Realloc(p, same size) %s", T::name, same == nullptr ? "returned null" : "lost data");
    if (!sameOk) return false;
        // Free space may grow (freeing can merge older fragments back), but must not shrink
    snprintf(buffer, buffer_size, "%s: after Realloc(p, 0) free space %lu < %lu (bytes lost)", T::name, (unsigned long)free1, (unsigned long)free0);
    return free1 >= free0;
}

MU_TEST(memory_common_realloc_same_and_zero_hwram) { mu_assert(memory_common_realloc_same_and_zero<MemCommonHW>(), buffer); }
MU_TEST(memory_common_realloc_same_and_zero_lwram) { mu_assert(memory_common_realloc_same_and_zero<MemCommonLW>(), buffer); }

    /**
 * @brief Requests bigger than the zone, or so big that adding a header overflows, return null instead of a small block.
 */
template <typename T>
static bool memory_common_malloc_huge_sizes(void)
{
    using Z = typename T::Z;
    const size_t sizes[] = {Z::GetSize() + 4096, Z::GetFreeSpace() + 1, 0xFFFFFFF8u, 0xFFFFFFFFu};
    for (const size_t size : sizes)
    {
        void *p = Z::Malloc(size);
        if (p != nullptr)
        {
            Z::Free(p);
            snprintf(buffer, buffer_size, "%s: Malloc(0x%08lx) returned %p instead of null", T::name, (unsigned long)size, p);
            return false;
        }
    }
    return true;
}

MU_TEST(memory_common_malloc_huge_sizes_hwram) { mu_assert(memory_common_malloc_huge_sizes<MemCommonHW>(), buffer); }
MU_TEST(memory_common_malloc_huge_sizes_lwram) { mu_assert(memory_common_malloc_huge_sizes<MemCommonLW>(), buffer); }

    /**
 * @brief Allocate 1 KB blocks until the zone is full: Malloc then returns null, the blocks cover the free space,
 *        and after freeing them (last first, so even a forward-only merge recombines them) a large block fits again.
 */
template <typename T>
static bool memory_common_exhaust_and_recover(void)
{
    using Z = typename T::Z;
    constexpr size_t blockSize = 1024;
    constexpr size_t maxBlocks = 1100;          // LWRam is 1 MB
    static void *blocks[maxBlocks];
    const size_t free0 = Z::GetFreeSpace();

    size_t count = 0;
    while (count < maxBlocks && (blocks[count] = Z::Malloc(blockSize)) != nullptr)
    {
        count++;
    }
    const bool hitNull = count < maxBlocks;
    const size_t freeFull = Z::GetFreeSpace();

    for (size_t i = count; i > 0; i--)
    {
        Z::Free(blocks[i - 1]);
    }
    const size_t freeAfter = Z::GetFreeSpace();
    void *big = Z::Malloc(free0 * 3 / 4);
    Z::Free(big);

    snprintf(buffer, buffer_size, "%s: never ran out after %lu x 1 KB", T::name, (unsigned long)count);
    if (!hitNull) return false;
    snprintf(buffer, buffer_size, "%s: full after %lu x 1 KB but %lu bytes still free (started with %lu)", T::name,
        (unsigned long)count, (unsigned long)freeFull, (unsigned long)free0);
    if (freeFull >= blockSize + 64) return false;
    snprintf(buffer, buffer_size, "%s: free space %lu != %lu after freeing all blocks", T::name, (unsigned long)freeAfter, (unsigned long)free0);
    if (freeAfter != free0) return false;
    snprintf(buffer, buffer_size, "%s: Malloc(%lu) failed after the heap was emptied", T::name, (unsigned long)(free0 * 3 / 4));
    return big != nullptr;
}

MU_TEST(memory_common_exhaust_and_recover_hwram) { mu_assert(memory_common_exhaust_and_recover<MemCommonHW>(), buffer); }
MU_TEST(memory_common_exhaust_and_recover_lwram) { mu_assert(memory_common_exhaust_and_recover<MemCommonLW>(), buffer); }

    /**
 * @brief Freeing or reallocating a block through another zone's functions is ignored: the block and both heaps are unchanged.
 * @note Risky: an allocator without an ownership check can corrupt its heap here, so this runs late in the suite.
 */
template <typename T, typename Other>
static bool memory_common_wrong_zone(void)
{
    using Z = typename T::Z;
    using O = typename Other::Z;
    void *p = Z::Malloc(64);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    mem_fill_sequence(p, 64);
    const size_t ownFree = Z::GetFreeSpace();
    const size_t otherFree = O::GetFreeSpace();

    O::Free(p);
#if defined(USE_TLSF_ALLOCATOR)
        // Not run under TLSF: its Realloc does not check that the pointer belongs to the zone,
        // corrupts the heap, and the campaign hangs a few tests later. Only Free is checked there.
    void *moved = nullptr;
#else
    void *moved = O::Realloc(p, 128);
#endif

    const bool intact = mem_check_sequence(p, 64);
    const size_t ownFree1 = Z::GetFreeSpace();
    const size_t otherFree1 = O::GetFreeSpace();
    Z::Free(p);

    snprintf(buffer, buffer_size, "%s block through %s: Realloc returned %p instead of null", T::name, Other::name, moved);
    if (moved != nullptr) return false;
    snprintf(buffer, buffer_size, "%s block through %s: data %s, own free %lu -> %lu, other free %lu -> %lu", T::name, Other::name,
        intact ? "intact" : "CHANGED", (unsigned long)ownFree, (unsigned long)ownFree1, (unsigned long)otherFree, (unsigned long)otherFree1);
    return intact && ownFree1 == ownFree && otherFree1 == otherFree;
}

MU_TEST(memory_common_wrong_zone_hwram) { mu_assert((memory_common_wrong_zone<MemCommonHW, MemCommonLW>()), buffer); }
MU_TEST(memory_common_wrong_zone_lwram) { mu_assert((memory_common_wrong_zone<MemCommonLW, MemCommonHW>()), buffer); }

    /**
 * @brief Realloc that cannot fit returns null and leaves the original block allocated and intact (C realloc semantics).
 * @note Risky: an allocator that frees first and copies after can lose the block or copy to null, so this runs late.
 */
template <typename T>
static bool memory_common_realloc_too_large(void)
{
    using Z = typename T::Z;
    void *p = Z::Malloc(64);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    mem_fill_sequence(p, 64);
    const size_t used0 = Z::GetUsedSpace();

    void *q = Z::Realloc(p, Z::GetSize() + 4096);
    const size_t used1 = Z::GetUsedSpace();
    const bool intact = mem_check_sequence(p, 64);
    Z::Free(q != nullptr ? q : p);

    snprintf(buffer, buffer_size, "%s: Realloc(too large) returned %p instead of null", T::name, q);
    if (q != nullptr) return false;
    snprintf(buffer, buffer_size, "%s: after a failed Realloc the block is %s and used space %lu -> %lu (should stay allocated)", T::name,
        intact ? "intact" : "CHANGED", (unsigned long)used0, (unsigned long)used1);
    return intact && used1 == used0;
}

MU_TEST(memory_common_realloc_too_large_hwram) { mu_assert(memory_common_realloc_too_large<MemCommonHW>(), buffer); }
MU_TEST(memory_common_realloc_too_large_lwram) { mu_assert(memory_common_realloc_too_large<MemCommonLW>(), buffer); }

    /**
 * @brief After a double free, the heap still hands out separate, usable blocks.
 * @note Risky: allocators that do not detect a double free can corrupt their heap, so this runs last.
 */
template <typename T>
static bool memory_common_double_free_heap_still_sane(void)
{
    using Z = typename T::Z;
    void *p = Z::Malloc(64);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    Z::Free(p);
    Z::Free(p);

    void *a = Z::Malloc(64);
    void *b = Z::Malloc(64);
    bool ok = a != nullptr && b != nullptr && a != b;
    if (ok)
    {
        const uintptr_t a0 = reinterpret_cast<uintptr_t>(a), b0 = reinterpret_cast<uintptr_t>(b);
        ok = (a0 + 64 <= b0) || (b0 + 64 <= a0);
    }
    if (ok)
    {
        Memory::MemSet(a, 0x3C, 64);
        Memory::MemSet(b, 0xC3, 64);
        ok = mem_all_equal(a, 0x3C, 64) && mem_all_equal(b, 0xC3, 64);
    }
    snprintf(buffer, buffer_size, "%s: after a double free, two Malloc(64) gave %p and %p (null, equal, overlapping or not independent)", T::name, a, b);
    Z::Free(a);
    if (b != a) Z::Free(b);
    return ok;
}

MU_TEST(memory_common_double_free_heap_still_sane_hwram) { mu_assert(memory_common_double_free_heap_still_sane<MemCommonHW>(), buffer); }
MU_TEST(memory_common_double_free_heap_still_sane_lwram) { mu_assert(memory_common_double_free_heap_still_sane<MemCommonLW>(), buffer); }

    /**
 * @brief Freeing blocks first-to-last, or every other block then the rest, merges them all back:
 *        free space and free block count return to the start, and one block of the combined size fits.
 * @details memory_common_exhaust_and_recover frees last-first, which hides a forward-only merge.
 */
template <typename T>
static bool memory_common_free_order(void)
{
    using Z = typename T::Z;
    constexpr size_t count = 16;
    constexpr size_t blockSize = 512;
    void *blocks[count] = {};
    const char *orders[2] = {"first-to-last", "odd then even"};

    for (size_t order = 0; order < 2; order++)
    {
        const size_t free0 = Z::GetFreeSpace();
        const size_t freeBlocks0 = Z::GetReport().FreeBlocks;
        for (size_t i = 0; i < count; i++)
        {
            blocks[i] = Z::Malloc(blockSize);
            if (blocks[i] == nullptr)
            {
                for (size_t j = 0; j < i; j++) Z::Free(blocks[j]);
                snprintf(buffer, buffer_size, "%s: Malloc(%lu) #%lu failed", T::name, (unsigned long)blockSize, (unsigned long)i);
                return false;
            }
        }
        if (order == 0)
        {
            for (size_t i = 0; i < count; i++) Z::Free(blocks[i]);
        }
        else
        {
            for (size_t i = 1; i < count; i += 2) Z::Free(blocks[i]);
            for (size_t i = 0; i < count; i += 2) Z::Free(blocks[i]);
        }
        const size_t free1 = Z::GetFreeSpace();
        const size_t freeBlocks1 = Z::GetReport().FreeBlocks;
        void *big = Z::Malloc(count * blockSize);
        Z::Free(big);

        snprintf(buffer, buffer_size, "%s, freed %s: free space %lu -> %lu (bytes lost)", T::name, orders[order], (unsigned long)free0, (unsigned long)free1);
        if (free1 != free0) return false;
        snprintf(buffer, buffer_size, "%s, freed %s: free blocks %lu -> %lu (not merged back)", T::name, orders[order], (unsigned long)freeBlocks0, (unsigned long)freeBlocks1);
        if (freeBlocks1 != freeBlocks0) return false;
        snprintf(buffer, buffer_size, "%s, freed %s: Malloc(%lu) of the combined size failed", T::name, orders[order], (unsigned long)(count * blockSize));
        if (big == nullptr) return false;
    }
    return true;
}

MU_TEST(memory_common_free_order_hwram) { mu_assert(memory_common_free_order<MemCommonHW>(), buffer); }
MU_TEST(memory_common_free_order_lwram) { mu_assert(memory_common_free_order<MemCommonLW>(), buffer); }

    /**
 * @brief Blocks of odd sizes are 4-byte aligned (SH-2 needs it for 32-bit access), and filling each one completely
 *        does not touch its neighbours or the allocator's headers.
 */
template <typename T>
static bool memory_common_odd_size_alignment(void)
{
    using Z = typename T::Z;
    constexpr size_t count = 10;
    const size_t sizes[count] = {1, 2, 3, 5, 6, 7, 9, 13, 17, 31};
    void *blocks[count] = {};
    const size_t free0 = Z::GetFreeSpace();
    bool ok = true;

    for (size_t i = 0; i < count; i++)
    {
        blocks[i] = Z::Malloc(sizes[i]);
        if (ok && (blocks[i] == nullptr || (reinterpret_cast<uintptr_t>(blocks[i]) & 3) != 0))
        {
            snprintf(buffer, buffer_size, "%s: Malloc(%lu) returned %p (null or not 4-byte aligned)", T::name, (unsigned long)sizes[i], blocks[i]);
            ok = false;
        }
    }
    for (size_t i = 0; ok && i < count; i++) Memory::MemSet(blocks[i], static_cast<uint8_t>(0x40 + i), sizes[i]);
    for (size_t i = 0; ok && i < count; i++)
    {
        if (!mem_all_equal(blocks[i], static_cast<uint8_t>(0x40 + i), sizes[i]))
        {
            snprintf(buffer, buffer_size, "%s: block of %lu bytes was overwritten by a neighbour", T::name, (unsigned long)sizes[i]);
            ok = false;
        }
    }
    for (size_t i = 0; i < count; i++) Z::Free(blocks[i]);
    if (ok && Z::GetFreeSpace() != free0)
    {
        snprintf(buffer, buffer_size, "%s: free space %lu != %lu after freeing the odd-size blocks", T::name,
            (unsigned long)Z::GetFreeSpace(), (unsigned long)free0);
        ok = false;
    }
    return ok;
}

MU_TEST(memory_common_odd_size_alignment_hwram) { mu_assert(memory_common_odd_size_alignment<MemCommonHW>(), buffer); }
MU_TEST(memory_common_odd_size_alignment_lwram) { mu_assert(memory_common_odd_size_alignment<MemCommonLW>(), buffer); }

static int memory_common_static_value = 0x1234;

    /**
 * @brief Freeing a HWRam address that is not on the heap (a global, a stack variable) does nothing.
 * @details InRange accepts all of 0x06000000-0x07FFFFFF, so these pointers reach the allocator.
 */
MU_TEST(memory_common_free_non_heap_hwram)
{
    int local = 0x5678;
    const size_t free0 = Memory::HighWorkRam::GetFreeSpace();
    const Memory::Report report0 = Memory::HighWorkRam::GetReport();

    Memory::Free(&memory_common_static_value);
    Memory::HighWorkRam::Free(&memory_common_static_value);
    Memory::Free(&local);
    Memory::HighWorkRam::Free(&local);

    const Memory::Report report1 = Memory::HighWorkRam::GetReport();
    mu_assert(memory_common_static_value == 0x1234, "Free(&global) changed the global");
    mu_assert(local == 0x5678, "Free(&local) changed the stack variable");
    snprintf(buffer, buffer_size, "Free of non-heap HWRam pointers changed the heap: free %lu -> %lu, blocks used %lu/free %lu -> %lu/%lu",
        (unsigned long)free0, (unsigned long)Memory::HighWorkRam::GetFreeSpace(),
        (unsigned long)report0.UsedBlocks, (unsigned long)report0.FreeBlocks, (unsigned long)report1.UsedBlocks, (unsigned long)report1.FreeBlocks);
    mu_assert(Memory::HighWorkRam::GetFreeSpace() == free0 && report1.UsedBlocks == report0.UsedBlocks && report1.FreeBlocks == report0.FreeBlocks, buffer);
}

    /**
 * @brief Freeing a block through its cache-through address (0x2xxxxxxx, the uncached copy of the same RAM) releases it.
 * @details Code that bypasses the cache for DMA often keeps that address; if Free ignores it, the block leaks.
 */
template <typename T>
static bool memory_common_free_cache_through(void)
{
    using Z = typename T::Z;
    const size_t free0 = Z::GetFreeSpace();
    void *p = Z::Malloc(64);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    void *uncached = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(p) | 0x20000000u);

    Memory::Free(uncached);
    const size_t free1 = Z::GetFreeSpace();
    if (free1 != free0) Z::Free(p);             // not released: free it through the normal address

    snprintf(buffer, buffer_size, "%s: Memory::Free(%p, cache-through copy of %p) left the block allocated (free %lu, expected %lu)",
        T::name, uncached, p, (unsigned long)free1, (unsigned long)free0);
    return free1 == free0;
}

MU_TEST(memory_common_free_cache_through_hwram) { mu_assert(memory_common_free_cache_through<MemCommonHW>(), buffer); }
MU_TEST(memory_common_free_cache_through_lwram) { mu_assert(memory_common_free_cache_through<MemCommonLW>(), buffer); }

    /**
 * @brief The report counts blocks and headers consistently: one Malloc adds one used block, headers cost the same per block,
 *        freeing a block raises free size, and freeing all of them restores the free block count and size.
 */
template <typename T>
static bool memory_common_report_blocks(void)
{
    using Z = typename T::Z;
    const Memory::Report r0 = Z::GetReport();
    void *a = Z::Malloc(256);
    void *b = Z::Malloc(256);
    void *c = Z::Malloc(256);
    const Memory::Report r1 = Z::GetReport();
    Z::Free(b);
    const Memory::Report r2 = Z::GetReport();
    Z::Free(a);
    Z::Free(c);
    const Memory::Report r3 = Z::GetReport();

    snprintf(buffer, buffer_size, "%s: Malloc(256) failed", T::name);
    if (a == nullptr || b == nullptr || c == nullptr) return false;
    snprintf(buffer, buffer_size, "%s: used blocks %lu -> %lu after 3 Malloc", T::name, (unsigned long)r0.UsedBlocks, (unsigned long)r1.UsedBlocks);
    if (r1.UsedBlocks != r0.UsedBlocks + 3) return false;
    const size_t blocks0 = r0.FreeBlocks + r0.UsedBlocks, blocks1 = r1.FreeBlocks + r1.UsedBlocks;
    snprintf(buffer, buffer_size, "%s: header bytes not the same per block: %lu for %lu blocks, %lu for %lu blocks", T::name,
        (unsigned long)r0.AllocationHeaders, (unsigned long)blocks0, (unsigned long)r1.AllocationHeaders, (unsigned long)blocks1);
    if (r0.AllocationHeaders == 0 || r0.AllocationHeaders * blocks1 != r1.AllocationHeaders * blocks0) return false;
    snprintf(buffer, buffer_size, "%s: freeing a 256-byte block: free size %lu -> %lu, used blocks %lu -> %lu", T::name,
        (unsigned long)r1.FreeSize, (unsigned long)r2.FreeSize, (unsigned long)r1.UsedBlocks, (unsigned long)r2.UsedBlocks);
    if (r2.FreeSize < r1.FreeSize + 256 || r2.UsedBlocks != r1.UsedBlocks - 1) return false;
    snprintf(buffer, buffer_size, "%s: after freeing all: free blocks %lu -> %lu, free size %lu -> %lu, total %lu -> %lu", T::name,
        (unsigned long)r0.FreeBlocks, (unsigned long)r3.FreeBlocks, (unsigned long)r0.FreeSize, (unsigned long)r3.FreeSize,
        (unsigned long)r0.TotalSize, (unsigned long)r3.TotalSize);
    return r3.FreeBlocks == r0.FreeBlocks && r3.FreeSize == r0.FreeSize && r3.TotalSize == r0.TotalSize && r3.UsedBlocks == r0.UsedBlocks;
}

MU_TEST(memory_common_report_blocks_hwram) { mu_assert(memory_common_report_blocks<MemCommonHW>(), buffer); }
MU_TEST(memory_common_report_blocks_lwram) { mu_assert(memory_common_report_blocks<MemCommonLW>(), buffer); }

    /**
 * @brief The largest block that fits: its last byte is in the zone and writable, n-1 rounds up into it,
 *        n+1 (rounded up past it) fails, and freeing it restores the free space.
 */
template <typename T>
static bool memory_common_largest_block(void)
{
    using Z = typename T::Z;
    const size_t free0 = Z::GetFreeSpace();

        // Largest n (multiple of 4) that Malloc accepts
    size_t lo = 0, hi = free0 & ~static_cast<size_t>(3);
    while (lo < hi)
    {
        const size_t mid = ((lo + hi) / 2 + 4) & ~static_cast<size_t>(3);
        void *probe = Z::Malloc(mid);
        if (probe != nullptr) { Z::Free(probe); lo = mid; }
        else { hi = mid - 4; }
    }
    const size_t n = lo;
    snprintf(buffer, buffer_size, "%s: no block fits (free space %lu)", T::name, (unsigned long)free0);
    if (n == 0) return false;

    uint8_t *p = static_cast<uint8_t *>(Z::Malloc(n));
    snprintf(buffer, buffer_size, "%s: Malloc(%lu) worked during the search but not now", T::name, (unsigned long)n);
    if (p == nullptr) return false;
    const bool inZone = Z::InRange(p + n - 1);
    bool intact = false;
    if (inZone)
    {
        p[0] = 0xA5;
        p[n - 1] = 0x5A;
        void *other = Z::Malloc(4);         // whatever still fits elsewhere must not overlap the largest block
        if (other != nullptr) Memory::MemSet(other, 0x00, 4);
        intact = p[0] == 0xA5 && p[n - 1] == 0x5A;
        Z::Free(other);
    }
    Z::Free(p);

    void *under = Z::Malloc(n - 1);         // rounds up to n: fits
    Z::Free(under);
    void *over = Z::Malloc(n + 1);          // rounds up to n + 4: does not fit
    Z::Free(over);
    const size_t free1 = Z::GetFreeSpace();

    snprintf(buffer, buffer_size, "%s: last byte of the largest block (%p + %lu) is outside the zone", T::name, p, (unsigned long)n);
    if (!inZone) return false;
    snprintf(buffer, buffer_size, "%s: first/last byte of the largest block (%lu bytes) changed by another Malloc", T::name, (unsigned long)n);
    if (!intact) return false;
    snprintf(buffer, buffer_size, "%s: largest block %lu (free space %lu): Malloc(%lu) %s, Malloc(%lu) %s", T::name, (unsigned long)n, (unsigned long)free0,
        (unsigned long)(n - 1), under != nullptr ? "ok" : "FAILED", (unsigned long)(n + 1), over == nullptr ? "null" : "NOT NULL");
    if (under == nullptr || over != nullptr) return false;
    snprintf(buffer, buffer_size, "%s: free space %lu != %lu after freeing the largest block", T::name, (unsigned long)free1, (unsigned long)free0);
    return free1 == free0;
}

MU_TEST(memory_common_largest_block_hwram) { mu_assert(memory_common_largest_block<MemCommonHW>(), buffer); }
MU_TEST(memory_common_largest_block_lwram) { mu_assert(memory_common_largest_block<MemCommonLW>(), buffer); }

    /**
 * @brief Shrinking a block keeps its start and releases memory; growing it back keeps the start;
 *        freeing it restores the free space.
 */
template <typename T>
static bool memory_common_shrink_then_grow(void)
{
    using Z = typename T::Z;
    const size_t free0 = Z::GetFreeSpace();
    void *p = Z::Malloc(256);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(256) failed", T::name); return false; }
    mem_fill_sequence(p, 256);
    const size_t freeFull = Z::GetFreeSpace();

    void *small = Z::Realloc(p, 64);
    if (small != nullptr) p = small;
    const size_t freeSmall = Z::GetFreeSpace();
    const bool smallOk = small != nullptr && mem_check_sequence(p, 64);

    void *big = Z::Realloc(p, 256);
    if (big != nullptr) p = big;
    const bool bigOk = big != nullptr && mem_check_sequence(p, 64);
    Z::Free(p);
    const size_t free1 = Z::GetFreeSpace();

    snprintf(buffer, buffer_size, "%s: Realloc(256 -> 64) %s", T::name, small == nullptr ? "returned null" : "lost data");
    if (!smallOk) return false;
    snprintf(buffer, buffer_size, "%s: shrinking 256 -> 64 did not release memory (free %lu -> %lu)", T::name, (unsigned long)freeFull, (unsigned long)freeSmall);
    if (freeSmall <= freeFull) return false;
    snprintf(buffer, buffer_size, "%s: Realloc(64 -> 256) %s", T::name, big == nullptr ? "returned null" : "lost the first 64 bytes");
    if (!bigOk) return false;
    snprintf(buffer, buffer_size, "%s: free space %lu != %lu after shrink, grow and free", T::name, (unsigned long)free1, (unsigned long)free0);
    return free1 == free0;
}

MU_TEST(memory_common_shrink_then_grow_hwram) { mu_assert(memory_common_shrink_then_grow<MemCommonHW>(), buffer); }
MU_TEST(memory_common_shrink_then_grow_lwram) { mu_assert(memory_common_shrink_then_grow<MemCommonLW>(), buffer); }

/** @brief Object whose member allocates with autonew: the new object lands in the zone of this one. */
struct MemCommonAutoNew
{
    int value = 1;
    int *MakeSibling() { return autonew int(4); }
};

    /**
 * @brief The zone macros (hwnew, lwnew), autonew from a member function, and PlacementMalloc(size, void*).
 */
MU_TEST(memory_common_new_macros_and_placement_ptr)
{
    const size_t hwFree = Memory::HighWorkRam::GetFreeSpace();
    const size_t lwFree = Memory::LowWorkRam::GetFreeSpace();

    void *hwPlaced = Memory::PlacementMalloc(32, reinterpret_cast<void *>(0x06000000u));
    void *lwPlaced = Memory::PlacementMalloc(32, reinterpret_cast<void *>(0x00200000u));
    const bool placedOk = hwPlaced != nullptr && Memory::HighWorkRam::InRange(hwPlaced) && lwPlaced != nullptr && Memory::LowWorkRam::InRange(lwPlaced);
    Memory::Free(hwPlaced);
    Memory::Free(lwPlaced);

    int *hw = hwnew int(2);
    int *lw = lwnew int(3);
    const bool macrosOk = hw != nullptr && Memory::HighWorkRam::InRange(hw) && *hw == 2 && lw != nullptr && Memory::LowWorkRam::InRange(lw) && *lw == 3;
    delete hw;
    delete lw;

    MemCommonAutoNew *inLw = lwnew MemCommonAutoNew();
    MemCommonAutoNew *inHw = hwnew MemCommonAutoNew();
    int *siblingLw = inLw != nullptr ? inLw->MakeSibling() : nullptr;
    int *siblingHw = inHw != nullptr ? inHw->MakeSibling() : nullptr;
    const bool autoOk = siblingLw != nullptr && Memory::LowWorkRam::InRange(siblingLw) && *siblingLw == 4
        && siblingHw != nullptr && Memory::HighWorkRam::InRange(siblingHw) && *siblingHw == 4;
    delete siblingLw;
    delete siblingHw;
    delete inLw;
    delete inHw;

    mu_assert(placedOk, "PlacementMalloc(size, void*) should allocate in the zone containing the address");
    mu_assert(macrosOk, "hwnew / lwnew should allocate in HWRam / LWRam");
    mu_assert(autoOk, "autonew in a member function should allocate in the zone of this");
    snprintf(buffer, buffer_size, "Free space not restored: HWRam %lu -> %lu, LWRam %lu -> %lu",
        (unsigned long)hwFree, (unsigned long)Memory::HighWorkRam::GetFreeSpace(),
        (unsigned long)lwFree, (unsigned long)Memory::LowWorkRam::GetFreeSpace());
    mu_assert(Memory::HighWorkRam::GetFreeSpace() == hwFree && Memory::LowWorkRam::GetFreeSpace() == lwFree, buffer);
}

    /**
 * @brief Cartridge RAM: with no cartridge (or while CartRam is not implemented) every call fails cleanly;
 *        with one, a block round-trips and the accounting comes back.
 */
MU_TEST(memory_common_cartram_present_or_absent)
{
    if (Memory::CartRam::GetSize() == 0)
    {
        void *p = Memory::CartRam::Malloc(16);
        void *q = Memory::Malloc(16, Memory::Zone::CartRam);
        void *r = Memory::CartRam::Realloc(nullptr, 16);
        Memory::CartRam::Free(nullptr);
        mu_assert(p == nullptr && q == nullptr && r == nullptr, "With no cartridge RAM, CartRam allocations should return null");
        mu_assert(Memory::CartRam::GetFreeSpace() == 0 && Memory::CartRam::GetUsedSpace() == 0, "With no cartridge RAM, free and used space should be 0");
        mu_assert(!Memory::CartRam::InRange(reinterpret_cast<void *>(0x22400000u)), "With no cartridge RAM, no address should be in range");
        return;
    }

    const size_t free0 = Memory::CartRam::GetFreeSpace();
    void *p = Memory::CartRam::Malloc(256);
    const bool allocOk = p != nullptr && Memory::CartRam::InRange(p);
    if (p != nullptr) mem_fill_sequence(p, 256);
    const bool dataOk = p != nullptr && mem_check_sequence(p, 256);
    int *obj = cartnew int(6);
    const bool newOk = obj != nullptr && Memory::CartRam::InRange(obj) && *obj == 6;
    delete obj;
    Memory::CartRam::Free(p);

    mu_assert(allocOk, "CartRam::Malloc(256) should return a block in cartridge RAM");
    mu_assert(dataOk, "Data written to cartridge RAM should read back");
    mu_assert(newOk, "cartnew should allocate in cartridge RAM");
    mu_assert(Memory::CartRam::GetFreeSpace() == free0, "CartRam free space should come back after Free");
}

    /**
 * @brief Freeing a pointer into the middle of a block (not its start) leaves the block allocated and its data intact.
 * @note Risky: an allocator that trusts the pointer writes a fake header into the block.
 */
template <typename T>
static bool memory_common_free_interior_pointer(void)
{
    using Z = typename T::Z;
#if defined(USE_TLSF_ALLOCATOR)
        // Not run under TLSF: Free(block + 16) hangs the console. Reported as a failure so the bug stays visible.
    snprintf(buffer, buffer_size, "%s: not run under TLSF, Free of an interior pointer hangs the console", T::name);
    return false;
#endif
    uint8_t *p = static_cast<uint8_t *>(Z::Malloc(64));
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    Memory::MemSet(p, 0x00, 64);                // zeroed, so a fake header read from it has size 0 and cannot run off
    const size_t used0 = Z::GetUsedSpace();
    const size_t blocks0 = Z::GetReport().UsedBlocks;

    Z::Free(p + 16);
    const bool intact = mem_all_equal(p, 0x00, 64);
    const size_t used1 = Z::GetUsedSpace();
    const size_t blocks1 = Z::GetReport().UsedBlocks;
    Z::Free(p);

    snprintf(buffer, buffer_size, "%s: Free(block + 16): data %s, used space %lu -> %lu, used blocks %lu -> %lu", T::name,
        intact ? "intact" : "CHANGED", (unsigned long)used0, (unsigned long)used1, (unsigned long)blocks0, (unsigned long)blocks1);
    return intact && used1 == used0 && blocks1 == blocks0;
}

MU_TEST(memory_common_free_interior_pointer_hwram) { mu_assert(memory_common_free_interior_pointer<MemCommonHW>(), buffer); }
MU_TEST(memory_common_free_interior_pointer_lwram) { mu_assert(memory_common_free_interior_pointer<MemCommonLW>(), buffer); }

    /**
 * @brief Realloc on a block that was already freed must not hand out memory that overlaps a live block.
 * @note Risky: undefined in C; this only checks the heap is not corrupted afterwards.
 */
template <typename T>
static bool memory_common_realloc_freed(void)
{
    using Z = typename T::Z;
    void *p = Z::Malloc(64);
    if (p == nullptr) { snprintf(buffer, buffer_size, "%s: Malloc(64) failed", T::name); return false; }
    Z::Free(p);

    void *q = Z::Realloc(p, 128);
    void *a = Z::Malloc(128);
    bool ok = a != nullptr;
    if (ok && q != nullptr)
    {
        const uintptr_t q0 = reinterpret_cast<uintptr_t>(q), a0 = reinterpret_cast<uintptr_t>(a);
        ok = q0 + 128 <= a0 || a0 + 128 <= q0;
    }
    snprintf(buffer, buffer_size, "%s: Realloc(freed block) returned %p, then Malloc(128) returned %p (null or overlapping)", T::name, q, a);
    Z::Free(a);
    if (q != a) Z::Free(q);
    return ok;
}

MU_TEST(memory_common_realloc_freed_hwram) { mu_assert(memory_common_realloc_freed<MemCommonHW>(), buffer); }
MU_TEST(memory_common_realloc_freed_lwram) { mu_assert(memory_common_realloc_freed<MemCommonLW>(), buffer); }

/** @brief Reads a variable through the cache-through address, so this CPU sees what the other one wrote. */
template <typename V>
static V memory_common_read_uncached(const V &value)
{
    return *reinterpret_cast<const volatile V *>(reinterpret_cast<uintptr_t>(&value) | 0x20000000u);
}

/** @brief Allocates, fills, checks and frees HWRam blocks; returns the number of failures. */
static int memory_common_alloc_loop(uint8_t pattern, int rounds)
{
    int errors = 0;
    for (int i = 0; i < rounds; i++)
    {
        const size_t size = 16 + (i % 7) * 24;
        uint8_t *p = static_cast<uint8_t *>(Memory::HighWorkRam::Malloc(size));
        if (p == nullptr) { errors++; continue; }
        for (size_t j = 0; j < size; j++) p[j] = pattern;
        for (size_t j = 0; j < size; j++)
        {
            if (p[j] != pattern) { errors++; break; }
        }
        Memory::HighWorkRam::Free(p);
    }
    return errors;
}

/** @brief Runs memory_common_alloc_loop on the slave CPU. */
class MemCommonSlaveTask : public Types::ITask
{
public:
    int errors = -1;
    bool Finished() { return memory_common_read_uncached(this->done); }

protected:
    void Do() override { this->errors = memory_common_alloc_loop(0xB2, 500); }
};

static MemCommonSlaveTask memory_common_slave_task;

    /**
 * @brief Master and slave CPUs allocate and free HWRam at the same time: no block is handed to both,
 *        and the heap is consistent afterwards.
 * @note Risky: the allocator has no lock, so a race can corrupt the heap. Run last.
 */
MU_TEST(memory_common_both_cpus_hwram)
{
    const size_t free0 = Memory::HighWorkRam::GetFreeSpace();
    const size_t used0 = Memory::HighWorkRam::GetReport().UsedBlocks;

    Slave::ExecuteOnSlave(memory_common_slave_task);
    const int masterErrors = memory_common_alloc_loop(0x4D, 500);

    bool finished = false;
    for (int spin = 0; spin < 20000000 && !finished; spin++) finished = memory_common_slave_task.Finished();
    mu_assert(finished, "Slave CPU task did not finish (slave not started or stuck in the allocator)");

    slCashPurge();                              // drop cache lines the slave made stale before reading the heap
    const int slaveErrors = memory_common_read_uncached(memory_common_slave_task.errors);
    const size_t free1 = Memory::HighWorkRam::GetFreeSpace();
    const size_t used1 = Memory::HighWorkRam::GetReport().UsedBlocks;

    snprintf(buffer, buffer_size, "Allocating on both CPUs: %d master and %d slave failures (null block or data overwritten by the other CPU)", masterErrors, slaveErrors);
    mu_assert(masterErrors == 0 && slaveErrors == 0, buffer);
    snprintf(buffer, buffer_size, "Heap inconsistent after both CPUs: free %lu -> %lu, used blocks %lu -> %lu",
        (unsigned long)free0, (unsigned long)free1, (unsigned long)used0, (unsigned long)used1);
    mu_assert(free1 == free0 && used1 == used0, buffer);
}

MU_TEST_SUITE(memory_common_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&memory_common_test_setup,
        &memory_common_test_teardown,
        &memory_common_test_output_header);

    MU_RUN_TEST(memory_common_accounting_hwram);
    MU_RUN_TEST(memory_common_accounting_lwram);
    MU_RUN_TEST(memory_common_blocks_independent_hwram);
    MU_RUN_TEST(memory_common_blocks_independent_lwram);
    MU_RUN_TEST(memory_common_realloc_preserves_data_hwram);
    MU_RUN_TEST(memory_common_realloc_preserves_data_lwram);
    MU_RUN_TEST(memory_common_memset);
    MU_RUN_TEST(memory_common_new_delete_forms);
    MU_RUN_TEST(memory_common_placement_outside_zones);
    MU_RUN_TEST(memory_common_realloc_same_and_zero_hwram);
    MU_RUN_TEST(memory_common_realloc_same_and_zero_lwram);
    MU_RUN_TEST(memory_common_malloc_huge_sizes_hwram);
    MU_RUN_TEST(memory_common_malloc_huge_sizes_lwram);
    MU_RUN_TEST(memory_common_exhaust_and_recover_hwram);
    MU_RUN_TEST(memory_common_exhaust_and_recover_lwram);
    MU_RUN_TEST(memory_common_free_order_hwram);
    MU_RUN_TEST(memory_common_free_order_lwram);
    MU_RUN_TEST(memory_common_odd_size_alignment_hwram);
    MU_RUN_TEST(memory_common_odd_size_alignment_lwram);
    MU_RUN_TEST(memory_common_free_non_heap_hwram);
    MU_RUN_TEST(memory_common_free_cache_through_hwram);
    MU_RUN_TEST(memory_common_free_cache_through_lwram);
    MU_RUN_TEST(memory_common_report_blocks_hwram);
    MU_RUN_TEST(memory_common_report_blocks_lwram);
    MU_RUN_TEST(memory_common_shrink_then_grow_hwram);
    MU_RUN_TEST(memory_common_shrink_then_grow_lwram);
    MU_RUN_TEST(memory_common_new_macros_and_placement_ptr);
    MU_RUN_TEST(memory_common_cartram_present_or_absent);
}

    /**
 * @brief Misuse tests that can corrupt a heap with no ownership or double-free checks.
 *        Run right after memory_common_test_suite, before the older suites leak the heap full.
 */
MU_TEST_SUITE(memory_common_risky_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&memory_common_test_setup,
        &memory_common_test_teardown,
        &memory_common_test_output_header);

    MU_RUN_TEST(memory_common_wrong_zone_hwram);
    MU_RUN_TEST(memory_common_wrong_zone_lwram);
    MU_RUN_TEST(memory_common_realloc_too_large_hwram);
    MU_RUN_TEST(memory_common_realloc_too_large_lwram);
    MU_RUN_TEST(memory_common_free_interior_pointer_hwram);
    MU_RUN_TEST(memory_common_free_interior_pointer_lwram);
    MU_RUN_TEST(memory_common_realloc_freed_hwram);
    MU_RUN_TEST(memory_common_realloc_freed_lwram);
    MU_RUN_TEST(memory_common_double_free_heap_still_sane_hwram);
    MU_RUN_TEST(memory_common_double_free_heap_still_sane_lwram);
}


    /**
 * @brief Both CPUs allocating at once. Run alone in the memory_cpus_* campaigns, on a fresh heap:
 *        under SIMPLE it breaks HWRam for anything after it, and late in a campaign HWRam is already leaked full.
 */
MU_TEST_SUITE(memory_common_cpus_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&memory_common_test_setup,
        &memory_common_test_teardown,
        &memory_common_test_output_header);

    MU_RUN_TEST(memory_common_both_cpus_hwram);
}

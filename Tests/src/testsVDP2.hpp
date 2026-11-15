// Tests/src/testsVDP2.hpp
// SRL::VDP2: the VRAM allocator (alignment, limits, access cycles, reset), converting a bitmap to a tilemap (Bmp2Tile),
// loading it into NBG1, ClearVRAM freeing its palette, loading a bitmap into NBG1 (LoadBitmap), and the TV resolution.
// Only NBG1 is used: NBG3 shows the results.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_vdp2.hpp>
#include <srl_tilemap_interfaces.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using VramBank = SRL::VDP2::VramBank;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void vdp2_test_setup(void) { VDP2::ClearVRAM(); }
void vdp2_test_teardown(void) { VDP2::ClearVRAM(); }

void vdp2_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_VDP2_ERROR(S)****");
    }
}

/** @brief Size of each VRAM bank the allocator hands out: B1 keeps its top 0x8000 bytes (ASCII font and map, back color). */
static uint32_t vdp2_bank_size(VramBank bank) { return bank == VramBank::B1 ? 0x18000 : 0x20000; }

/** @brief The first block starts at the bank start; the next one is moved up to its boundary; free space drops by both. */
MU_TEST(vdp2_vram_allocate_aligned)
{
    mu_assert(VDP2::VRAM::GetAvailable(VramBank::B0) == 0x20000, "Bank B0 is not empty after ClearVRAM");

    uint8_t *first = static_cast<uint8_t *>(VDP2::VRAM::Allocate(100, 32, VramBank::B0));
    uint8_t *second = static_cast<uint8_t *>(VDP2::VRAM::Allocate(64, 0x800, VramBank::B0));

    snprintf(buffer, buffer_size, "First block at 0x%08x, expected the bank start 0x%08x",
        reinterpret_cast<unsigned>(first), static_cast<unsigned>(VDP2_VRAM_B0));
    mu_assert(reinterpret_cast<uint32_t>(first) == VDP2_VRAM_B0, buffer);
    snprintf(buffer, buffer_size, "Second block at 0x%08x: not on a 0x800 boundary after the first", reinterpret_cast<unsigned>(second));
    mu_assert(second != nullptr && (reinterpret_cast<uint32_t>(second) & 0x7ff) == 0 && second >= first + 100, buffer);

    const uint32_t used = static_cast<uint32_t>((second + 64) - first);
    snprintf(buffer, buffer_size, "B0 has %u bytes left, expected %u", static_cast<unsigned>(VDP2::VRAM::GetAvailable(VramBank::B0)),
        static_cast<unsigned>(0x20000 - used));
    mu_assert(VDP2::VRAM::GetAvailable(VramBank::B0) == 0x20000 - used, buffer);
}

/** @brief A block larger than the bank fails without using space; a block of exactly the bank size fills it. */
MU_TEST(vdp2_vram_allocate_too_big)
{
    mu_assert(VDP2::VRAM::Allocate(0x20001, 32, VramBank::A1) == nullptr, "A block larger than bank A1 was allocated");
    mu_assert(VDP2::VRAM::GetAvailable(VramBank::A1) == 0x20000, "The failed allocation used space");
    mu_assert(VDP2::VRAM::Allocate(0x20000, 32, VramBank::A1) != nullptr, "A block of exactly the bank size failed");
    mu_assert(VDP2::VRAM::GetAvailable(VramBank::A1) == 0, "Bank A1 is not full after a bank-sized block");
}

/** @brief A bank has 8 access cycles; B1 keeps 2 for the ASCII layer, so it has 6 to give. */
MU_TEST(vdp2_vram_access_cycles)
{
    mu_assert(VDP2::VRAM::Allocate(16, 32, VramBank::A0, 8) != nullptr, "Bank A0 did not give its 8 access cycles");
    mu_assert(VDP2::VRAM::Allocate(16, 32, VramBank::A0, 1) == nullptr, "Bank A0 gave a 9th access cycle");

    mu_assert(VDP2::VRAM::Allocate(16, 32, VramBank::B1, 6) != nullptr, "Bank B1 did not give 6 access cycles (8 minus 2 for ASCII)");
    mu_assert(VDP2::VRAM::Allocate(16, 32, VramBank::B1, 1) == nullptr, "Bank B1 gave more than 6 access cycles");
}

/** @brief ClearVRAM empties every bank. */
MU_TEST(vdp2_clear_vram_empties_banks)
{
    const VramBank banks[4] = { VramBank::A0, VramBank::A1, VramBank::B0, VramBank::B1 };
    for (int i = 0; i < 4; i++) VDP2::VRAM::Allocate(0x1000, 32, banks[i]);

    VDP2::ClearVRAM();
    for (int i = 0; i < 4; i++)
    {
        snprintf(buffer, buffer_size, "Bank %d has %u bytes free after ClearVRAM, expected %u", i,
            static_cast<unsigned>(VDP2::VRAM::GetAvailable(banks[i])), static_cast<unsigned>(vdp2_bank_size(banks[i])));
        mu_assert(VDP2::VRAM::GetAvailable(banks[i]) == vdp2_bank_size(banks[i]), buffer);
    }
}

/** @brief 16-color pixel of the 32x16 test bitmap: the left 16x16 tile is empty, the right one is not. */
static uint8_t vdp2_pixel(int x, int y) { return x < 16 ? 0 : static_cast<uint8_t>(1 + (x + y * 3) % 15); }

/** @brief The 32x16 test bitmap, two pixels per byte (left pixel in the high nibble), with a 16-color palette. */
static uint8_t vdp2_bitmap_data[32 * 16 / 2];

static void vdp2_fill_bitmap()
{
    for (int y = 0; y < 16; y++)
    {
        for (int x = 0; x < 32; x += 2)
        {
            vdp2_bitmap_data[(y * 32 + x) / 2] = static_cast<uint8_t>((vdp2_pixel(x, y) << 4) | vdp2_pixel(x + 1, y));
        }
    }
}

/**
 * @brief Bmp2Tile turns a 16-color bitmap into 16x16 characters: an empty character first, empty tiles map to it,
 *        the other tile's pixels are stored as four 8x8 cells (top left, top right, bottom left, bottom right).
 */
MU_TEST(vdp2_bmp2tile_16_colors)
{
    const size_t blocksBefore = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    {
        vdp2_fill_bitmap();
        Bitmap::Palette palette(16);
        for (int i = 0; i < 16; i++) palette.Colors[i] = Types::HighColor(static_cast<uint16_t>(0x8000 | (i * 0x421)));
        MockBitmap bitmap(vdp2_bitmap_data, Bitmap::BitmapInfo(32, 16, &palette));
        Tilemap::Interfaces::Bmp2Tile tiles(bitmap);
        const Tilemap::TilemapInfo info = tiles.GetInfo();

        snprintf(buffer, buffer_size, "Color mode %d, char size %d, map %dx%d, cell bytes %d (expected Paletted16, 2x2, 32x32, 384)",
            static_cast<int>(info.ColorMode), info.CharSize, info.MapWidth, info.MapHeight, static_cast<int>(info.CellByteSize));
        mu_assert(info.ColorMode == CRAM::TextureColorMode::Paletted16 && info.CharSize == CHAR_SIZE_2x2 &&
            info.MapWidth == 32 && info.MapHeight == 32 && info.CellByteSize == 384, buffer);

        const uint8_t *cells = static_cast<const uint8_t *>(tiles.GetCellData());
        for (int i = 0; i < 128; i++)
        {
            snprintf(buffer, buffer_size, "Byte %d of the empty first character is 0x%02x", i, cells[i]);
            mu_assert(cells[i] == 0, buffer);
        }

        for (int cell = 0; cell < 4; cell++)
        {
            const int cellX = 16 + (cell & 1) * 8;
            const int cellY = (cell >> 1) * 8;
            for (int row = 0; row < 8; row++)
            {
                for (int b = 0; b < 4; b++)
                {
                    const int x = cellX + b * 2;
                    const uint8_t expected = static_cast<uint8_t>((vdp2_pixel(x, cellY + row) << 4) | vdp2_pixel(x + 1, cellY + row));
                    const uint8_t actual = cells[128 + cell * 32 + row * 4 + b];
                    snprintf(buffer, buffer_size, "Cell %d row %d byte %d is 0x%02x, expected 0x%02x", cell, row, b, actual, expected);
                    mu_assert(actual == expected, buffer);
                }
            }
        }

        const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());
        snprintf(buffer, buffer_size, "Map entries %d and %d: the empty tile should be 0, the other one not", map[0], map[1]);
        mu_assert(map[0] == 0 && map[1] != 0, buffer);

        const uint16_t *copiedPalette = static_cast<const uint16_t *>(tiles.GetPalData());
        mu_assert(copiedPalette != nullptr && copiedPalette[5] == static_cast<uint16_t>(palette.Colors[5]), "The palette was not copied");
    }
    const size_t blocksAfter = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after destroying the bitmap and tiles", blocksBefore, blocksAfter);
    mu_assert(blocksAfter == blocksBefore, buffer);
}

/** @brief LoadTilemap copies the cells into VRAM and takes a CRAM palette; ClearVRAM gives the palette back. */
MU_TEST(vdp2_load_tilemap_then_clear)
{
    vdp2_fill_bitmap();
    Bitmap::Palette palette(16);
    MockBitmap bitmap(vdp2_bitmap_data, Bitmap::BitmapInfo(32, 16, &palette));
    Tilemap::Interfaces::Bmp2Tile tiles(bitmap);

    VDP2::NBG1::LoadTilemap(tiles);
    const bool loaded = VDP2::NBG1::TilePalette.GetData() != nullptr;
    const uint16_t bank = VDP2::NBG1::TilePalette.GetId();
    const bool usedAfterLoad = loaded && CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted16);

    bool cellsCopied = loaded;
    const volatile uint8_t *vram = static_cast<const volatile uint8_t *>(VDP2::NBG1::CellAddress);
    const uint8_t *cells = static_cast<const uint8_t *>(tiles.GetCellData());
    for (int i = 0; cellsCopied && i < tiles.GetInfo().CellByteSize; i++) cellsCopied = vram[i] == cells[i];

    VDP2::ClearVRAM();
    const bool usedAfterClear = loaded && CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted16);
    if (usedAfterClear) CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted16, false); // leave CRAM clean

    mu_assert(loaded && usedAfterLoad, "LoadTilemap did not take a 16-color CRAM palette for NBG1");
    mu_assert(cellsCopied, "The cell data in VRAM differs from the converted cells");
    mu_assert(VDP2::NBG1::TilePalette.GetData() == nullptr, "ClearVRAM did not reset the NBG1 palette");
    snprintf(buffer, buffer_size, "CRAM bank %d is still marked used after ClearVRAM", bank);
    mu_assert(!usedAfterClear, buffer);
}

/** @brief The 320x4 16-color test bitmap for LoadBitmap: 160 bytes per row, every byte different from its neighbours. */
static uint8_t vdp2_wide_bitmap_data[320 * 4 / 2];

static void vdp2_fill_wide_bitmap()
{
    for (int i = 0; i < 320 * 4 / 2; i++) vdp2_wide_bitmap_data[i] = static_cast<uint8_t>(1 + (i * 7) % 251);
}

/**
 * @brief LoadBitmap on NBG1 (16 colors, 320 pixels wide): each 160-byte row lands at the start of a 512-pixel
 *        (256-byte) VRAM row, and the screen takes a CRAM palette on a 256-color boundary (bitmap palettes need it).
 */
MU_TEST(vdp2_load_bitmap_rows_and_palette)
{
    vdp2_fill_wide_bitmap();
    Bitmap::Palette palette(16);
    MockBitmap bitmap(vdp2_wide_bitmap_data, Bitmap::BitmapInfo(320, 4, &palette));

    VDP2::NBG1::LoadBitmap(&bitmap);
    slDMAWait();

    const volatile uint8_t *vram = static_cast<const volatile uint8_t *>(VDP2::NBG1::CellAddress);
    const bool loaded = reinterpret_cast<uint32_t>(VDP2::NBG1::CellAddress) >= VDP2_VRAM_A0 && VDP2::NBG1::TilePalette.GetData() != nullptr;
    const uint16_t bank = VDP2::NBG1::TilePalette.GetId();
    const bool bankUsed = loaded && CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted16);
    if (bankUsed) CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted16, false); // ClearVRAM frees it with the wrong mode

    mu_assert(loaded, "LoadBitmap did not give NBG1 VRAM and a CRAM palette");
    for (int y = 0; y < 4; y++)
    {
        for (int i = 0; i < 160; i++)
        {
            snprintf(buffer, buffer_size, "Row %d byte %d is 0x%02x in VRAM, expected 0x%02x (rows are 256 bytes apart)", y, i,
                vram[y * 256 + i], vdp2_wide_bitmap_data[y * 160 + i]);
            mu_assert(vram[y * 256 + i] == vdp2_wide_bitmap_data[y * 160 + i], buffer);
        }
    }

    snprintf(buffer, buffer_size, "CRAM bank %d: used %d, expected a used bank that is a multiple of 16", bank, bankUsed);
    mu_assert(bankUsed && (bank & 0xf) == 0, buffer);
}

/** @brief A 16-color bitmap up to 512x256 takes 512 * 256 / 2 = 64 KB of VRAM: half of bank A0 stays free. */
MU_TEST(vdp2_load_bitmap_16_colors_vram_size)
{
    vdp2_fill_wide_bitmap();
    Bitmap::Palette palette(16);
    MockBitmap bitmap(vdp2_wide_bitmap_data, Bitmap::BitmapInfo(320, 4, &palette));

    VDP2::NBG1::LoadBitmap(&bitmap);
    slDMAWait();

    const uint16_t bank = VDP2::NBG1::TilePalette.GetId();
    if (VDP2::NBG1::TilePalette.GetData() != nullptr) CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted16, false);

    snprintf(buffer, buffer_size, "Bank A0 has %u bytes free, expected %u (block size %d)", static_cast<unsigned>(VDP2::VRAM::GetAvailable(VramBank::A0)),
        0x10000u, static_cast<int>(VDP2::NBG1::CellAllocSize));
    mu_assert(VDP2::VRAM::GetAvailable(VramBank::A0) == 0x10000, buffer);
}

/** @brief The campaign starts in the default NTSC resolution, 320x240. */
MU_TEST(vdp2_tv_default_resolution)
{
    snprintf(buffer, buffer_size, "TV is %dx%d (resolution %d), expected 320x240 (%d)", TV::Width, TV::Height,
        static_cast<int>(TV::Resolution), static_cast<int>(TV::Resolutions::Normal320x240));
    mu_assert(TV::Width == 320 && TV::Height == 240 && TV::Resolution == TV::Resolutions::Normal320x240, buffer);
}

MU_TEST_SUITE(vdp2_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&vdp2_test_setup, &vdp2_test_teardown, &vdp2_test_output_header);

    MU_RUN_TEST(vdp2_vram_allocate_aligned);
    MU_RUN_TEST(vdp2_vram_allocate_too_big);
    MU_RUN_TEST(vdp2_vram_access_cycles);
    MU_RUN_TEST(vdp2_clear_vram_empties_banks);
    MU_RUN_TEST(vdp2_bmp2tile_16_colors);
    MU_RUN_TEST(vdp2_load_tilemap_then_clear);
    MU_RUN_TEST(vdp2_load_bitmap_rows_and_palette);
    MU_RUN_TEST(vdp2_load_bitmap_16_colors_vram_size);
    MU_RUN_TEST(vdp2_tv_default_resolution);
}

// Tests/src/testsVDP2.hpp
// SRL::VDP2: the VRAM allocator (alignment, limits, access cycles, reset), converting a bitmap to a tilemap (Bmp2Tile),
// loading it into NBG1, ClearVRAM freeing its palette, loading a bitmap into NBG1 (LoadBitmap), and the TV resolution.
// Only NBG1 is used: NBG3 shows the results.
// vdp2_settings_test_suite: what the scroll, sprite and rotation settings write in SGL's copy of the VDP2 registers
// (SGL sends it to VDP2 at the next frame; each test puts it back before that).
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

/** @brief SGL's copy of the VDP2 registers (0x25F80000-0x25F8011F), sent to VDP2 at each frame */
extern "C" {
extern uint16_t VDP2_TVMD;
extern uint16_t VDP2_BGON, VDP2_PLSZ, VDP2_ZMXIN0, VDP2_ZMXDN0, VDP2_ZMYIN0, VDP2_ZMYDN0, VDP2_ZMCTL;
extern uint16_t VDP2_RPMD, VDP2_KTCTL, VDP2_OVPNRA, VDP2_SPCTL, VDP2_CCCTL, VDP2_PRISA, VDP2_PRISB;
extern uint16_t VDP2_PRINA, VDP2_PRINB, VDP2_PRIR, VDP2_CCRSA, VDP2_CCRNA, VDP2_CLOFEN, VDP2_CLOFSL;
extern uint16_t VDP2_COAR, VDP2_COAG, VDP2_COAB, VDP2_COBR, VDP2_COBG, VDP2_COBB;
}

/** @brief Register copy and SRL's screen masks, saved before each settings test and put back after it */
static uint16_t vdp2_saved_regs[0x90];
static uint16_t vdp2_saved_masks[5];
static uint16_t *const vdp2_masks[5] = { &VDP2::ActiveScrolls, &VDP2::ColorCalcScrolls, &VDP2::OffsetAScrolls,
    &VDP2::OffsetBScrolls, &VDP2::TransparentScrolls };

/** @brief Back color: one color in VRAM at B1 + 0x1FEE0 (VDP2::SetBackColor) */
static volatile uint16_t *const vdp2_back_color_ram = reinterpret_cast<volatile uint16_t *>(VDP2_VRAM_B1 + 0x1FEE0);
static uint16_t vdp2_saved_back_color;

void vdp2_settings_setup(void)
{
    for (int i = 0; i < 0x90; i++) vdp2_saved_regs[i] = (&VDP2_TVMD)[i];
    for (int i = 0; i < 5; i++) vdp2_saved_masks[i] = *vdp2_masks[i];
    vdp2_saved_back_color = *vdp2_back_color_ram;
}

void vdp2_settings_teardown(void)
{
    for (int i = 0; i < 0x90; i++) (&VDP2_TVMD)[i] = vdp2_saved_regs[i];
    for (int i = 0; i < 5; i++) *vdp2_masks[i] = vdp2_saved_masks[i];
    *vdp2_back_color_ram = vdp2_saved_back_color;
}

/** @brief VDP2::Initialize gives sprite priority bank 0 Layer3 and bank 1 Layer4 (documented in SetColorCondition).
 *  First test: nothing has changed the registers yet. */
MU_TEST(vdp2_default_sprite_priorities)
{
    snprintf(buffer, buffer_size, "PRISA is 0x%04x: bank 0 on layer %d, bank 1 on layer %d (expected 3 and 4)", VDP2_PRISA,
        VDP2_PRISA & 7, (VDP2_PRISA >> 8) & 7);
    mu_assert((VDP2_PRISA & 7) == 3 && ((VDP2_PRISA >> 8) & 7) == 4, buffer);
}

/** @brief Scroll priorities: NBG1 in PRINA bits 8-10, NBG2 in PRINB bits 0-2, RBG0 in PRIR bits 0-2; neighbors kept. */
MU_TEST(vdp2_scroll_priority)
{
    const uint16_t nbg0 = VDP2_PRINA & 7;
    const uint16_t nbg3 = (VDP2_PRINB >> 8) & 7;

    VDP2::NBG1::SetPriority(VDP2::Priority::Layer5);
    VDP2::NBG2::SetPriority(VDP2::Priority::Layer2);
    VDP2::RBG0::SetPriority(VDP2::Priority::Layer6);

    snprintf(buffer, buffer_size, "PRINA 0x%04x, PRINB 0x%04x, PRIR 0x%04x: expected NBG1 5, NBG2 2, RBG0 6", VDP2_PRINA, VDP2_PRINB, VDP2_PRIR);
    mu_assert(((VDP2_PRINA >> 8) & 7) == 5 && (VDP2_PRINB & 7) == 2 && (VDP2_PRIR & 7) == 6, buffer);
    mu_assert((VDP2_PRINA & 7) == nbg0 && ((VDP2_PRINB >> 8) & 7) == nbg3, "Setting NBG1 and NBG2 changed NBG0 or NBG3");
}

/** @brief Sprite priority banks: bank 1 in PRISA bits 8-10, bank 2 in PRISB bits 0-2. */
MU_TEST(vdp2_sprite_priority)
{
    const uint16_t bank0 = VDP2_PRISA & 7;
    VDP2::SpriteLayer::SetPriority(VDP2::Priority::Layer6, VDP2::SpriteBank::Bank1);
    VDP2::SpriteLayer::SetPriority(VDP2::Priority::Layer1, VDP2::SpriteBank::Bank2);

    snprintf(buffer, buffer_size, "PRISA 0x%04x, PRISB 0x%04x: expected bank 1 on 6 and bank 2 on 1", VDP2_PRISA, VDP2_PRISB);
    mu_assert(((VDP2_PRISA >> 8) & 7) == 6 && (VDP2_PRISB & 7) == 1, buffer);
    mu_assert((VDP2_PRISA & 7) == bank0, "Setting bank 1 changed bank 0");
}

/** @brief Scroll opacity: below 1.0 it sets the ratio (31 - opacity * 32, NBG1 in CCRNA bits 8-12) and turns color
 *  calculation on (CCCTL bit 1); 1.0 turns it off; a negative opacity changes nothing. */
MU_TEST(vdp2_scroll_opacity)
{
    VDP2::NBG1::SetOpacity(0.5);
    snprintf(buffer, buffer_size, "Opacity 0.5: CCRNA 0x%04x, CCCTL 0x%04x (expected NBG1 ratio 15, bit 1 set)", VDP2_CCRNA, VDP2_CCCTL);
    mu_assert(((VDP2_CCRNA >> 8) & 0x1F) == 15 && (VDP2_CCCTL & 0x2) != 0, buffer);

    VDP2::NBG1::SetOpacity(0.0);
    snprintf(buffer, buffer_size, "Opacity 0.0: NBG1 ratio %d, expected 31", (VDP2_CCRNA >> 8) & 0x1F);
    mu_assert(((VDP2_CCRNA >> 8) & 0x1F) == 31, buffer);

    VDP2::NBG1::SetOpacity(-0.5);
    mu_assert(((VDP2_CCRNA >> 8) & 0x1F) == 31 && (VDP2_CCCTL & 0x2) != 0, "A negative opacity changed the settings");

    VDP2::NBG1::SetOpacity(1.0);
    snprintf(buffer, buffer_size, "Opacity 1.0: CCCTL 0x%04x, expected bit 1 clear", VDP2_CCCTL);
    mu_assert((VDP2_CCCTL & 0x2) == 0, buffer);
}

/** @brief Sprite opacity sets the ratio of a bank (bank 0 in CCRSA bits 0-4) and nothing else; out of [0, 1] it is ignored. */
MU_TEST(vdp2_sprite_opacity_ratio)
{
    const uint16_t bank1 = VDP2_CCRSA & 0x1F00;

    VDP2::SpriteLayer::SetOpacity(0.5, VDP2::SpriteBank::Bank0);
    snprintf(buffer, buffer_size, "Opacity 0.5: CCRSA 0x%04x, expected bank 0 ratio 15", VDP2_CCRSA);
    mu_assert((VDP2_CCRSA & 0x1F) == 15, buffer);

    VDP2::SpriteLayer::SetOpacity(0.0, VDP2::SpriteBank::Bank0);
    snprintf(buffer, buffer_size, "Opacity 0.0: CCRSA 0x%04x, expected bank 0 ratio 31", VDP2_CCRSA);
    mu_assert((VDP2_CCRSA & 0x1F) == 31, buffer);

    VDP2::SpriteLayer::SetOpacity(1.5, VDP2::SpriteBank::Bank0);
    VDP2::SpriteLayer::SetOpacity(-0.5, VDP2::SpriteBank::Bank0);
    mu_assert((VDP2_CCRSA & 0x1F) == 31, "An opacity out of [0, 1] changed the ratio");
    mu_assert((VDP2_CCRSA & 0x1F00) == bank1, "Setting bank 0 changed bank 1");
}

/** @brief Opacity 1.0 (accepted, documented range 0.0-1.0) gives the most opaque ratio, 0, and leaves bank 1 alone. */
MU_TEST(vdp2_sprite_opacity_full)
{
    const uint16_t bank1 = VDP2_CCRSA & 0x1F00;
    VDP2::SpriteLayer::SetOpacity(1.0, VDP2::SpriteBank::Bank0);

    snprintf(buffer, buffer_size, "Opacity 1.0: CCRSA 0x%04x, expected bank 0 ratio 0 and bank 1 bits 0x%04x", VDP2_CCRSA, bank1);
    mu_assert((VDP2_CCRSA & 0x1F) == 0 && (VDP2_CCRSA & 0x1F00) == bank1, buffer);
}

/** @brief ScrollDisable and ScrollEnable clear and set the screen's display bit (NBG3: BGON bit 3). */
MU_TEST(vdp2_scroll_enable_disable)
{
    VDP2::NBG3::ScrollDisable();
    const uint16_t disabled = VDP2_BGON;
    VDP2::NBG3::ScrollEnable();

    snprintf(buffer, buffer_size, "BGON 0x%04x after ScrollDisable, 0x%04x after ScrollEnable (NBG3 is bit 3)", disabled, VDP2_BGON);
    mu_assert((disabled & 0x8) == 0 && (VDP2_BGON & 0x8) != 0, buffer);
}

/** @brief TransparentDisable makes color 0 visible (NBG3: BGON bit 11 set), TransparentEnable makes it transparent again. */
MU_TEST(vdp2_transparency)
{
    VDP2::NBG3::TransparentDisable();
    const uint16_t disabled = VDP2_BGON;
    VDP2::NBG3::TransparentEnable();

    snprintf(buffer, buffer_size, "BGON 0x%04x after TransparentDisable, 0x%04x after TransparentEnable (NBG3 is bit 11)", disabled, VDP2_BGON);
    mu_assert((disabled & 0x800) != 0 && (VDP2_BGON & 0x800) == 0, buffer);
}

/** @brief Color offsets: values in COAR-COBB (9-bit signed, clamped to +-255), and the screen's choice in CLOFEN/CLOFSL (NBG1: bit 1). */
MU_TEST(vdp2_color_offset)
{
    VDP2::ColorOffset a(10, -20, 300);
    VDP2::ColorOffset b(-300, 0, 255);
    VDP2::SetColorOffsetA(a);
    VDP2::SetColorOffsetB(b);

    snprintf(buffer, buffer_size, "Offset A %03x %03x %03x, B %03x %03x %03x (expected 00a 1ec 0ff, 101 000 0ff)",
        VDP2_COAR & 0x1FF, VDP2_COAG & 0x1FF, VDP2_COAB & 0x1FF, VDP2_COBR & 0x1FF, VDP2_COBG & 0x1FF, VDP2_COBB & 0x1FF);
    mu_assert((VDP2_COAR & 0x1FF) == 0x00A && (VDP2_COAG & 0x1FF) == 0x1EC && (VDP2_COAB & 0x1FF) == 0x0FF, buffer);
    mu_assert((VDP2_COBR & 0x1FF) == 0x101 && (VDP2_COBG & 0x1FF) == 0 && (VDP2_COBB & 0x1FF) == 0x0FF, buffer);

    VDP2::NBG1::UseColorOffset(VDP2::OffsetChannel::OffsetB);
    snprintf(buffer, buffer_size, "OffsetB: CLOFEN 0x%04x, CLOFSL 0x%04x (expected bit 1 set in both)", VDP2_CLOFEN, VDP2_CLOFSL);
    mu_assert((VDP2_CLOFEN & 0x2) != 0 && (VDP2_CLOFSL & 0x2) != 0, buffer);

    VDP2::NBG1::UseColorOffset(VDP2::OffsetChannel::OffsetA);
    snprintf(buffer, buffer_size, "OffsetA: CLOFEN 0x%04x, CLOFSL 0x%04x (expected bit 1 set, then clear)", VDP2_CLOFEN, VDP2_CLOFSL);
    mu_assert((VDP2_CLOFEN & 0x2) != 0 && (VDP2_CLOFSL & 0x2) == 0, buffer);

    VDP2::NBG1::UseColorOffset(VDP2::OffsetChannel::NoOffset);
    snprintf(buffer, buffer_size, "NoOffset: CLOFEN 0x%04x (expected bit 1 clear)", VDP2_CLOFEN);
    mu_assert((VDP2_CLOFEN & 0x2) == 0, buffer);
}

/** @brief SetBackColor writes the color where the back screen reads it. */
MU_TEST(vdp2_back_color)
{
    VDP2::SetBackColor(Types::HighColor(255, 0, 0));
    snprintf(buffer, buffer_size, "Back color is 0x%04x, expected 0x%04x", *vdp2_back_color_ram, static_cast<uint16_t>(Types::HighColor(255, 0, 0)));
    mu_assert(*vdp2_back_color_ram == static_cast<uint16_t>(Types::HighColor(255, 0, 0)), buffer);
}

/** @brief Color calculation mode: addition is CCCTL bit 8, second-screen ratio bit 9, extended bit 10. */
MU_TEST(vdp2_color_calc_mode)
{
    VDP2::SetColorCalcMode(VDP2::ColorCalcMode::UseColorAddition);
    const uint16_t addition = VDP2_CCCTL & 0x700;
    VDP2::SetColorCalcMode(VDP2::ColorCalcMode::UseColorRatios2nd, true);
    const uint16_t second = VDP2_CCCTL & 0x700;
    VDP2::SetColorCalcMode();
    const uint16_t top = VDP2_CCCTL & 0x700;

    snprintf(buffer, buffer_size, "CCCTL mode bits 0x%03x, 0x%03x, 0x%03x (expected 0x100, 0x600, 0x000)", addition, second, top);
    mu_assert(addition == 0x100 && second == 0x600 && top == 0, buffer);
}

/** @brief Sprite color calculation: condition in SPCTL bits 12-13, priority in bits 8-10, on/off in CCCTL bit 6. */
MU_TEST(vdp2_sprite_color_calculation)
{
    VDP2::SpriteLayer::SetColorCondition(VDP2::ColorCondition::PriorityGreaterThan, VDP2::Priority::Layer6);
    snprintf(buffer, buffer_size, "SPCTL 0x%04x, expected condition 2 and priority 6", VDP2_SPCTL);
    mu_assert(((VDP2_SPCTL >> 12) & 3) == 2 && ((VDP2_SPCTL >> 8) & 7) == 6, buffer);

    VDP2::SpriteLayer::ColorCalcON();
    const uint16_t on = VDP2_CCCTL;
    VDP2::SpriteLayer::ColorCalcOFF();
    snprintf(buffer, buffer_size, "CCCTL 0x%04x after ColorCalcON, 0x%04x after ColorCalcOFF (sprite is bit 6)", on, VDP2_CCCTL);
    mu_assert((on & 0x40) != 0 && (VDP2_CCCTL & 0x40) == 0, buffer);
}

/** @brief Scale limit: NBG1 1/2x is ZMCTL bit 8, 1/4x bit 9. Scale 2.0 shows the screen twice as big: coordinate step 0.5. */
MU_TEST(vdp2_scale)
{
    VDP2::NBG1::SetScaleLimit(VDP2::ScaleLimit::Half);
    const uint16_t half = VDP2_ZMCTL & 0x300;
    VDP2::NBG1::SetScaleLimit(VDP2::ScaleLimit::Quarter);
    const uint16_t quarter = VDP2_ZMCTL & 0x300;
    VDP2::NBG1::SetScaleLimit(VDP2::ScaleLimit::One);

    snprintf(buffer, buffer_size, "ZMCTL NBG1 bits 0x%03x, 0x%03x, 0x%03x (expected 0x100, 0x200, 0)", half, quarter, VDP2_ZMCTL & 0x300);
    mu_assert(half == 0x100 && quarter == 0x200 && (VDP2_ZMCTL & 0x300) == 0, buffer);

    Math::Vector2D scale(2.0, 1.0);
    VDP2::NBG0::SetScale(scale);
    snprintf(buffer, buffer_size, "Scale (2, 1): X step %d.%04x, Y step %d.%04x (expected 0.8000 and 1.0000)", VDP2_ZMXIN0, VDP2_ZMXDN0, VDP2_ZMYIN0, VDP2_ZMYDN0);
    mu_assert(VDP2_ZMXIN0 == 0 && VDP2_ZMXDN0 == 0x8000 && VDP2_ZMYIN0 == 1 && VDP2_ZMYDN0 == 0, buffer);
}

/** @brief Rotation: parameter mode in RPMD bits 0-1, screen-over mode in PLSZ bits 10-11 with its tile in OVPNRA,
 *  and 2-axis rotation turns the coefficient table on (KTCTL bit 0) in bank B0; 1-axis turns it off. */
MU_TEST(vdp2_rotation_settings)
{
    VDP2::RBG0::SetParameterMode(VDP2::SwitchMode::OnlySecondary);
    const uint16_t secondary = VDP2_RPMD & 3;
    VDP2::RBG0::SetParameterMode(VDP2::SwitchMode::WindowSwitch);
    snprintf(buffer, buffer_size, "RPMD %d then %d, expected 1 then 3", secondary, VDP2_RPMD & 3);
    mu_assert(secondary == 1 && (VDP2_RPMD & 3) == 3, buffer);

    VDP2::RBG0::SetOverDisplay(VDP2::RotationParameter::Primary, VDP2::OverPattern::Transparent, 5);
    snprintf(buffer, buffer_size, "PLSZ 0x%04x, OVPNRA %d: expected over mode 2, tile 5", VDP2_PLSZ, VDP2_OVPNRA);
    mu_assert(((VDP2_PLSZ >> 10) & 3) == 2 && VDP2_OVPNRA == 5, buffer);

    VDP2::RBG0::SetRotationMode(VDP2::RotationMode::TwoAxis);
    const uint16_t twoAxis = VDP2_KTCTL;
    const uintptr_t table = reinterpret_cast<uintptr_t>(VDP2::RBG0::KtableAddress);
    VDP2::RBG0::SetRotationMode(VDP2::RotationMode::OneAxis);

    snprintf(buffer, buffer_size, "KTCTL 0x%04x with 2 axes (table at %p), 0x%04x with 1 axis: expected bit 0 set then clear, table in B0",
        twoAxis, VDP2::RBG0::KtableAddress, VDP2_KTCTL);
    mu_assert((twoAxis & 1) != 0 && (VDP2_KTCTL & 1) == 0, buffer);
    mu_assert(table >= VDP2_VRAM_B0 && table < VDP2_VRAM_B1, buffer);
    VDP2::ClearVRAM();
}

MU_TEST_SUITE(vdp2_settings_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&vdp2_settings_setup, &vdp2_settings_teardown, &vdp2_test_output_header);

    MU_RUN_TEST(vdp2_default_sprite_priorities);
    MU_RUN_TEST(vdp2_scroll_priority);
    MU_RUN_TEST(vdp2_sprite_priority);
    MU_RUN_TEST(vdp2_scroll_opacity);
    MU_RUN_TEST(vdp2_sprite_opacity_ratio);
    MU_RUN_TEST(vdp2_sprite_opacity_full);
    MU_RUN_TEST(vdp2_scroll_enable_disable);
    MU_RUN_TEST(vdp2_transparency);
    MU_RUN_TEST(vdp2_color_offset);
    MU_RUN_TEST(vdp2_back_color);
    MU_RUN_TEST(vdp2_color_calc_mode);
    MU_RUN_TEST(vdp2_sprite_color_calculation);
    MU_RUN_TEST(vdp2_scale);
    MU_RUN_TEST(vdp2_rotation_settings);
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

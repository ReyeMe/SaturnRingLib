// Tests/src/testsTilemap.hpp
// SRL::Tilemap: TilemapInfo color modes, SGLTile, Bmp2Tile (256-color and RGB conversion, size checks, pages, CopyMap,
// ApplyVdp2Offsets) and loading a tilemap into NBG1 with 1-word and 2-word map data. Only NBG1 is used: NBG3 shows the
// results. The 16-color Bmp2Tile conversion is in testsVDP2.hpp, CubeTile (needs the disc) in testsCubeTile.hpp.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_vdp2.hpp>
#include <srl_tilemap_interfaces.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Tilemap::Coord;
using SRL::Tilemap::TilemapInfo;
using SRL::Tilemap::Interfaces::Bmp2Tile;
using SRL::Tilemap::Interfaces::SGLTile;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Gives back the CRAM bank NBG1 took (ClearVRAM leaves it marked used), then empties VDP2 VRAM. */
static void tilemap_release_nbg1()
{
    if (VDP2::NBG1::TilePalette.GetData() != nullptr)
    {
        CRAM::SetBankUsedState(VDP2::NBG1::TilePalette.GetId(), VDP2::NBG1::TilePalette.GetMode(), false);
    }

    VDP2::ClearVRAM();
}

void tilemap_test_setup(void) { VDP2::ClearVRAM(); }
void tilemap_test_teardown(void) { tilemap_release_nbg1(); }

/** @brief Used blocks of both work RAM heaps, to check that tilemaps give their memory back. */
static size_t tilemap_used_blocks()
{
    return Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
}

/** @brief VDP2 color type for each texture color mode: RGB 32K colors, 16 colors, or 256 colors for 64 to 256. */
MU_TEST(tilemap_info_sgl_color_mode)
{
    struct { CRAM::TextureColorMode mode; uint16_t expected; } cases[] = {
        { CRAM::TextureColorMode::RGB555, COL_TYPE_32768 },
        { CRAM::TextureColorMode::Paletted16, COL_TYPE_16 },
        { CRAM::TextureColorMode::Paletted64, COL_TYPE_256 },
        { CRAM::TextureColorMode::Paletted128, COL_TYPE_256 },
        { CRAM::TextureColorMode::Paletted256, COL_TYPE_256 },
    };

    for (const auto &c : cases)
    {
        TilemapInfo info;
        info.ColorMode = c.mode;
        snprintf(buffer, buffer_size, "Color mode %d gives 0x%02x (TilemapInfo) and 0x%02x (VDP2), expected 0x%02x",
            static_cast<int>(c.mode), info.SGLColorMode(), VDP2::GetSGLColorMode(c.mode), c.expected);
        mu_assert(info.SGLColorMode() == c.expected && VDP2::GetSGLColorMode(c.mode) == c.expected, buffer);
    }
}

/** @brief SGLTile hands back the data and info it was given. */
MU_TEST(tilemap_sgltile_passes_data_through)
{
    static uint8_t cells[32];
    static uint16_t map[4];
    static uint16_t palette[16];
    const TilemapInfo info(CRAM::TextureColorMode::Paletted16, PNB_1WORD | CN_12BIT, CHAR_SIZE_1x1, PL_SIZE_1x1, 64, 64, 32);
    SGLTile tile(cells, map, palette, info);

    mu_assert(tile.GetCellData() == cells && tile.GetMapData() == map && tile.GetPalData() == palette, "SGLTile changed a data pointer");
    const TilemapInfo got = tile.GetInfo();
    mu_assert(got.ColorMode == info.ColorMode && got.MapMode == info.MapMode && got.CharSize == info.CharSize &&
        got.MapWidth == 64 && got.MapHeight == 64 && got.CellByteSize == 32, "SGLTile changed the tilemap info");
}

/** @brief 256-color pixel of the 32x32 test bitmap: the top-left and bottom-right 16x16 tiles are empty. */
static uint8_t tilemap_pixel256(int x, int y)
{
    const bool empty = (x < 16) == (y < 16);
    return empty ? 0 : static_cast<uint8_t>(1 + (x * 5 + y * 3) % 250);
}

static uint8_t tilemap_bitmap256[32 * 32];

static void tilemap_fill_bitmap256()
{
    for (int y = 0; y < 32; y++)
    {
        for (int x = 0; x < 32; x++) tilemap_bitmap256[y * 32 + x] = tilemap_pixel256(x, y);
    }
}

/** @brief Checks the four 8x8 cells of the 16x16 tile at pixel (tileX, tileY), stored from cells[start]. */
static bool tilemap_check_tile256(const uint8_t *cells, int start, int tileX, int tileY)
{
    for (int cell = 0; cell < 4; cell++)
    {
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                const int x = tileX + (cell & 1) * 8 + col;
                const int y = tileY + (cell >> 1) * 8 + row;
                const uint8_t actual = cells[start + cell * 64 + row * 8 + col];
                if (actual != tilemap_pixel256(x, y))
                {
                    snprintf(buffer, buffer_size, "Tile at (%d, %d), cell %d, row %d, column %d is %d, expected %d", tileX, tileY,
                        cell, row, col, actual, tilemap_pixel256(x, y));
                    return false;
                }
            }
        }
    }

    return true;
}

/**
 * @brief Bmp2Tile with 256 colors: an empty 256-byte character first, then each non-empty 16x16 tile as four 64-byte
 *        cells. Map entries count in 128-byte units (1-word map, 16x16 characters), so each 256-color tile adds 2.
 */
MU_TEST(tilemap_bmp2tile_256_colors)
{
    tilemap_fill_bitmap256();
    Bitmap::Palette palette(256);
    MockBitmap bitmap(tilemap_bitmap256, Bitmap::BitmapInfo(32, 32, &palette));
    Bmp2Tile tiles(bitmap);
    const TilemapInfo info = tiles.GetInfo();

    snprintf(buffer, buffer_size, "Color mode %d, char size %d, cell bytes %d (expected Paletted256, 2x2, 1280)",
        static_cast<int>(info.ColorMode), info.CharSize, static_cast<int>(info.CellByteSize));
    mu_assert(info.ColorMode == CRAM::TextureColorMode::Paletted256 && info.CharSize == CHAR_SIZE_2x2 && info.CellByteSize == 1280, buffer);

    const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());
    snprintf(buffer, buffer_size, "Map entries %d %d / %d %d, expected 0 2 / 4 0", map[0], map[1], map[32], map[33]);
    mu_assert(map[0] == 0 && map[1] == 2 && map[32] == 4 && map[33] == 0, buffer);

    const uint8_t *cells = static_cast<const uint8_t *>(tiles.GetCellData());
    for (int i = 0; i < 256; i++)
    {
        snprintf(buffer, buffer_size, "Byte %d of the empty first character is %d", i, cells[i]);
        mu_assert(cells[i] == 0, buffer);
    }
    mu_assert(tilemap_check_tile256(cells, 256, 16, 0), buffer);
    mu_assert(tilemap_check_tile256(cells, 512, 0, 16), buffer);
}

/** @brief Bmp2Tile with RGB pixels: a 16x16 tile is four 128-byte cells, and its map entry is 4 (512 bytes in). */
MU_TEST(tilemap_bmp2tile_rgb)
{
    static uint16_t pixels[16 * 16];
    for (int i = 0; i < 16 * 16; i++) pixels[i] = static_cast<uint16_t>(0x8000 | (i * 37));
    MockBitmap bitmap(reinterpret_cast<uint8_t *>(pixels), Bitmap::BitmapInfo(16, 16));
    Bmp2Tile tiles(bitmap);
    const TilemapInfo info = tiles.GetInfo();

    snprintf(buffer, buffer_size, "Color mode %d, cell bytes %d (expected RGB555, 1024)", static_cast<int>(info.ColorMode), static_cast<int>(info.CellByteSize));
    mu_assert(info.ColorMode == CRAM::TextureColorMode::RGB555 && info.CellByteSize == 1024, buffer);
    mu_assert(tiles.GetPalData() == nullptr, "An RGB tilemap got a palette");

    const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());
    snprintf(buffer, buffer_size, "Map entry %d, expected 4", map[0]);
    mu_assert(map[0] == 4, buffer);

    const uint16_t *cells = static_cast<const uint16_t *>(tiles.GetCellData()) + 256;
    for (int cell = 0; cell < 4; cell++)
    {
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 8; col++)
            {
                const uint16_t expected = pixels[((cell >> 1) * 8 + row) * 16 + (cell & 1) * 8 + col];
                const uint16_t actual = cells[cell * 64 + row * 8 + col];
                snprintf(buffer, buffer_size, "Cell %d row %d column %d is 0x%04x, expected 0x%04x", cell, row, col, actual, expected);
                mu_assert(actual == expected, buffer);
            }
        }
    }
}

/** @brief A 16-color bitmap one tile (16 pixels, 8 bytes) wide is converted. */
MU_TEST(tilemap_bmp2tile_16_colors_one_tile_wide)
{
    static uint8_t pixels[16 * 16 / 2];
    for (int i = 0; i < 16 * 16 / 2; i++) pixels[i] = 0x12;
    Bitmap::Palette palette(16);
    MockBitmap bitmap(pixels, Bitmap::BitmapInfo(16, 16, &palette));
    Bmp2Tile tiles(bitmap);

    mu_assert(tiles.GetCellData() != nullptr && tiles.GetMapData() != nullptr, "A 16x16 16-color bitmap was refused");
    const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());
    snprintf(buffer, buffer_size, "Map entry %d, expected 1 (after the empty 128-byte character)", map[0]);
    mu_assert(map[0] == 1, buffer);
}

/** @brief Characters are 16x16, so a bitmap 24 pixels wide or 24 high cannot be cut into whole tiles and is refused. */
MU_TEST(tilemap_bmp2tile_refuses_partial_tiles)
{
    static uint8_t pixels[24 * 24];
    Bitmap::Palette palette(256);
    MockBitmap wide(pixels, Bitmap::BitmapInfo(24, 16, &palette));
    MockBitmap high(pixels, Bitmap::BitmapInfo(16, 24, &palette));
    Bmp2Tile wideTiles(wide);
    Bmp2Tile highTiles(high);

    mu_assert(wideTiles.GetCellData() == nullptr, "A 24x16 bitmap was converted (its last 8 columns are lost and the rows drift)");
    mu_assert(highTiles.GetCellData() == nullptr, "A 16x24 bitmap was converted (its last 8 rows are lost)");
}

/** @brief A refused bitmap leaves nothing allocated behind. */
MU_TEST(tilemap_bmp2tile_refused_no_leak)
{
    static uint8_t pixels[20 * 16];
    Bitmap::Palette palette(256);
    MockBitmap bitmap(pixels, Bitmap::BitmapInfo(20, 16, &palette));

    const size_t before = tilemap_used_blocks();
    bool refused;
    {
        Bmp2Tile tiles(bitmap);
        refused = tiles.GetCellData() == nullptr && tiles.GetMapData() == nullptr;
    }
    const size_t after = tilemap_used_blocks();

    mu_assert(refused, "A 20-pixel-wide bitmap was converted");
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after a refused conversion", before, after);
    mu_assert(after == before, buffer);
}

/** @brief With 2 pages the map is 32x64; GetPageAddress points into the right page; every page starts cleared. */
MU_TEST(tilemap_bmp2tile_pages)
{
    tilemap_fill_bitmap256();
    Bitmap::Palette palette(256);
    MockBitmap bitmap(tilemap_bitmap256, Bitmap::BitmapInfo(32, 32, &palette));
    Bmp2Tile tiles(bitmap, 2);
    const TilemapInfo info = tiles.GetInfo();
    uint16_t *map = static_cast<uint16_t *>(tiles.GetMapData());

    snprintf(buffer, buffer_size, "Map is %dx%d, expected 32x64", info.MapWidth, info.MapHeight);
    mu_assert(info.MapWidth == 32 && info.MapHeight == 64, buffer);

    Coord coord(3, 4);
    mu_assert(tiles.GetPageAddress(1, coord) == map + 1024 + 3 + 4 * 32, "GetPageAddress(1, (3, 4)) points to the wrong entry");

    for (int i = 1024; i < 2048; i++)
    {
        snprintf(buffer, buffer_size, "Page 1 entry %d is %d, expected 0", i - 1024, map[i]);
        mu_assert(map[i] == 0, buffer);
    }
}

/** @brief CopyMap copies a block of entries to another page, and crops it at the page edge instead of wrapping. */
MU_TEST(tilemap_bmp2tile_copy_map)
{
    tilemap_fill_bitmap256();
    Bitmap::Palette palette(256);
    MockBitmap bitmap(tilemap_bitmap256, Bitmap::BitmapInfo(32, 32, &palette));
    Bmp2Tile tiles(bitmap, 2);
    tiles.ApplyVdp2Offsets(0x10);   // every entry non-zero, so a stray copy shows
    tiles.ClearPage(1);
    const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());
    const uint16_t *page1 = map + 1024;

    tiles.CopyMap(0, Coord(0, 0), Coord(1, 1), 1, Coord(5, 6));
    snprintf(buffer, buffer_size, "Copied block %d %d / %d %d, expected %d %d / %d %d", page1[6 * 32 + 5], page1[6 * 32 + 6],
        page1[7 * 32 + 5], page1[7 * 32 + 6], map[0], map[1], map[32], map[33]);
    mu_assert(page1[6 * 32 + 5] == map[0] && page1[6 * 32 + 6] == map[1] && page1[7 * 32 + 5] == map[32] && page1[7 * 32 + 6] == map[33], buffer);
    mu_assert(page1[6 * 32 + 7] == 0 && page1[8 * 32 + 5] == 0, "CopyMap wrote outside the 2x2 block");

    // Two columns copied to column 31: only one fits
    tiles.CopyMap(0, Coord(0, 0), Coord(1, 0), 1, Coord(31, 0));
    snprintf(buffer, buffer_size, "Entry (31, 0) is %d, expected %d; entry (0, 1) is %d, expected 0 (no wrap)", page1[31], map[0], page1[32]);
    mu_assert(page1[31] == map[0] && page1[32] == 0, buffer);
}

/** @brief ApplyVdp2Offsets adds the cell offset to every entry and ORs in the palette bits. */
MU_TEST(tilemap_bmp2tile_apply_offsets)
{
    tilemap_fill_bitmap256();
    Bitmap::Palette palette(256);
    MockBitmap bitmap(tilemap_bitmap256, Bitmap::BitmapInfo(32, 32, &palette));
    Bmp2Tile tiles(bitmap);
    tiles.ApplyVdp2Offsets(0x100, 0x3000);
    const uint16_t *map = static_cast<const uint16_t *>(tiles.GetMapData());

    snprintf(buffer, buffer_size, "Entries 0x%04x 0x%04x 0x%04x, expected 0x3100 0x3102 0x3104", map[0], map[1], map[32]);
    mu_assert(map[0] == 0x3100 && map[1] == 0x3102 && map[32] == 0x3104, buffer);
}

/** @brief 16-color cells, map and palette for the NBG1 loading tests. */
static uint8_t tilemap_cells[256];
static uint16_t tilemap_palette[16];

static void tilemap_fill_cells_and_palette()
{
    for (int i = 0; i < 256; i++) tilemap_cells[i] = static_cast<uint8_t>(i * 7 + 3);
    for (int i = 0; i < 16; i++) tilemap_palette[i] = static_cast<uint16_t>(0x8000 | (i * 0x0421));
}

/**
 * @brief LoadTilemap with a 1-word map (16x16 characters): cells and palette are copied, and each map entry gets the
 *        cell offset (128-byte units from VRAM start) and the palette number in bits 12-15.
 */
MU_TEST(tilemap_load_into_nbg1_1word)
{
    static uint16_t map[32 * 32];
    tilemap_fill_cells_and_palette();
    for (int i = 0; i < 32 * 32; i++) map[i] = static_cast<uint16_t>(i & 1);
    SGLTile tile(tilemap_cells, map, tilemap_palette,
        TilemapInfo(CRAM::TextureColorMode::Paletted16, PNB_1WORD | CN_12BIT, CHAR_SIZE_2x2, PL_SIZE_1x1, 32, 32, 256));

    VDP2::NBG1::LoadTilemap(tile);
    mu_assert(VDP2::NBG1::TilePalette.GetData() != nullptr, "LoadTilemap did not take a CRAM palette");

    const volatile uint8_t *vramCells = static_cast<const volatile uint8_t *>(VDP2::NBG1::CellAddress);
    for (int i = 0; i < 256; i++)
    {
        snprintf(buffer, buffer_size, "VRAM cell byte %d is %d, expected %d", i, vramCells[i], tilemap_cells[i]);
        mu_assert(vramCells[i] == tilemap_cells[i], buffer);
    }

    const volatile uint16_t *cram = reinterpret_cast<const volatile uint16_t *>(VDP2::NBG1::TilePalette.GetData());
    for (int i = 0; i < 16; i++)
    {
        snprintf(buffer, buffer_size, "CRAM color %d is 0x%04x, expected 0x%04x", i, cram[i], tilemap_palette[i]);
        mu_assert(cram[i] == tilemap_palette[i], buffer);
    }

    const uint16_t cellOffset = static_cast<uint16_t>((reinterpret_cast<uint32_t>(VDP2::NBG1::CellAddress) - VDP2_VRAM_A0) >> 7);
    const uint16_t paletteBits = static_cast<uint16_t>(VDP2::NBG1::TilePalette.GetId() << 12);
    const volatile uint16_t *vramMap = static_cast<const volatile uint16_t *>(VDP2::NBG1::MapAddress);
    for (int i = 0; i < 32 * 32; i++)
    {
        const uint16_t expected = static_cast<uint16_t>((map[i] + cellOffset) | paletteBits);
        snprintf(buffer, buffer_size, "VRAM map entry %d is 0x%04x, expected 0x%04x", i, vramMap[i], expected);
        mu_assert(vramMap[i] == expected, buffer);
    }
}

/**
 * @brief LoadTilemap with a 2-word map (8x8 characters, 16 colors): each entry gets the cell offset (32-byte units)
 *        and the palette number in bits 16-22 (bits 0-6 of the first word).
 */
MU_TEST(tilemap_load_into_nbg1_2word)
{
    static uint32_t map[64 * 64];
    tilemap_fill_cells_and_palette();
    for (int i = 0; i < 64 * 64; i++) map[i] = static_cast<uint32_t>(i & 7);

    // A free bank other than 0, so wrong palette bits show
    uint16_t bank = 1;
    while (bank < 128 && CRAM::GetBankUsedState(bank, CRAM::TextureColorMode::Paletted16)) bank++;
    mu_assert(bank < 128, "No free 16-color CRAM bank");
    CRAM::SetBankUsedState(bank, CRAM::TextureColorMode::Paletted16, true);
    VDP2::NBG1::SetPalette(CRAM::Palette(CRAM::TextureColorMode::Paletted16, bank));

    SGLTile tile(tilemap_cells, map, tilemap_palette,
        TilemapInfo(CRAM::TextureColorMode::Paletted16, PNB_2WORD, CHAR_SIZE_1x1, PL_SIZE_1x1, 64, 64, 256));
    VDP2::NBG1::LoadTilemap(tile);

    const uint32_t cellOffset = (reinterpret_cast<uint32_t>(VDP2::NBG1::CellAddress) - VDP2_VRAM_A0) >> 5;
    const volatile uint32_t *vramMap = static_cast<const volatile uint32_t *>(VDP2::NBG1::MapAddress);
    for (int i = 0; i < 64 * 64; i++)
    {
        const uint32_t expected = (map[i] + cellOffset) | (static_cast<uint32_t>(bank) << 16);
        snprintf(buffer, buffer_size, "VRAM map entry %d is 0x%08x, expected 0x%08x (palette %d)", i,
            static_cast<unsigned>(vramMap[i]), static_cast<unsigned>(expected), bank);
        mu_assert(vramMap[i] == expected, buffer);
    }
}

MU_TEST_SUITE(tilemap_test_suite)
{
    MU_SUITE_CONFIGURE(&tilemap_test_setup, &tilemap_test_teardown);

    MU_RUN_TEST(tilemap_info_sgl_color_mode);
    MU_RUN_TEST(tilemap_sgltile_passes_data_through);
    MU_RUN_TEST(tilemap_bmp2tile_256_colors);
    MU_RUN_TEST(tilemap_bmp2tile_rgb);
    MU_RUN_TEST(tilemap_bmp2tile_16_colors_one_tile_wide);
    MU_RUN_TEST(tilemap_bmp2tile_refuses_partial_tiles);
    MU_RUN_TEST(tilemap_bmp2tile_refused_no_leak);
    MU_RUN_TEST(tilemap_bmp2tile_pages);
    MU_RUN_TEST(tilemap_bmp2tile_copy_map);
    MU_RUN_TEST(tilemap_bmp2tile_apply_offsets);
    MU_RUN_TEST(tilemap_load_into_nbg1_1word);
    MU_RUN_TEST(tilemap_load_into_nbg1_2word);
}

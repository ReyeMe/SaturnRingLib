// Tests/src/testsCubeTile.hpp
// SRL::Tilemap::Interfaces::CubeTile: loading tilemap files from the disc. The files come from
// Tests/campaigns/media/make_data.py: palette byte i = i * 3 + seed, map byte i = i * 5 + seed, cell byte i = i * 7 + seed.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_tilemap_interfaces.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Tilemap::TilemapInfo;
using SRL::Tilemap::Interfaces::CubeTile;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void cubetile_test_setup(void) {}
void cubetile_test_teardown(void) {}

void cubetile_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_CUBETILE_ERROR(S)****");
    }
}

/** @brief What one test file holds. */
struct CubeTileFile
{
    const char *name;
    uint8_t seed;
    CRAM::TextureColorMode colorMode;
    uint16_t charSize;
    uint16_t planeSize;
    uint16_t mapMode;
    uint16_t mapWidth;
    uint16_t mapHeight;
    int32_t cellSize;
    int32_t mapSize;
    int32_t paletteSize;
};

/** @brief Checks that count bytes of data follow byte i = i * step + seed. */
static bool cubetile_check_bytes(const char *what, const CubeTileFile &file, const uint8_t *data, int32_t count, int step)
{
    if (data == nullptr)
    {
        snprintf(buffer, buffer_size, "%s: no %s data", file.name, what);
        return false;
    }

    for (int32_t i = 0; i < count; i++)
    {
        const uint8_t expected = static_cast<uint8_t>(i * step + file.seed);
        if (data[i] != expected)
        {
            snprintf(buffer, buffer_size, "%s: %s byte %d is %d, expected %d", file.name, what, static_cast<int>(i), data[i], expected);
            return false;
        }
    }

    return true;
}

/** @brief Loads one file and checks its info, palette, map and cells. */
static bool cubetile_check_file(const CubeTileFile &file)
{
    CubeTile tile(file.name);
    const TilemapInfo info = tile.GetInfo();

    if (info.ColorMode != file.colorMode || info.CharSize != file.charSize || info.PlaneSize != file.planeSize ||
        info.MapMode != file.mapMode || info.CellByteSize != file.cellSize)
    {
        snprintf(buffer, buffer_size, "%s: color %d char %d plane %d map mode 0x%04x cells %d, expected %d %d %d 0x%04x %d", file.name,
            static_cast<int>(info.ColorMode), info.CharSize, info.PlaneSize, info.MapMode, static_cast<int>(info.CellByteSize),
            static_cast<int>(file.colorMode), file.charSize, file.planeSize, file.mapMode, static_cast<int>(file.cellSize));
        return false;
    }

    if (info.MapWidth != file.mapWidth || info.MapHeight != file.mapHeight)
    {
        snprintf(buffer, buffer_size, "%s: map is %dx%d, expected %dx%d", file.name, info.MapWidth, info.MapHeight, file.mapWidth, file.mapHeight);
        return false;
    }

    if (file.paletteSize == 0 && tile.GetPalData() != nullptr)
    {
        snprintf(buffer, buffer_size, "%s: an RGB tilemap got a palette", file.name);
        return false;
    }

    return (file.paletteSize == 0 || cubetile_check_bytes("palette", file, static_cast<const uint8_t *>(tile.GetPalData()), file.paletteSize, 3)) &&
        cubetile_check_bytes("map", file, static_cast<const uint8_t *>(tile.GetMapData()), file.mapSize, 5) &&
        cubetile_check_bytes("cell", file, static_cast<const uint8_t *>(tile.GetCellData()), file.cellSize, 7);
}

/** @brief 16 colors, 16x16 characters, one 32x32 page: palette, map and cells are read in that order. */
MU_TEST(cubetile_16_colors)
{
    const CubeTileFile file = { "TILE16.CUB", 1, CRAM::TextureColorMode::Paletted16, CHAR_SIZE_2x2, PL_SIZE_1x1, PNB_1WORD | CN_12BIT,
        32, 32, 256, 64, 32 };
    mu_assert(cubetile_check_file(file), buffer);
}

/** @brief 256 colors, 8x8 characters (twice as many per side) and 2x1 planes (twice as wide): a 256x128 map. */
MU_TEST(cubetile_256_colors_plane_2x1)
{
    const CubeTileFile file = { "TILE256.CUB", 2, CRAM::TextureColorMode::Paletted256, CHAR_SIZE_1x1, PL_SIZE_2x1, PNB_2WORD,
        256, 128, 128, 32, 512 };
    mu_assert(cubetile_check_file(file), buffer);
}

/** @brief RGB with no palette; the header gives 3x2 pages, and 2x2 planes double both sides: a 192x128 map. */
MU_TEST(cubetile_rgb_width_and_height)
{
    const CubeTileFile file = { "TILERGB.CUB", 3, CRAM::TextureColorMode::RGB555, CHAR_SIZE_2x2, PL_SIZE_2x2, PNB_1WORD,
        192, 128, 512, 16, 0 };
    mu_assert(cubetile_check_file(file), buffer);
}

/** @brief A CubeTile gives back all its memory when destroyed. */
MU_TEST(cubetile_no_leak)
{
    const size_t before = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    {
        CubeTile tile("TILE256.CUB");
        mu_assert(tile.GetCellData() != nullptr, "TILE256.CUB did not load");
    }
    const size_t after = Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after loading and destroying a CubeTile", before, after);
    mu_assert(after == before, buffer);
}

MU_TEST_SUITE(cubetile_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&cubetile_test_setup, &cubetile_test_teardown, &cubetile_test_output_header);

    MU_RUN_TEST(cubetile_16_colors);
    MU_RUN_TEST(cubetile_256_colors_plane_2x1);
    MU_RUN_TEST(cubetile_rgb_width_and_height);
    MU_RUN_TEST(cubetile_no_leak);
}

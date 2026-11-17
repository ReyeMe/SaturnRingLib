// Tests/src/testsTGA.hpp
// SRL::Bitmap::TGA: true color (16, 24, 32 bit) and paletted (16, 256 colors) images, plain and RLE, in all four
// origins. The 8x4 images are written by Tests/campaigns/media/make_data.py: the formulas below must match it.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_tga.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

static constexpr uint16_t TgaWidth = 8;
static constexpr uint16_t TgaHeight = 4;

/** @brief Pixel key of the plain images, (x, y) from the top left corner. */
static int tga_plain_key(int x, int y) { return x * 3 + y * 5; }

/** @brief Pixel key of the RLE images: a run of 4 equal pixels, then 4 different ones, on every row. */
static int tga_rle_key(int x, int y) { return (x < 4 ? 0 : x) + y * 9; }

/** @brief Color of a true color pixel with key k. */
static Types::HighColor tga_true_color(int k)
{
    return Types::HighColor::FromRGB24((((k * 7) & 0xff) << 16) | (((k * 13) & 0xff) << 8) | ((255 - k * 5) & 0xff));
}

/** @brief Color of palette entry j. */
static Types::HighColor tga_palette_color(int j)
{
    return Types::HighColor::FromRGB24((((j * 8) & 0xff) << 16) | (((255 - j * 8) & 0xff) << 8) | ((0x40 + j) & 0xff));
}

/** @brief Loads a true color image and checks its size and every pixel; describes the first problem in buffer. */
static bool tga_check_true_color(const char *file, int (*key)(int, int))
{
    Bitmap::TGA image(file);
    const Bitmap::BitmapInfo info = image.GetInfo();

    if (image.GetData() == nullptr || info.Width != TgaWidth || info.Height != TgaHeight ||
        info.Palette != nullptr || info.ColorMode != CRAM::TextureColorMode::RGB555)
    {
        snprintf(buffer, buffer_size, "%s: %dx%d, palette %s, data %s (expected 8x4 RGB555)", file,
            info.Width, info.Height, info.Palette ? "set" : "none", image.GetData() ? "set" : "null");
        return false;
    }

    const Types::HighColor *pixels = reinterpret_cast<const Types::HighColor *>(image.GetData());

    for (int y = 0; y < TgaHeight; y++)
    {
        for (int x = 0; x < TgaWidth; x++)
        {
            const uint16_t actual = pixels[y * TgaWidth + x];
            const uint16_t expected = tga_true_color(key(x, y));

            if (actual != expected)
            {
                snprintf(buffer, buffer_size, "%s: pixel (%d,%d) is 0x%04x, expected 0x%04x", file, x, y, actual, expected);
                return false;
            }
        }
    }

    return true;
}

/** @brief Loads a paletted image and checks its size, palette and every pixel index; describes the first problem in buffer. */
static bool tga_check_paletted(const char *file, int (*key)(int, int), int colors)
{
    Bitmap::TGA image(file);
    const Bitmap::BitmapInfo info = image.GetInfo();
    const CRAM::TextureColorMode mode = colors == 16 ? CRAM::TextureColorMode::Paletted16 : CRAM::TextureColorMode::Paletted256;

    if (image.GetData() == nullptr || info.Width != TgaWidth || info.Height != TgaHeight ||
        info.Palette == nullptr || info.Palette->Count != static_cast<size_t>(colors) || info.ColorMode != mode)
    {
        snprintf(buffer, buffer_size, "%s: %dx%d, palette of %d colors, data %s (expected 8x4, %d colors)", file,
            info.Width, info.Height, info.Palette ? static_cast<int>(info.Palette->Count) : 0,
            image.GetData() ? "set" : "null", colors);
        return false;
    }

    for (int j = 0; j < colors; j++)
    {
        const uint16_t actual = info.Palette->Colors[j];
        const uint16_t expected = tga_palette_color(j);

        if (actual != expected)
        {
            snprintf(buffer, buffer_size, "%s: palette entry %d is 0x%04x, expected 0x%04x", file, j, actual, expected);
            return false;
        }
    }

    const uint8_t *data = image.GetData();

    for (int y = 0; y < TgaHeight; y++)
    {
        for (int x = 0; x < TgaWidth; x++)
        {
            const int i = y * TgaWidth + x;
            // 16 colors: two pixels per byte, the left one in the high nibble
            const int actual = colors == 16 ? ((x & 1) ? data[i >> 1] & 0x0f : data[i >> 1] >> 4) : data[i];
            const int expected = key(x, y) % colors;

            if (actual != expected)
            {
                snprintf(buffer, buffer_size, "%s: pixel (%d,%d) has index %d, expected %d", file, x, y, actual, expected);
                return false;
            }
        }
    }

    return true;
}

MU_TEST(tga_true_color_16bit_bottom_left) { mu_assert(tga_check_true_color("TC16BL.TGA", tga_plain_key), buffer); }
MU_TEST(tga_true_color_24bit_top_left) { mu_assert(tga_check_true_color("TC24TL.TGA", tga_plain_key), buffer); }
MU_TEST(tga_true_color_32bit_top_right) { mu_assert(tga_check_true_color("TC32TR.TGA", tga_plain_key), buffer); }
MU_TEST(tga_true_color_24bit_bottom_right) { mu_assert(tga_check_true_color("TC24BR.TGA", tga_plain_key), buffer); }
MU_TEST(tga_rle_true_color_24bit) { mu_assert(tga_check_true_color("TCRLE24.TGA", tga_rle_key), buffer); }
MU_TEST(tga_rle_true_color_32bit) { mu_assert(tga_check_true_color("TCRLE32.TGA", tga_rle_key), buffer); }
MU_TEST(tga_paletted_256) { mu_assert(tga_check_paletted("PAL256.TGA", tga_plain_key, 256), buffer); }
MU_TEST(tga_paletted_16) { mu_assert(tga_check_paletted("PAL16.TGA", tga_plain_key, 16), buffer); }
MU_TEST(tga_paletted_16_bottom_right) { mu_assert(tga_check_paletted("PAL16BR.TGA", tga_plain_key, 16), buffer); }
MU_TEST(tga_rle_paletted_256) { mu_assert(tga_check_paletted("PALRLE.TGA", tga_rle_key, 256), buffer); }
MU_TEST(tga_rle_paletted_16) { mu_assert(tga_check_paletted("PALRLE16.TGA", tga_rle_key, 16), buffer); }

/** @brief The palette entry chosen by TransparentColorIndex becomes transparent (0), the others keep their color. */
MU_TEST(tga_transparent_palette_index)
{
    Bitmap::TGA::LoaderSettings settings;
    settings.TransparentColorIndex = 3;
    Bitmap::TGA image("PAL16.TGA", settings);
    const Bitmap::BitmapInfo info = image.GetInfo();

    mu_assert(info.Palette != nullptr && info.Palette->Count == 16, "PAL16.TGA has no 16-color palette");
    snprintf(buffer, buffer_size, "Entry 3 is 0x%04x, expected 0 (transparent)", static_cast<uint16_t>(info.Palette->Colors[3]));
    mu_assert(static_cast<uint16_t>(info.Palette->Colors[3]) == 0, buffer);
    mu_assert(static_cast<uint16_t>(info.Palette->Colors[2]) == static_cast<uint16_t>(tga_palette_color(2)), "Entry 2 lost its color");
}

/** @brief Loading and destroying images gives all their memory back. */
MU_TEST(tga_no_leak)
{
    const size_t hw = Memory::HighWorkRam::GetReport().UsedBlocks;
    const size_t lw = Memory::LowWorkRam::GetReport().UsedBlocks;
    {
        Bitmap::TGA trueColor("TC24TL.TGA");
        Bitmap::TGA paletted("PAL256.TGA");
    }
    snprintf(buffer, buffer_size, "Used blocks after: HWRam %d -> %d, LWRam %d -> %d", hw,
        Memory::HighWorkRam::GetReport().UsedBlocks, lw, Memory::LowWorkRam::GetReport().UsedBlocks);
    mu_assert(Memory::HighWorkRam::GetReport().UsedBlocks == hw && Memory::LowWorkRam::GetReport().UsedBlocks == lw, buffer);
}

MU_TEST_SUITE(tga_test_suite)
{
    MU_RUN_TEST(tga_true_color_16bit_bottom_left);
    MU_RUN_TEST(tga_true_color_24bit_top_left);
    MU_RUN_TEST(tga_true_color_32bit_top_right);
    MU_RUN_TEST(tga_true_color_24bit_bottom_right);
    MU_RUN_TEST(tga_rle_true_color_24bit);
    MU_RUN_TEST(tga_rle_true_color_32bit);
    MU_RUN_TEST(tga_paletted_256);
    MU_RUN_TEST(tga_paletted_16);
    MU_RUN_TEST(tga_paletted_16_bottom_right);
    MU_RUN_TEST(tga_rle_paletted_256);
    MU_RUN_TEST(tga_transparent_palette_index);
    MU_RUN_TEST(tga_no_leak);
    MU_RUN_TEST(tga_rle_paletted_16);
}

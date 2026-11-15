// Tests/src/testsASCII.hpp
// SRL::ASCII (text on NBG3): what Print, SetPalette and SetFont write in the 64x64 text map, what SetColor writes in
// color RAM, and the font tiles LoadFont and LoadFontSG write in VRAM. MockBitmap comes from campaign.hpp.
#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_bitmap.hpp> // for IBitmap

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Logger;

extern "C" {

extern const uint8_t buffer_size;
extern char buffer[];
extern uint32_t suite_error_counter;

/** @brief Text map set up by VDP2::Initialize: 64x64 one-word cells */
static volatile uint16_t *const ascii_map = reinterpret_cast<volatile uint16_t *>(VDP2_VRAM_B1 + 0x1E000);

/** @brief Color RAM, 2048 colors of 16 bits (CRM16_2048) */
static volatile uint16_t *const ascii_cram = reinterpret_cast<volatile uint16_t *>(VDP2_COLRAM);

/** @brief Character cells start at VDP2_VRAM_B1 + 0x1D000, which is character number 640: font 0 (char c = 640 + c).
 *  Font n is 0x1000 bytes lower (128 characters of 32 bytes). */
static constexpr uint16_t ascii_font0_base = 640;

/** @brief Tile of a character number, 32 bytes (8x8 pixels, 4 bits each) */
static volatile uint8_t *ascii_tile(uint16_t character)
{
    return reinterpret_cast<volatile uint8_t *>(VDP2_VRAM_B1 + 0x1D000) + (static_cast<int32_t>(character) - ascii_font0_base) * 32;
}

/** @brief Font 1 area (characters 32-127), saved and restored by the tests that load a font into it */
static volatile uint8_t *const ascii_font1 = reinterpret_cast<volatile uint8_t *>(VDP2_VRAM_B1 + 0x1D400 - 0x1000);
static uint8_t ascii_font1_saved[0xC00];

static void ascii_save_font1() { for (int i = 0; i < 0xC00; i++) ascii_font1_saved[i] = ascii_font1[i]; }
static void ascii_restore_font1() { for (int i = 0; i < 0xC00; i++) ascii_font1[i] = ascii_font1_saved[i]; }

void ascii_test_setup(void)
{
    ASCII::Clear();
}

void ascii_test_teardown(void)
{
    ASCII::Clear();
    ASCII::SetPalette(0);
    ASCII::SetFont(0);
}

    // UT output header function, called on the first test failure
void ascii_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_ASCII****");
        }
        else
        {
            LogInfo("****UT_ASCII_ERROR(S)****");
        }
    }
}

/** @brief Print writes one map cell per character from (x, y), with font 0 and palette 0, and nothing around them. */
MU_TEST(ascii_print_writes_map)
{
    mu_assert(ASCII::Print("AB", 3, 2), "Print(\"AB\", 3, 2) returned false");

    const int cell = 3 + 2 * 64;
    snprintf(buffer, buffer_size, "Cells around (3, 2): 0x%04x [0x%04x 0x%04x] 0x%04x, expected 0 [0x%04x 0x%04x] 0",
        ascii_map[cell - 1], ascii_map[cell], ascii_map[cell + 1], ascii_map[cell + 2], 'A' + ascii_font0_base, 'B' + ascii_font0_base);
    mu_assert(ascii_map[cell] == 'A' + ascii_font0_base && ascii_map[cell + 1] == 'B' + ascii_font0_base, buffer);
    mu_assert(ascii_map[cell - 1] == 0 && ascii_map[cell + 2] == 0, buffer);

    mu_assert(ASCII::Print("", 0, 0) && ascii_map[0] == 0, "Print(\"\") wrote a cell");
}

/** @brief Clear empties the whole map. */
MU_TEST(ascii_clear_empties_map)
{
    ASCII::Print("A", 0, 0);
    ASCII::Print("Z", 63, 63);
    mu_assert(ASCII::Clear(), "Clear returned false");

    for (int cell = 0; cell < 64 * 64; cell++)
    {
        snprintf(buffer, buffer_size, "Cell %d is 0x%04x after Clear", cell, ascii_map[cell]);
        mu_assert(ascii_map[cell] == 0, buffer);
    }
}

/** @brief Out of the map, Print returns false and writes at the clamped position (63, 63). */
MU_TEST(ascii_print_out_of_bounds_clamps)
{
    mu_assert(!ASCII::Print("X", 127, 89), "Print at (127, 89) returned true");

    snprintf(buffer, buffer_size, "Cell (63, 63) is 0x%04x, expected 0x%04x", ascii_map[63 + 63 * 64], 'X' + ascii_font0_base);
    mu_assert(ascii_map[63 + 63 * 64] == 'X' + ascii_font0_base, buffer);
}

/** @brief The palette goes in bits 12-15 of the cell; out of range it is clamped to palette 7. */
MU_TEST(ascii_palette_in_map)
{
    mu_assert(ASCII::SetPalette(2), "SetPalette(2) returned false");
    ASCII::Print("A", 0, 0);
    snprintf(buffer, buffer_size, "Palette 2 cell is 0x%04x, expected 0x%04x", ascii_map[0], ('A' + ascii_font0_base) | (2 << 12));
    mu_assert(ascii_map[0] == (('A' + ascii_font0_base) | (2 << 12)), buffer);

    mu_assert(!ASCII::SetPalette(255), "SetPalette(255) returned true");
    ASCII::Print("A", 0, 0);
    snprintf(buffer, buffer_size, "SetPalette(255) cell is 0x%04x, expected palette 7 (0x%04x)", ascii_map[0], ('A' + ascii_font0_base) | (7 << 12));
    mu_assert(ascii_map[0] == (('A' + ascii_font0_base) | (7 << 12)), buffer);
}

/** @brief Font n uses characters 128 * (5 - n) + c; out of range it is clamped to font 5. */
MU_TEST(ascii_font_in_map)
{
    mu_assert(ASCII::SetFont(1), "SetFont(1) returned false");
    ASCII::Print("A", 0, 0);
    snprintf(buffer, buffer_size, "Font 1 cell is 0x%04x, expected 0x%04x", ascii_map[0], 'A' + 512);
    mu_assert(ascii_map[0] == 'A' + 512, buffer);

    mu_assert(!ASCII::SetFont(255), "SetFont(255) returned true");
    ASCII::Print("A", 0, 0);
    snprintf(buffer, buffer_size, "SetFont(255) cell is 0x%04x, expected font 5 (0x%04x)", ascii_map[0], 'A');
    mu_assert(ascii_map[0] == 'A', buffer);
}

/** @brief With palette 0, SetColor writes color RAM entry 0-15; an index over 15 is clamped to 15. */
MU_TEST(ascii_set_color_palette_0)
{
    const uint16_t saved3 = ascii_cram[3];
    const uint16_t saved15 = ascii_cram[15];

    const bool inRange = ASCII::SetColor(0x7C1F, 3);
    const uint16_t color3 = ascii_cram[3];
    const bool outOfRange = ASCII::SetColor(0x03E0, 255);
    const uint16_t color15 = ascii_cram[15];

    ascii_cram[3] = saved3;
    ascii_cram[15] = saved15;

    snprintf(buffer, buffer_size, "Color 3 is 0x%04x (SetColor returned %d), expected 0x7c1f", color3, inRange);
    mu_assert(inRange && color3 == 0x7C1F, buffer);
    snprintf(buffer, buffer_size, "Color 15 is 0x%04x (SetColor(255) returned %d), expected 0x03e0 and false", color15, outOfRange);
    mu_assert(!outOfRange && color15 == 0x03E0, buffer);
}

/** @brief With palette 1, SetColor(c, 1) writes color RAM entry 17, the one the text uses (palette 1 = colors 16-31). */
MU_TEST(ascii_set_color_palette_1)
{
    const uint16_t saved17 = ascii_cram[17];
    const uint16_t saved33 = ascii_cram[33];

    ASCII::SetPalette(1);
    ASCII::SetColor(0x7C1F, 1);
    const uint16_t color17 = ascii_cram[17];
    const uint16_t color33 = ascii_cram[33];

    ascii_cram[17] = saved17;
    ascii_cram[33] = saved33;

    snprintf(buffer, buffer_size, "Color 17 is 0x%04x, expected 0x7c1f (color 33, palette 2, is 0x%04x)", color17, color33);
    mu_assert(color17 == 0x7C1F, buffer);
}

/** @brief LoadFontSG packs two 8-bit pixels into one byte, and a character printed with that font points at its tile. */
MU_TEST(ascii_load_font_sg)
{
    static uint8_t source[0x1800];
    for (int i = 0; i < 0x1800; i++) source[i] = (i * 7) & 0x0F;

    ascii_save_font1();
    ASCII::LoadFontSG(source, 1);

    int wrong = -1;
    for (int i = 0; i < 0xC00 && wrong < 0; i++)
    {
        if (ascii_font1[i] != ((source[2 * i] << 4) | source[2 * i + 1])) wrong = i;
    }

    // 'A' is the 34th character of the font (it starts at the empty tile of ' ')
    ASCII::SetFont(1);
    ASCII::Print("A", 0, 0);
    const bool tileMatches = ascii_tile(ascii_map[0] & 0x3FF) == ascii_font1 + ('A' - ' ') * 32;

    ascii_restore_font1();

    snprintf(buffer, buffer_size, "Font 1 byte %d is not the two packed source pixels", wrong);
    mu_assert(wrong < 0, buffer);
    snprintf(buffer, buffer_size, "Cell 0x%04x does not point at the 'A' tile of font 1", ascii_map[0]);
    mu_assert(tileMatches, buffer);
}

/** @brief LoadFont cuts a 4-bit bitmap into 8x8 tiles, left to right: a 16x8 bitmap gives the tiles of ' ' and '!'. */
MU_TEST(ascii_load_font_bitmap)
{
    // 16x8 pixels, 4 bits each: 8 bytes per row
    static uint8_t pixels[8 * 8];
    for (int i = 0; i < 64; i++) pixels[i] = static_cast<uint8_t>(i + 1);
    MockBitmap bitmap(pixels, Bitmap::BitmapInfo(16, 8));

    ascii_save_font1();
    ASCII::LoadFont(&bitmap, 1);

    int wrongTile = -1;
    int wrongRow = -1;
    for (int tile = 0; tile < 2 && wrongTile < 0; tile++)
    {
        for (int row = 0; row < 8 && wrongTile < 0; row++)
        {
            for (int b = 0; b < 4; b++)
            {
                if (ascii_font1[tile * 32 + row * 4 + b] != pixels[row * 8 + tile * 4 + b])
                {
                    wrongTile = tile;
                    wrongRow = row;
                }
            }
        }
    }

    const bool nextUntouched = ascii_font1[64] == ascii_font1_saved[64];
    ascii_restore_font1();

    snprintf(buffer, buffer_size, "Tile %d row %d is not the bitmap's pixels", wrongTile, wrongRow);
    mu_assert(wrongTile < 0, buffer);
    mu_assert(nextUntouched, "LoadFont wrote past the 2 tiles of the bitmap");
}

MU_TEST_SUITE(ascii_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&ascii_test_setup,
        &ascii_test_teardown,
        &ascii_test_output_header);

    MU_RUN_TEST(ascii_print_writes_map);
    MU_RUN_TEST(ascii_clear_empties_map);
    MU_RUN_TEST(ascii_print_out_of_bounds_clamps);
    MU_RUN_TEST(ascii_palette_in_map);
    MU_RUN_TEST(ascii_font_in_map);
    MU_RUN_TEST(ascii_set_color_palette_0);
    MU_RUN_TEST(ascii_set_color_palette_1);
    MU_RUN_TEST(ascii_load_font_sg);
    MU_RUN_TEST(ascii_load_font_bitmap);
}
}

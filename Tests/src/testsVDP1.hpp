// Tests/src/testsVDP1.hpp
// SRL::VDP1 texture heap: where textures go in VDP1 VRAM, loading pixels, the slot and memory limits, free memory,
// resetting the heap, and loading paletted bitmaps. The campaign sets SRL_MAX_TEXTURES to 8.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_vdp1.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void vdp1_test_setup(void) { VDP1::ResetTextureHeap(); }
void vdp1_test_teardown(void) { VDP1::ResetTextureHeap(); }

/** @brief VRAM byte address of texture id. */
static uint32_t vdp1_texture_start(int32_t id) { return SpriteVRAM + (VDP1::Textures[id].Address << 3); }

/** @brief Size in bytes of a width x height texture: 2 bytes per pixel in RGB, 1 with 64 to 256 colors, half with 16. */
static uint32_t vdp1_texture_bytes(uint16_t width, uint16_t height, CRAM::TextureColorMode mode)
{
    const uint32_t pixels = width * height;
    switch (mode)
    {
    case CRAM::TextureColorMode::RGB555: return pixels * 2;
    case CRAM::TextureColorMode::Paletted16: return pixels / 2;
    default: return pixels;
    }
}

/** @brief The first texture starts where SGL's sprite command table ends, with the given size and metadata. */
MU_TEST(vdp1_first_texture_at_start)
{
    const int32_t id = VDP1::TryAllocateTexture(16, 8, CRAM::TextureColorMode::Paletted16, 5);

    mu_assert(id == 0 && VDP1::GetTextureCount() == 1, "The first texture did not get id 0");
    snprintf(buffer, buffer_size, "First texture at VRAM offset 0x%x, expected CGADDRESS 0x%x",
        static_cast<unsigned>(VDP1::Textures[0].Address << 3), static_cast<unsigned>(CGADDRESS));
    mu_assert(static_cast<uint32_t>(VDP1::Textures[0].Address << 3) == CGADDRESS, buffer);
    mu_assert(VDP1::Textures[0].Width == 16 && VDP1::Textures[0].Height == 8, "Width or height not kept");
    snprintf(buffer, buffer_size, "Size word is 0x%04x, expected 0x%04x", VDP1::Textures[0].Size, ((16 & 0x1f8) << 5) | 8);
    mu_assert(VDP1::Textures[0].Size == (((16 & 0x1f8) << 5) | 8), buffer);
    mu_assert(VDP1::Metadata[0].ColorMode == CRAM::TextureColorMode::Paletted16 && VDP1::Metadata[0].PaletteId == 5, "Metadata not kept");
}

/** @brief Textures of different color modes follow each other without overlapping, on 32-byte boundaries. */
MU_TEST(vdp1_textures_do_not_overlap)
{
    struct { uint16_t w, h; CRAM::TextureColorMode mode; } textures[] = {
        { 16, 8, CRAM::TextureColorMode::RGB555 },
        { 8, 8, CRAM::TextureColorMode::Paletted256 },
        { 16, 16, CRAM::TextureColorMode::Paletted16 },
        { 24, 8, CRAM::TextureColorMode::Paletted64 },
        { 8, 8, CRAM::TextureColorMode::RGB555 },
    };

    for (int i = 0; i < 5; i++)
    {
        const int32_t id = VDP1::TryAllocateTexture(textures[i].w, textures[i].h, textures[i].mode, 0);
        snprintf(buffer, buffer_size, "Texture %d got id %d", i, id);
        mu_assert(id == i, buffer);

        snprintf(buffer, buffer_size, "Texture %d starts at 0x%x, not on a 32-byte boundary", i, static_cast<unsigned>(vdp1_texture_start(id)));
        mu_assert((vdp1_texture_start(id) & 0x1f) == 0, buffer);

        if (i > 0)
        {
            const uint32_t previousEnd = vdp1_texture_start(i - 1) + vdp1_texture_bytes(textures[i - 1].w, textures[i - 1].h, textures[i - 1].mode);
            snprintf(buffer, buffer_size, "Texture %d starts at 0x%x, before the end of texture %d (0x%x)", i,
                static_cast<unsigned>(vdp1_texture_start(id)), i - 1, static_cast<unsigned>(previousEnd));
            mu_assert(vdp1_texture_start(id) >= previousEnd, buffer);
        }
    }
}

/** @brief TryLoadTexture copies the pixels into VDP1 VRAM. */
MU_TEST(vdp1_load_copies_pixels)
{
    static uint16_t pixels[64];
    for (int i = 0; i < 64; i++) pixels[i] = static_cast<uint16_t>(0x8000 | (i * 517));

    const int32_t id = VDP1::TryLoadTexture(8, 8, CRAM::TextureColorMode::RGB555, 0, pixels);
    mu_assert(id == 0, "TryLoadTexture failed");

    const volatile uint16_t *vram = static_cast<const volatile uint16_t *>(VDP1::Textures[id].GetData());
    for (int i = 0; i < 64; i++)
    {
        snprintf(buffer, buffer_size, "VRAM pixel %d is 0x%04x, expected 0x%04x", i, vram[i], pixels[i]);
        mu_assert(vram[i] == pixels[i], buffer);
    }
}

/** @brief Once all SRL_MAX_TEXTURES slots are used, allocation fails and the count stays the same. */
MU_TEST(vdp1_heap_full_by_count)
{
    for (int i = 0; i < SRL_MAX_TEXTURES; i++)
    {
        snprintf(buffer, buffer_size, "Texture %d of %d failed", i, SRL_MAX_TEXTURES);
        mu_assert(VDP1::TryAllocateTexture(8, 8, CRAM::TextureColorMode::RGB555, 0) == i, buffer);
    }

    mu_assert(VDP1::TryAllocateTexture(8, 8, CRAM::TextureColorMode::RGB555, 0) == -1, "Allocation past SRL_MAX_TEXTURES did not fail");
    mu_assert(VDP1::GetTextureCount() == SRL_MAX_TEXTURES, "The failed allocation changed the texture count");
}

/** @brief A texture that does not fit in the VRAM left fails, and the count stays the same. */
MU_TEST(vdp1_heap_full_by_memory)
{
    // 504x255 RGB is 257040 bytes: one fits in VDP1 VRAM, two do not
    mu_assert(VDP1::TryAllocateTexture(504, 255, CRAM::TextureColorMode::RGB555, 0) == 0, "The first big texture failed");
    mu_assert(VDP1::TryAllocateTexture(504, 255, CRAM::TextureColorMode::RGB555, 0) == -1, "A second big texture did not fail");
    mu_assert(VDP1::GetTextureCount() == 1, "The failed allocation changed the texture count");
}

/** @brief GetAvailableMemory is the VRAM left after the last texture, also when there is no texture yet. */
MU_TEST(vdp1_available_memory)
{
    const uint32_t empty = VDP1::GetAvailableMemory();
    const uint32_t expectedEmpty = VDP1::UserAreaEnd - (SpriteVRAM + CGADDRESS);

    VDP1::TryAllocateTexture(16, 8, CRAM::TextureColorMode::RGB555, 0);
    const uint32_t afterOne = VDP1::GetAvailableMemory();

    snprintf(buffer, buffer_size, "After a 256-byte texture: %u bytes left, expected %u",
        static_cast<unsigned>(afterOne), static_cast<unsigned>(expectedEmpty - 256));
    mu_assert(afterOne == expectedEmpty - 256, buffer);
    snprintf(buffer, buffer_size, "With no texture: %u bytes left, expected %u", static_cast<unsigned>(empty), static_cast<unsigned>(expectedEmpty));
    mu_assert(empty == expectedEmpty, buffer);
}

/** @brief ResetTextureHeap(index) frees the textures from index on; the next texture reuses that slot and address. */
MU_TEST(vdp1_reset_to_index)
{
    VDP1::TryAllocateTexture(8, 8, CRAM::TextureColorMode::RGB555, 0);
    VDP1::TryAllocateTexture(16, 16, CRAM::TextureColorMode::RGB555, 0);
    VDP1::TryAllocateTexture(8, 8, CRAM::TextureColorMode::RGB555, 0);
    const uint16_t secondAddress = VDP1::Textures[1].Address;

    VDP1::ResetTextureHeap(1);
    mu_assert(VDP1::GetTextureCount() == 1, "ResetTextureHeap(1) did not leave 1 texture");
    mu_assert(VDP1::TryAllocateTexture(32, 8, CRAM::TextureColorMode::RGB555, 0) == 1, "The next texture did not get id 1");
    mu_assert(VDP1::Textures[1].Address == secondAddress, "The next texture did not reuse the freed address");

    VDP1::ResetTextureHeap(5);
    mu_assert(VDP1::GetTextureCount() == 2, "ResetTextureHeap past the count changed the count");
}

static int16_t vdp1_palette_seven(Bitmap::BitmapInfo *) { return 7; }
static int16_t vdp1_palette_refused(Bitmap::BitmapInfo *) { return -1; }

/** @brief A paletted bitmap needs a palette: from the handler, or given directly. Without one, or if refused, loading fails. */
MU_TEST(vdp1_load_paletted_bitmap)
{
    static uint8_t pixels[8 * 8 / 2];
    Bitmap::Palette palette(16);
    MockBitmap bitmap(pixels, Bitmap::BitmapInfo(8, 8, &palette));

    mu_assert(VDP1::TryLoadTexture(&bitmap) == -1, "A paletted bitmap loaded without a palette handler");
    mu_assert(VDP1::TryLoadTexture(&bitmap, vdp1_palette_refused) == -1, "Loading did not fail when the handler refused");
    mu_assert(VDP1::GetTextureCount() == 0, "A failed load used a texture slot");

    const int32_t fromHandler = VDP1::TryLoadTexture(&bitmap, vdp1_palette_seven);
    mu_assert(fromHandler == 0, "Loading with a palette handler failed");
    mu_assert(VDP1::Metadata[0].ColorMode == CRAM::TextureColorMode::Paletted16 && VDP1::Metadata[0].PaletteId == 7,
        "The texture did not get 16 colors and palette 7 from the handler");

    const int32_t direct = VDP1::TryLoadTexture(&bitmap, static_cast<int16_t>(3));
    mu_assert(direct == 1 && VDP1::Metadata[1].PaletteId == 3, "Loading with palette 3 given directly failed");
}

MU_TEST_SUITE(vdp1_test_suite)
{
    MU_SUITE_CONFIGURE(&vdp1_test_setup, &vdp1_test_teardown);

    MU_RUN_TEST(vdp1_first_texture_at_start);
    MU_RUN_TEST(vdp1_textures_do_not_overlap);
    MU_RUN_TEST(vdp1_load_copies_pixels);
    MU_RUN_TEST(vdp1_heap_full_by_count);
    MU_RUN_TEST(vdp1_heap_full_by_memory);
    MU_RUN_TEST(vdp1_available_memory);
    MU_RUN_TEST(vdp1_reset_to_index);
    MU_RUN_TEST(vdp1_load_paletted_bitmap);
}

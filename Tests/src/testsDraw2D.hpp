// Tests/src/testsDraw2D.hpp
// SRL::Scene2D drawing: sprite effects, the sprite attribute and command words they produce, and the VDP1 commands
// that lines, polygons, the clipping rectangle and sprites (scaled and rotated) put in VDP1 VRAM.
// SGL copies a frame's commands into VDP1 VRAM at Core::Synchronize, after its 3 system commands; the tests read them
// back from there, up to the end command.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_scene2d.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Math::Types::Angle;
using SRL::Math::Types::Fxp;
using SRL::Math::Types::Vector2D;
using SRL::Math::Types::Vector3D;
using SpriteEffect = SRL::Scene2D::SpriteEffect;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Every effect, for resetting them. */
static const SpriteEffect draw2d_effects[] = { SpriteEffect::Gouraud, SpriteEffect::ScreenDoors, SpriteEffect::HalfTransparency,
    SpriteEffect::Clipping, SpriteEffect::Flip, SpriteEffect::OpacityBank, SpriteEffect::EnableHSS, SpriteEffect::EnableECD,
    SpriteEffect::DisablePreClip, SpriteEffect::HalfBrightness };

static void draw2d_reset_effects()
{
    for (const SpriteEffect effect : draw2d_effects) Scene2D::SetEffect(effect, -1);
}

/** @brief The 16x8 RGB test texture, loaded by setup. */
static int32_t draw2d_texture = -1;

void draw2d_test_setup(void)
{
    static uint16_t pixels[16 * 8];
    for (int i = 0; i < 16 * 8; i++) pixels[i] = static_cast<uint16_t>(0x8000 | i);

    draw2d_reset_effects();
    VDP1::ResetTextureHeap();
    draw2d_texture = VDP1::TryLoadTexture(16, 8, CRAM::TextureColorMode::RGB555, 0, pixels);
}

void draw2d_test_teardown(void)
{
    draw2d_reset_effects();
    Core::Synchronize();    // sends anything left to VDP1, so the next test starts on an empty frame
    VDP1::ResetTextureHeap();
}

/** @brief Depth used for every drawing: in front of the camera, as in the samples. */
static const Fxp draw2d_depth = 500.0;

/** @brief Copy of one VDP1 command. */
struct Draw2DCommand
{
    uint16_t Ctrl;
    uint16_t Pmod;
    uint16_t Colr;
    uint16_t Srca;
    uint16_t Size;
    int16_t X[4];
    int16_t Y[4];
    uint16_t Grda;

    uint16_t Type() const { return Ctrl & 0xf; }
    uint16_t ZoomPoint() const { return (Ctrl >> 8) & 0xf; }
};

static Draw2DCommand draw2d_frame[32];

/** @brief Sends this frame to VDP1 and copies its commands from VDP1 VRAM (system commands included). */
static int draw2d_read_frame()
{
    Core::Synchronize();

    const volatile uint16_t *vram = reinterpret_cast<const volatile uint16_t *>(SpriteVRAM);
    int count = 0;
    for (uint32_t offset = 0; offset < CGADDRESS && count < 32; offset += 32)
    {
        const volatile uint16_t *words = vram + offset / 2;
        if (words[0] & 0x8000) break;   // end command

        Draw2DCommand &command = draw2d_frame[count++];
        command.Ctrl = words[0];
        command.Pmod = words[2];
        command.Colr = words[3];
        command.Srca = words[4];
        command.Size = words[5];
        for (int i = 0; i < 4; i++)
        {
            command.X[i] = static_cast<int16_t>(words[6 + i * 2]);
            command.Y[i] = static_cast<int16_t>(words[7 + i * 2]);
        }
        command.Grda = words[14];
    }

    return count;
}

/** @brief First command of the frame with this type and point A, or nullptr. */
static const Draw2DCommand *draw2d_find(int count, uint16_t type, int16_t x, int16_t y)
{
    for (int i = 0; i < count; i++)
    {
        if (draw2d_frame[i].Type() == type && draw2d_frame[i].X[0] == x && draw2d_frame[i].Y[0] == y) return &draw2d_frame[i];
    }

    return nullptr;
}

/** @brief First command of the frame drawing the test texture, or nullptr. */
static const Draw2DCommand *draw2d_find_sprite(int count)
{
    for (int i = 0; i < count; i++)
    {
        if (draw2d_frame[i].Srca == VDP1::Textures[draw2d_texture].Address && draw2d_frame[i].Type() <= 2) return &draw2d_frame[i];
    }

    return nullptr;
}

/** @brief Each effect keeps the value it is set to, within its bit width; -1 turns it off. */
MU_TEST(draw2d_effects_round_trip)
{
    struct { SpriteEffect effect; int32_t set; int32_t expected; } cases[] = {
        { SpriteEffect::Gouraud, 12, 12 },
        { SpriteEffect::ScreenDoors, 1, 1 },
        { SpriteEffect::HalfTransparency, 1, 1 },
        { SpriteEffect::Clipping, Scene2D::ClipInside, Scene2D::ClipInside },
        { SpriteEffect::Clipping, 7, 3 },
        { SpriteEffect::Flip, Scene2D::HorizontalFlip | Scene2D::VerticalFlip, 3 },
        { SpriteEffect::OpacityBank, 9, 1 },
        { SpriteEffect::EnableHSS, 1, 1 },
        { SpriteEffect::EnableECD, 1, 1 },
        { SpriteEffect::DisablePreClip, 1, 1 },
        { SpriteEffect::HalfBrightness, 1, 1 },
        { SpriteEffect::HalfBrightness, 2, 0 },
    };

    for (const auto &c : cases)
    {
        Scene2D::SetEffect(c.effect, c.set);
        const int32_t got = Scene2D::GetEffect(c.effect);
        Scene2D::SetEffect(c.effect, -1);
        const int32_t off = Scene2D::GetEffect(c.effect);

        snprintf(buffer, buffer_size, "Effect %d set to %d reads %d (expected %d), then %d after -1 (expected 0)",
            static_cast<int>(c.effect), static_cast<int>(c.set), static_cast<int>(got), static_cast<int>(c.expected), static_cast<int>(off));
        mu_assert(got == c.expected && off == 0, buffer);
    }
}

/** @brief With no effect, an RGB sprite attribute has end codes off, pre-clipping on, RGB colors and no gouraud. */
MU_TEST(draw2d_sprite_attribute_defaults)
{
    CRAM::Palette rgb;
    const SPR_ATTR attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb, Scene2D::ZoomPoint::UpperLeft);

    snprintf(buffer, buffer_size, "texno %d atrb 0x%04x colno 0x%04x gstb 0x%04x dir 0x%04x, expected %d 0x%04x 0 0 0x%04x",
        attr.texno, attr.atrb, attr.colno, attr.gstb, attr.dir, static_cast<int>(draw2d_texture), ECdis | Pclpon | HSSoff | CL32KRGB,
        FUNC_Texture | (Scene2D::ZoomPoint::UpperLeft << 8));
    mu_assert(attr.texno == draw2d_texture && attr.atrb == (ECdis | Pclpon | HSSoff | CL32KRGB) && attr.colno == 0 &&
        attr.gstb == 0 && attr.dir == (FUNC_Texture | (Scene2D::ZoomPoint::UpperLeft << 8)), buffer);
}

/** @brief Each effect sets its own bits in the sprite attribute. */
MU_TEST(draw2d_sprite_attribute_effects)
{
    CRAM::Palette rgb;
    CRAM::Palette palette16(CRAM::TextureColorMode::Paletted16, 3);
    SPR_ATTR attr;

    Scene2D::SetEffect(SpriteEffect::Flip, Scene2D::HorizontalFlip);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    mu_assert((attr.dir & 0x30) == 0x10, "Horizontal flip did not set direction bit 4");
    Scene2D::SetEffect(SpriteEffect::Flip, Scene2D::VerticalFlip);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    mu_assert((attr.dir & 0x30) == 0x20, "Vertical flip did not set direction bit 5");
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::EnableHSS, 1);
    Scene2D::SetEffect(SpriteEffect::EnableECD, 1);
    Scene2D::SetEffect(SpriteEffect::DisablePreClip, 1);
    Scene2D::SetEffect(SpriteEffect::ScreenDoors, 1);
    Scene2D::SetEffect(SpriteEffect::Clipping, Scene2D::ClipOutside);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    snprintf(buffer, buffer_size, "atrb 0x%04x: expected high-speed shrink, end codes on, no pre-clipping, mesh and window-in bits", attr.atrb);
    mu_assert((attr.atrb & HSSon) && !(attr.atrb & ECdis) && (attr.atrb & Pclpoff) && (attr.atrb & MESHon) &&
        (attr.atrb & (3 << 9)) == Window_In, buffer);
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::HalfTransparency, 1);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    mu_assert((attr.atrb & 7) == CL_Trans, "Half transparency did not set color calculation 3");
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::HalfBrightness, 1);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    mu_assert((attr.atrb & 7) == CL_Half, "Half brightness did not set color calculation 2");
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::Gouraud, 3);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &rgb);
    snprintf(buffer, buffer_size, "Gouraud 3: atrb 0x%04x gstb 0x%04x, expected the gouraud bit and table 0xe003", attr.atrb, attr.gstb);
    mu_assert((attr.atrb & 7) == CL_Gouraud && attr.gstb == 0xe003, buffer);
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::OpacityBank, 2);
    attr = Scene2D::GetSpriteAttribute(draw2d_texture, &palette16);
    snprintf(buffer, buffer_size, "16-color palette 3, opacity bank 2: colno 0x%04x, expected 0x%04x", attr.colno, (3 << 4) | (2 << 11));
    mu_assert(attr.colno == ((3 << 4) | (2 << 11)), buffer);
}

/** @brief The command word for lines and polygons: type, color, end codes off, and the effect bits. */
MU_TEST(draw2d_command_effects)
{
    const Types::HighColor color(0x8123);
    SPRITE command = Scene2D::GetSpriteCommand(Scene2D::CommandType::LineSegment, color);
    snprintf(buffer, buffer_size, "No effect: CTRL 0x%04x PMOD 0x%04x COLR 0x%04x GRDA 0x%04x, expected 6, 0x0080, 0x8123, 0",
        command.CTRL, command.PMOD, command.COLR, command.GRDA);
    mu_assert(command.CTRL == 6 && command.PMOD == 0x0080 && command.COLR == 0x8123 && command.GRDA == 0, buffer);

    Scene2D::SetEffect(SpriteEffect::HalfTransparency, 1);
    Scene2D::SetEffect(SpriteEffect::ScreenDoors, 1);
    Scene2D::SetEffect(SpriteEffect::Clipping, Scene2D::ClipInside);
    command = Scene2D::GetSpriteCommand(Scene2D::CommandType::Polygon, color);
    snprintf(buffer, buffer_size, "PMOD 0x%04x: expected half transparency (3), mesh (bit 8) and window-out (bits 9-10)", command.PMOD);
    mu_assert((command.PMOD & 7) == CL_Trans && (command.PMOD & MESHon) && (command.PMOD & (3 << 9)) == Window_Out, buffer);
    draw2d_reset_effects();

    Scene2D::SetEffect(SpriteEffect::Gouraud, 5);
    command = Scene2D::GetSpriteCommand(Scene2D::CommandType::Polygon, color);
    snprintf(buffer, buffer_size, "Gouraud 5: PMOD 0x%04x GRDA 0x%04x, expected the gouraud bit and table 0xe005", command.PMOD, command.GRDA);
    mu_assert((command.PMOD & 7) == CL_Gouraud && command.GRDA == 0xe005, buffer);
}

/** @brief DrawLine puts a line command with its two points and color in VDP1 VRAM. */
MU_TEST(draw2d_line_reaches_vdp1)
{
    mu_assert(Scene2D::DrawLine(Vector2D(37, 41), Vector2D(73, 59), Types::HighColor(0x8123), draw2d_depth), "DrawLine failed");
    const int count = draw2d_read_frame();
    const Draw2DCommand *line = draw2d_find(count, 6, 37, 41);

    mu_assert(line != nullptr, "No line command from (37, 41) in VDP1 VRAM");
    snprintf(buffer, buffer_size, "Line to (%d, %d) color 0x%04x, expected (73, 59) 0x8123", line->X[1], line->Y[1], line->Colr);
    mu_assert(line->X[1] == 73 && line->Y[1] == 59 && line->Colr == 0x8123, buffer);
}

/** @brief DrawPolygon puts a filled polygon or an outline (polyline) with its 4 points in VDP1 VRAM. */
MU_TEST(draw2d_polygon_fill_and_outline)
{
    Vector2D filled[4] = { Vector2D(1, 2), Vector2D(30, 3), Vector2D(31, 33), Vector2D(4, 34) };
    Vector2D outline[4] = { Vector2D(-50, -40), Vector2D(-20, -40), Vector2D(-20, -10), Vector2D(-50, -10) };
    mu_assert(Scene2D::DrawPolygon(filled, true, Types::HighColor(0x8456), draw2d_depth), "DrawPolygon (filled) failed");
    mu_assert(Scene2D::DrawPolygon(outline, false, Types::HighColor(0x8789), draw2d_depth), "DrawPolygon (outline) failed");
    const int count = draw2d_read_frame();

    const Draw2DCommand *polygon = draw2d_find(count, 4, 1, 2);
    mu_assert(polygon != nullptr, "No polygon command from (1, 2) in VDP1 VRAM");
    snprintf(buffer, buffer_size, "Polygon B (%d, %d) C (%d, %d) D (%d, %d) color 0x%04x, expected (30, 3) (31, 33) (4, 34) 0x8456",
        polygon->X[1], polygon->Y[1], polygon->X[2], polygon->Y[2], polygon->X[3], polygon->Y[3], polygon->Colr);
    mu_assert(polygon->X[1] == 30 && polygon->Y[1] == 3 && polygon->X[2] == 31 && polygon->Y[2] == 33 && polygon->X[3] == 4 &&
        polygon->Y[3] == 34 && polygon->Colr == 0x8456, buffer);

    const Draw2DCommand *polyline = draw2d_find(count, 5, -50, -40);
    mu_assert(polyline != nullptr, "No polyline command from (-50, -40) in VDP1 VRAM");
    mu_assert(polyline->X[2] == -20 && polyline->Y[2] == -10 && polyline->Colr == 0x8789, "The polyline points or color are wrong");
}

/** @brief SetClippingRectangle puts a user clipping command from the corner to corner + size. */
MU_TEST(draw2d_clipping_rectangle)
{
    mu_assert(Scene2D::SetClippingRectangle(Vector3D(30, 20, draw2d_depth), Vector2D(50, 60)), "SetClippingRectangle failed");
    const int count = draw2d_read_frame();
    const Draw2DCommand *clip = draw2d_find(count, 8, 30, 20);

    mu_assert(clip != nullptr, "No user clipping command from (30, 20) in VDP1 VRAM");
    snprintf(buffer, buffer_size, "Clipping rectangle ends at (%d, %d), expected (80, 80)", clip->X[2], clip->Y[2]);
    mu_assert(clip->X[2] == 80 && clip->Y[2] == 80, buffer);
}

/** @brief An unrotated sprite is a scaled sprite at its zoom point; VDP1 sizes count from 0, so 16x8 pixels is 15x7. */
MU_TEST(draw2d_sprite_scaled)
{
    mu_assert(draw2d_texture >= 0, "The test texture did not load");
    mu_assert(Scene2D::DrawSprite(draw2d_texture, Vector3D(20, 10, draw2d_depth), Vector2D(1.0, 1.0), Scene2D::ZoomPoint::UpperLeft), "DrawSprite failed");
    int count = draw2d_read_frame();
    const Draw2DCommand *sprite = draw2d_find_sprite(count);

    mu_assert(sprite != nullptr, "No command with the test texture in VDP1 VRAM");
    snprintf(buffer, buffer_size, "Type %d zoom 0x%x at (%d, %d) size (%d, %d) texture size 0x%04x, expected 1 0x5 (20, 10) (15, 7) 0x%04x",
        sprite->Type(), sprite->ZoomPoint(), sprite->X[0], sprite->Y[0], sprite->X[1], sprite->Y[1], sprite->Size, VDP1::Textures[draw2d_texture].Size);
    mu_assert(sprite->Type() == 1 && sprite->ZoomPoint() == 0x5 && sprite->X[0] == 20 && sprite->Y[0] == 10 && sprite->X[1] == 15 &&
        sprite->Y[1] == 7 && sprite->Size == VDP1::Textures[draw2d_texture].Size, buffer);

    mu_assert(Scene2D::DrawSprite(draw2d_texture, Vector3D(-40, -30, draw2d_depth), Vector2D(2.0, 1.0)), "DrawSprite at scale 2 failed");
    count = draw2d_read_frame();
    sprite = draw2d_find_sprite(count);
    mu_assert(sprite != nullptr, "No command with the test texture after the scaled draw");
    snprintf(buffer, buffer_size, "Scale (2, 1): zoom 0x%x size (%d, %d), expected 0xa (center) and (31, 7)", sprite->ZoomPoint(), sprite->X[1], sprite->Y[1]);
    mu_assert(sprite->ZoomPoint() == 0xa && sprite->X[1] == 31 && sprite->Y[1] == 7, buffer);
}

/** @brief A rotated sprite keeps its zoom point at the location: turned by 1 degree, UpperLeft starts near (20, 10). */
MU_TEST(draw2d_rotated_sprite_zoom_point)
{
    mu_assert(Scene2D::DrawSprite(draw2d_texture, Vector3D(20, 10, draw2d_depth), Angle::FromDegrees(1), Vector2D(1.0, 1.0),
        Scene2D::ZoomPoint::UpperLeft), "DrawSprite with an angle failed");
    const int count = draw2d_read_frame();
    const Draw2DCommand *sprite = draw2d_find_sprite(count);

    mu_assert(sprite != nullptr && sprite->Type() == 2, "No distorted sprite command with the test texture in VDP1 VRAM");
    snprintf(buffer, buffer_size, "Corners (%d, %d) (%d, %d) (%d, %d) (%d, %d): the upper left one should be about (20, 10)",
        sprite->X[0], sprite->Y[0], sprite->X[1], sprite->Y[1], sprite->X[2], sprite->Y[2], sprite->X[3], sprite->Y[3]);
    mu_assert(sprite->X[0] >= 19 && sprite->X[0] <= 21 && sprite->Y[0] >= 9 && sprite->Y[0] <= 11, buffer);
}

/** @brief A 16x8 sprite turned by 90 degrees covers 8x16 pixels (coordinates 7 and 15 apart), like an unrotated one. */
MU_TEST(draw2d_rotated_sprite_size)
{
    mu_assert(Scene2D::DrawSprite(draw2d_texture, Vector3D(0, 0, draw2d_depth), Angle::FromDegrees(90)), "DrawSprite with an angle failed");
    const int count = draw2d_read_frame();
    const Draw2DCommand *sprite = draw2d_find_sprite(count);
    mu_assert(sprite != nullptr && sprite->Type() == 2, "No distorted sprite command with the test texture in VDP1 VRAM");

    int16_t minX = sprite->X[0], maxX = sprite->X[0], minY = sprite->Y[0], maxY = sprite->Y[0];
    for (int i = 1; i < 4; i++)
    {
        minX = sprite->X[i] < minX ? sprite->X[i] : minX;
        maxX = sprite->X[i] > maxX ? sprite->X[i] : maxX;
        minY = sprite->Y[i] < minY ? sprite->Y[i] : minY;
        maxY = sprite->Y[i] > maxY ? sprite->Y[i] : maxY;
    }

    snprintf(buffer, buffer_size, "Corners span %d x %d, expected 7 x 15 (8x16 pixels)", maxX - minX, maxY - minY);
    mu_assert(maxX - minX == 7 && maxY - minY == 15, buffer);
}

/** @brief Once the frame's command buffer is full, drawing is refused; the next frame accepts drawing again. */
MU_TEST(draw2d_draw_until_full)
{
    const int limit = 4 * SGL_MAX_POLYGONS;
    int accepted = 0;
    while (accepted < limit && Scene2D::DrawLine(Vector2D(0, 0), Vector2D(5, 5), Types::HighColor(0x8001), draw2d_depth)) accepted++;
    Core::Synchronize();
    const bool acceptedAgain = Scene2D::DrawLine(Vector2D(0, 0), Vector2D(5, 5), Types::HighColor(0x8001), draw2d_depth);

    snprintf(buffer, buffer_size, "DrawLine accepted %d lines in one frame, expected at least 1 and a refusal before %d", accepted, limit);
    mu_assert(accepted >= 1 && accepted < limit, buffer);
    mu_assert(acceptedAgain, "DrawLine was refused in the next frame");
}

MU_TEST_SUITE(draw2d_test_suite)
{
    MU_SUITE_CONFIGURE(&draw2d_test_setup, &draw2d_test_teardown);

    MU_RUN_TEST(draw2d_effects_round_trip);
    MU_RUN_TEST(draw2d_sprite_attribute_defaults);
    MU_RUN_TEST(draw2d_sprite_attribute_effects);
    MU_RUN_TEST(draw2d_command_effects);
    MU_RUN_TEST(draw2d_line_reaches_vdp1);
    MU_RUN_TEST(draw2d_polygon_fill_and_outline);
    MU_RUN_TEST(draw2d_clipping_rectangle);
    MU_RUN_TEST(draw2d_sprite_scaled);
    MU_RUN_TEST(draw2d_rotated_sprite_zoom_point);
    MU_RUN_TEST(draw2d_rotated_sprite_size);
    MU_RUN_TEST(draw2d_draw_until_full);
}

#include "vdp_demo.hpp"

#include <srl_log.hpp>

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::Math::Types;

/**
 * @brief 32x32 checkerboard (four 16x16 cells), 16 colors, built in memory.
 * @details Used when a texture is not on the CD. That is the usual case: `ftx -x`
 * runs the program from RAM with no disc mounted.
 * Pixel value 1 is colorA, 0 is colorB. VDP2 always draws value 0 as transparent.
 */
struct CheckerBitmap : public SRL::Bitmap::IBitmap
{
private:
    static constexpr int Size = 32;
    uint8_t* data; // 4 bits per pixel, 2 pixels per byte, high nibble first
    SRL::Bitmap::Palette* palette;

public:
    CheckerBitmap(const HighColor& colorA, const HighColor& colorB) :
        data(new uint8_t[(Size * Size) / 2]), palette(new SRL::Bitmap::Palette(2))
    {
        this->palette->Colors[0] = colorB;
        this->palette->Colors[1] = colorA;

        for (int y = 0; y < Size; ++y)
        {
            for (int x = 0; x < Size; x += 2)
            {
                // Both pixels of a byte are in the same cell, since a cell is 16 pixels wide.
                const bool cellA = (x < (Size / 2)) == (y < (Size / 2));
                this->data[((y * Size) + x) / 2] = cellA ? 0x11 : 0x00;
            }
        }
    }

    ~CheckerBitmap() override
    {
        delete[] this->data;
        delete this->palette;
    }

    uint8_t* GetData() override
    {
        return this->data;
    }

    SRL::Bitmap::BitmapInfo GetInfo() const override
    {
        return SRL::Bitmap::BitmapInfo(Size, Size, this->palette);
    }
};

/**
 * @brief Loads a 16-color TGA from the CD, or builds a CheckerBitmap if it is missing.
 * @details Use 16-color textures: VDP2 has a fixed VRAM read budget per scanline,
 * and 16-bit color costs 4 times as much. The caller deletes the result.
 */
static SRL::Bitmap::IBitmap* LoadTextureOrFallback(const char* filename, const HighColor& colorA, const HighColor& colorB)
{
    if (SRL::Cd::File(filename).Exists())
    {
        return new SRL::Bitmap::TGA(filename);
    }

    Log::LogPrint("Texture '%s' not reachable -- using in-memory checker fallback", filename);
    return new CheckerBitmap(colorA, colorB);
}

void SetupSkyAndFloor()
{
    // Skip CRAM bank 1, so the floor and ceiling palettes land in banks 2 and 3,
    // the addresses the readme's color tutorial uses.
    SRL::CRAM::SetBankUsedState(1, SRL::CRAM::TextureColorMode::Paletted16, true);

    // RBG0 first: it can only use VRAM bank A0, so it must claim it before NBG1.
    {
        SRL::Bitmap::IBitmap* bmp = LoadTextureOrFallback("FLOOR.TGA", HighColor(220, 30, 30), HighColor(110, 15, 15));
        SRL::Tilemap::Interfaces::Bmp2Tile* tile = new SRL::Tilemap::Interfaces::Bmp2Tile(*bmp, 1);
        delete bmp;

        // The texture is 2x2 tiles; repeat it over the 32x32-tile page RBG0 needs.
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 1), 0, SRL::Tilemap::Coord(i * 2, j * 2));
            }
        }

        SRL::VDP2::RBG0::LoadTilemap(*tile);
        delete tile;

        SRL::VDP2::RBG0::SetPriority(SRL::VDP2::Priority::Layer3);
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);
        SRL::VDP2::RBG0::ScrollEnable();
    }

    // NBG1: a 2-tile-high striped band at the top of the screen.
    {
        SRL::Bitmap::IBitmap* bmp = LoadTextureOrFallback("CEIL.TGA", HighColor(70, 110, 200), HighColor(30, 55, 120));
        SRL::Tilemap::Interfaces::Bmp2Tile* tile = new SRL::Tilemap::Interfaces::Bmp2Tile(*bmp, 1);
        delete bmp;

        // Only the texture's top tile row, copied into map rows 0 and 1: stripes, no checks.
        for (int i = 0; i < 16; ++i)
        {
            tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 0), 0, SRL::Tilemap::Coord(i * 2, 0));
            tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 0), 0, SRL::Tilemap::Coord(i * 2, 1));
        }

        SRL::VDP2::NBG1::LoadTilemap(*tile);
        delete tile;

        // Above the sprites (Layer1): on a priority tie the sprite would win.
        SRL::VDP2::NBG1::SetPriority(SRL::VDP2::Priority::Layer2);
        SRL::VDP2::NBG1::ScrollEnable();
    }
}

/**
 * @details The floor spins instead of scrolling: moving the camera changes how much
 * screen the plane covers, a spin does not. Angle is 16 bits and wraps on its own;
 * a translation accumulated in Fxp would overflow after a few thousand frames.
 */
void UpdateFloorTransform(SRL::Math::Types::Fxp xOffset, SRL::Math::Types::Angle spinAngle)
{
    SRL::Scene3D::LoadIdentity();
    SRL::Scene3D::LookAt(Vector3D(0, -2.75, -6.25), Vector3D(), Angle::FromDegrees(0.0));
    SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));
    // Push/pop: without it RBG0 drew nothing.
    SRL::Scene3D::PushMatrix();
    SRL::Scene3D::Translate(Vector3D(xOffset, 0, 0));
    SRL::Scene3D::RotateZ(spinAngle);
    SRL::VDP2::RBG0::SetCurrentTransform();
    SRL::Scene3D::PopMatrix();
}

static HighColor RasterPalette[RasterPaletteSize];

void BuildRasterPalette()
{
    struct Stop { uint8_t r, g, b; };
    constexpr Stop dark = {30, 110, 30};
    constexpr Stop bright = {220, 255, 220};
    constexpr int halfSize = RasterPaletteSize / 2;

    for (int i = 0; i < RasterPaletteSize; ++i)
    {
        // Dark at both ends and bright in the middle, so the palette loops without a seam.
        const int t = i < halfSize ? i : (RasterPaletteSize - i);
        const uint8_t r = dark.r + (((int)bright.r - dark.r) * t) / halfSize;
        const uint8_t g = dark.g + (((int)bright.g - dark.g) * t) / halfSize;
        const uint8_t b = dark.b + (((int)bright.b - dark.b) * t) / halfSize;
        RasterPalette[i] = HighColor(r, g, b);
    }
}

void DrawRasterbar(uint16_t phase, SRL::Math::Types::Angle sweepAngle)
{
    const int16_t halfWidth = SRL::TV::Width >> 1;
    const int16_t sweepAmplitude = static_cast<int16_t>((SRL::TV::Height >> 1) - (RasterPaletteSize / 2) - 2);
    const int16_t centerY = (SRL::Math::Trigonometry::Sin(sweepAngle) * sweepAmplitude).As<int16_t>();
    const int16_t top = static_cast<int16_t>(centerY - (RasterPaletteSize / 2));

    SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, true);

    // One 1-pixel line per palette color.
    for (int i = 0; i < RasterPaletteSize; ++i)
    {
        const int16_t y0 = static_cast<int16_t>(top + i);
        const int16_t y1 = static_cast<int16_t>(y0 + 1);

        Vector2D corners[4] =
        {
            Vector2D(static_cast<int16_t>(-halfWidth), y0),
            Vector2D(halfWidth, y0),
            Vector2D(halfWidth, y1),
            Vector2D(static_cast<int16_t>(-halfWidth), y1),
        };

        SRL::Scene2D::DrawPolygon(corners, true, RasterPalette[(phase + i) % RasterPaletteSize], 100.0);
    }

    SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, false);
}

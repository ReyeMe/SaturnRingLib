#include "vdp_demo.hpp"

#include <srl_log.hpp>

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::Math::Types;

/**
 * @brief Minimal in-memory 32x32 Paletted16 checkerboard (16x16 quadrants),
 * used as a fallback texture when a CD texture file isn't reachable.
 *
 * @details Confirmed on real hardware that this fallback is not a rare
 * edge case for this sample: SRL::Cd::File(...).Exists() returns false for
 * both FLOOR.TGA and CEIL.TGA every time, despite them genuinely being on
 * the built ISO (`isoinfo -l` shows FLOOR.TGA;1 / CEIL.TGA;1 present) --
 * this sample runs via `ftx -x` direct-RAM execution with no real disc
 * mounted, and GFS file lookups need an actual disc. So THIS fallback path
 * is what actually renders every time this sample is tested this way, not
 * the real TGA loader -- keep it in whatever color mode the real textures
 * are meant to use (Paletted16, not RGB555 -- see SetupSkyAndFloor()'s doc
 * comment for why that distinction turned out to matter a lot).
 *
 * Implements SRL::Bitmap::IBitmap so it can feed the exact same
 * Bmp2Tile/CopyMap pipeline as a real loaded TGA -- see SetupSkyAndFloor().
 * 32x32 (not 16x16) so each checker cell is a full 16x16 tile rather than
 * an 8x8 sub-tile pattern -- the finer version produced enough moire in a
 * photo of a real TV to bury the debug text underneath it.
 * Pixel value 1 = colorA, 0 = colorB -- packed 2 pixels/byte, high nibble
 * first, matching the packing SRL::Bitmap::TGA's own paletted decoder uses.
 * @param transparentB When true, checker cell B (pixel value 0) is left
 * transparent -- VDP2 hardware always treats indexed pixel value 0 as
 * transparent regardless of what's in that palette slot, so colorB is
 * simply never read from CRAM in that case. Used for the floor so the
 * ceiling shows through its gaps; see SetupSkyAndFloor().
 */
struct CheckerBitmap : public SRL::Bitmap::IBitmap
{
private:
    static constexpr int Size = 32;
    uint8_t* data; // packed 4bpp: Size*Size/2 bytes
    SRL::Bitmap::Palette* palette;

public:
    CheckerBitmap(const HighColor& colorA, const HighColor& colorB, bool transparentB = false) :
        data(new uint8_t[(Size * Size) / 2]), palette(new SRL::Bitmap::Palette(2))
    {
        this->palette->Colors[0] = colorB; // unused/never sampled when transparentB
        this->palette->Colors[1] = colorA;

        for (int y = 0; y < Size; ++y)
        {
            for (int x = 0; x < Size; x += 2)
            {
                uint8_t packed = 0;
                for (int half = 0; half < 2; ++half)
                {
                    const int px = x + half;
                    const bool quadrantA = (px < (Size / 2)) == (y < (Size / 2));
                    packed |= (quadrantA ? 1 : 0) << (half == 0 ? 4 : 0);
                }
                this->data[((y * Size) + x) / 2] = packed;
            }
        }

        (void)transparentB; // pixel value 0 is always hardware-transparent; nothing else to do
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
 * @brief Opens a CD texture file if reachable, otherwise builds an
 * in-memory CheckerBitmap fallback with the given colors -- see
 * CheckerBitmap's doc comment for why this is needed.
 * @param filename CD texture file to try first. Must be an indexed
 * (paletted) TGA with <=16 colors -- see SetupSkyAndFloor()'s doc comment
 * for why (VDP2 VRAM-access cycle budget: RGB555 costs 4x the cycles per
 * layer that Paletted16 does). If cellBTransparent, its palette index 0
 * must be the "gap" color -- VDP2 hardware always treats indexed-mode pixel
 * value 0 as transparent, regardless of what color sits in that palette
 * slot, so the TGA needs no special transparency settings; it's automatic.
 * @param colorA,colorB Checker colors to use for the fallback.
 * @param cellBTransparent If true, the fallback's checker cell B is made
 * transparent too (matching the TGA's index-0 convention above), so both
 * paths look the same regardless of which one actually loads.
 * @return Heap-allocated IBitmap; caller owns it (delete when done).
 */
static SRL::Bitmap::IBitmap* LoadTextureOrFallback(
    const char* filename, const HighColor& colorA, const HighColor& colorB,
    bool cellBTransparent = false)
{
    if (SRL::Cd::File(filename).Exists())
    {
        return new SRL::Bitmap::TGA(filename);
    }

    Log::LogPrint("Texture '%s' not reachable -- using in-memory checker fallback", filename);
    return new CheckerBitmap(colorA, colorB, cellBTransparent);
}

/**
 * @brief Builds RBG0 (a genuine rotating/perspective "floor" ground plane)
 * and NBG1 (a flat "ceiling" stripe band pinned to the top of the screen).
 * Textures come from CD (FLOOR.TGA/CEIL.TGA, indexed/paletted) with an
 * in-memory fallback -- see LoadTextureOrFallback().
 *
 * @details Fourth design for this pair of planes -- history, in case
 * someone else lands here later:
 *
 * 1. RBG0 for the floor, camera at Vector3D(0,-6,-15) (and two other
 *    distances/heights tried before that). VRAM/CRAM allocation was
 *    confirmed correct via GDB every time, but nothing ever appeared
 *    on screen -- abandoned as an unsolved mystery at the time.
 * 2. A flat NBG0 floor, band-confined to a couple of tile rows, using the
 *    same 2D checker pattern as a full-screen design. Positionally correct
 *    but read as noise at ~32px tall (not enough room for a "board").
 * 3. Same band-confined NBG0, but with a horizontal-stripe-only source
 *    slice instead of a 2D checker -- this is what NBG1 (ceiling, below)
 *    still uses, and it reads cleanly at band height.
 *
 * This version swaps the floor back to RBG0, following
 * srl-tutorials/12_Interlude's ground-plane pattern almost verbatim -- its
 * camera position (0,-5.5,-12.5) turned out to be nearly identical to
 * attempt #1's (0,-6,-15), which never worked. The one structural
 * difference is PushMatrix()/PopMatrix() wrapping the per-frame
 * Rotate/Translate calls (attempt #1 didn't), on the theory that
 * SetCurrentTransform() may need the matrix stack left exactly where
 * LookAt+RotateX put it, with the moving part pushed on top and popped
 * back off afterward, rather than compounding onto it frame after frame.
 * This theory held up -- with PushMatrix/PopMatrix, RBG0 finally renders
 * for the first time in this file's history. At the tutorial's own
 * distance it was already too large (covered well over a third of the
 * screen). The instinct was to pull the camera FARTHER back
 * (Vector3D(0,-11,-25), ~2x) to shrink it, on ordinary "farther = smaller"
 * camera intuition -- that was backwards for this rig: it made the visible
 * ground area LARGER (~75% of the screen) and introduced visible
 * scanline noise/corruption in the upper-middle of the screen, confirmed
 * via photo. Camera Y here is altitude above a downward-facing view (post
 * RotateX(90)) -- MORE negative Y means a HIGHER vantage point, which
 * shows MORE ground, same as a satellite seeing more than someone
 * standing at ground level. So the correct direction to shrink the
 * footprint toward <=25% of screen height is CLOSER, not farther --
 * half the tutorial's own distance (Vector3D(0,-2.75,-6.25)) measured at
 * ~27% of screen height via photo (close, but over the 25% requirement)
 * and mostly (not fully) cleared the scanline noise/corruption seen at
 * the farther distance -- weak support for that being a VDP2
 * per-scanline VRAM-access-cycle overrun (more visible ground = more
 * distinct tiles sampled per scanline = more bandwidth).
 *
 * The next step pulled in further (Vector3D(0,-2.3,-5.3), only ~15%
 * closer) and, confirmed via photo, the footprint JUMPED to ~70-80% of
 * the screen -- bigger, not smaller, despite being closer. That
 * measurement and the ~27% one above have nearly the SAME camera angle
 * (Z/Y ratio 2.27 vs 2.30) but wildly different results, which rules out
 * "just tune the distance/angle a bit more" -- this shallow-angle
 * (near-horizontal LookAt) regime is genuinely unstable for a ground
 * plane: near a grazing viewing angle, screen coverage can blow up
 * non-linearly as the angle flattens toward the horizon, so tiny changes
 * swing the result unpredictably in either direction.
 *
 * A near-top-down camera (Vector3D(0,-8,-1)) was tried next, on the theory
 * that a steep angle avoids the unstable grazing regime -- confirmed via
 * photo, this was WRONG and made things much worse (~90%+ of the screen).
 * The reason is obvious in hindsight: looking straight down at an
 * infinite ground plane, every ray from the camera hits the ground --
 * there's no horizon, so top-down coverage approaches 100% by
 * construction. A shallow/near-horizontal angle is the ONLY regime where
 * a horizon line exists to limit ground coverage below some fraction of
 * the screen, so shallow angles are not a mistake to abandon -- they're
 * the only way to get under 25% at all. The instability documented above
 * is real, but the fix is to stop fighting it by repositioning the
 * (very sensitive) camera, and instead fix the camera at the best
 * measured point so far -- back to Vector3D(0,-2.75,-6.25), ~27% -- and
 * trim the remainder with an explicit Scale(0.75) inside the pushed
 * matrix, as an isolated, hopefully more linear knob layered on top of a
 * setup that's already known to render cleanly, rather than continuing to
 * re-roll the camera position itself. That made things WORSE (photo
 * confirmed a large solid block plus new scanline corruption near the
 * top of the screen), which is what finally prompted reading RBG0's
 * actual state via GDB instead of guessing another number.
 *
 * ACTUAL ROOT CAUSE, found via GDB: none of the above camera-tuning was
 * ever a controlled experiment. RBG0::SetCurrentTransform() writes a
 * fixed hardware ROTSCROLL struct (see sl_def.h) at a fixed VRAM address
 * (VDP2_VRAM_B1 + 0x1ff00, set up by slRparaInitSet() in
 * VDP2::Initialize()) -- reading that struct live via
 * `x/24xw 0x25e7ff00` showed its MY field (Y translation) was a huge,
 * seemingly-garbage value that changed between two reads moments apart
 * (0x90d5aab6 -> 0x7b0fa9b6, flipping sign). MY is derived from this
 * function's own `scrollY` accumulator, which the caller incremented by
 * 2 every frame with NO WRAPAROUND -- after a few thousand frames it
 * silently overflows Fxp's 32-bit fixed-point range, corrupting the
 * value fed into RBG0's per-pixel transform math. Every photo across
 * every camera experiment above was taken at a different frame count
 * (different, uncorrelated overflow phase), so identical camera angles
 * genuinely produced different, unreproducible results each time -- the
 * camera was never the variable that mattered. Fixed in the caller by
 * wrapping scrollY at 512 (FLOOR.TGA's tile period) before it can
 * overflow; see that wrap's own comment for detail. With that fixed, the
 * camera has been set back to the one clean, reproducible measurement so
 * far -- Vector3D(0,-2.75,-6.25), no Scale() -- as a new, actually
 * trustworthy baseline to tune from.
 *
 * RBG0 is loaded before NBG1 -- its map allocator only tries VRAM bank A0
 * with no fallback, so it must claim that bank first (see this function's
 * earlier git history for how that was root-caused). CRAM Paletted16 bank
 * 1 is reserved up front for SRL::ASCII's debug console font (its default
 * `colorBank = 1 << 12`), which never registers itself via
 * SetBankUsedState -- load-bearing, see this function's earlier history.
 */
void SetupSkyAndFloor()
{
    SRL::CRAM::SetBankUsedState(1, SRL::CRAM::TextureColorMode::Paletted16, true);

    // ---- RBG0: floor (rotating perspective ground, tiled with FLOOR.TGA) ----
    {
        SRL::Bitmap::IBitmap* bmp = LoadTextureOrFallback("FLOOR.TGA", HighColor(220, 30, 30), HighColor(110, 15, 15), true);
        SRL::Tilemap::Interfaces::Bmp2Tile* tile = new SRL::Tilemap::Interfaces::Bmp2Tile(*bmp, 1);
        delete bmp;

        // Source is 32x32px = a 2x2 block of 16x16 tiles. Repeat that whole
        // block to fill the entire 32x32-tile (512x512px) page RBG0 requires.
        for (int i = 0; i < 16; ++i)
        {
            for (int j = 0; j < 16; ++j)
            {
                tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 1), 0, SRL::Tilemap::Coord(i * 2, j * 2));
            }
        }

        SRL::VDP2::RBG0::LoadTilemap(*tile);
        delete tile;

        // Above NBG1's ceiling (Layer2) and the green VDP1 rasterbar
        // (Layer1, see main()'s SpriteLayer::SetPriority).
        SRL::VDP2::RBG0::SetPriority(SRL::VDP2::Priority::Layer3);
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);
        SRL::VDP2::RBG0::ScrollEnable();
    }

    // ---- NBG1: ceiling (stripe band, pinned to tile rows 0-1 / top of screen) ----
    {
        SRL::Bitmap::IBitmap* bmp = LoadTextureOrFallback("CEIL.TGA", HighColor(70, 110, 200), HighColor(30, 55, 120));
        SRL::Tilemap::Interfaces::Bmp2Tile* tile = new SRL::Tilemap::Interfaces::Bmp2Tile(*bmp, 1);
        delete bmp;

        // Source tile-row 0 only (Coord Y stays 0) -- a solid colorA/colorB
        // stripe, no vertical variation. Stamped at both map rows 0 and 1
        // so the 32px band is stripe all the way down.
        for (int i = 0; i < 16; ++i)
        {
            tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 0), 0, SRL::Tilemap::Coord(i * 2, 0));
            tile->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(1, 0), 0, SRL::Tilemap::Coord(i * 2, 1));
        }

        SRL::VDP2::NBG1::LoadTilemap(*tile);
        delete tile;

        // Layer2, not Layer1: the green VDP1 rasterbar sits at Layer1 (see
        // main()'s SpriteLayer::SetPriority doc comment -- Layer0 is a
        // hardware sentinel for "layer off", not usable), and needs to
        // stay strictly below this ceiling (confirmed via photo that an
        // exact tie resolves in the sprite layer's favor).
        SRL::VDP2::NBG1::SetPriority(SRL::VDP2::Priority::Layer2);
        SRL::VDP2::NBG1::ScrollEnable();
    }
}

/**
 * @details The moving part is a continuous Z rotation (a slow spin, like
 * a rotating sign), NOT a translation. This is a deliberate choice, not
 * just a style pick: translating this plane (the original "scroll
 * forward" design) was measured via GDB to change its on-screen footprint
 * size smoothly but substantially across its range (~27% up to ~80%+ of
 * the screen) -- there's no way to animate a translation here while also
 * holding a maximum size. Rotation doesn't have that problem: it changes
 * orientation, not camera-to-plane distance, so the footprint stays a
 * constant size while still visibly, continuously moving. It's also
 * immune to the overflow bug that motivated wrapping the old scrollY
 * value -- SRL::Math::Types::Angle stores its value as a plain uint16_t
 * (0x0000-0xFFFF = 0-1 turns), which wraps for free on plain integer
 * overflow, unlike Fxp's 32-bit fixed-point translate accumulator.
 *
 * A fixed local-X translate (before the rotation, so it isn't spun by
 * it) shifts the whole plane right, off of the debug text columns on the
 * left -- see this function's caller for the actual offset/angle values.
 */
void UpdateFloorTransform(SRL::Math::Types::Fxp xOffset, SRL::Math::Types::Angle spinAngle)
{
    SRL::Scene3D::LoadIdentity();
    SRL::Scene3D::LookAt(Vector3D(0, -2.75, -6.25), Vector3D(), Angle::FromDegrees(0.0));
    SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));
    SRL::Scene3D::PushMatrix();
    SRL::Scene3D::Translate(Vector3D(xOffset, 0, 0));
    SRL::Scene3D::RotateZ(spinAngle);
    SRL::VDP2::RBG0::SetCurrentTransform();
    SRL::Scene3D::PopMatrix();
}

/** @brief Single-hue glow gradient used by the VDP1 rasterbar, built once at startup. */
static HighColor RasterPalette[RasterPaletteSize];

/**
 * @details It's a symmetric triangle wave over the whole palette (index 0
 * and RasterPaletteSize-1 are both the dark end), so it loops seamlessly
 * under DrawRasterbar()'s cyclic (% RasterPaletteSize) indexing. See
 * https://www.c64-wiki.de/images/f/f2/Rasterbar.png for the reference look
 * this is going for -- a single-hue glow, not a multi-hue rainbow.
 */
void BuildRasterPalette()
{
    struct Stop { uint8_t r, g, b; };
    constexpr Stop dark = {30, 110, 30};
    constexpr Stop bright = {220, 255, 220};
    constexpr int halfSize = RasterPaletteSize / 2;

    for (int i = 0; i < RasterPaletteSize; ++i)
    {
        // Triangle wave 0 (dark, at both ends) -> halfSize (bright, middle) -> 0.
        const int t = i < halfSize ? i : (RasterPaletteSize - i);
        const uint8_t r = dark.r + (((int)bright.r - dark.r) * t) / halfSize;
        const uint8_t g = dark.g + (((int)bright.g - dark.g) * t) / halfSize;
        const uint8_t bl = dark.b + (((int)bright.b - dark.b) * t) / halfSize;
        RasterPalette[i] = HighColor(r, g, bl);
    }
}

/**
 * @details One bar per palette entry (RasterPaletteSize of them), so the
 * gradient is sampled at full resolution; advancing phase each frame
 * scrolls the gradient through the fixed bar positions, making the
 * bright highlight appear to travel up and down within the band. The
 * whole stack also sweeps up and down the screen: a sine wave of
 * sweepAngle gives the smooth back-and-forth motion for free, no
 * separate direction/bounce bookkeeping needed.
 */
void DrawRasterbar(uint16_t phase, SRL::Math::Types::Angle sweepAngle)
{
    constexpr int barCount = RasterPaletteSize;
    constexpr int16_t barHeight = 1;
    const int16_t halfWidth = SRL::TV::Width >> 1;

    const int16_t halfScreenHeight = SRL::TV::Height >> 1;
    const int16_t sweepAmplitude = static_cast<int16_t>(halfScreenHeight - ((barCount * barHeight) / 2) - 2);
    const int16_t centerY = (SRL::Math::Trigonometry::Sin(sweepAngle) * sweepAmplitude).As<int16_t>();
    const int16_t top = static_cast<int16_t>(centerY - (barCount * barHeight) / 2);

    SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, true);

    for (int i = 0; i < barCount; ++i)
    {
        const int16_t y0 = static_cast<int16_t>(top + (i * barHeight));
        const int16_t y1 = static_cast<int16_t>(y0 + barHeight);

        Vector2D corners[4] =
        {
            Vector2D(static_cast<int16_t>(-halfWidth), y0),
            Vector2D(halfWidth, y0),
            Vector2D(halfWidth, y1),
            Vector2D(static_cast<int16_t>(-halfWidth), y1),
        };

        const uint16_t colorIndex = (phase + i) % RasterPaletteSize;
        SRL::Scene2D::DrawPolygon(corners, true, RasterPalette[colorIndex], 100.0);
    }

    SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, false);
}

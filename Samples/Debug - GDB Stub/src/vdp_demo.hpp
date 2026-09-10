#pragma once

#include <srl.hpp>

/**
 * @brief VDP1/VDP2 background demo -- purely cosmetic, unrelated to the GDB
 * stub itself. Exists so the sample has something visually alive on
 * screen: RBG0 as a genuine rotating/perspective floor, NBG1 as a flat
 * ceiling stripe band, and a VDP1 rasterbar with a rotating single-hue
 * glow palette. All three run behind/below the debug text (NBG3,
 * Priority::Layer7).
 *
 * Split into its own compilation unit (separate from main.cxx and the
 * GDB-stub crash triggers in debug_triggers.hpp/.cxx) specifically to
 * prove the GDB stub works correctly across a multi-file build.
 */

/**
 * @brief Builds RBG0 (a genuine rotating/perspective "floor" ground plane)
 * and NBG1 (a flat "ceiling" stripe band pinned to the top of the screen).
 * Textures come from CD (FLOOR.TGA/CEIL.TGA, indexed/paletted) with an
 * in-memory fallback. See vdp_demo.cxx for the full design history.
 */
void SetupSkyAndFloor();

/**
 * @brief Updates RBG0's rotation/perspective transform for this frame. See
 * vdp_demo.cxx for why rotation (not translation) is used for motion.
 * @param xOffset Fixed rightward shift, applied before rotation.
 * @param spinAngle Current spin phase; advance by a fixed step each frame
 * in the caller. Wraps for free -- no bounds-checking needed.
 */
void UpdateFloorTransform(SRL::Math::Types::Fxp xOffset, SRL::Math::Types::Angle spinAngle);

/** @brief Number of colors in RasterPalette -- one full dark-bright-dark cycle. */
constexpr int RasterPaletteSize = 32;

/**
 * @brief Fills RasterPalette with a single-hue "glow" gradient -- dark
 * blue/purple at both ends, rising smoothly to near-white in the middle,
 * then back down. Must be called once before the first DrawRasterbar() call.
 */
void BuildRasterPalette();

/**
 * @brief Draws a classic C64-style "rasterbar": a single thin, full-width,
 * half-transparent stack of VDP1 polygons whose colors sample
 * RasterPalette's single-hue glow gradient. See vdp_demo.cxx for the
 * reference look and mechanics.
 * @param phase Current rotation phase (0..RasterPaletteSize-1); advance by
 * 1 each frame in the caller.
 * @param sweepAngle Current position in the up/down sweep cycle; advance
 * by a fixed step each frame in the caller.
 */
void DrawRasterbar(uint16_t phase, SRL::Math::Types::Angle sweepAngle);

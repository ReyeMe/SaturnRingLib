#pragma once

#include <srl.hpp>

/**
 * @brief Moving backgrounds, so the screen is not static while you debug.
 * Not needed by the GDB stub. All of it sits behind the debug text (NBG3).
 */

/** @brief Builds the spinning floor (RBG0) and the scrolling ceiling band (NBG1). */
void SetupSkyAndFloor();

/**
 * @brief Positions the floor for this frame.
 * @param xOffset Shift to the right, applied before the spin.
 * @param spinAngle Spin angle; the caller advances it each frame.
 */
void UpdateFloorTransform(SRL::Math::Types::Fxp xOffset, SRL::Math::Types::Angle spinAngle);

/** @brief Number of colors in the rasterbar's palette. */
constexpr int RasterPaletteSize = 32;

/** @brief Builds the rasterbar's dark-bright-dark green palette. Call once before DrawRasterbar(). */
void BuildRasterPalette();

/**
 * @brief Draws a C64-style rasterbar: a stack of 1-pixel lines that sweeps up and down.
 * @param phase Palette offset (0..RasterPaletteSize-1); the caller adds 1 each frame.
 * @param sweepAngle Position in the up/down sweep; the caller advances it each frame.
 */
void DrawRasterbar(uint16_t phase, SRL::Math::Types::Angle sweepAngle);

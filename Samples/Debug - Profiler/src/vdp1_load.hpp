#pragma once

#include <cstdint>

/** @brief Full-screen polygons drawn per frame in each load step; the step changes every 120 frames. */
namespace Vdp1Load
{
    constexpr uint16_t Polygons[] = {0, 1, 2, 4, 8, 16};
    constexpr uint32_t Steps = sizeof(Polygons) / sizeof(Polygons[0]);
    constexpr uint32_t FramesPerStep = 120;
}

/** @brief Queues this frame's full-screen polygons for VDP1.
 *  @details Marks "VDP1 load N" on the profiler timeline when the step changes, so the PC can
 *  match VDP1 draw times to polygon counts (Tests/Profiler/run_profiler_test.py --mode vdp1).
 *  @param frame Frame number
 */
void DrawVdp1Load(uint32_t frame);

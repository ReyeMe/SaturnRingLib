#include <srl.hpp>

#include "vdp1_load.hpp"

using SRL::Math::Types::Vector2D;

void DrawVdp1Load(uint32_t frame)
{
    // One name per step: the timeline stores the address of a string literal.
    static const char *const marks[Vdp1Load::Steps] = {
        "VDP1 load 0", "VDP1 load 1", "VDP1 load 2", "VDP1 load 4", "VDP1 load 8", "VDP1 load 16"};
    static const SRL::Types::HighColor colors[] = {
        SRL::Types::HighColor::Colors::Red, SRL::Types::HighColor::Colors::Green,
        SRL::Types::HighColor::Colors::Blue, SRL::Types::HighColor::Colors::Yellow};

    const uint32_t step = (frame / Vdp1Load::FramesPerStep) % Vdp1Load::Steps;

    if (frame % Vdp1Load::FramesPerStep == 0)
    {
        SRL_PROF_MARK(marks[step]);
    }

    // The whole 320x240 screen, in centred coordinates.
    Vector2D corners[4] = {Vector2D(-160, -120), Vector2D(159, -120), Vector2D(159, 119), Vector2D(-160, 119)};

    for (uint16_t i = 0; i < Vdp1Load::Polygons[step]; i++)
    {
        SRL::Scene2D::DrawPolygon(corners, true, colors[i % 4], 500.0);
    }
}

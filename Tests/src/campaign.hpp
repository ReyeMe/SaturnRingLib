// Tests/src/campaign.hpp
// Shared start/end of every test campaign (Tests/campaigns/*/main.cxx).
// Include it first, then the test headers, then call:
//   BeginCampaign(); RUN_AND_DISPLAY_SUITE(...); ... EndCampaign();
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

#include "display.hpp"

// Test headers use SRL names (Logger::, Memory::, ...) unqualified.
using namespace SRL;

/** @brief Tags the log scripts look for (run_tests.bat, ctrf_converter.py). */
inline const char *const StrStart = "***UT_START***";
inline const char *const StrEnd = "***UT_END***";

/** @brief Initializes SRL and tags the start of the campaign. */
inline void BeginCampaign()
{
    SRL::Core::Initialize(SRL::Types::HighColor(20, 10, 50));
    SRL::ASCII::Clear();

    SRL::Logger::LogInfo(StrStart);
    PushResultLine(StrStart);
}

/** @brief Reports results, tags the end of the campaign, then lets the user scroll the results forever. */
[[noreturn]] inline void EndCampaign()
{
    using namespace SRL::Logger;

    MU_REPORT();

    BuildStatsLine(resultsBuffer, BufferSize,
        static_cast<unsigned int>(minunit_run),
        static_cast<unsigned int>(minunit_assert),
        static_cast<unsigned int>(minunit_fail));
    PushResultLine(resultsBuffer);

    LogInfo(StrEnd);
    PushResultLine(StrEnd);

    if (gResults.size() > DisplayLines)
    {
        startIndex = gResults.size() - DisplayLines;
    }

    RenderResults(startIndex);

    SRL::Input::Digital pad(0);
    uint8_t upHoldFrames = 0;
    uint8_t downHoldFrames = 0;
    const uint8_t repeatDelay = 20;
    const uint8_t repeatRate = 3;

    while (1)
    {
        SRL::Core::Synchronize();
        bool refresh = false;

        const bool upHeld = pad.IsHeld(SRL::Input::Digital::Button::Up);
        const bool downHeld = pad.IsHeld(SRL::Input::Digital::Button::Down);

        if (upHeld && !downHeld)
        {
            if (upHoldFrames < 255)
            {
                ++upHoldFrames;
            }
            downHoldFrames = 0;
        }
        else if (downHeld && !upHeld)
        {
            if (downHoldFrames < 255)
            {
                ++downHoldFrames;
            }
            upHoldFrames = 0;
        }
        else
        {
            upHoldFrames = 0;
            downHoldFrames = 0;
        }

        if (pad.WasPressed(SRL::Input::Digital::Button::Up))
        {
            if (startIndex > 0)
            {
                --startIndex;
                refresh = true;
            }
        }
        else if (upHeld && upHoldFrames > repeatDelay &&
                 ((upHoldFrames - repeatDelay) % repeatRate == 0))
        {
            if (startIndex > 0)
            {
                --startIndex;
                refresh = true;
            }
        }

        if (pad.WasPressed(SRL::Input::Digital::Button::Down))
        {
            if (startIndex + DisplayLines < gResults.size())
            {
                ++startIndex;
                refresh = true;
            }
        }
        else if (downHeld && downHoldFrames > repeatDelay &&
                 ((downHoldFrames - repeatDelay) % repeatRate == 0))
        {
            if (startIndex + DisplayLines < gResults.size())
            {
                ++startIndex;
                refresh = true;
            }
        }

        if (refresh)
        {
            RenderResults(startIndex);
        }
    }
}

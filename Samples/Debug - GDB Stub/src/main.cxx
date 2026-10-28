#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_input.hpp>

#include "debug_triggers.hpp"
#include "slave_counter_task.hpp"
#include "vdp_demo.hpp"

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::Math::Types;

/**
 * @brief Copies at most destSize - 1 characters of src into dest.
 * @note SRL::Debug::Print overruns its buffer on long %s arguments and ignores
 * "%.Ns", so long GDB command text must be cut here before printing.
 */
static void TruncateForDisplay(char* dest, size_t destSize, const char* src)
{
    size_t i = 0;
    while (i < destSize - 1 && src[i] != '\0') { dest[i] = src[i]; ++i; }
    dest[i] = '\0';
}

/**
 * @brief Runs the GDB stub sample.
 * @details The code is split over several files on purpose: breakpoints, stepping
 * and symbol lookup must work across translation units, not only inside main.cxx.
 * The GDB stub needs no setup call: SRL::Core::Synchronize() calls
 * SRL::GDBStub::Poll() every frame, and that is when GDB can connect or break in.
 */
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);

    SetupSkyAndFloor();
    BuildRasterPalette();

    // VDP2 must enable color calculation too, or VDP1 half-transparency renders opaque.
    SRL::VDP2::SpriteLayer::ColorCalcON();
    // Lowest visible priority (0 means "layer off"), so sprites sit behind both backgrounds.
    SRL::VDP2::SpriteLayer::SetPriority(SRL::VDP2::Priority::Layer1);

    SRL::Debug::Print(1, 1, "GDB Stub Sample");
    SRL::Debug::Print(1, 2, "Stub active - connect GDB to break");
    SRL::Debug::Print(1, 4, "Press:");
    SRL::Debug::Print(2, 5, "B: Illegal Inst.");
    SRL::Debug::Print(2, 6, "A: CPU Addr Err");
    SRL::Debug::Print(2, 7, "C: Reserved Inst.");
    SRL::Debug::Print(2, 8, "X: Slot Illegal");
    SRL::Debug::Print(2, 9, "Y: Slot Reserved");
    SRL::Debug::Print(22, 5, "Z: Gen. Illegal");
    SRL::Debug::Print(22, 6, "L: DMA Addr Err");
    SRL::Debug::Print(22, 7, "R: UBC Break");
    SRL::Debug::Print(22, 8, "START: TRAPA 35");
    SRL::Debug::Print(22, 9, "UP: Step demo");
    SRL::Debug::Print(22, 10, "DOWN: Touch var");
    Log::LogPrint("GDB Stub active, waiting for GDB connection via Poll()");

    SRL::Core::Synchronize();
    // To stop here at boot instead, call SRL::GDBStub::Break(). It waits for GDB,
    // so only use it when GDB is already connected.

    // Catch breakpoints hit by code running on the slave SH-2 (e.g. SlaveCounterTask::Do()).
    SRL::GDBStub::InstallSlaveExceptionTask installExceptionTask;
    SRL::Slave::ExecuteOnSlave(installExceptionTask);
    // Bounded wait, so a lost completion flag cannot hang the sample before its main loop.
    uint32_t installWait = 0;
    while (installExceptionTask.IsRunning() && installWait < 5000000U) { ++installWait; }

    SlaveCounterTask slaveTask;
    slaveTask.ResetTask();
    // About 4 jobs a second at 60 fps; a slave that never rests slows the master
    // down, because both CPUs share one memory bus.
    constexpr int kSlaveDispatchInterval = 15;

    int counter = 0;
    uint32_t lastMonitorCount = 0;
    SRL::Input::Digital gamepad(0);

    constexpr Fxp floorXOffset = 8; // keeps the floor away from the text on the left
    Angle floorSpinAngle = Angle::Zero();
    Vector2D ceilingPosition(0, 0);
    uint16_t rasterPhase = 0;
    Angle rasterSweepAngle = Angle::Zero();

    while (true)
    {
        // Never block on the slave: start a job only when the previous one is done.
        if (!slaveTask.IsRunning() && (counter % kSlaveDispatchInterval) == 0)
        {
            SRL::Slave::ExecuteOnSlave(slaveTask);
        }

        SRL::Debug::Print(1, 11, "Loop counter: %d", counter++);
        SRL::Debug::Print(1, 12, "Slave jobs: %u primes: %u",
            static_cast<unsigned int>(slaveTask.GetCounter()), static_cast<unsigned int>(slaveTask.GetPrimeCount()));
        SRL::Debug::Print(1, 13, "USB link: %s", SRL::DevCart::CS0::IsConnected() ? "connected" : "disconnected");
        SRL::Debug::Print(1, 14, "GDB session: %s", SRL::GDBStub::IsConnected() ? "active" : "waiting");
        SRL::Debug::Print(1, 15, "GDB handlers: %s", SRL::GDBStub::IsHandlersInstalled() ? "installed" : "pending");
        SRL::Debug::Print(1, 16, "GDB thunk count: %u", static_cast<unsigned int>(SRL::GDBStub::GetExceptionThunkCount()));
        SRL::Debug::Print(1, 19, "GDB cmd count:   %u", static_cast<unsigned int>(SRL::GDBStub::GetCommandCount()));
        char lastGdbCmd[26];
        TruncateForDisplay(lastGdbCmd, sizeof(lastGdbCmd),
            SRL::GDBStub::GetLastCommand()[0] ? SRL::GDBStub::GetLastCommand() : "<none>");
        SRL::Debug::Print(1, 20, "Last GDB cmd: %s", lastGdbCmd);
        SRL::Debug::Print(1, 22, "Port avail:      %s", SRL::GDBStub::IsDevCartPortAvailable() ? "yes" : "no");
        SRL::Debug::Print(1, 25, "TestVar (watch me): %d", static_cast<int>(g_testVariable));
        char lastMonitorCmd[22];
        TruncateForDisplay(lastMonitorCmd, sizeof(lastMonitorCmd),
            SRL::GDBStub::GetLastMonitorCommand()[0] ? SRL::GDBStub::GetLastMonitorCommand() : "<none>");
        SRL::Debug::Print(1, 26, "Last monitor cmd: %s", lastMonitorCmd);
        // Row 27 is taken by CrashProgram()'s message.
        SRL::Debug::Print(1, 28, "Slave BP hits: %u", static_cast<unsigned int>(SRL::GDBStub::GetSlaveBreakpointCount()));

        if (gamepad.WasPressed(SRL::Input::Digital::Button::B)) { CrashProgram(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::A)) { CPUAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::C)) { ReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::X)) { SlotIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Y)) { SlotReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Z)) { GeneralIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::L)) { DMAAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::R)) { UserBreakController(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::START)) { Trapa35(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Up)) { SteppableFunction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Down)) { g_testVariable = g_testVariable + 1; }

        // `(gdb) monitor <text>` fires the same triggers without a gamepad.
        const uint32_t monitorCount = SRL::GDBStub::GetMonitorCommandCount();
        if (monitorCount != lastMonitorCount)
        {
            lastMonitorCount = monitorCount;
            HandleMonitorCommand(SRL::GDBStub::GetLastMonitorCommand());
        }

        UpdateFloorTransform(floorXOffset, floorSpinAngle);
        floorSpinAngle += Angle::FromDegrees(1.5);
        ceilingPosition += Vector2D(-1, 0);
        SRL::VDP2::NBG1::SetPosition(ceilingPosition);
        DrawRasterbar(rasterPhase, rasterSweepAngle);
        rasterPhase = (rasterPhase + 1) % RasterPaletteSize;
        rasterSweepAngle += Angle::FromDegrees(1.0);

        SRL::Core::Synchronize();
    }

    return 0;
}

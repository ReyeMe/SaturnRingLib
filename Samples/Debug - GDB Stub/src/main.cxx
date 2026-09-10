#include <srl.hpp>
#include <srl_log.hpp>     // Logging system
#include <srl_input.hpp>   // Gamepad input

#include "debug_triggers.hpp"
#include "vdp_demo.hpp"

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::DevCart;
using namespace SRL::Math::Types;

/**
 * @brief Main program entry point.
 *
 * Initializes the SaturnRingLib core, the GDB stub, and polls continuously
 * for USB gamepad inputs and GDB commands.
 *
 * @details This sample is deliberately split across three compilation
 * units -- this file (orchestration only), debug_triggers.hpp/.cxx (the
 * GDB-stub exercise triggers), and vdp_demo.hpp/.cxx (the VDP1/VDP2
 * background demo) -- specifically to prove the GDB stub works correctly
 * across a multi-file build: breakpoints set in one .cxx must still hit
 * when execution reaches them via a call from another, symbol/address
 * resolution must find functions and globals regardless of which
 * translation unit defines them, and single-step must follow calls across
 * file boundaries the same way it does within one file. A stub that only
 * worked correctly when everything happened to live in a single main.cxx
 * would be a much weaker proof than one that works here.
 *
 * @return Returns 0 on standard completion (though typically loops forever).
 */
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);

    SetupSkyAndFloor();
    BuildRasterPalette();

    // VDP1's CMDPMOD "half-transparency" bit (set per-polygon by
    // SRL::Scene2D::SetEffect(HalfTransparency, true) in DrawRasterbar())
    // only controls how VDP1 draws into its OWN framebuffer. Making that
    // actually blend against the VDP2 layers underneath (debug text,
    // NBG1 ceiling, RBG0 floor) needs VDP2's separate sprite-layer color
    // calculation enabled too -- without this, the rasterbar renders
    // fully opaque despite the VDP1-side flag being set correctly.
    SRL::VDP2::SpriteLayer::ColorCalcON();

    // Explicitly place VDP1 sprites (the rasterbar) BEHIND RBG0's floor
    // (Layer2) and NBG1's ceiling (Layer1) -- Layer1, the lowest priority
    // that still actually displays. Layer0 was tried first and made the
    // rasterbar disappear ENTIRELY, even over the plain black background
    // where nothing should have occluded it -- confirmed via photo. On
    // Saturn VDP2, priority 0 is a hardware sentinel meaning "this layer
    // is off", not merely "lowest priority"; every displayable layer
    // needs priority 1-7. RBG0's floor is also confirmed (see
    // SetupSkyAndFloor()'s doc comment on the checker/CheckerBitmap
    // texture) to render BOTH of its checker colors opaque, not
    // transparent the way flat NBG screens treat palette index 0 -- so
    // behind RBG0's floor specifically, the rasterbar will still be fully
    // hidden wherever the floor itself is opaque; that's real occlusion,
    // not a priority bug, and there's no VDP2-side fix for it without
    // changing the floor's own texture/transparency setup.
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
    SRL::Debug::Print(22, 8, "START: TRAPA 3");
    SRL::Debug::Print(22, 9, "UP: Step demo");
    SRL::Debug::Print(22, 10, "DOWN: Touch var");
    Log::LogPrint("GDB Stub active, waiting for GDB connection via Poll()");
    Log::LogPrint("monitor commands: crash illegal|addr|reserved|slotillegal|"
        "slotreserved|genillegal|dma|ubc|trapa3, step, touch, regs slave, regs vdp");

    SRL::Core::Synchronize();
    // NOTE: Break() issues trapa #32 which blocks the Saturn in the RSP command loop
    // waiting for a GDB client. Only call it when GDB is already connected.
    // SRL::GDBStub::Break();

    // Prove SRL::Slave::ExecuteOnSlave() itself works: dispatch a small,
    // fixed number of jobs to the Slave SH-2 at startup. See
    // SlaveCounterTask's doc comment for why SRL::GDBStub::
    // InstallSlaveFreezeHandler() is deliberately NOT used in this sample --
    // hardware testing confirmed it does not coexist with SRL::Slave usage.
    SlaveCounterTask slaveTask;
    slaveTask.ResetTask();
    for (int i = 0; i < 5; ++i)
    {
        SRL::Slave::ExecuteOnSlave(slaveTask);
        while (slaveTask.IsRunning()) { }
    }

    int counter = 0;
    uint32_t lastMonitorCount = 0;
    SRL::Input::Digital gamepad(0);

    // RBG0 floor's fixed rightward shift (keeps it off the debug text
    // columns -- see UpdateFloorTransform()) and its continuously
    // advancing spin phase (Angle wraps for free, see the same doc
    // comment for why rotation is used instead of translation for
    // motion). NBG1 ceiling's pinned-band scroll position (only X may
    // move, see SetupSkyAndFloor()), and the VDP1 rasterbar's rotating
    // palette phase and up/down sweep phase (see DrawRasterbar()).
    constexpr Fxp floorXOffset = 8;
    Angle floorSpinAngle = Angle::Zero();
    Vector2D ceilingPosition(0, 0);
    uint16_t rasterPhase = 0;
    Angle rasterSweepAngle = Angle::Zero();

    while (true)
    {
        SRL::Debug::Print(1, 11, "Loop counter: %d", counter++);
        SRL::Debug::Print(1, 12, "Slave jobs done: %lu", static_cast<unsigned long>(slaveTask.GetCounter()));
        const bool usbConnected = SRL::DevCart::CS0::IsConnected();
        const bool gdbConnected = SRL::GDBStub::IsConnected();

        SRL::Debug::Print(1, 13, "USB link: %s", usbConnected ? "connected" : "disconnected");
        SRL::Debug::Print(1, 14, "GDB session: %s", gdbConnected ? "active" : "waiting");
        SRL::Debug::Print(1, 15, "GDB handlers: %s", SRL::GDBStub::IsHandlersInstalled() ? "installed" : "pending");
        SRL::Debug::Print(1, 16, "GDB thunk count: %lu", static_cast<unsigned long>(SRL::GDBStub::GetExceptionThunkCount()));
        SRL::Debug::Print(1, 17, "GDB RX bytes:    %lu", static_cast<unsigned long>(SRL::GDBStub::GetRxDetectCount()));
        SRL::Debug::Print(1, 18, "GDB RX ready:    %lu", static_cast<unsigned long>(SRL::GDBStub::GetRxReadyCount()));
        SRL::Debug::Print(1, 19, "GDB cmd count:   %lu", static_cast<unsigned long>(SRL::GDBStub::g_command_count));
        SRL::Debug::Print(1, 20, "Last GDB cmd: %.45s", SRL::GDBStub::g_last_command[0] ? SRL::GDBStub::g_last_command : "<none>");
        SRL::Debug::Print(1, 21, "DevCart probe:   %s", SRL::GDBStub::IsDevCartReady() ? "ok" : "failed");
        SRL::Debug::Print(1, 22, "Port avail:      %s", SRL::GDBStub::IsDevCartPortAvailable() ? "yes" : "no");
        SRL::Debug::Print(1, 23, "USB_FLAGS:       0x%02X", static_cast<unsigned int>(SRL::GDBStub::GetLastUsbFlags()));
        SRL::Debug::Print(1, 24, "Poll fallback:   %lu", static_cast<unsigned long>(SRL::GDBStub::GetPollFallbackCount()));
        SRL::Debug::Print(1, 25, "TestVar (watch me): %ld", static_cast<long>(g_testVariable));
        SRL::Debug::Print(1, 26, "Last monitor cmd: %.30s",
            SRL::GDBStub::GetLastMonitorCommand()[0] ? SRL::GDBStub::GetLastMonitorCommand() : "<none>");

        if (gamepad.WasPressed(SRL::Input::Digital::Button::B)) { CrashProgram(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::A)) { CPUAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::C)) { ReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::X)) { SlotIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Y)) { SlotReservedInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Z)) { GeneralIllegalInstruction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::L)) { DMAAddressError(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::R)) { UserBreakController(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::START)) { Trapa3(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Up)) { SteppableFunction(); }
        else if (gamepad.WasPressed(SRL::Input::Digital::Button::Down)) { g_testVariable = g_testVariable + 1; }

        // Headless trigger path: a GDB `monitor <text>` command (or any scripted
        // qRcmd sender) can fire the same test paths as the gamepad buttons above,
        // without needing a human at the controller.
        const uint32_t monitorCount = SRL::GDBStub::GetMonitorCommandCount();
        if (monitorCount != lastMonitorCount)
        {
            lastMonitorCount = monitorCount;
            HandleMonitorCommand(SRL::GDBStub::GetLastMonitorCommand());
        }

        // Spin RBG0's floor in place -- see UpdateFloorTransform()'s doc
        // comment for why rotation (not translation) is used for motion
        // here: it keeps the footprint size constant while still moving.
        UpdateFloorTransform(floorXOffset, floorSpinAngle);
        floorSpinAngle += Angle::FromDegrees(1.5);
        // Negative X: increasing the scroll offset samples further right
        // across the tilemap, which makes the visible content appear to
        // slide LEFT on screen (same as panning a camera right makes the
        // world seem to move left) -- decreasing it instead makes the
        // ceiling's stripe content visibly slide left-to-right.
        ceilingPosition += Vector2D(-1, 0);
        SRL::VDP2::NBG1::SetPosition(ceilingPosition);
        DrawRasterbar(rasterPhase, rasterSweepAngle);
        rasterPhase = (rasterPhase + 1) % RasterPaletteSize;
        rasterSweepAngle += Angle::FromDegrees(1.0);

        SRL::Core::Synchronize();
    }

    return 0;
}

#include <srl.hpp>
#include <srl_gdbstub.hpp>
#include <srl_log.hpp>      // Logging system
#include <srl_devcart.hpp>  // DevCart USB and CPLD access
#include <srl_input.hpp>    // Gamepad input

using namespace SRL::Types;
using namespace SRL::Logger;
using namespace SRL::DevCart;

// Deliberately triggers an Illegal Instruction exception that the GDB stub catches,
// allowing post-mortem inspection of the register state and call stack.
[[noreturn]] static void CrashProgram()
{
    SRL::Debug::Print(1, 27, "*** CRASH TRIGGERED ***");
    SRL::Core::Synchronize();
    // Emit 0xFFFF — the SH-2 Illegal Instruction opcode.
    // The GDB stub's exception thunk catches this and enters the RSP command loop.
    asm volatile(".word 0xFFFF" ::: "memory");
    __builtin_unreachable();
}

// Main program entry
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);
     Log::LogPrint("Before GDBStub::Init");

    SRL::GDBStub::Init();

     Log::LogPrint("After GDBStub::Init");

    SRL::Debug::Print(1, 1, "GDB Stub Sample");
    SRL::Debug::Print(1, 3, "Stub active - connect GDB to break");
    SRL::Debug::Print(1, 5, "Press START to crash into GDB");
     Log::LogPrint("GDB Stub active, waiting for GDB connection via Poll()");
    
    SRL::Core::Synchronize();
    // NOTE: Break() issues trapa #32 which blocks the Saturn in the RSP command loop
    // waiting for a GDB client. Only call it when GDB is already connected.
    // SRL::GDBStub::Break();

    int counter = 0;
    SRL::Input::Digital gamepad(0);

    while (true)
    {
        SRL::Debug::Print(1, 11, "Loop counter: %d", counter++);
        const bool usbConnected = SRL::DevCart::CS0::isConnected();
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

        if (gamepad.WasPressed(SRL::Input::Digital::Button::START))
        {
            CrashProgram();
        }

        SRL::GDBStub::Poll();

        SRL::Core::Synchronize();
    }

    return 0;
}

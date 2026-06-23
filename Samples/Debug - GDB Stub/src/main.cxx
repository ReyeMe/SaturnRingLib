#include <srl.hpp>
#include <srl_gdbstub.hpp>

using namespace SRL::Types;

// Main program entry
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::GDBStub::Init();
    SRL::GDBStub::Break();

    SRL::Debug::Print(1, 1, "GDB Stub Sample");
    SRL::Debug::Print(1, 3, "Initializing GDB Stub...");


    int counter = 0;

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

        SRL::GDBStub::Poll();

        SRL::Core::Synchronize();
    }

    return 0;
}


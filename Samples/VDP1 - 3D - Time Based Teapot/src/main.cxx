// Samples/VDP1 - 3D - Time-based Teapot/src/main.cxx
// Time-based animation example using SRL Timer DeltaTime
#include <srl.hpp>
#include <srl_timer.hpp>
#include "modelObject.hpp"

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Input;

// Main program entry
int main()
{
    // Initialize library
    SRL::Core::Initialize(HighColor(0x31, 0x14, 0x32));
    SRL::Debug::Print(1, 1, "VDP1 3D Time-based teapot");

    // Load teapot
    ModelObject teapot = ModelObject("FPOT.NYA");

    // Setup camera location
    Vector3D cameraLocation = Vector3D(0.0, -7.0, -40.0);

    // Setup light
    Vector3D lightDirection = Vector3D(0.2, 0.0, 0.2);
    SRL::Scene3D::SetDirectionalLight(lightDirection);

    // Initialize rotation angle using Angle type for BAM compatibility
    Angle rotation = Angle::Zero();

    // Rotation speed: 45 degrees per second (time-based, not frame-based!)
    // This will rotate at exactly 45°/second regardless of frame rate
    const Angle rotationSpeed = Angle::FromDegrees(45.0);

    // Time tracking
    uint32_t frameCount = 0;
    auto startTime = SRL::Timer::Capture();

    // Rotation direction switching every 1 minute
    const SRL::Tickstamp switchInterval = SRL::Tickstamp::FromMinutes<1.0f>();
    auto switchDeadline = startTime + switchInterval;
    bool rotateForward = true;

    // Main program loop
    while (1)
    {
        frameCount++;

        // Check if we've reached the rotation switch deadline
        // Use Capture() here because we need a fresh timestamp for deadline checking
        auto now = SRL::Timer::Capture();
        if (now >= switchDeadline)
        {
            // Switch rotation direction
            rotateForward = !rotateForward;
            // Set new deadline
            switchDeadline = now + switchInterval;
        }

        // Update rotation based on elapsed time (time-based animation)
        // This works at any frame rate - 30fps, 60fps, variable, etc.
        if (rotateForward)
            rotation += SRL::Timer::DeltaSeconds() * rotationSpeed.ToTurns();
        else
            rotation -= SRL::Timer::DeltaSeconds() * rotationSpeed.ToTurns();

        // Calculate elapsed time and clock display
        // Use CurrentTickstamp() here for display - no hardware read overhead
        // CurrentTickstamp() returns the timestamp captured by Update() at frame start
        auto elapsed = SRL::Timer::CurrentTickstamp() - startTime;
        auto clock = elapsed.ToClock();

        // Calculate ETA for next rotation switch (reuse 'now' from deadline check)
        auto timeUntilSwitch = switchDeadline - now;
        auto countdown = timeUntilSwitch.ToClock();

        Fxp fps = Fxp(0);
        if (SRL::Timer::DeltaSeconds() > Fxp(0))
            fps = Fxp(1) / SRL::Timer::DeltaSeconds();
        SRL::Debug::PrintClearLine(2);
        SRL::Debug::Print(1, 2, "Frames Per Second: %f", fps);
        SRL::Debug::PrintClearLine(3);
        SRL::Debug::Print(1, 3, "Total Frames: %u", frameCount);
        SRL::Debug::PrintClearLine(4);
        SRL::Debug::Print(1, 4, "Delta Milliseconds: %f", SRL::Timer::DeltaMilliseconds());
        SRL::Debug::PrintClearLine(5);
        SRL::Debug::Print(1, 5, "Delta Seconds: %f", SRL::Timer::DeltaSeconds());
        SRL::Debug::PrintClearLine(6);
        SRL::Debug::Print(1, 6, "Delta Minutes: %f", SRL::Timer::DeltaMinutes());
        SRL::Debug::PrintClearLine(7);
        SRL::Debug::Print(1, 7, "Total Milliseconds: %f", elapsed.ToMilliseconds());
        SRL::Debug::PrintClearLine(8);
        SRL::Debug::Print(1, 8, "Total Seconds: %f", elapsed.ToSeconds());
        SRL::Debug::PrintClearLine(9);
        SRL::Debug::Print(1, 9, "Total Minutes: %f", elapsed.ToMinutes());
        SRL::Debug::PrintClearLine(10);
        SRL::Debug::Print(1, 10, "Clock: %02u:%02u:%02u.%03u", clock.Hours(), clock.Minutes(), clock.Seconds(), clock.Milliseconds());
        SRL::Debug::PrintClearLine(22);
        SRL::Debug::Print(1, 22, "Rotation: %s", rotateForward ? "Forward" : "Reverse");
        SRL::Debug::PrintClearLine(23);
        SRL::Debug::Print(1, 23, "Next Switch ETA: %02u:%02u.%03u", countdown.Minutes(), countdown.Seconds(), countdown.Milliseconds());

        // Measure rendering time using Capture()
        // Use Capture() for benchmarking/profiling - reads hardware registers
        auto renderStart = SRL::Timer::Capture();

        // Load identity matrix
        SRL::Scene3D::LoadIdentity();

        // Set camera location and direction
        SRL::Scene3D::LookAt(cameraLocation, Vector3D(), Angle::FromDegrees(0.0));

        // Rotate teapot using time-based angle
        SRL::Scene3D::RotateY(rotation);

        // Draw teapot
        teapot.Draw();

        // Capture end time for rendering measurement
        auto renderEnd = SRL::Timer::Capture();
        auto renderTime = renderEnd - renderStart;
        SRL::Debug::PrintClearLine(24);
        SRL::Debug::Print(1, 24, "Render Time: %f ms", renderTime.ToMilliseconds());

        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

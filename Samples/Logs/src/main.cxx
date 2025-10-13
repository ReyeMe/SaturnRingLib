#include <srl.hpp>          // Main library header (includes Core, Debug, Cartridge, etc.)
#include <srl_log.hpp>      // Logging system
#include <srl_devcart.hpp>  // DevCart USB and CPLD access

// Using directives to shorten names for Vector and HighColor types
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;
using namespace SRL::DevCart;

// Main program entry point
// Note: On embedded systems like Saturn, main may be wrapped or replaced by CRT0.
// This appears to be a user-level entry after library init.
int main()
{
    // Sample variables for formatted logging
    const int32_t myNumber = 555;
    const char * myString = "666";

    // Log the configured minimum log level (compile-time)
    // Note: Log::LogPrint defaults to INFO; overrides output to DefaultLogger.
    Log::LogPrint("Current log level : %s", Log::LogLevelHelper(Log::MinLevel).ToString());

    // Pre-init log
    Log::LogPrint("Before Initialize");
    
    // Initialize the SRL core (graphics/video setup?)
    // HighColor(20,10,50) likely sets background color in high-color mode (5-5-5 RGB?).
    SRL::Core::Initialize(HighColor(20,10,50));
    
    // Print directly to debug overlay (bypasses logger)
    SRL::Debug::Print(1,1, "This is a text !");
    
    // Post-init log
    LogPrint("After Initialize");

    // Test various log levels via direct Log::LogPrint template
    // Assuming SRL_LOG_LEVEL=INFO, TRACE and TESTING should be compiled out.
    Log::LogPrint<LogLevels::TRACE>("I am a happy TRACE message, but I should not be displayed");
    Log::LogPrint<LogLevels::TESTING>("I am a happy DEBUG message, but I should not be displayed");
    Log::LogPrint<LogLevels::INFO>("I am a happy INFO message");
    Log::LogPrint<LogLevels::WARNING>("I am a happy WARNING message");
    Log::LogPrint<LogLevels::FATAL>("I am a happy FATAL message");

    // Formatted log with variables (FATAL level)
    Log::LogPrint<LogLevels::FATAL>("If you're %d, then I'm %s", myNumber, myString);

    // Convenience wrappers (template-based, level-fixed)
    // These should respect MinLevel filtering.
    LogTrace("I am another happy TRACE message, but I should not be displayed");
    LogInfo("I am another happy INFO message");
    LogDebug("I am another happy DEBUG message, but I should not be displayed");
    LogWarning("I am another happy WARNING message");
    LogFatal("I am another happy FATAL message");

    // Another formatted fatal log
    LogFatal("Here we go again, If you're %d, then I'm %s", myNumber, myString);

    // Detect inserted cartridge (RAM, Data, USB, etc.)
    SRL::Cartridge::CartridgeId cid = SRL::Cartridge::DetectCartridgeType();

    // Main program loop
    // Infinite loop for embedded; no OS to exit.
    while(1)
    {
        // Display detected cartridge type on debug overlay
        // SRL::Cartridge::GetStringFromType returns name string.
        SRL::Debug::Print(1,2, "Cartridge type detected: %s", SRL::Cartridge::GetStringFromType(cid));

        // Check if USB DevCart is connected (via flags)
        if (SRL::DevCart::CS0::isConnected())
        {
            static uint16_t counter = 0;
            SRL::Debug::Print(1,3, "DevCart Detected !");
            
            // Log directly to DevCartLogger (bypasses DefaultLogger)
            // Sends via USB FIFO; may block if full.
            Log::LogPrint<LogLevels::FATAL, DevCartLogger>("HELLO WORLD ! (%d)", counter++);
        }
        else
        {
            SRL::Debug::Print(1,3, "DevCart Not Detected !");
            
            // Read potential DataCartridge HWId from memory
            char tmp[32];
            memcpy(tmp, (char *)SRL::Cartridge::CartridgeData::Address, sizeof(tmp));
            tmp[sizeof(SRL::Cartridge::CartridgeData::HWId)] = '\0'; // Ensure null termination after HWId length
            
            // Display read HWId (e.g., "SEGA SEGASA")
            SRL::Debug::Print(1,4,"Detected HWId: %s", tmp);
        }

        // Synchronize with frame/video update (VBlank wait?)
        // Prevents screen tear and paces the loop.
        SRL::Core::Synchronize();
    }

    // Unreachable return; kept for C++ compliance
    return 0;
}
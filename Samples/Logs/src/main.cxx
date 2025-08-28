#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_devcart.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;
using namespace SRL::DevCart;

// Main program entry
int main()
{
	const int32_t myNumber = 555;
	const char * myString = "666";

	Log::LogPrint("Current log level : %s", Log::LogLevelHelper(Log::MinLevel).ToString());

	Log::LogPrint("Before Initialize");
	SRL::Core::Initialize(HighColor(20,10,50));
	SRL::Debug::Print(1,1, "This is a text !");
	LogPrint("After Initialize");

	Log::LogPrint<LogLevels::TRACE>("I am a happy TRACE message, but I should not be displayed");
	Log::LogPrint<LogLevels::TESTING>("I am a happy DEBUG message, but I should not be displayed");
	Log::LogPrint<LogLevels::INFO>("I am a happy INFO message");
	Log::LogPrint<LogLevels::WARNING>("I am a happy WARNING message");
	Log::LogPrint<LogLevels::FATAL>("I am a happy FATAL message");

	Log::LogPrint<LogLevels::FATAL>("If you're %d, then I'm %s", myNumber, myString);

	LogTrace("I am another happy TRACE message, but I should not be displayed");
	LogInfo("I am another happy INFO message");
	LogDebug("I am another happy DEBUG message, but I should not be displayed");
	LogWarning("I am another happy WARNING message");
	LogFatal("I am another happy FATAL message");

	LogFatal("Here we go again, If you're %d, then I'm %s", myNumber, myString);

	SRL::Cartridge::CartridgeId cid = SRL::Cartridge::DetectCartridgeType();

	// Main program loop
	while(1)
	{
		SRL::Core::Synchronize();

		SRL::Debug::Print(1,2, "Cartridge type detected: %s", SRL::Cartridge::GetStringFromType(cid));

		if (SRL::DevCart::CS0::isAvailable())
		{
			static uint16_t counter = 0;
			SRL::Debug::Print(1,3, "DevCart Detected !");
			Log::LogPrint<LogLevels::FATAL, DevCartLogger>("HELLO WORLD ! (%d)", counter++);
		}
		else
		{
			SRL::Debug::Print(1,3, "DevCart Not Detected !");
            char tmp[32];
            memcpy(tmp, (char *)SRL::Cartridge::CartridgeData::Address, sizeof(tmp));
			tmp[sizeof(SRL::Cartridge::CartridgeData::HWId)] = '\0'; // Ensure null termination
			SRL::Debug::Print(1,4,"Detected HWId: %s", tmp);
		}
	}

	return 0;
}

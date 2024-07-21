#include <srl.hpp>
#include <srl_log.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Logger;

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
	Log::LogPrint<LogLevels::DEBUG>("I am a happy DEBUG message, but I should not be displayed");
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

	// Main program loo
	while(1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

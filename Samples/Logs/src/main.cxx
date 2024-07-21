#include <srl.hpp>


// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Log;

// Main program entry
int main()
{
	const int32_t myNumber = 555;
	const char * myString = "666";

	SRL::Debug::Log("Before Initialize");
	SRL::Core::Initialize(HighColor(20,10,50));
	SRL::Debug::Print(1,1, "This is a text !");
	SRL::Debug::Log("After Initialize");

	SRL::Debug::Log<Levels::TRACE>("I am a happy TRACE message, but I should not be displayed");
	SRL::Debug::Log<Levels::INFO>("I am a happy INFO message");
	SRL::Debug::Log<Levels::WARNING>("I am a happy WARNING message");
	SRL::Debug::Log<Levels::FATAL>("I am a happy FATAL message");
	SRL::Debug::Log<Levels::DEBUG>("I am a happy DEBUG message");

	SRL::Debug::Log<Levels::FATAL>("If you're %d, then I'm %s", myNumber, myString);

	// Main program loo
	while(1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

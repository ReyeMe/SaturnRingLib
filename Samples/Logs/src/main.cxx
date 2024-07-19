#include <srl.hpp>


// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Log;

// Main program entry
int main()
{
	SRL::Debug::Log("Before Initialize");
	SRL::Core::Initialize(HighColor(20,10,50));
	SRL::Debug::Print(1,1, "This is a text !");
	SRL::Debug::Log("After Initialize");

	SRL::Debug::Log(Levels::TRACE, "I am a happy TRACE message");
	SRL::Debug::Log(Levels::INFO, "I am a happy INFO message");
	SRL::Debug::Log(Levels::WARNING, "I am a happy WARNING message");
	SRL::Debug::Log(Levels::FATAL, "I am a happy FATAL message");
	SRL::Debug::Log(Levels::DEBUG, "I am a happy DEBUG message");



	// Main program loo
	while(1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

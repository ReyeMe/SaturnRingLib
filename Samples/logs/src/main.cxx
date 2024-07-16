#include <srl.hpp>


// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
	SRL::Debug::Log("Before Initialize");
	SRL::Core::Initialize(HighColor(20,10,50));
	SRL::Debug::Print(1,1, "This is a text !");
	SRL::Debug::Log("After Initialize");

	// Main program loo
	while(1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

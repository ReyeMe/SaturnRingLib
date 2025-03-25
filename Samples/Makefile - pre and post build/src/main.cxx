#include <srl.hpp>

#include "tmp.h"

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;


// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
	SRL::Debug::Print(1,1, VERY_IMPORTANT_MESSAGE);

	// Main program loop
	while(1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

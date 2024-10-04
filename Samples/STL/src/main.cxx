#include <srl.hpp>

// STL headers
#include <iostream>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

using namespace std;


// Main program entry
int main()
{
  // Initialize library
  SRL::Core::Initialize(HighColor::Colors::Black);
  SRL::Debug::Print(1,1, "STL Sample");

  while(1)
  {


    // Refresh screen
    SRL::Core::Synchronize();
  }

  return 0;
}

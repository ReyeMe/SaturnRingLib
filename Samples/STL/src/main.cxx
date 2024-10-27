#include <srl.hpp>

// Project includes
#include "array.h"
#include "vector.h"
//#include "string.h"
#include "algorithm.h"

using namespace SRL::Types;

// Main program entry
int main()
{
  // Initialize library
  SRL::Core::Initialize(HighColor::Colors::Black);
  SRL::Debug::Print(1, 1, "STL samples");

  test_array();
  test_vector();
  //test_string();
  test_algorithm();

  while (1)
  {



    // Refresh screen
    SRL::Core::Synchronize();
  }

  return 0;
}

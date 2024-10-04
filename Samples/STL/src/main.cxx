#include <srl.hpp>

// STL array header
#include <array>

using namespace SRL::Types;

// Main program entry
int main()
{
  // Initialize library
  SRL::Core::Initialize(HighColor::Colors::Black);
  SRL::Debug::Print(1, 1, "STL array sample");

  std::array<int, 5> numbers = { 1, 2, 3, 4, 5 };

  // Access elements using the index operator
  for (size_t i = 0; i < numbers.size(); ++i)
  {
    SRL::Debug::Print(1, i, "Pos %d: value: %d;", i, numbers[i]);
  }

  while (1)
  {
    // Refresh screen
    SRL::Core::Synchronize();
  }

  return 0;
}

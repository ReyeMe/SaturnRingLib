#pragma once

// STL array header
#include <array>

static void test_array()
{
  std::array<int, 5> numbers = { 1, 2, 3, 4, 5 };

  for(auto i : numbers )
  {
    static auto cpt = 0;
    cpt += i;
  }
}

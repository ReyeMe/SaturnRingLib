#pragma once

// STL array header
#include <vector>

static void test_vector()
{

  std::vector<int> numbers = { 1, 2, 3, 4, 5 };
  static auto cpt = 0;

  for(auto i : numbers )
  {

    cpt += i;
  }

  numbers.push_back(cpt);

}

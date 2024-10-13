#pragma once

#include <algorithm>
#include <vector>
#include <numeric> //For accumulate operation

static void test_algorithm()
{
  // Initializing vector with array values
  int arr[] = {10, 20, 5, 23 ,42 , 15};
  int n = sizeof(arr)/sizeof(arr[0]);
  std::vector<int> vect(arr, arr+n);

  // Sorting the Vector in Ascending order
  std::sort(vect.begin(), vect.end());

  // Sorting the Vector in Descending order
  std::sort(vect.begin(),vect.end(), std::greater<int>());

  // Reversing the Vector (descending to ascending , ascending to descending)
  std::reverse(vect.begin(), vect.end());
}

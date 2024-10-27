#pragma once

// STL array header
#include <string>

static void test_string()
{
  std::string txt = "this is a test";

  const char * c = txt.c_char();

  c[0] = ' ';
}

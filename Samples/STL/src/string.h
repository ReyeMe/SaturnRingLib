#pragma once

// STL array header
#include <string>
#include <string_view>

static void test_string()
{
  std::string txt = "this is a test";
  std::string_view sv { txt };

  txt += sv;

}

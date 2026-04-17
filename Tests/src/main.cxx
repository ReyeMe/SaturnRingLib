// Tests/src/main.cxx
#include <stddef.h>
#include <stdint.h>
#include <vector>
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

#include "testsASCII.hpp"
#include "testsAngle.hpp"
// #include "testsEulerAngles.hpp" // Include the header for Euler angles tests
#include "testsCD.hpp"
#include "testsCRAM.hpp"
#include "testsFxp.hpp"
#include "testsHighColor.hpp"
#include "testsMath.hpp"
#include "testsMat33.hpp"        // Include the header for Matrix33 tests
#include "testsMat43.hpp"        // Include the header for Matrix43 tests
#include "testsPlane.hpp"        // Include the header for Plane tests
#include "testsSphere.hpp"       // Include the header for Sphere tests
#include "testsCollision.hpp"    // Include the header for Collision tests
#include "testsFrustum.hpp"      // Include the header for Frustum tests
#include "testsMemory.hpp"        // Include the header for memory tests
#include "testsBase.hpp"          // Include the header for SGL tests
#include "testsBitmap.hpp"        // Include the header for bitmap tests
#include "testsAABB.hpp"          // Include the header for AABB tests
#include "testsVector2D.hpp"      // Include the header for Vector2D tests
#include "testsVector3D.hpp"      // Include the header for Vector3D tests
#include "testsMatrixStack.hpp"   // Include the header for MatrixStack tests
#include "testsPrecision.hpp"     // Include the header for Precision tests
#include "testsRandom.hpp"        // Include the header for Random tests
#include "testsSortOrder.hpp"     // Include the header for SortOrder tests
#include "testsTrigonometry.hpp"  // Include the header for Trigonometry tests
#include "testsUtils.hpp"         // Include the header for Utils tests
#include "testsMemoryHWRam.hpp"   // Include the header for memory HWRam tests
#include "testsMemoryLWRam.hpp"   // Include the header for memory LWRam tests
#include "testsMemoryCartRam.hpp" // Include the header for memory Cart Ram tests
#include "testsString.hpp"        // Include the header for string tests
#include "testsSystem.hpp"        // Include the header for system tests
#include "testsInterrupt.hpp"        // Include the header for vector tests
#include "testsTimer.hpp"        // Include the header for vector tests

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

namespace
{
  constexpr size_t kBufferSize = 255;
  constexpr uint8_t kDisplayColumns = 44;
  constexpr uint8_t kDisplayLines = 27;
  char results_buffer[kBufferSize] = {};

  struct ResultLine
  {
    char text[kBufferSize];
  };

  std::vector<ResultLine> g_results;

  void AppendChar(char *dst, const size_t size, size_t &pos, const char ch)
  {
    if (pos + 1 < size)
    {
      dst[pos] = ch;
    }
    ++pos;
  }

  void AppendStr(char *dst, const size_t size, size_t &pos, const char *src)
  {
    if (!src)
    {
      return;
    }

    for (size_t i = 0; src[i] != '\0'; ++i)
    {
      AppendChar(dst, size, pos, src[i]);
    }
  }

  void AppendUnsigned(char *dst, const size_t size, size_t &pos, unsigned int value)
  {
    char tmp[10];
    size_t tmp_len = 0;

    if (value == 0)
    {
      tmp[tmp_len++] = '0';
    }
    else
    {
      while (value > 0 && tmp_len < sizeof(tmp))
      {
        tmp[tmp_len++] = static_cast<char>('0' + (value % 10));
        value /= 10;
      }
    }

    for (size_t i = 0; i < tmp_len; ++i)
    {
      AppendChar(dst, size, pos, tmp[tmp_len - 1 - i]);
    }
  }

  void FinalizeBuffer(char *dst, const size_t size, const size_t pos)
  {
    if (size == 0)
    {
      return;
    }

    const size_t write_pos = (pos < size) ? pos : (size - 1);
    dst[write_pos] = '\0';
  }

  void BuildSuiteLine(char *out, const size_t size, const char *suite_name, const int failures)
  {
    if (!out || size == 0)
    {
      return;
    }

    size_t pos = 0;
    const char *name = suite_name ? suite_name : "";

    for (size_t i = 0; i < 20 && name[i] != '\0'; ++i)
    {
      AppendChar(out, size, pos, name[i]);
    }

    if (failures)
    {
      AppendStr(out, size, pos, " : ");
      AppendUnsigned(out, size, pos, static_cast<unsigned int>(failures));
      AppendStr(out, size, pos, " failures");
    }
    else
    {
      AppendStr(out, size, pos, " SUCCESS !");
    }

    FinalizeBuffer(out, size, pos);
  }

  void BuildStatsLine(char *out, const size_t size, const unsigned int tests,
                      const unsigned int assertions, const unsigned int failures)
  {
    if (!out || size == 0)
    {
      return;
    }

    size_t pos = 0;
    AppendUnsigned(out, size, pos, tests);
    AppendStr(out, size, pos, " tests, ");
    AppendUnsigned(out, size, pos, assertions);
    AppendStr(out, size, pos, " assertions, ");
    AppendUnsigned(out, size, pos, failures);
    AppendStr(out, size, pos, " failures");
    FinalizeBuffer(out, size, pos);
  }

  void PushResultLine(const char *text)
  {
    ResultLine line = {};
    size_t pos = 0;
    AppendStr(line.text, kBufferSize, pos, text ? text : "");
    FinalizeBuffer(line.text, kBufferSize, pos);
    g_results.push_back(line);
  }

  void RenderResults(const size_t start_index)
  {
    char line_buffer[kDisplayColumns + 1];
    for (uint8_t i = 0; i < kDisplayLines; ++i)
    {
      const size_t line_index = start_index + i;
      const char *src = (line_index < g_results.size()) ? g_results[line_index].text : "";
      for (uint8_t col = 0; col < kDisplayColumns; ++col)
      {
        line_buffer[col] = ' ';
      }

      uint8_t col = 0;
      while (src[col] != '\0' && col < kDisplayColumns)
      {
        line_buffer[col] = src[col];
        ++col;
      }

      line_buffer[kDisplayColumns] = '\0';
      ASCII::Print(line_buffer, 0, i);
    }
  }

  void UpdateDisplay(size_t &start_index)
  {
    if (g_results.size() > kDisplayLines)
    {
      start_index = g_results.size() - kDisplayLines;
    }

    RenderResults(start_index);
    SRL::Core::Synchronize();
  }
} // namespace

extern "C"
{
  const uint8_t buffer_size = 255;
  char buffer[buffer_size] = {};
}

// Define a macro to capture test suite results
#define MU_DISPLAY_SATURN(suite_name)                 \
  BuildSuiteLine(results_buffer, kBufferSize,         \
                 #suite_name, suite_error_counter);   \
  PushResultLine(results_buffer);

#define RUN_AND_DISPLAY_SUITE(suite) \
  MU_RUN_SUITE(suite);               \
  MU_DISPLAY_SATURN(suite);          \
  UpdateDisplay(start_index);

// Define tags for test start and end
const char *const strStart = "***UT_START***";
const char *const strEnd = "***UT_END***";

/**
 * Main program entry
 *
 * This function initializes the SRL core, runs various test suites,
 * and displays the results.
 *
 * @return int
 */
int main()
{
  SRL::Input::Digital pad(0);
  size_t start_index = 0;

  // Initialize SRL core with a high color
  SRL::Core::Initialize(HighColor(20, 10, 50));
  ASCII::Clear();

  // Tag the beginning of the tests
  LogInfo(strStart);

  PushResultLine(strStart);

  RUN_AND_DISPLAY_SUITE(aabb_test_suite);
  RUN_AND_DISPLAY_SUITE(angle_test_suite);
  RUN_AND_DISPLAY_SUITE(ascii_test_suite);

  // Run angle test suite
  RUN_AND_DISPLAY_SUITE(angle_test_suite);

  // Run CD test suite
  RUN_AND_DISPLAY_SUITE(cd_test_suite);

  // Run CRAM test suite
  RUN_AND_DISPLAY_SUITE(cram_test_suite);

  // Run FXP test suite
  RUN_AND_DISPLAY_SUITE(fxp_test_suite);

  // Run HighColor test suite
  RUN_AND_DISPLAY_SUITE(highcolor_test_suite);

  // Run Math test suite
  RUN_AND_DISPLAY_SUITE(math_test_suite);

  // Run Memory test suite
  RUN_AND_DISPLAY_SUITE(memory_test_suite);

  // Run Base test suite (SGL)
  RUN_AND_DISPLAY_SUITE(base_test_suite);

  // Run Bitmap test suite
  RUN_AND_DISPLAY_SUITE(bitmap_test_suite);

  // Run Memory HWRam test suite
  RUN_AND_DISPLAY_SUITE(memory_HWRam_test_suite);
  RUN_AND_DISPLAY_SUITE(memory_LWRam_test_suite);

  // Run Memory CartRam test suite
  RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

  // Run Interrupt test suite
  RUN_AND_DISPLAY_SUITE(interrupt_test_suite);

  // Run System test suite
  RUN_AND_DISPLAY_SUITE(system_test_suite);

  // Run Timer test suite
  RUN_AND_DISPLAY_SUITE(test_timer_suite);

  // Generate tests report
  MU_REPORT();

  // Display test statistics
  BuildStatsLine(results_buffer, kBufferSize,
                 static_cast<unsigned int>(minunit_run),
                 static_cast<unsigned int>(minunit_assert),
                 static_cast<unsigned int>(minunit_fail));

  PushResultLine(results_buffer);

  // Tag the end of the tests
  LogInfo(strEnd);
  PushResultLine(strEnd);

  if (g_results.size() > kDisplayLines)
  {
    start_index = g_results.size() - kDisplayLines;
  }

  RenderResults(start_index);

  // Main program loop
  uint8_t up_hold_frames = 0;
  uint8_t down_hold_frames = 0;
  const uint8_t repeat_delay = 20;
  const uint8_t repeat_rate = 3;

  while (1)
  {
    SRL::Core::Synchronize();
    bool refresh = false;

    const bool up_held = pad.IsHeld(SRL::Input::Digital::Button::Up);
    const bool down_held = pad.IsHeld(SRL::Input::Digital::Button::Down);

    if (up_held && !down_held)
    {
      if (up_hold_frames < 255)
      {
        ++up_hold_frames;
      }
      down_hold_frames = 0;
    }
    else if (down_held && !up_held)
    {
      if (down_hold_frames < 255)
      {
        ++down_hold_frames;
      }
      up_hold_frames = 0;
    }
    else
    {
      up_hold_frames = 0;
      down_hold_frames = 0;
    }

    if (pad.WasPressed(SRL::Input::Digital::Button::Up))
    {
      if (start_index > 0)
      {
        --start_index;
        refresh = true;
      }
    }
    else if (up_held && up_hold_frames > repeat_delay &&
             ((up_hold_frames - repeat_delay) % repeat_rate == 0))
    {
      if (start_index > 0)
      {
        --start_index;
        refresh = true;
      }
    }

    if (pad.WasPressed(SRL::Input::Digital::Button::Down))
    {
      if (start_index + kDisplayLines < g_results.size())
      {
        ++start_index;
        refresh = true;
      }
    }
    else if (down_held && down_hold_frames > repeat_delay &&
             ((down_hold_frames - repeat_delay) % repeat_rate == 0))
    {
      if (start_index + kDisplayLines < g_results.size())
      {
        ++start_index;
        refresh = true;
      }
    }

    if (refresh)
    {
      RenderResults(start_index);
    }

  }

  return 0;
}
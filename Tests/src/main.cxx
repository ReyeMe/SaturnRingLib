// Tests/src/main.cxx
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
#include "testDSP.hpp"            // Include the header for DSP tests
#include "testsSystem.hpp"        // Include the header for System tests
#include "testsInterrupt.hpp"     // Include the header for Interrupt tests

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

// Define a macro to display test suite results
#define MU_DISPLAY_SATURN(suite_name)           \
  memset(buffer, 0, buffer_size);               \
  if (suite_error_counter)                      \
  {                                             \
    snprintf(buffer, buffer_size,               \
             "%.20s : %d failures",             \
             #suite_name, suite_error_counter); \
  }                                             \
  else                                          \
  {                                             \
    snprintf(buffer, buffer_size,               \
             "%.20s SUCCESS !",                 \
             #suite_name);                      \
  }                                             \
  ASCII::Print(buffer, 0, line++);

#define RUN_AND_DISPLAY_SUITE(suite) \
  MU_RUN_SUITE(suite);               \
  MU_DISPLAY_SATURN(suite);

extern "C"
{
  const uint8_t buffer_size = 255;
  char buffer[buffer_size] = {};
}

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
  uint8_t line = 0;

  // Initialize SRL core with a high color
  SRL::Core::Initialize(HighColor(20, 10, 50));

  // Tag the beginning of the tests
  LogInfo(strStart);

  // Print the start tag on the screen
  ASCII::Print(strStart, 0, line++);

  // Run ASCII test suite
  RUN_AND_DISPLAY_SUITE(ascii_test_suite);

  // Run angle test suite
  RUN_AND_DISPLAY_SUITE(angle_test_suite);

  // // Run CD test suite
  RUN_AND_DISPLAY_SUITE(cd_test_suite);

  // // Run CRAM test suite
  RUN_AND_DISPLAY_SUITE(cram_test_suite);

  // // Run FXP test suite
  RUN_AND_DISPLAY_SUITE(fxp_test_suite);

  // // Run HighColor test suite
  RUN_AND_DISPLAY_SUITE(highcolor_test_suite);

  // // Run Math test suite
  RUN_AND_DISPLAY_SUITE(math_test_suite);

  // Run Matrix33 test suite
  RUN_AND_DISPLAY_SUITE(mat33_test_suite);

  // Run Matrix43 test suite
  RUN_AND_DISPLAY_SUITE(mat43_test_suite);

  // Run Plane test suite
  RUN_AND_DISPLAY_SUITE(plane_test_suite);

  // Run Sphere test suite
  RUN_AND_DISPLAY_SUITE(sphere_test_suite);

  // Run Collision test suite
  RUN_AND_DISPLAY_SUITE(collision_test_suite);

  // Run Frustum test suite
  RUN_AND_DISPLAY_SUITE(frustum_test_suite);

  // Run Vector2D test suite
  RUN_AND_DISPLAY_SUITE(vector2d_test_suite);

  // Run Vector3D test suite
  RUN_AND_DISPLAY_SUITE(vector3d_test_suite);

  // Run MatrixStack test suite
  RUN_AND_DISPLAY_SUITE(matrix_stack_test_suite);

  // Run Precision test suite
  RUN_AND_DISPLAY_SUITE(precision_test_suite);

  // Run SortOrder test suite
  RUN_AND_DISPLAY_SUITE(sort_order_test_suite);

  // Run Random test suite
  RUN_AND_DISPLAY_SUITE(random_test_suite);

  // Run Trigonometry test suite
  RUN_AND_DISPLAY_SUITE(trigonometry_test_suite);

  // Run Utils test suite
  RUN_AND_DISPLAY_SUITE(utils_test_suite);

  // Run AABB test suite
  RUN_AND_DISPLAY_SUITE(aabb_test_suite);

  // // Run Memory test suite
  RUN_AND_DISPLAY_SUITE(memory_test_suite);

  // Run Base test suite (SGL)
  RUN_AND_DISPLAY_SUITE(base_test_suite);

  // // Run Bitmap test suite
  RUN_AND_DISPLAY_SUITE(bitmap_test_suite);

  // // Run Memory HWRam test suite
  RUN_AND_DISPLAY_SUITE(memory_HWRam_test_suite);

  // Run Memory LWRam test suite
  RUN_AND_DISPLAY_SUITE(memory_LWRam_test_suite);

  // // Run Memory CartRam test suite
  RUN_AND_DISPLAY_SUITE(memory_CartRam_test_suite);

  // Run DSP test suite
  RUN_AND_DISPLAY_SUITE(dsp_test_suite);

  // Run System test suite
  RUN_AND_DISPLAY_SUITE(system_test_suite);

  // Run Interrupt test suite
  RUN_AND_DISPLAY_SUITE(interrupt_test_suite);

  // // Generate tests report
  MU_REPORT();

  // Display test statistics
  snprintf(buffer, buffer_size,
           "%d tests, %d assertions, %d failures",
           minunit_run, minunit_assert, minunit_fail);

  ASCII::Print(buffer, 0, line + 2);

  // Tag the end of the tests
  LogInfo(strEnd);

  // Main program loop
  while (1)
  {
    // Synchronize SRL core
    SRL::Core::Synchronize();
  }

  return 0;
}
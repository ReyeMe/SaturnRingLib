// Tests/src/main.cxx
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

#include "testsASCII.hpp"
#include "testsAngle.hpp"
//#include "testsEulerAngles.hpp" // Include the header for Euler angles tests
#include "testsCD.hpp"
#include "testsCRAM.hpp"
#include "testsFxp.hpp"
#include "testsHighColor.hpp"
#include "testsMath.hpp"
#include "testsMemory.hpp" // Include the header for memory tests
#include "testsBase.hpp" // Include the header for SGL tests
#include "testsBitmap.hpp" // Include the header for bitmap tests
#include "testsMemoryHWRam.hpp" // Include the header for memory HWRam tests
#include "testsMemoryLWRam.hpp" // Include the header for memory LWRam tests
#include "testsMemoryCartRam.hpp" // Include the header for memory Cart Ram tests
#include "testsString.hpp" // Include the header for string tests

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

// Define a macro to display test suite results
#define MU_DISPLAY_SATURN(suite_name) \
  memset(buffer, 0, buffer_size); \
  if(suite_error_counter) \
  { \
    snprintf(buffer, buffer_size, \
             "%.20s : %d failures", \
             #suite_name, suite_error_counter); \
  } \
  else \
  { \
    snprintf(buffer, buffer_size, \
             "%.20s SUCCESS !", \
             #suite_name); \
  } \
  ASCII::Print(buffer,0, line++);

extern "C"
{
  const uint8_t buffer_size = 255;
  char buffer[buffer_size] = {};
}

// Define tags for test start and end
const char * const strStart = "***UT_START***"; 
const char * const strEnd = "***UT_END***"; 

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
  uint8_t line =0;

  // Initialize SRL core with a high color
  SRL::Core::Initialize(HighColor(20,10,50));

  // Tag the beginning of the tests
  LogInfo(strStart);

  // Print the start tag on the screen
  ASCII::Print(strStart,0, line++);

  // Run ASCII test suite
  MU_RUN_SUITE(ascii_test_suite);
  MU_DISPLAY_SATURN(ascii_test_suite);

  // Run angle test suite
  MU_RUN_SUITE(angle_test_suite);
  MU_DISPLAY_SATURN(angle_test_suite);

  // Run CD test suite
  MU_RUN_SUITE(cd_test_suite);
  MU_DISPLAY_SATURN(cd_test_suite);
    
  // Run CRAM test suite
  MU_RUN_SUITE(cram_test_suite);
  MU_DISPLAY_SATURN(cram_test_suite);

  // Run FXP test suite
  MU_RUN_SUITE(fxp_test_suite);
  MU_DISPLAY_SATURN(fxp_test_suite);
    
  // Run HighColor test suite
  MU_RUN_SUITE(highcolor_test_suite);
  MU_DISPLAY_SATURN(highcolor_test_suite);

  // Run Math test suite
  MU_RUN_SUITE(math_test_suite);
  MU_DISPLAY_SATURN(math_test_suite);

  // Run Memory test suite
  MU_RUN_SUITE(memory_test_suite); 
  MU_DISPLAY_SATURN(memory_test_suite);

  // Run Base test suite (SGL)
  MU_RUN_SUITE(base_test_suite); 
  MU_DISPLAY_SATURN(base_test_suite);

  // Run Bitmap test suite
  MU_RUN_SUITE(bitmap_test_suite); 
  MU_DISPLAY_SATURN(bitmap_test_suite);

  // Run Memory HWRam test suite
  MU_RUN_SUITE(memory_HWRam_test_suite); 
  MU_DISPLAY_SATURN(memory_HWRam_test_suite);

  // Run Memory LWRam test suite
  MU_RUN_SUITE(memory_LWRam_test_suite); 
  MU_DISPLAY_SATURN(memory_LWRam_test_suite);

  // Run Memory CartRam test suite
  MU_RUN_SUITE(memory_CartRam_test_suite); 
  MU_DISPLAY_SATURN(memory_CartRam_test_suite);

  // Run String test suite
  MU_RUN_SUITE(string_test_suite);
  MU_DISPLAY_SATURN(string_test_suite);

  // Generate tests report
  MU_REPORT();

  // Display test statistics
  snprintf(buffer, buffer_size,
           "%d tests, %d assertions, %d failures",
           minunit_run, minunit_assert, minunit_fail); 
	
  ASCII::Print(buffer,0, line+2);

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
#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

// #include "testsASCII.hpp"
// #include "testsAngle.hpp"
//#include "testsEulerAngles.hpp" // Include the header for Euler angles tests
#include "testsCD.hpp"
// #include "testsCRAM.hpp"
// #include "testsFxp.hpp"
// #include "testsHighColor.hpp"
// #include "testsMath.hpp"
// #include "testsMemory.hpp" // Include the header for memory tests
// #include "testsBase.hpp" // Include the header for SGL tests
// #include "testsBitmap.hpp" // Include the header for bitmap tests

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Logger;

extern "C"
{
	const uint8_t buffer_size = 255;
	char buffer[buffer_size] = {};
}

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20, 10, 50));

	// Tag the begining of the tests, will be used from script to detect the begin of
	//	the tests and filter out  everything above
	LogInfo("***UT_START***");

	// MU_RUN_SUITE(ascii_test_suite);
	// MU_RUN_SUITE(angle_test_suite);
	//MU_RUN_SUITE(euler_angles_test_suite); // Add the Euler angles test suite
	MU_RUN_SUITE(cd_test_suite);
	// MU_RUN_SUITE(cram_test_suite);
	//MU_RUN_SUITE(fxp_test_suite);
	// MU_RUN_SUITE(highcolor_test_suite);
	// MU_RUN_SUITE(math_test_suite);
	// MU_RUN_SUITE(memory_test_suite); // Add the memory test suite
	// MU_RUN_SUITE(base_test_suite); // Add the SGL test suite
	// MU_RUN_SUITE(bitmap_test_suite); // Add the bitmap test suite

	// Generate tests report
	MU_REPORT();

	// Tag the end of the tests, will be used from script to detect the end of
	//	the tests and filter out everything below
	LogInfo("***UT_END***");

	// Main program loop, nothing special here, the running script shall
	//  detects the end of the test procedure and close the emulator
	while (1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

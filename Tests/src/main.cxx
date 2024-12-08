#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

#include "testsASCII.h"
#include "testsAngle.h"
#include "testsCD.h"
#include "testsCRAM.h"
#include "testsFxp.h"
#include "testsHighColor.h"
#include "testsMath.h"

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
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

	LogInfo("***UT_START***");
	MU_RUN_SUITE(ascii_test_suite);
	MU_RUN_SUITE(angle_test_suite);
	MU_RUN_SUITE(cd_test_suite);
	MU_RUN_SUITE(cram_test_suite);
	MU_RUN_SUITE(fxp_test_suite);
	MU_RUN_SUITE(highcolor_test_suite);
    MU_RUN_SUITE(math_test_suite);
	MU_REPORT();
	LogInfo("***UT_END***");

	// Main program loop
	while (1)
	{
		SRL::Core::Synchronize();
	}

	return 0;
}

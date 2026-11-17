#include <srl.hpp>
#include <srl_log.hpp>
#include "srl_base.hpp"

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {

extern const uint8_t buffer_size;
extern char buffer[];

    /**
 * @brief Set up routine for SGL unit tests
 *
 * This function is called before each test in the SGL test suite.
 * Currently, it does not perform any specific setup operations,
 * but provides a hook for future initialization requirements.
 */
void sgl_test_setup(void)
{
        // Placeholder for any necessary test initialization
        // Future implementations might include resetting SGL state,
        // clearing buffers, or preparing test environments
}

    /**
 * @brief Tear down routine for SGL unit tests
 *
 * This function is called after each test in the SGL test suite.
 * Currently, it does not perform any specific cleanup operations,
 * but provides a hook for future resource release or state reset.
 */
void sgl_test_teardown(void)
{
        // Placeholder for any necessary test cleanup
        // Future implementations might include freeing resources,
        // resetting global state, or clearing temporary data
}

    /**
 * @brief Output header for test suite error reporting
 *
 * This function is called on the first test failure to print
 * a header indicating that SGL unit test errors have occurred.
 * It increments a global error counter to ensure the header
 * is printed only once per test suite run.
 */
void sgl_test_output_header(void)
{
        // Print error header only on the first test failure
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_SGL****");
        }
        else
        {
            LogInfo("****UT_SGL_ERROR(S)****");
        }
    }
}

    /**
 * @brief Tests the `SglType` wrapper, used as SRL types use it (`struct Polygon : SglType<Polygon, POLYGON>`).
 * @details SglPtr() and SglConstPtr() must point at the object itself, so SGL reads the C++ object's fields.
 */
struct SglTypeTestSgl
{
    int value;
};

struct SglTypeTestClass : public SRL::SGL::SglType<SglTypeTestClass, SglTypeTestSgl>
{
    int value;
};

MU_TEST(sgl_test_sgltype)
{
    // Pointers only: reading the fields through both types breaks strict aliasing (the build uses -O2)
    SglTypeTestClass object;
    const SglTypeTestClass &constObject = object;

    mu_assert(static_cast<void*>(object.SglPtr()) == static_cast<void*>(&object), "SglPtr() does not point at the object");
    mu_assert(static_cast<const void*>(constObject.SglConstPtr()) == static_cast<const void*>(&object), "SglConstPtr() does not point at the object");
}

    /**
 * @brief SGL test suite configuration and test case registration
 *
 * Configures the test suite with setup, teardown, and error reporting functions.
 * Registers individual test cases to be executed during the test run.
 * Currently only runs the SglType test.
 */
MU_TEST_SUITE(base_test_suite)
{
        // Configure test suite with setup, teardown, and error reporting functions
    MU_SUITE_CONFIGURE_WITH_HEADER(&sgl_test_setup,
        &sgl_test_teardown,
        &sgl_test_output_header);

        // Register test cases to be executed
    MU_RUN_TEST(sgl_test_sgltype);
}
}

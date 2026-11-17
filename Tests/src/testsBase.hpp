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
 * Registers individual test cases to be executed during the test run.
 * Currently only runs the SglType test.
 */
MU_TEST_SUITE(base_test_suite)
{
        // Register test cases to be executed
    MU_RUN_TEST(sgl_test_sgltype);
}
}

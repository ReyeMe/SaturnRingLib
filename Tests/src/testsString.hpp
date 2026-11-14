// Tests/src/testsString.hpp
// Include necessary headers for the SRL library, logging, and bitmap functionality
#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_bitmap.hpp> // for IBitmap interface

// Include the minunit testing framework from GitHub
// https://github.com/siu/minunit
#include "minunit.h"

// Use SRL and Logger namespaces to avoid repetitive qualification
using namespace SRL;
using namespace SRL::Logger;

// C linkage for compatibility with C-based testing framework
extern "C" {
    // External declarations for global variables used in testing
extern const uint8_t buffer_size; // Size of the test buffer
extern char buffer[]; // Test buffer for string operations
extern uint32_t suite_error_counter; // Counter for tracking test suite errors

    /**
 * @brief Setup function called before each test to initialize the environment.
 *
 * This function is used to perform any necessary initialization before each test case.
 */
void string_test_setup(void)
{
        // Initialization logic, if necessary (currently empty)
}

    /**
 * @brief Teardown function called after each test to clean up resources.
 *
 * This function is used to perform any necessary cleanup after each test case.
 */
void string_test_teardown(void)
{
        // Cleanup logic to reset the ASCII display state
    ASCII::Clear(); // Clear the ASCII display
    ASCII::SetPalette(0); // Reset the palette to default
}

    /**
 * @brief Output header function called on the first test failure to log the suite status.
 *
 * This function is used to log the test suite status when the first test failure occurs.
 */
void string_test_output_header(void)
{
        // Increment error counter and check if this is the first failure
    if (!suite_error_counter++)
    {
            // Log based on the current log level
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_STRING****"); // Log test suite start in debug mode
        }
        else
        {
            LogInfo("****UT_STRING_ERROR(S)****"); // Log error header in info mode
        }
    }
}

    /**
 * @brief Test case: Verify default constructor creates an empty string.
 *
 * This test case checks if the default constructor of the SRL::string class correctly creates an empty string.
 */
MU_TEST(string_test_default_constructor)
{
    SRL::string str; // Create a default-constructed string
    mu_assert(str.c_str() == nullptr, "Default constructor failed"); // Check if string is null
}

    /**
 * @brief Test case: Verify constructor with C-string source.
 *
 * This test case checks if the constructor of the SRL::string class correctly constructs a string from a C-string source.
 */
MU_TEST(string_test_constructor_with_src)
{
    const char *src = "Hello, World!"; // Source string for testing
    SRL::string str(src); // Construct string with source
    mu_assert(strcmp(str.c_str(), src) == 0, "Constructor with src failed"); // Compare content
}

    /**
 * @brief Test case: Verify constructor with format string and arguments.
 *
 * This test case checks if the constructor of the SRL::string class correctly constructs a string from a format string and arguments.
 */
MU_TEST(string_test_constructor_with_format)
{
    const char *format = "%s %d"; // Format string
    const char *str1 = "Hello"; // String argument
    int num = 42; // Integer argument
    SRL::string str(format, str1, num); // Construct with format
    snprintf(buffer, buffer_size, "Constructor with format gave '%s', expected 'Hello 42'", str.c_str());
    mu_assert(strcmp(str.c_str(), "Hello 42") == 0, buffer); // Verify result
}

    /**
 * @brief Test case: Verify constructor with integer argument.
 *
 * This test case checks if the constructor of the SRL::string class correctly constructs a string from an integer argument.
 */
MU_TEST(string_test_constructor_with_integer)
{
    int num = 42; // Integer input
    SRL::string str(num); // Construct string from integer
    mu_assert(strcmp(str.c_str(), "42") == 0, "Constructor with integer failed"); // Verify string representation
}

    /**
 * @brief Test case: Verify copy constructor.
 *
 * This test case checks if the copy constructor of the SRL::string class correctly copies the content of another string.
 */
MU_TEST(string_test_copy_constructor)
{
    SRL::string str1("Hello, World!"); // Source string
    SRL::string str2(str1); // Copy construct
    mu_assert(strcmp(str2.c_str(), str1.c_str()) == 0, "Copy constructor failed"); // Verify content equality
}

    /**
 * @brief Test case: Verify copy assignment operator.
 *
 * This test case checks if the copy assignment operator of the SRL::string class correctly copies the content of another string.
 */
MU_TEST(string_test_copy_assignment_operator)
{
    SRL::string str1("Hello, World!"); // Source string
    SRL::string str2; // Default-constructed string
    str2 = str1; // Copy assign
    mu_assert(strcmp(str2.c_str(), str1.c_str()) == 0, "Copy assignment operator failed"); // Verify content equality
}

    /**
 * @brief Test case: Verify move constructor.
 *
 * This test case checks if the move constructor of the SRL::string class correctly moves the content of another string.
 */
MU_TEST(string_test_move_constructor)
{
    SRL::string str1("Hello, World!"); // Source string
    SRL::string str2(std::move(str1)); // Move construct
    mu_assert(str1.c_str() == nullptr, "Move constructor failed"); // Source should be null
    mu_assert(strcmp(str2.c_str(), "Hello, World!") == 0, "Move constructor failed"); // Verify moved content
}

    /**
 * @brief Test case: Verify move assignment operator.
 *
 * This test case checks if the move assignment operator of the SRL::string class correctly moves the content of another string.
 */
MU_TEST(string_test_move_assignment_operator)
{
    SRL::string str1("Hello, World!"); // Source string
    SRL::string str2; // Default-constructed string
    str2 = std::move(str1); // Move assign
    mu_assert(str1.c_str() == nullptr, "Move assignment operator failed"); // Source should be null
    mu_assert(strcmp(str2.c_str(), "Hello, World!") == 0, "Move assignment operator failed"); // Verify moved content
}

    /**
 * @brief Test case: Verify string concatenation.
 *
 * This test case checks if the string concatenation operator of the SRL::string class correctly concatenates two strings.
 */
MU_TEST(string_test_concat)
{
    SRL::string str1("Hello, "); // First string
    SRL::string str2("World!"); // Second string
    SRL::string str3 = str1 + str2; // Concatenate
    mu_assert(strcmp(str3.c_str(), "Hello, World!") == 0, "Concat failed"); // Verify result
}

    /**
 * @brief Test case: Verify c_str() method returns correct string.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation of the string.
 */
MU_TEST(string_test_c_str)
{
    SRL::string str("Hello, World!"); // Test string
    mu_assert(strcmp(str.c_str(), "Hello, World!") == 0, "c_str failed"); // Verify content
}

    /**
 * @brief Test case: Verify c_str() for default-constructed string (null).
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns nullptr for a default-constructed string.
 */
MU_TEST(string_test_c_str_null)
{
    SRL::string str; // Default-constructed string
    mu_assert(str.c_str() == nullptr, "c_str null failed"); // Verify null
}

    /**
 * @brief Test case: Verify c_str() for empty string.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns an empty string for an empty string object.
 */
MU_TEST(string_test_c_str_empty)
{
    SRL::string str(""); // Empty string
    mu_assert(strcmp(str.c_str(), "") == 0, "c_str empty failed"); // Verify empty string
}

    /**
 * @brief Test case: Verify c_str() for single-character string.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation of a single-character string.
 */
MU_TEST(string_test_c_str_single_char)
{
    SRL::string str("a"); // Single-character string
    mu_assert(strcmp(str.c_str(), "a") == 0, "c_str single char failed"); // Verify content
}

    /**
 * @brief Test case: Verify c_str() for long string.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation of a long string.
 */
MU_TEST(string_test_c_str_long_string)
{
    const char *longStr = "This is a very long string that should not cause any issues"; // Long string
    SRL::string str(longStr); // Construct string
    mu_assert(strcmp(str.c_str(), longStr) == 0, "c_str long string failed"); // Verify content
}

    /**
 * @brief Test case: Verify c_str() after string modification.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation after string modification.
 */
MU_TEST(string_test_c_str_after_modification)
{
    SRL::string str("Hello"); // Initial string
    str = str + " World!"; // Modify by concatenation
    mu_assert(strcmp(str.c_str(), "Hello World!") == 0, "c_str after modification failed"); // Verify result
}

    /**
 * @brief Test case: Verify c_str() after multiple assignments.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation after multiple assignments.
 */
MU_TEST(string_test_c_str_multiple_assignments)
{
    SRL::string str("Hello"); // Initial string
    str = "World"; // Reassign
    str = str + "!"; // Concatenate
    mu_assert(strcmp(str.c_str(), "World!") == 0, "c_str multiple assignments failed"); // Verify result
}

    /**
 * @brief Test case: Verify c_str() after move operation.
 *
 * This test case checks if the c_str() method of the SRL::string class correctly returns the C-string representation after a move operation.
 */
MU_TEST(string_test_c_str_after_move)
{
    SRL::string str1("Hello"); // Source string
    SRL::string str2 = std::move(str1); // Move string
    mu_assert(strcmp(str2.c_str(), "Hello") == 0, "c_str after move failed"); // Verify moved content
    mu_assert(str1.c_str() == nullptr, "c_str after move failed"); // Verify source is null
}

/** @brief Checks a snprintfEx result (return value = length of the full text); describes a mismatch in buffer. */
static bool string_check_format(const char *got, int written, const char *expected)
{
    snprintf(buffer, buffer_size, "snprintfEx gave '%s' (returned %d), expected '%s' (%d)", got, written, expected, (int)strlen(expected));
    return strcmp(got, expected) == 0 && written == (int)strlen(expected);
}

/** @brief %s, %d, %u, %x and %c, with the literal text between them kept. */
MU_TEST(string_snprintfEx_conversions)
{
    char out[64];
    int written = SRL::string().snprintfEx(out, sizeof(out), "%s %d", "Hello", 42);
    mu_assert(string_check_format(out, written, "Hello 42"), buffer);

    written = SRL::string().snprintfEx(out, sizeof(out), "%d|%u|%x|%c", -7, 42u, 255, '!');
    mu_assert(string_check_format(out, written, "-7|42|ff|!"), buffer);
}

/** @brief %0Nd pads with zeros up to N digits, and never cuts a longer number. */
MU_TEST(string_snprintfEx_zero_padding)
{
    char out[64];
    int written = SRL::string().snprintfEx(out, sizeof(out), "[%05d]", 42);
    mu_assert(string_check_format(out, written, "[00042]"), buffer);

    written = SRL::string().snprintfEx(out, sizeof(out), "[%03d]", 1234);
    mu_assert(string_check_format(out, written, "[1234]"), buffer);
}

/** @brief %f prints a 16.16 fixed point number (passed by pointer) with 5 decimals. */
MU_TEST(string_snprintfEx_fixed_point)
{
    char out[64];
    SRL::Math::Types::Fxp value(1.5);
    int written = SRL::string().snprintfEx(out, sizeof(out), "%f", &value);
    mu_assert(string_check_format(out, written, "1.50000"), buffer);

    value = SRL::Math::Types::Fxp(0.0);
    written = SRL::string().snprintfEx(out, sizeof(out), "%f", &value);
    mu_assert(string_check_format(out, written, "0.00000"), buffer);

    value = SRL::Math::Types::Fxp(-2.25);
    written = SRL::string().snprintfEx(out, sizeof(out), "%f", &value);
    mu_assert(string_check_format(out, written, "-2.25000"), buffer);
}

/** @brief %% prints one percent sign and does not eat the next character. */
MU_TEST(string_snprintfEx_percent_literal)
{
    char out[64];
    int written = SRL::string().snprintfEx(out, sizeof(out), "%d%% done", 50);
    mu_assert(string_check_format(out, written, "50% done"), buffer);
}

/** @brief A text longer than the buffer is cut and terminated inside it; nothing past the buffer changes. */
MU_TEST(string_snprintfEx_truncates_inside_buffer)
{
    char out[16];
    memset(out, 0x55, sizeof(out));
    const int written = SRL::string().snprintfEx(out, 8, "%s", "0123456789AB");

    snprintf(buffer, buffer_size, "Returned %d, expected 12 (length of the full text)", written);
    mu_assert(written == 12, buffer);
    mu_assert(memcmp(out, "0123456", 7) == 0 && out[7] == '\0', "The text is not cut to 7 characters and terminated at out[7]");

    for (int i = 8; i < 16; i++)
    {
        snprintf(buffer, buffer_size, "out[%d] past the 8-byte buffer was changed to 0x%02x", i, (uint8_t)out[i]);
        mu_assert((uint8_t)out[i] == 0x55, buffer);
    }
}

/** @brief Copying, assigning and concatenating a default-constructed (empty) string gives an empty text. */
MU_TEST(string_copy_of_empty)
{
    SRL::string empty;
    SRL::string copy(empty);
    SRL::string assigned("x");
    assigned = empty;
    SRL::string joined = empty + SRL::string("abc");

    mu_assert(copy.c_str() == nullptr || copy.c_str()[0] == '\0', "A copy of an empty string is not empty");
    mu_assert(assigned.c_str() == nullptr || assigned.c_str()[0] == '\0', "Assigning an empty string does not give an empty string");
    snprintf(buffer, buffer_size, "empty + \"abc\" gave '%s'", joined.c_str() ? joined.c_str() : "(null)");
    mu_assert(joined.c_str() != nullptr && strcmp(joined.c_str(), "abc") == 0, buffer);
}

    /**
 * @brief Define the test suite for string-related functionality.
 *
 * This test suite configures and runs a comprehensive set of tests for the SRL::string class.
 */
MU_TEST_SUITE(string_test_suite)
{
        // Configure the test suite with setup, teardown, and error header functions
    MU_SUITE_CONFIGURE_WITH_HEADER(&string_test_setup,
        &string_test_teardown,
        &string_test_output_header);

        // Register all test cases
    MU_RUN_TEST(string_test_default_constructor);
    MU_RUN_TEST(string_test_constructor_with_src);
    MU_RUN_TEST(string_test_constructor_with_format);
    MU_RUN_TEST(string_test_constructor_with_integer);
    MU_RUN_TEST(string_test_copy_constructor);
    MU_RUN_TEST(string_test_copy_assignment_operator);
    MU_RUN_TEST(string_test_move_constructor);
    MU_RUN_TEST(string_test_move_assignment_operator);
    MU_RUN_TEST(string_test_concat);
    MU_RUN_TEST(string_test_c_str);
    MU_RUN_TEST(string_test_c_str_null);
    MU_RUN_TEST(string_test_c_str_empty);
    MU_RUN_TEST(string_test_c_str_single_char);
    MU_RUN_TEST(string_test_c_str_long_string);
    MU_RUN_TEST(string_test_c_str_after_modification);
    MU_RUN_TEST(string_test_c_str_multiple_assignments);
    MU_RUN_TEST(string_test_c_str_after_move);
    MU_RUN_TEST(string_snprintfEx_conversions);
    MU_RUN_TEST(string_snprintfEx_zero_padding);
    MU_RUN_TEST(string_snprintfEx_fixed_point);
    MU_RUN_TEST(string_snprintfEx_percent_literal);
    MU_RUN_TEST(string_snprintfEx_truncates_inside_buffer);
    MU_RUN_TEST(string_copy_of_empty);
}
}
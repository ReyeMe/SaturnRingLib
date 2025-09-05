#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_bitmap.hpp> // for IBitmap

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Logger;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];
    extern uint32_t suite_error_counter;

    // UT setup function, called before every tests
    void string_test_setup(void)
    {
        // Initialization logic, if necessary
    }

    // UT teardown function, called after every tests
    void string_test_teardown(void)
    {
        // Cleanup logic,
        ASCII::Clear();
        ASCII::SetPalette(0);
    }

    // UT output header function, called on the first test failure
    void string_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_STRING****");
            }
            else
            {
                LogInfo("****UT_STRING_ERROR(S)****");
            }
        }
    }

    MU_TEST(string_test_default_constructor)
    {
        SRL::string str;
        mu_assert(str.c_str() == nullptr, "Default constructor failed");
    }

    MU_TEST(string_test_constructor_with_src)
    {
        const char *src = "Hello, World!";
        SRL::string str(src);
        mu_assert(strcmp(str.c_str(), src) == 0, "Constructor with src failed");
    }

    MU_TEST(string_test_constructor_with_format)
    {
        const char *format = "%s %d";
        const char *str1 = "Hello";
        int num = 42;
        SRL::string str(format, str1, num);
        mu_assert(strcmp(str.c_str(), "Hello 42") == 0, "Constructor with format failed");
    }

    MU_TEST(string_test_constructor_with_integer)
    {
        int num = 42;
        SRL::string str(num);
        mu_assert(strcmp(str.c_str(), "42") == 0, "Constructor with integer failed");
    }

    MU_TEST(string_test_copy_constructor)
    {
        SRL::string str1("Hello, World!");
        SRL::string str2(str1);
        mu_assert(strcmp(str2.c_str(), str1.c_str()) == 0, "Copy constructor failed");
    }

    MU_TEST(string_test_copy_assignment_operator)
    {
        SRL::string str1("Hello, World!");
        SRL::string str2;
        str2 = str1;
        mu_assert(strcmp(str2.c_str(), str1.c_str()) == 0, "Copy assignment operator failed");
    }

    MU_TEST(string_test_move_constructor)
    {
        SRL::string str1("Hello, World!");
        SRL::string str2(std::move(str1));
        mu_assert(str1.c_str() == nullptr, "Move constructor failed");
        mu_assert(strcmp(str2.c_str(), "Hello, World!") == 0, "Move constructor failed");
    }

    MU_TEST(string_test_move_assignment_operator)
    {
        SRL::string str1("Hello, World!");
        SRL::string str2;
        str2 = std::move(str1);
        mu_assert(str1.c_str() == nullptr, "Move assignment operator failed");
        mu_assert(strcmp(str2.c_str(), "Hello, World!") == 0, "Move assignment operator failed");
    }

    MU_TEST(string_test_concat)
    {
        SRL::string str1("Hello, ");
        SRL::string str2("World!");
        SRL::string str3 = str1 + str2;
        mu_assert(strcmp(str3.c_str(), "Hello, World!") == 0, "Concat failed");
    }

    MU_TEST(string_test_c_str)
    {
        SRL::string str("Hello, World!");
        mu_assert(strcmp(str.c_str(), "Hello, World!") == 0, "c_str failed");
    }

    MU_TEST(string_test_c_str_null)
    {
        SRL::string str;
        mu_assert(str.c_str() == nullptr, "c_str null failed");
    }

    MU_TEST(string_test_c_str_empty)
    {
        SRL::string str("");
        mu_assert(strcmp(str.c_str(), "") == 0, "c_str empty failed");
    }

    MU_TEST(string_test_c_str_single_char)
    {
        SRL::string str("a");
        mu_assert(strcmp(str.c_str(), "a") == 0, "c_str single char failed");
    }

    MU_TEST(string_test_c_str_long_string)
    {
        const char *longStr = "This is a very long string that should not cause any issues";
        SRL::string str(longStr);
        mu_assert(strcmp(str.c_str(), longStr) == 0, "c_str long string failed");
    }

    MU_TEST(string_test_c_str_after_modification)
    {
        SRL::string str("Hello");
        str = str + " World!";
        mu_assert(strcmp(str.c_str(), "Hello World!") == 0, "c_str after modification failed");
    }

    MU_TEST(string_test_c_str_multiple_assignments)
    {
        SRL::string str("Hello");
        str = "World";
        str = str + "!";
        mu_assert(strcmp(str.c_str(), "World!") == 0, "c_str multiple assignments failed");
    }

    MU_TEST(string_test_c_str_after_move)
    {
        SRL::string str1("Hello");
        SRL::string str2 = std::move(str1);
        mu_assert(strcmp(str2.c_str(), "Hello") == 0, "c_str after move failed");
        mu_assert(str1.c_str() == nullptr, "c_str after move failed");
    }

    MU_TEST(string_test_snprintfEx)
    {
        char buffer[100] = {0};
        SRL::string str;
        int writtenChars = str.snprintfEx(buffer, 100, "%s %d", "Hello", 42);
        mu_assert(writtenChars == 13, "snprintfEx failed");
        mu_assert(strcmp(buffer, "Hello 42") == 0, "snprintfEx failed");

        // Test simple string
        writtenChars = str.snprintfEx(buffer, 100, "%s", "Hello");
        mu_assert(writtenChars == 5, "snprintfEx simple string failed");
        mu_assert(strcmp(buffer, "Hello") == 0, "snprintfEx simple string failed");

        // Test string with integer
        writtenChars = str.snprintfEx(buffer, 100, "%s %d", "Hello", 42);
        mu_assert(writtenChars == 7, "snprintfEx string with integer failed");
        mu_assert(strcmp(buffer, "Hello42") == 0, "snprintfEx string with integer failed");

        // Test string with unsigned integer
        writtenChars = str.snprintfEx(buffer, 100, "%s %u", "Hello", 42u);
        mu_assert(writtenChars == 7, "snprintfEx string with unsigned integer failed");
        mu_assert(strcmp(buffer, "Hello42") == 0, "snprintfEx string with unsigned integer failed");

        // Test string with character
        writtenChars = str.snprintfEx(buffer, 100, "%s %c", "Hello", '!');
        mu_assert(writtenChars == 7, "snprintfEx string with character failed");
        mu_assert(strcmp(buffer, "Hello!") == 0, "snprintfEx string with character failed");

        // Test string with FXP
        SRL::Math::Types::Fxp fxp(123.456);
        writtenChars = str.snprintfEx(buffer, 100, "%s %f", "Hello", &fxp);
        mu_assert(writtenChars > 7, "snprintfEx string with FXP failed");
        mu_assert(strcmp(buffer, "Hello123.46") == 0, "snprintfEx string with FXP failed");

        // Test string with padding
        writtenChars = str.snprintfEx(buffer, 100, "%s %0d", "Hello", 42);
        mu_assert(writtenChars == 7, "snprintfEx string with padding failed");
        mu_assert(strcmp(buffer, "Hello42") == 0, "snprintfEx string with padding failed");

        // Test buffer overflow
        char smallBuffer[5];
        writtenChars = str.snprintfEx(smallBuffer, 5, "%s %d", "Hello", 42);
        mu_assert(writtenChars > 5, "snprintfEx buffer overflow failed");
        mu_assert(smallBuffer[4] == '\0', "snprintfEx buffer overflow failed");
    }

    // Define the test suite for ASCII-related functionality
    // Configures and runs a comprehensive set of tests for the ASCII display class
    MU_TEST_SUITE(string_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&string_test_setup,
                                       &string_test_teardown,
                                       &string_test_output_header);

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
        MU_RUN_TEST(string_test_snprintfEx);
    }
}

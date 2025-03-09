#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    // UT setup function, called before every tests
    void cd_test_setup(void)
    {
        // Setup for CD tests, if necessary
    }

    // UT teardown function, called after every tests
    void cd_test_teardown(void)
    {
        // Cleanup for CD tests, if necessary
    }

    // UT output header function, called on the first test failure
    void cd_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_CD_ERROR(S)****");
        }
    }

    // Test: Initialize CD system
    MU_TEST(cd_test_initialize)
    {
        SRL::Memory::Initialize();
        bool success = SRL::Cd::Initialize();
        snprintf(buffer, buffer_size, "CD system initialization failed");
        mu_assert(success, buffer);
    }

    // Test: File existence
    MU_TEST(cd_test_file_exists)
    {
        const char *filename = "CD_UT.TXT";

        // Already initialized into main
        // SRL::Cd::Initialize();

        SRL::Cd::File file(filename);
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "File '%s' access pointer is not 0 : %d", filename, accessPointer);
        mu_assert(accessPointer == 0, buffer);

        int32_t identifier = file.GetIdentifier();
        snprintf(buffer, buffer_size, "File '%s' identifier is -1 : %d", filename, identifier);
        mu_assert(accessPointer != -1, buffer);

        file.Close();

        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);

        exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        // identifier = file.GetIdentifier();
        // snprintf(buffer, buffer_size, "File '%s' identifier is not -1 : %d", filename, identifier);
        // mu_assert(accessPointer == -1, buffer);
    }

    // Test: File reading
    MU_TEST(cd_test_read_file)
    {
        const char *filename = "CD_UT.TXT";
        static const uint16_t file_buffer_size = 255;

        const char *lines[] = {"UT1", "UT12", "UT123"};

        // Already initialized into main
        // SRL::Cd::Initialize();

        SRL::Cd::File file(filename);
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        char byteBuffer[file_buffer_size];

        // Clear buffer
        SRL::Memory::MemSet(byteBuffer, '\0', buffer_size);

        // Read from file into a buffer
        int32_t size = file.Read(file_buffer_size, byteBuffer);

        snprintf(buffer, file_buffer_size, "File '%s' : Read did not return any data", filename);
        mu_assert(size > 0, buffer);

        char *pch = byteBuffer;

        for (auto line : lines)
        {

            snprintf(buffer,
                     buffer_size,
                     "Read Buffer error ! : %s",
                     byteBuffer);
            mu_assert(pch != NULL, buffer);

            int cmp = strncmp(line, pch, strlen(line));

            snprintf(buffer,
                     buffer_size,
                     "File '%s' : Read did not return %s, but %s instead",
                     filename,
                     line,
                     pch);
            mu_assert(cmp == 0, buffer);

            pch = strchr(pch + 1, '\n');

            snprintf(buffer,
                     buffer_size,
                     "Read Buffer error ! : %s",
                     byteBuffer);
            mu_assert(pch != NULL, buffer);

            pch++;
        }

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "File '%s' access pointer is not > 0 : %d", filename, accessPointer);
        mu_assert(accessPointer > 0, buffer);
    }

    // Test: File reading
    MU_TEST(cd_test_read_file2)
    {
        const char *dirname = "ROOT";
        const char *filename = "FILE.TXT";
        static const uint16_t file_buffer_size = 255;

        const char *lines[] = {"ExpectedContent"};

        // Already initialized into main
        // SRL::Cd::Initialize();

        SRL::Cd::ChangeDir(dirname);

        SRL::Cd::File file(filename);

        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        int32_t identifier = file.GetIdentifier();
        snprintf(buffer, buffer_size, "File '%s' identifier < 0 : %d", filename, identifier);
        mu_assert(identifier >= 0, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        char byteBuffer[file_buffer_size];

        // Clear buffer
        SRL::Memory::MemSet(byteBuffer, '\0', buffer_size);

        // Read from file into a buffer
        int32_t size = file.Read(file_buffer_size, byteBuffer);

        snprintf(buffer, file_buffer_size, "File '%s' : Read did not return any data", filename);
        mu_assert(size > 0, buffer);

        char *pch = byteBuffer;

        for (auto line : lines)
        {

            snprintf(buffer,
                     buffer_size,
                     "Read Buffer error ! : %s",
                     byteBuffer);
            mu_assert(pch != NULL, buffer);

            int cmp = strncmp(line, pch, strlen(line));

            snprintf(buffer,
                     buffer_size,
                     "File '%s' : Read did not return %s, but %s instead",
                     filename,
                     line,
                     pch);
            mu_assert(cmp == 0, buffer);

            pch = strchr(pch + 1, '\n');

            snprintf(buffer,
                     buffer_size,
                     "Read Buffer error ! : %s",
                     byteBuffer);
            mu_assert(pch != NULL, buffer);

            pch++;
        }

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "File '%s' access pointer is not > 0 : %d", filename, accessPointer);
        mu_assert(accessPointer > 0, buffer);
    }

    MU_TEST(cd_test_null_file)
    {
        SRL::Cd::File file(nullptr);
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File NULL does exist but should not");
        mu_assert(!exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File NULL does open but should not");
        mu_assert(!open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File NULL is  open but should not");
        mu_assert(!isopen, buffer);

        file.Close();

        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File NULL is open but should not");
        mu_assert(!isopen, buffer);
    }

    // Test: Handle missing file
    MU_TEST(cd_test_missing_file)
    {
        const char *filename = "MISSING.TXT";
        SRL::Cd::File file(filename);
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does exist but should not", filename);
        mu_assert(!exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does open but should not", filename);
        mu_assert(!open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is  open but should not", filename);
        mu_assert(!isopen, buffer);

        file.Close();

        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);
    }


    MU_TEST_SUITE(cd_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&cd_test_setup,
                                       &cd_test_teardown,
                                       &cd_test_output_header);

        MU_RUN_TEST(cd_test_initialize);
        MU_RUN_TEST(cd_test_file_exists);
        MU_RUN_TEST(cd_test_read_file);
        MU_RUN_TEST(cd_test_read_file2);
        MU_RUN_TEST(cd_test_null_file);
        MU_RUN_TEST(cd_test_missing_file);
    }
}

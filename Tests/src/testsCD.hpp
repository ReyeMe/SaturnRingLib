#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    // Setup function for CD tests
    // This function is called before every test to initialize the test environment.
    void cd_test_setup(void)
    {
        // Setup for CD tests, if necessary
    }

    // Teardown function for CD tests
    // This function is called after every test to clean up the test environment.
    void cd_test_teardown(void)
    {
        // Reset the current directory to the root directory
        SRL::Cd::ChangeDir(static_cast<const char*>(nullptr));
    }

    // Output header function for CD tests
    // This function is called when the first test failure occurs to log an error header.
    void cd_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_CD****");
            }
            else
            {
                LogInfo("****UT_CD_ERROR(S)****");
            }
        }
    }

    // Test: Verify that a file exists and can be opened and closed properly.
    MU_TEST(cd_test_file_exists)
    {
        const char *filename = "CD_UT.TXT";

        SRL::Cd::File file(filename);

        // Check if the file exists
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        // Open the file and verify
        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        // Check if the file is open
        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        // Verify the access pointer and identifier
        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "File '%s' access pointer is not 0 : %d", filename, accessPointer);
        mu_assert(accessPointer == 0, buffer);

        int32_t identifier = file.GetIdentifier();
        snprintf(buffer, buffer_size, "File '%s' identifier is -1 : %d", filename, identifier);
        mu_assert(accessPointer != -1, buffer);

        // Close the file and verify
        file.Close();
        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);

        // Verify the file still exists after closing
        exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);
    }

    // Test: Verify that a file can be read and its contents match expected values.
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

        // Check if the file exists
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        int32_t identifier = file.GetIdentifier();
        snprintf(buffer, buffer_size, "File '%s' identifier < 0 : %d", filename, identifier);
        mu_assert(identifier >= 0, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        // Check if the file is open
        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        char byteBuffer[file_buffer_size];

        // Clear the buffer
        SRL::Memory::MemSet(byteBuffer, '\0', buffer_size);

        // Read from the file into the buffer
        int32_t size = file.Read(file_buffer_size, byteBuffer);
        snprintf(buffer, file_buffer_size, "File '%s' : Read did not return any data", filename);
        mu_assert(size > 0, buffer);

        // Verify the contents of the buffer match the expected lines
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

        // Verify the access pointer is updated
        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "File '%s' access pointer is not > 0 : %d", filename, accessPointer);
        mu_assert(accessPointer > 0, buffer);
    }

    // Test: Verify behavior when attempting to open a null file.
    MU_TEST(cd_test_null_file)
    {
        SRL::Cd::File file(nullptr);

        // Verify the file does not exist
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File NULL does exist but should not");
        mu_assert(!exists, buffer);

        // Verify the file cannot be opened
        bool open = file.Open();
        snprintf(buffer, buffer_size, "File NULL does open but should not");
        mu_assert(!open, buffer);

        // Verify the file is not open
        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File NULL is open but should not");
        mu_assert(!isopen, buffer);

        // Close the file and verify
        file.Close();
        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File NULL is open but should not");
        mu_assert(!isopen, buffer);
    }

    // Test: Verify behavior when attempting to open a missing file.
    MU_TEST(cd_test_missing_file)
    {
        const char *filename = "MISSING.TXT";
        SRL::Cd::File file(filename);

        // Verify the file does not exist
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does exist but should not", filename);
        mu_assert(!exists, buffer);

        // Verify the file cannot be opened
        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does open but should not", filename);
        mu_assert(!open, buffer);

        // Verify the file is not open
        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);

        // Close the file and verify
        file.Close();
        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);
    }

    // Test: Verify seeking to the beginning of a file.
    MU_TEST(cd_file_seek_test_beginning)
    {
        const char *filename = "TESTFILE.UTS";
        Cd::File file(filename);

        // Verify the file exists and can be opened
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t result = file.Seek(0);
        snprintf(buffer, buffer_size, "Seek to beginning failed: %d != 0", result);
        mu_assert(result == 0, buffer);

        char byte;
        int32_t bytesRead = file.Read(1, &byte);
        snprintf(buffer, buffer_size, "Read 1 byte failed: %d != 0", bytesRead);
        mu_assert(bytesRead == 0, buffer);
    }

    // Test seeking to a specific offset
    MU_TEST(cd_file_seek_test_offset)
    {
        const char *filename = "TESTFILE.UTS";
        Cd::File file(filename);

        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t offset = 100;
        int32_t result = file.Seek(offset);
        snprintf(buffer, buffer_size, "Seek to offset failed: %d != %d", result, offset);
        mu_assert(result == offset, buffer);

        char byte;
        int32_t bytesRead = file.Read(1, &byte);
        snprintf(buffer, buffer_size, "Read 1 byte failed: %d != 99", bytesRead);
        mu_assert(bytesRead == 99, buffer);
    }

    // Test seeking relative to the current position
    MU_TEST(cd_file_seek_test_relative)
    {
        const char *filename = "TESTFILE.UTS";
        Cd::File file(filename);

        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t initial_offset = 50;
        file.Seek(initial_offset);
        const int32_t new_offset = 30;
        int32_t result = file.Seek(new_offset);
        snprintf(buffer, buffer_size, "Seek failed: %d != %d", result, new_offset);
        mu_assert(result ==  new_offset, buffer);
    }

    // Test seeking to an invalid offset (negative)
    MU_TEST(cd_file_seek_test_invalid_negative)
    {
        const char *filename = "TESTFILE.UTS";
        Cd::File file(filename);

        // Verify the file exists and can be opened
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t result = file.Seek(-10);
        snprintf(buffer, buffer_size, "Seek to invalid negative offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Verify seeking to an invalid offset (beyond file size).
    MU_TEST(cd_file_seek_test_invalid_beyond)
    {
        const char *filename = "TESTFILE.UTS";
        Cd::File file(filename);

        // Verify the file exists and can be opened
        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        mu_assert(open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        mu_assert(isopen, buffer);

        int32_t result = file.Seek(file.Size.Bytes + 10);
        snprintf(buffer, buffer_size, "Seek to invalid beyond offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Verify changing to a valid directory.
    // MU_TEST(cd_test_change_to_valid_directory)
    // {
    //     const char *validDir = "ROOT";

    //     // Change to the valid directory
    //     bool changed = SRL::Cd::ChangeDir(validDir);
    //     snprintf(buffer, buffer_size, "Failed to change to valid directory '%s'", validDir);
    //     mu_assert(changed, buffer);

    //     // Verify the current directory is the valid directory
    //     const char *currentDir = SRL::Cd::GetCurrentDir();
    //     snprintf(buffer, buffer_size, "Current directory is not '%s', but '%s'", validDir, currentDir);
    //     mu_assert(strcmp(currentDir, validDir) == 0, buffer);
    // }

    // Test: Verify changing to an invalid directory.
    // MU_TEST(cd_test_change_to_invalid_directory)
    // {
    //     const char *invalidDir = "INVALID";

    //     // Attempt to change to the invalid directory
    //     bool changed = SRL::Cd::ChangeDir(invalidDir);
    //     snprintf(buffer, buffer_size, "Changed to invalid directory '%s' but should not", invalidDir);
    //     mu_assert(!changed, buffer);

    //     // Verify the current directory remains unchanged
    //     const char *currentDir = SRL::Cd::GetCurrentDir();
    //     snprintf(buffer, buffer_size, "Current directory changed unexpectedly to '%s'", currentDir);
    //     mu_assert(strcmp(currentDir, invalidDir) != 0, buffer);
    // }

    // Test: Verify navigating back to the parent directory.
    // MU_TEST(cd_test_navigate_to_parent_directory)
    // {
    //     const char *initialDir = SRL::Cd::GetCurrentDir();
    //     const char *subDir = "ROOT";

    //     // Change to a subdirectory
    //     bool changed = SRL::Cd::ChangeDir(subDir);
    //     snprintf(buffer, buffer_size, "Failed to change to subdirectory '%s'", subDir);
    //     mu_assert(changed, buffer);

    //     // Navigate back to the parent directory
    //     changed = SRL::Cd::ChangeDir("..");
    //     snprintf(buffer, buffer_size, "Failed to navigate back to parent directory from '%s'", subDir);
    //     mu_assert(changed, buffer);

    //     // Verify the current directory is the initial directory
    //     const char *currentDir = SRL::Cd::GetCurrentDir();
    //     snprintf(buffer, buffer_size, "Current directory is not '%s', but '%s'", initialDir, currentDir);
    //     mu_assert(strcmp(currentDir, initialDir) == 0, buffer);
    // }

    // Test: Verify navigating to the root directory.
    // MU_TEST(cd_test_navigate_to_root_directory)
    // {
    //     // Change to the root directory
    //     bool changed = SRL::Cd::ChangeDir("/");
    //     snprintf(buffer, buffer_size, "Failed to change to root directory");
    //     mu_assert(changed, buffer);

    //     // Verify the current directory is the root directory
    //     const char *currentDir = SRL::Cd::GetCurrentDir();
    //     snprintf(buffer, buffer_size, "Current directory is not root '/', but '%s'", currentDir);
    //     mu_assert(strcmp(currentDir, "/") == 0, buffer);
    // }

    // Test suite for CD-related tests
    MU_TEST_SUITE(cd_test_suite)
    {
        // Configure the test suite with setup, teardown, and output header functions
        MU_SUITE_CONFIGURE_WITH_HEADER(&cd_test_setup,
                                       &cd_test_teardown,
                                       &cd_test_output_header);

        // Run individual test cases
        MU_RUN_TEST(cd_test_file_exists);
        MU_RUN_TEST(cd_test_read_file);
        MU_RUN_TEST(cd_test_read_file2);
        MU_RUN_TEST(cd_test_null_file);
        MU_RUN_TEST(cd_test_missing_file);
        MU_RUN_TEST(cd_file_seek_test_beginning);
        MU_RUN_TEST(cd_file_seek_test_offset);
        MU_RUN_TEST(cd_file_seek_test_relative);
        MU_RUN_TEST(cd_file_seek_test_invalid_negative);
        MU_RUN_TEST(cd_file_seek_test_invalid_beyond);
        //MU_RUN_TEST(cd_test_change_to_valid_directory);       // New test
        //MU_RUN_TEST(cd_test_change_to_invalid_directory);     // New test
        //MU_RUN_TEST(cd_test_navigate_to_parent_directory);    // New test
        //MU_RUN_TEST(cd_test_navigate_to_root_directory);      // New test
    }
}

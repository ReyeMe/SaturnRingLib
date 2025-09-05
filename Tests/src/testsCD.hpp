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
    void cd_test_setup(void)
    {
        // Initialize the CD system for testing
        SRL::Cd::Initialize();
    }

    // Teardown function for CD tests
    void cd_test_teardown(void)
    {
        // Reset the current directory to the root directory
        SRL::Cd::ChangeDir(static_cast<const char *>(nullptr));
    }

    // Output header function for CD tests
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
        mu_assert(identifier != -1, buffer);

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
        SRL::Memory::MemSet(byteBuffer, '\0', file_buffer_size);

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

    // Test: File reading in a specific directory
    MU_TEST(cd_test_read_file2)
    {
        const char *dirname = "ROOT";
        const char *filename = "FILE.TXT";
        static const uint16_t file_buffer_size = 255;

        const char *lines[] = {"ExpectedContent"};

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

        SRL::Memory::MemSet(byteBuffer, '\0', file_buffer_size);

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

    // Test: Verify behavior when attempting to open a null file.
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
        snprintf(buffer, buffer_size, "File NULL is open but should not");
        mu_assert(!isopen, buffer);

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

        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does exist but should not", filename);
        mu_assert(!exists, buffer);

        bool open = file.Open();
        snprintf(buffer, buffer_size, "File '%s' does open but should not", filename);
        mu_assert(!open, buffer);

        bool isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);

        file.Close();
        isopen = file.IsOpen();
        snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        mu_assert(!isopen, buffer);
    }

    // Test: Verify seeking to the beginning of a file.
    MU_TEST(cd_file_seek_test_beginning)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        int32_t result = file.Seek(0);
        snprintf(buffer, buffer_size, "Seek to beginning failed: %d != 0", result);
        mu_assert(result == 0, buffer);

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "Access pointer not at beginning: %d != 0", accessPointer);
        mu_assert(accessPointer == 0, buffer);
    }

    // Test: Verify seeking to a specific offset
    MU_TEST(cd_file_seek_test_offset)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "Access pointer not at offset: %d != %d", accessPointer, offset);
        mu_assert(accessPointer == offset, buffer);
    }

    // Test: Verify seeking relative to the current position
    MU_TEST(cd_file_seek_test_relative)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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
        int32_t new_offset = 30;
        int32_t result = file.Seek(new_offset);
        snprintf(buffer, buffer_size, "Seek failed: %d != %d", result, new_offset);
        mu_assert(result == new_offset, buffer);

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "Access pointer not at new offset: %d != %d", accessPointer, new_offset);
        mu_assert(accessPointer == new_offset, buffer);
    }

    // Test: Verify seeking to an invalid negative offset
    MU_TEST(cd_file_seek_test_invalid_negative)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        int32_t result = file.Seek(-10);
        snprintf(buffer, buffer_size, "Seek to invalid negative offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Verify seeking to an invalid offset (beyond file size)
    MU_TEST(cd_file_seek_test_invalid_beyond)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        int32_t result = file.Seek(file.Size.Bytes + 10);
        snprintf(buffer, buffer_size, "Seek to invalid beyond offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Verify seeking to the exact file size
    MU_TEST(cd_file_seek_test_file_size)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        int32_t result = file.Seek(file.Size.Bytes);
        snprintf(buffer, buffer_size, "Seek to file size failed: %d != %d", result, file.Size.Bytes);
        mu_assert(result == file.Size.Bytes, buffer);

        int32_t accessPointer = file.GetCurrentAccessPointer();
        snprintf(buffer, buffer_size, "Access pointer not at file size: %d != %d", accessPointer, file.Size.Bytes);
        mu_assert(accessPointer == file.Size.Bytes, buffer);
    }

    // Test: Verify reading zero bytes
    MU_TEST(cd_test_read_zero_bytes)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        char byteBuffer[10];
        SRL::Memory::MemSet(byteBuffer, '\0', 10);

        int32_t size = file.Read(0, byteBuffer);
        snprintf(buffer, buffer_size, "Reading zero bytes should return -1: %d", size);
        mu_assert(size == -1, buffer);
    }

    // Test: Verify LoadBytes functionality
    MU_TEST(cd_test_load_bytes)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";
        static const uint16_t file_buffer_size = 255;

        SRL::Cd::ChangeDir(dirname);
        Cd::File file(filename);

        bool exists = file.Exists();
        snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        mu_assert(exists, buffer);

        char byteBuffer[file_buffer_size];
        SRL::Memory::MemSet(byteBuffer, '\0', file_buffer_size);

        int32_t size = file.LoadBytes(0, file_buffer_size, byteBuffer);
        snprintf(buffer, buffer_size, "LoadBytes did not return any data for '%s'", filename);
        mu_assert(size > 0, buffer);

        // Verify content (assuming known content)
        const char *expected = "ExpectedContent";
        int cmp = strncmp(byteBuffer, expected, strlen(expected));
        snprintf(buffer, buffer_size, "LoadBytes content mismatch: expected '%s', got '%s'", expected, byteBuffer);
        mu_assert(cmp == 0, buffer);
    }

    // Test: Verify ReadSectors functionality
    MU_TEST(cd_test_read_sectors)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";
        static const uint16_t file_buffer_size = 2048; // Typical sector size

        SRL::Cd::ChangeDir(dirname);
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

        char byteBuffer[file_buffer_size];
        SRL::Memory::MemSet(byteBuffer, '\0', file_buffer_size);

        int32_t size = file.ReadSectors(1, byteBuffer);
        snprintf(buffer, buffer_size, "ReadSectors did not return any data for '%s'", filename);
        mu_assert(size > 0, buffer);

        // Verify content (assuming known content)
        const char *expected = "ExpectedContent";
        int cmp = strncmp(byteBuffer, expected, strlen(expected));
        snprintf(buffer, buffer_size, "ReadSectors content mismatch: expected '%s', got '%s'", expected, byteBuffer);
        mu_assert(cmp == 0, buffer);
    }

    // Test: Verify IsEOF functionality
    MU_TEST(cd_test_is_eof)
    {
        const char *dirname = "ROOT";
        const char *filename = "TESTFILE.UTS";

        SRL::Cd::ChangeDir(dirname);
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

        // Seek to end of file
        file.Seek(file.Size.Bytes);

        bool isEOF = file.IsEOF();
        snprintf(buffer, buffer_size, "File '%s' should be at EOF", filename);
        mu_assert(isEOF, buffer);

        // Seek to beginning and check not EOF
        file.Seek(0);
        isEOF = file.IsEOF();
        snprintf(buffer, buffer_size, "File '%s' should not be at EOF", filename);
        mu_assert(!isEOF, buffer);
    }

    // Test: Verify changing to a valid directory
    MU_TEST(cd_test_change_to_valid_directory)
    {
        const char *validDir = "ROOT";

        // Change to the valid directory
        int32_t result = SRL::Cd::ChangeDir(validDir);
        snprintf(buffer, buffer_size, "Failed to change to valid directory '%s': %d", validDir, result);
        mu_assert(result >= Cd::ErrorCode::ErrorOk, buffer);
    }

    // Test: Verify changing to an invalid directory
    MU_TEST(cd_test_change_to_invalid_directory)
    {
        const char *invalidDir = "INVALID";

        // Attempt to change to the invalid directory
        int32_t result = SRL::Cd::ChangeDir(invalidDir);
        snprintf(buffer, buffer_size, "Changed to invalid directory '%s' but should not: %d", invalidDir, result);
        mu_assert(result == Cd::ErrorCode::ErrorNoName || result == Cd::ErrorCode::ErrorNExit, buffer);
    }

    // Test: Verify navigating to the parent directory
    MU_TEST(cd_test_navigate_to_parent_directory)
    {
        const char *subDir = "ROOT";

        // Change to a subdirectory
        int32_t result = SRL::Cd::ChangeDir(subDir);
        snprintf(buffer, buffer_size, "Failed to change to subdirectory '%s': %d", subDir, result);
        mu_assert(result >= Cd::ErrorCode::ErrorOk, buffer);

        // Navigate back to the parent directory
        result = SRL::Cd::ChangeDir("..");
        snprintf(buffer, buffer_size, "Failed to navigate back to parent directory from '%s': %d", subDir, result);
        mu_assert(result >= Cd::ErrorCode::ErrorOk, buffer);
    }

    // Test: Verify navigating to the root directory
    // MU_TEST(cd_test_navigate_to_root_directory)
    // {
    //     // Change to a subdirectory to ensure we're not already at root
    //     SRL::Cd::ChangeDir("ROOT");

    //     // Change to the root directory
    //     int32_t result = SRL::Cd::ChangeDir(nullptr);
    //     snprintf(buffer, buffer_size, "Failed to change to root directory: %d", result);
    //     mu_assert(result >= Cd::ErrorCode::ErrorOk, buffer);
    // }

    // Test: Verify TableOfContents retrieval
    MU_TEST(cd_test_table_of_contents)
    {
        Cd::TableOfContents toc = Cd::TableOfContents::GetTable();

        // Verify that the first track has a valid number
        snprintf(buffer, buffer_size, "First track number is invalid: %d", toc.FirstTrack.Number);
        mu_assert(toc.FirstTrack.Number >= 1, buffer);

        // Verify that the last track number is valid
        snprintf(buffer, buffer_size, "Last track number is invalid: %d", toc.LastTrack.Number);
        mu_assert(toc.LastTrack.Number <= Cd::MaxTrackCount, buffer);

        // Verify track type for the first track
        Cd::TableOfContents::TrackType type = toc.FirstTrack.GetType();
        snprintf(buffer, buffer_size, "First track type is invalid: %d", type);
        mu_assert(type == Cd::TableOfContents::TrackType::Data || type == Cd::TableOfContents::TrackType::Audio, buffer);
    }

    // Test suite for CD-related tests
    MU_TEST_SUITE(cd_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&cd_test_setup,
                                       &cd_test_teardown,
                                       &cd_test_output_header);

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
        MU_RUN_TEST(cd_file_seek_test_file_size);
        MU_RUN_TEST(cd_test_read_zero_bytes);
        MU_RUN_TEST(cd_test_load_bytes);
        MU_RUN_TEST(cd_test_read_sectors);
        MU_RUN_TEST(cd_test_is_eof);
        MU_RUN_TEST(cd_test_change_to_valid_directory);
        MU_RUN_TEST(cd_test_change_to_invalid_directory);
        MU_RUN_TEST(cd_test_navigate_to_parent_directory);
        //MU_RUN_TEST(cd_test_navigate_to_root_directory);
        MU_RUN_TEST(cd_test_table_of_contents);
    }
}
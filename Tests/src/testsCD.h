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
        // Reset current directory to root
        SRL::Cd::ChangeDir(static_cast<const char*>(nullptr));

    }

    // UT output header function, called on the first test failure
    void cd_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            LogInfo("****UT_CD_ERROR(S)****");
        }
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

    // Test seeking to the beginning of the file
    MU_TEST(cd_file_seek_test_beginning)
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

        int32_t result = file.Seek(0, Cd::SeekMode::Absolute);
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
        int32_t result = file.Seek(offset, Cd::SeekMode::Absolute);
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
        file.Seek(initial_offset, Cd::SeekMode::Absolute);
        int32_t relative_offset = 30;
        int32_t result = file.Seek(relative_offset, Cd::SeekMode::Relative);
        snprintf(buffer, buffer_size, "Seek relative failed: %d != %d", result, initial_offset + relative_offset);
        mu_assert(result == initial_offset + relative_offset, buffer);
    }

    // Test seeking to an invalid offset (negative)
    MU_TEST(cd_file_seek_test_invalid_negative)
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

        int32_t result = file.Seek(-10, Cd::SeekMode::Absolute);
        snprintf(buffer, buffer_size, "Seek to invalid negative offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test seeking to an invalid offset (beyond file size)
    MU_TEST(cd_file_seek_test_invalid_beyond)
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

        int32_t result = file.Seek(file.Size.Bytes + 10, Cd::SeekMode::Absolute);
        snprintf(buffer, buffer_size, "Seek to invalid beyond offset failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Seek to the end of the file with offset 0
    MU_TEST(cd_file_seek_test_end_offset_zero)
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

        int32_t result = file.Seek(0, Cd::SeekMode::EndOfFile);
        snprintf(buffer, buffer_size, "Seek to end with offset 0 failed: %d != %d", result, file.Size.Bytes);
        mu_assert(result == file.Size.Bytes, buffer);
    }

    // Test: Seek to the end of the file with a positive offset
    MU_TEST(cd_file_seek_test_end_positive_offset)
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

        int32_t offset = 50;
        int32_t result = file.Seek(offset, Cd::SeekMode::EndOfFile);
        snprintf(buffer, buffer_size, "Seek to end with positive offset failed: %d != %d", result, file.Size.Bytes + offset);
        mu_assert(result == file.Size.Bytes + offset, buffer);
    }

    // Test: Seek to the end of the file with a negative offset
    MU_TEST(cd_file_seek_test_end_negative_offset)
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

        int32_t offset = -50;
        int32_t result = file.Seek(offset, Cd::SeekMode::EndOfFile);
        snprintf(buffer, buffer_size, "Seek to end with negative offset failed: %d != %d", result, file.Size.Bytes + offset);
        mu_assert(result == file.Size.Bytes + offset, buffer);
    }

    // Test: Seek to the end of the file with an offset beyond the file size
    MU_TEST(cd_file_seek_test_end_offset_beyond)
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

        int32_t offset = file.Size.Bytes + 50;
        int32_t result = file.Seek(offset, Cd::SeekMode::EndOfFile);
        snprintf(buffer, buffer_size, "Seek to end with offset beyond file size failed: %d != %d", result, Cd::ErrorCode::ErrorSeek);
        mu_assert(result == Cd::ErrorCode::ErrorSeek, buffer);
    }

    // Test: Seek from EndOfFile and read 1 byte at the position
    MU_TEST(cd_file_seek_and_read_from_end)
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

        int32_t offset = -1; // Seek to the last byte
        int32_t result = file.Seek(offset, Cd::SeekMode::EndOfFile);
        snprintf(buffer, buffer_size, "Seek to end with offset -1 failed: %d != %d", result, file.Size.Bytes + offset);
        mu_assert(result == file.Size.Bytes + offset, buffer);

        char byte;
        int32_t bytesRead = file.Read(1, &byte);
        snprintf(buffer, buffer_size, "Read 1 byte failed: %d != 1", bytesRead);
        mu_assert(bytesRead == 1, buffer);

        snprintf(buffer, buffer_size, "Read byte is not as expected: %c", byte);
        mu_assert(byte == 'expected_byte_value', buffer); // Replace 'expected_byte_value' with the actual expected value
    }

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
        MU_RUN_TEST(cd_file_seek_test_end_offset_zero);
        MU_RUN_TEST(cd_file_seek_test_end_positive_offset);
        MU_RUN_TEST(cd_file_seek_test_end_negative_offset);
        MU_RUN_TEST(cd_file_seek_test_end_offset_beyond);
        MU_RUN_TEST(cd_file_seek_and_read_from_end);
    }
}

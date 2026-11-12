#include <srl.hpp>
#include <srl_log.hpp>
#include <stddef.h>
#include <type_traits>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];

    /**
 * @brief Sets up the environment for CD (Compact Disc) unit tests.
 */
void cd_test_setup(void)
{
        // Initialize the CD system for testing
    SRL::Cd::Initialize();
}

    /**
 * @brief Cleans up the environment after each CD unit test.
 */
void cd_test_teardown(void)
{
        // Reset the current directory to the root directory
    SRL::Cd::ChangeDir(static_cast<const char *>(nullptr));
}

    /**
 * @brief Displays a header for the CD test suite upon the first error.
 */
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

    /**
 * @brief Tests basic file operations: existence check, opening, and closing.
 * @details Verifies that a known file exists, can be successfully opened (retrieving a valid identifier),
 *          and then properly closed.
 */
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

    /**
 * @brief Tests reading the contents of a file and verifying its content.
 * @details Opens a known text file, reads its content into a buffer, and then compares
 *          the content line-by-line against an expected set of strings.
 */
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

    /**
 * @brief Tests reading a file located within a specific directory.
 * @details Changes the current directory, then attempts to open and read a file within that
 *          directory to verify its contents.
 */
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

    /**
 * @brief Tests the system's behavior when attempting to operate on a `nullptr` file.
 * @details Ensures that attempting to check existence, open, or close a file initialized
 *          with `nullptr` fails gracefully and does not lead to crashes.
 */
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

    /**
 * @brief Tests the system's behavior when attempting to operate on a non-existent file.
 * @details Verifies that all operations on a file that does not exist on the disc
 *          (existence check, open, close) fail as expected.
 */
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

    /**
 * @brief Tests seeking to the beginning of a file.
 * @details Verifies that `Seek(0)` correctly moves the file's access pointer to the start.
 */
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

    /**
 * @brief Tests seeking to a specific offset within a file.
 * @details Verifies that seeking to a valid, non-zero offset correctly updates the file's access pointer.
 */
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

    /**
 * @brief Tests seeking relative to the current position in a file.
 * @details Verifies that a seek operation correctly updates the access pointer from its current
 *          position rather than from the beginning of the file.
 */
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

    /**
 * @brief Tests seeking to an invalid negative offset.
 * @details Seek is documented to return a negative value on error, and must leave the access pointer where it was.
 */
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
    snprintf(buffer, buffer_size, "Seek to invalid negative offset should return a negative error, got %d", result);
    mu_assert(result < 0, buffer);

    int32_t accessPointer = file.GetCurrentAccessPointer();
    snprintf(buffer, buffer_size, "Failed seek moved the access pointer: %d != 0", accessPointer);
    mu_assert(accessPointer == 0, buffer);
}

    /**
 * @brief Tests seeking to an offset beyond the end of the file.
 * @details Seek is documented to return a negative value on error, and must leave the access pointer where it was.
 */
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
    snprintf(buffer, buffer_size, "Seek beyond the end should return a negative error, got %d", result);
    mu_assert(result < 0, buffer);

    int32_t accessPointer = file.GetCurrentAccessPointer();
    snprintf(buffer, buffer_size, "Failed seek moved the access pointer: %d != 0", accessPointer);
    mu_assert(accessPointer == 0, buffer);
}

    /**
 * @brief Tests seeking to the exact end of the file.
 * @details Verifies that seeking to an offset equal to the file's size is a valid operation.
 */
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

    /**
 * @brief Tests the behavior of reading zero bytes from a file.
 * @details Verifies that a read operation with a length of zero returns an error code.
 */
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

    /**
 * @brief Tests the `LoadBytes` functionality for directly loading file content.
 * @details Verifies that `LoadBytes` can read a specified number of bytes from a file
 *          into a buffer and that the content is correct.
 */
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

        // TESTFILE.UTS holds the bytes 0, 1, 2, ... (i % 256)
    int32_t bad = -1;
    for (int32_t i = 0; i < file_buffer_size && bad < 0; i++)
    {
        if (static_cast<uint8_t>(byteBuffer[i]) != static_cast<uint8_t>(i % 256)) bad = i;
    }
    snprintf(buffer, buffer_size, "LoadBytes content mismatch at byte %d: 0x%02x instead of 0x%02x", bad,
        bad >= 0 ? static_cast<uint8_t>(byteBuffer[bad]) : 0, bad >= 0 ? bad % 256 : 0);
    mu_assert(bad < 0, buffer);
}

    /**
 * @brief Tests reading a file's content on a sector-by-sector basis.
 * @details Verifies that `ReadSectors` successfully reads a sector of data from a file
 *          and that the content matches expectations.
 */
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

        // TESTFILE.UTS holds the bytes 0, 1, 2, ... (i % 256): the whole first sector must match
    int32_t bad = -1;
    for (int32_t i = 0; i < size && i < file_buffer_size && bad < 0; i++)
    {
        if (static_cast<uint8_t>(byteBuffer[i]) != static_cast<uint8_t>(i % 256)) bad = i;
    }
    snprintf(buffer, buffer_size, "ReadSectors content mismatch at byte %d: 0x%02x instead of 0x%02x", bad,
        bad >= 0 ? static_cast<uint8_t>(byteBuffer[bad]) : 0, bad >= 0 ? bad % 256 : 0);
    mu_assert(bad < 0, buffer);
}

    /**
 * @brief Tests the end-of-file (`IsEOF`) detection functionality.
 * @details Verifies that `IsEOF` returns true only when the file's access pointer
 *          is at the end of the file.
 */
MU_TEST(cd_test_is_eof)
{
    const char *dirname = "ROOT";
    const char *filename = "TESTFILE.UTS";

    SRL::Cd::ChangeDir(dirname);

        // A one-sector file is not at EOF after reading part of it, and is after reading the rest
    Cd::File small("FILE.TXT");
    mu_assert(small.Open(), "File 'FILE.TXT' does not open but should");
    char part[4];
    int32_t read = small.Read(sizeof(part), part);
    snprintf(buffer, buffer_size, "File 'FILE.TXT' (%d bytes) is at EOF after reading %d bytes", small.Size.Bytes, read);
    mu_assert(read == sizeof(part) && !small.IsEOF(), buffer);

    char rest[32];
    read = small.Read(sizeof(rest), rest);
    snprintf(buffer, buffer_size, "File 'FILE.TXT' (%d bytes) is not at EOF after reading the remaining %d bytes", small.Size.Bytes, read);
    mu_assert(small.IsEOF(), buffer);
    small.Close();

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

    /**
 * @brief Tests changing to a known valid directory.
 * @details Verifies that the `ChangeDir` function returns a success code when navigating
 *          to a directory that is known to exist.
 */
MU_TEST(cd_test_change_to_valid_directory)
{
    const char *validDir = "ROOT";

        // Change to the valid directory
    int32_t result = SRL::Cd::ChangeDir(validDir);
    snprintf(buffer, buffer_size, "Failed to change to valid directory '%s': %d", validDir, result);
    mu_assert(result >= Cd::ErrorCode::ErrorOk, buffer);
}

    /**
 * @brief Tests changing to a non-existent directory.
 * @details `ChangeDir` is documented to return the number of files or an error code: a missing directory must give a negative value.
 */
MU_TEST(cd_test_change_to_invalid_directory)
{
    const char *invalidDir = "INVALID";

        // Attempt to change to the invalid directory
    int32_t result = SRL::Cd::ChangeDir(invalidDir);
    snprintf(buffer, buffer_size, "Changed to invalid directory '%s' but should not: %d", invalidDir, result);
    mu_assert(result < 0, buffer);
}

    /**
 * @brief Tests navigating to the parent directory ("..").
 * @details Verifies that after changing into a subdirectory, using ".." successfully
 *          returns to the parent directory.
 */
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

    /**
 * @brief Tests retrieving and validating the CD's Table of Contents (TOC).
 * @details Verifies that the `GetTable` function returns a TOC with valid first and last
 *          track numbers and that the first track has a valid type (Data or Audio).
 */
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

/** @brief POSFILE.BIN: 40 sectors minus 100 bytes; byte p is (p + 7 * (p >> 8)) & 0xFF, so a read shifted by any number of sectors shows. */
static const char *const cd_posfile_name = "POSFILE.BIN";
static const int32_t cd_posfile_size = 40 * 2048 - 100;

static uint8_t cd_posfile_byte(int32_t p)
{
    return static_cast<uint8_t>((p + 7 * (p >> 8)) & 0xFF);
}

/** @brief Index of the first byte of data that is not POSFILE.BIN content at start, or -1 if all match. */
static int32_t cd_posfile_mismatch(const uint8_t *data, int32_t start, int32_t length)
{
    for (int32_t i = 0; i < length; i++)
    {
        if (data[i] != cd_posfile_byte(start + i)) return i;
    }
    return -1;
}

    /**
 * @brief Seek then Read returns the bytes at the new position, for offsets inside a sector, on sector bounds,
 *        on the 5-sector read-ahead buffer bound, and near the end (where Read returns only what is left).
 */
MU_TEST(cd_test_seek_then_read)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    const int32_t offsets[] = {100, 0, 1, 2047, 2048, 2049, 5000, 10239, 10240, 10241, 30007, cd_posfile_size - 10};
    for (int32_t offset : offsets)
    {
        uint8_t data[64];
        const int32_t sought = file.Seek(offset);
        const int32_t read = file.Read(sizeof(data), data);
        const int32_t expected = SRL::Math::Min<int32_t>(sizeof(data), cd_posfile_size - offset);
        const int32_t bad = read > 0 ? cd_posfile_mismatch(data, offset, read) : -1;

        snprintf(buffer, buffer_size, "Seek(%ld) returned %ld", (long)offset, (long)sought);
        mu_assert(sought == offset, buffer);
        snprintf(buffer, buffer_size, "Read after Seek(%ld) returned %ld bytes instead of %ld", (long)offset, (long)read, (long)expected);
        mu_assert(read == expected, buffer);
        snprintf(buffer, buffer_size, "Read after Seek(%ld): byte %ld is 0x%02x, file has 0x%02x (wrong position in the file)",
            (long)offset, (long)bad, bad >= 0 ? data[bad] : 0, bad >= 0 ? cd_posfile_byte(offset + bad) : 0);
        mu_assert(bad < 0, buffer);
    }
}

    /**
 * @brief Reading the whole file in odd-sized pieces crosses sector and read-ahead buffer bounds:
 *        every byte is right, the total is the file size, then Read returns 0 and IsEOF is true.
 */
MU_TEST(cd_test_read_whole_file_in_chunks)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    int32_t total = 0;
    int32_t bad = -1;
    uint8_t data[1000];
    while (total < cd_posfile_size)
    {
        const int32_t read = file.Read(sizeof(data), data);
        if (read <= 0) break;
        const int32_t mismatch = cd_posfile_mismatch(data, total, read);
        if (mismatch >= 0 && bad < 0) bad = total + mismatch;
        total += read;
    }

    snprintf(buffer, buffer_size, "Reading POSFILE.BIN in 1000-byte pieces: first wrong byte at %ld", (long)bad);
    mu_assert(bad < 0, buffer);
    snprintf(buffer, buffer_size, "Reading POSFILE.BIN in 1000-byte pieces gave %ld bytes instead of %ld", (long)total, (long)cd_posfile_size);
    mu_assert(total == cd_posfile_size, buffer);
    mu_assert(file.IsEOF(), "IsEOF should be true after reading the whole file");
    mu_assert(file.GetCurrentPosition() == static_cast<size_t>(cd_posfile_size), "GetCurrentPosition should be the file size after reading it all");

    const int32_t more = file.Read(sizeof(data), data);
    snprintf(buffer, buffer_size, "Read at the end of the file returned %ld instead of 0", (long)more);
    mu_assert(more == 0, buffer);
}

    /**
 * @brief ReadSectors reads whole sectors in order, stops at the end of the file, and then returns 0.
 */
MU_TEST(cd_test_read_sectors_to_end)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    static uint8_t data[2 * 2048];
    int32_t total = 0;
    int32_t bad = -1;
    int32_t calls = 0;
    while (calls++ < 40)
    {
        const int32_t read = file.ReadSectors(2, data);
        if (read <= 0) break;
        const int32_t valid = SRL::Math::Min<int32_t>(read, cd_posfile_size - total);
        const int32_t mismatch = valid > 0 ? cd_posfile_mismatch(data, total, valid) : -1;
        if (mismatch >= 0 && bad < 0) bad = total + mismatch;
        total += read;
    }

    snprintf(buffer, buffer_size, "ReadSectors(2) loop: first wrong byte at %ld", (long)bad);
    mu_assert(bad < 0, buffer);
    snprintf(buffer, buffer_size, "ReadSectors(2) loop read %ld bytes for a %ld-byte file (40 sectors)", (long)total, (long)cd_posfile_size);
    mu_assert(total >= cd_posfile_size && total <= 40 * 2048, buffer);
    mu_assert(file.IsEOF(), "IsEOF should be true after ReadSectors reached the end");
    mu_assert(file.ReadSectors(1, data) == 0, "ReadSectors at the end of the file should return 0");
}

    /**
 * @brief File sizes match the files on disc: bytes, sector count, sector size and last sector size.
 */
MU_TEST(cd_test_file_sizes)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File small("FILE.TXT");
    Cd::File pos(cd_posfile_name);
    Cd::File big("TESTFILE.UTS");

    snprintf(buffer, buffer_size, "FILE.TXT size: %ld bytes, %ld sectors of %ld, last %ld (expected 16, 1, 2048, 16)",
        (long)small.Size.Bytes, (long)small.Size.Sectors, (long)small.Size.SectorSize, (long)small.Size.LastSectorSize);
    mu_assert(small.Size.Bytes == 16 && small.Size.Sectors == 1 && small.Size.SectorSize == 2048 && small.Size.LastSectorSize == 16, buffer);
    snprintf(buffer, buffer_size, "POSFILE.BIN size: %ld bytes, %ld sectors, last %ld (expected %ld, 40, 1948)",
        (long)pos.Size.Bytes, (long)pos.Size.Sectors, (long)pos.Size.LastSectorSize, (long)cd_posfile_size);
    mu_assert(pos.Size.Bytes == cd_posfile_size && pos.Size.Sectors == 40 && pos.Size.LastSectorSize == 1948, buffer);
    snprintf(buffer, buffer_size, "TESTFILE.UTS size: %ld bytes, %ld sectors (expected 524288, 256)", (long)big.Size.Bytes, (long)big.Size.Sectors);
    mu_assert(big.Size.Bytes == 524288 && big.Size.Sectors == 256, buffer);
}

    /**
 * @brief GetSectorCount gives the number of sectors a byte count takes (rounded up), and -1 on a closed file.
 */
MU_TEST(cd_test_get_sector_count)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.GetSectorCount(100) == -1, "GetSectorCount on a closed file should return -1");
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    const int32_t bytes[] = {0, 1, 2048, 2049, 10240};
    const int32_t sectors[] = {0, 1, 1, 2, 5};
    for (int32_t i = 0; i < 5; i++)
    {
        const int32_t got = file.GetSectorCount(bytes[i]);
        snprintf(buffer, buffer_size, "GetSectorCount(%ld) returned %ld instead of %ld", (long)bytes[i], (long)got, (long)sectors[i]);
        mu_assert(got == sectors[i], buffer);
    }
}

    /**
 * @brief LoadBytes with a sector offset reads from that sector; on an open file it keeps the file open and its
 *        access pointer where it was; on a missing file it reads nothing.
 */
MU_TEST(cd_test_load_bytes_offset_and_state)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);

    static uint8_t data[2048];
    const int32_t loaded = file.LoadBytes(3, 2048, data);
    const int32_t bad = loaded > 0 ? cd_posfile_mismatch(data, 3 * 2048, SRL::Math::Min<int32_t>(loaded, 2048)) : -1;
    snprintf(buffer, buffer_size, "LoadBytes(sector 3, 2048) returned %ld", (long)loaded);
    mu_assert(loaded > 0, buffer);
    snprintf(buffer, buffer_size, "LoadBytes(sector 3): byte %ld is not the file's byte at %ld", (long)bad, (long)(3 * 2048 + bad));
    mu_assert(bad < 0, buffer);

    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");
    uint8_t first[10];
    file.Read(sizeof(first), first);
    file.LoadBytes(0, 2048, data);
    snprintf(buffer, buffer_size, "After LoadBytes on an open file: open %d, access pointer %ld (was 10)", file.IsOpen() ? 1 : 0, (long)file.GetCurrentAccessPointer());
    mu_assert(file.IsOpen() && file.GetCurrentAccessPointer() == 10, buffer);

    Cd::File missing("MISSING.TXT");
    const int32_t none = missing.LoadBytes(0, 2048, data);
    snprintf(buffer, buffer_size, "LoadBytes on a missing file returned %ld (should read nothing)", (long)none);
    mu_assert(none <= 0, buffer);
}

    /**
 * @brief Read, Seek and ReadSectors on a closed file fail; Open twice and Close twice are harmless.
 */
MU_TEST(cd_test_closed_file_and_double_calls)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file("FILE.TXT");
    uint8_t data[2048];

    mu_assert(file.Read(4, data) < 0, "Read on a closed file should fail");
    mu_assert(file.Seek(0) < 0, "Seek on a closed file should fail");
    mu_assert(file.ReadSectors(1, data) <= 0, "ReadSectors on a closed file should read nothing");
    mu_assert(file.GetCurrentAccessPointer() == -1, "GetCurrentAccessPointer on a closed file should return -1");

    mu_assert(file.Open() && file.Open(), "Opening an open file again should succeed");
    file.Close();
    file.Close();
    mu_assert(!file.IsOpen(), "File should be closed after Close twice");
    mu_assert(file.Open(), "File should open again after Close twice");

    char text[17] = {};
    mu_assert(file.Read(16, text) == 16 && strncmp(text, "ExpectedContent", 15) == 0, "File should read normally after Open/Close twice");
}

    /**
 * @brief Two files at once: with the default settings (SRL_MAX_CD_BACKGROUND_JOBS = 1) GFS opens one file at a time.
 *        Constructing a File while another is open must still find it, and the second opens once the first is closed.
 */
MU_TEST(cd_test_two_files)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File first("FILE1.TXT");
    mu_assert(first.Open(), "File 'FILE1.TXT' does not open but should");

    Cd::File second("FILE2.TXT");
    snprintf(buffer, buffer_size, "File 'FILE2.TXT' constructed while 'FILE1.TXT' is open: Exists %d, size %ld (expected 1, 14)",
        second.Exists() ? 1 : 0, (long)second.Size.Bytes);
    mu_assert(second.Exists() && second.Size.Bytes == 14, buffer);

    first.Close();
    mu_assert(second.Open(), "File 'FILE2.TXT' should open once 'FILE1.TXT' is closed");
    char text[15] = {};
    mu_assert(second.Read(14, text) == 14 && strncmp(text, "File 2 Content", 14) == 0, "File 'FILE2.TXT' content");
}

    /**
 * @brief Files in ROOT read back their full content (OVERFLOW.TXT, FILE1.TXT).
 */
MU_TEST(cd_test_other_files_content)
{
    SRL::Cd::ChangeDir("ROOT");
    const char *names[] = {"OVERFLOW.TXT", "FILE1.TXT"};
    const char *contents[] = {"Overflow File Content", "File 1 Content"};
    for (int i = 0; i < 2; i++)
    {
        Cd::File file(names[i]);
        char text[32] = {};
        const int32_t length = static_cast<int32_t>(strlen(contents[i]));
        const bool open = file.Open();
        const int32_t read = open ? file.Read(sizeof(text) - 1, text) : -1;
        snprintf(buffer, buffer_size, "File '%s': open %d, read %ld bytes, content '%s' (expected '%s')", names[i], open ? 1 : 0, (long)read, text, contents[i]);
        mu_assert(read == length && strcmp(text, contents[i]) == 0, buffer);
    }
}

    /**
 * @brief ChangeDir(nullptr) and ChangeDir("") go back to the root from two levels down; ChangeDir on a file fails.
 */
MU_TEST(cd_test_navigate_to_root_directory)
{
    const char *roots[] = {nullptr, ""};
    for (const char *root : roots)
    {
        int32_t result = SRL::Cd::ChangeDir("ROOT");
        snprintf(buffer, buffer_size, "ChangeDir(ROOT) failed: %ld", (long)result);
        mu_assert(result >= 0, buffer);
        result = SRL::Cd::ChangeDir("SUB");
        snprintf(buffer, buffer_size, "ChangeDir(SUB) from ROOT failed: %ld", (long)result);
        mu_assert(result >= 0, buffer);

        result = SRL::Cd::ChangeDir(root);
        snprintf(buffer, buffer_size, "ChangeDir(%s) from ROOT/SUB failed: %ld", root == nullptr ? "nullptr" : "\"\"", (long)result);
        mu_assert(result >= 0, buffer);

        Cd::File inRoot("CD_UT.TXT");
        snprintf(buffer, buffer_size, "After ChangeDir(%s), CD_UT.TXT (in the root) is not found", root == nullptr ? "nullptr" : "\"\"");
        mu_assert(inRoot.Exists(), buffer);
    }

    SRL::Cd::ChangeDir("ROOT");
    const int32_t result = SRL::Cd::ChangeDir("FILE.TXT");
    snprintf(buffer, buffer_size, "ChangeDir on a file (FILE.TXT) should fail, returned %ld", (long)result);
    mu_assert(result < 0, buffer);
}

    /**
 * @brief Cd::File must not be copyable: a copy would close the same GFS handle and free the same buffer twice.
 */
MU_TEST(cd_test_file_not_copyable)
{
    mu_assert(!std::is_copy_constructible<Cd::File>::value, "Cd::File is copyable: copies close the GFS handle and free the read buffer twice");
    mu_assert(!std::is_copy_assignable<Cd::File>::value, "Cd::File is copy-assignable: copies close the GFS handle and free the read buffer twice");
}

    /**
 * @brief The tray is closed (a disc is being read); the table of contents has the CD block's layout (102 words)
 *        and describes a disc whose first track is data at frame 150.
 */
MU_TEST(cd_test_tray_and_table_details)
{
    mu_assert(!Cd::IsTrayOpen(), "IsTrayOpen should be false while the disc is read");

        // The CD block writes 102 words: 99 tracks, first track, last track, lead-out
    snprintf(buffer, buffer_size, "TableOfContents is %lu bytes but the CD block writes 408 (102 x 4): FirstTrack at byte %lu is never filled",
        (unsigned long)sizeof(Cd::TableOfContents), (unsigned long)offsetof(Cd::TableOfContents, FirstTrack));
    mu_assert(sizeof(Cd::TableOfContents) == 102 * sizeof(uint32_t), buffer);

    Cd::TableOfContents toc = Cd::TableOfContents::GetTable();
    snprintf(buffer, buffer_size, "TOC: first track %d, last track %d", toc.FirstTrack.Number, toc.LastTrack.Number);
    mu_assert(toc.FirstTrack.Number == 1 && toc.LastTrack.Number >= toc.FirstTrack.Number, buffer);

    snprintf(buffer, buffer_size, "TOC: track 1 type %d, frame address %lu (expected data at 150)", toc.Tracks[0].GetType(), (unsigned long)toc.Tracks[0].FrameAddress);
    mu_assert(toc.Tracks[0].GetType() == Cd::TableOfContents::TrackType::Data && toc.Tracks[0].FrameAddress == 150, buffer);

    snprintf(buffer, buffer_size, "TOC: lead-out frame address %lu should be after track 1 (%lu)", (unsigned long)toc.Session.fad, (unsigned long)toc.Tracks[0].FrameAddress);
    mu_assert(toc.Session.fad > toc.Tracks[0].FrameAddress, buffer);
}

    /**
 * @brief Cd::Initialize succeeds, and calling it again is harmless.
 */
MU_TEST(cd_test_initialize_twice)
{
    mu_assert(Cd::Initialize(), "Cd::Initialize should succeed");
    mu_assert(Cd::Initialize(), "Calling Cd::Initialize again should still succeed");
    Cd::File file("CD_UT.TXT");
    mu_assert(file.Exists(), "Files should still be found after Initialize is called twice");
}

    /**
 * @brief An empty file: size 0 (not negative), opens, is at EOF at once, reads nothing, Seek(0) works.
 */
MU_TEST(cd_test_empty_file)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file("EMPTY.TXT");
    mu_assert(file.Exists(), "File 'EMPTY.TXT' does not exist but should");

    snprintf(buffer, buffer_size, "EMPTY.TXT size: %ld bytes, %ld sectors, last %ld (expected 0 bytes)",
        (long)file.Size.Bytes, (long)file.Size.Sectors, (long)file.Size.LastSectorSize);
    mu_assert(file.Size.Bytes == 0, buffer);

    mu_assert(file.Open(), "File 'EMPTY.TXT' does not open but should");
    mu_assert(file.IsEOF(), "An empty file should be at EOF right after Open");

    uint8_t data[16];
    const int32_t read = file.Read(sizeof(data), data);
    snprintf(buffer, buffer_size, "Read on an empty file returned %ld (should read nothing)", (long)read);
    mu_assert(read <= 0, buffer);

    const int32_t sought = file.Seek(0);
    snprintf(buffer, buffer_size, "Seek(0) on an empty file returned %ld instead of 0", (long)sought);
    mu_assert(sought == 0, buffer);
}

    /**
 * @brief One large Read (20 000 bytes, then 30 001) refills the 5-sector buffer several times in one call.
 */
MU_TEST(cd_test_large_read)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    static uint8_t data[30001];
    const int32_t sizes[] = {20000, 30001};
    int32_t position = 0;
    for (int32_t size : sizes)
    {
        const int32_t read = file.Read(size, data);
        const int32_t bad = read > 0 ? cd_posfile_mismatch(data, position, read) : -1;
        snprintf(buffer, buffer_size, "Read(%ld) at %ld returned %ld bytes", (long)size, (long)position, (long)read);
        mu_assert(read == size, buffer);
        snprintf(buffer, buffer_size, "Read(%ld) at %ld: byte %ld is wrong", (long)size, (long)position, (long)bad);
        mu_assert(bad < 0, buffer);
        position += read;
    }
}

    /**
 * @brief After Seek to the end, Read returns 0 bytes and the access pointer stays at the end.
 */
MU_TEST(cd_test_read_after_seek_to_end)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    uint8_t data[64];
    mu_assert(file.Seek(cd_posfile_size) == cd_posfile_size, "Seek to the end should succeed");
    const int32_t read = file.Read(sizeof(data), data);
    snprintf(buffer, buffer_size, "Read after Seek to the end returned %ld instead of 0; access pointer %ld",
        (long)read, (long)file.GetCurrentAccessPointer());
    mu_assert(read == 0 && file.GetCurrentAccessPointer() == cd_posfile_size, buffer);
}

    /**
 * @brief Seek, ReadSectors and Read share one position: after Seek to a sector start, ReadSectors reads that sector,
 *        and Read continues right after it.
 */
MU_TEST(cd_test_mix_seek_read_sectors_read)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);
    mu_assert(file.Open(), "File 'POSFILE.BIN' does not open but should");

    static uint8_t sector[2048];
    mu_assert(file.Seek(2 * 2048) == 2 * 2048, "Seek(4096) should succeed");
    const int32_t read = file.ReadSectors(1, sector);
    const int32_t bad = read > 0 ? cd_posfile_mismatch(sector, 2 * 2048, SRL::Math::Min<int32_t>(read, 2048)) : -1;
    snprintf(buffer, buffer_size, "ReadSectors(1) after Seek(4096) returned %ld bytes, first wrong byte %ld (should be sector 2)", (long)read, (long)bad);
    mu_assert(read == 2048 && bad < 0, buffer);

    snprintf(buffer, buffer_size, "Access pointer after ReadSectors is %ld instead of 6144", (long)file.GetCurrentAccessPointer());
    mu_assert(file.GetCurrentAccessPointer() == 3 * 2048, buffer);

    uint8_t data[16];
    const int32_t next = file.Read(sizeof(data), data);
    const int32_t badNext = next > 0 ? cd_posfile_mismatch(data, 3 * 2048, next) : -1;
    snprintf(buffer, buffer_size, "Read after ReadSectors returned %ld bytes, first wrong byte %ld (should continue at 6144)", (long)next, (long)badNext);
    mu_assert(next == sizeof(data) && badNext < 0, buffer);
}

    /**
 * @brief LoadBytes stops at the end of the file: from the last (partial) sector it returns only the bytes left,
 *        and from past the last sector it reads nothing.
 */
MU_TEST(cd_test_load_bytes_past_end)
{
    SRL::Cd::ChangeDir("ROOT");
    Cd::File file(cd_posfile_name);

    static uint8_t data[4096];
    const int32_t lastSize = cd_posfile_size - 39 * 2048;
    const int32_t loaded = file.LoadBytes(39, sizeof(data), data);
    const int32_t bad = loaded > 0 ? cd_posfile_mismatch(data, 39 * 2048, SRL::Math::Min<int32_t>(loaded, lastSize)) : -1;
    snprintf(buffer, buffer_size, "LoadBytes(last sector, 4096) returned %ld instead of %ld (bytes left), first wrong byte %ld", (long)loaded, (long)lastSize, (long)bad);
    mu_assert(loaded == lastSize && bad < 0, buffer);

    const int32_t beyond = file.LoadBytes(40, sizeof(data), data);
    snprintf(buffer, buffer_size, "LoadBytes(past the last sector) returned %ld (should read nothing)", (long)beyond);
    mu_assert(beyond <= 0, buffer);
}

    /**
 * @brief A File built from an already open GFS handle and its identifier is open, has its size, and reads.
 */
MU_TEST(cd_test_file_from_handle)
{
    SRL::Cd::ChangeDir("ROOT");
    const int32_t fid = GFS_NameToId((int8_t *)"FILE.TXT");
    mu_assert(fid >= 0, "GFS_NameToId(FILE.TXT) failed");
    GfsHn handle = GFS_Open(fid);
    mu_assert(handle != nullptr, "GFS_Open(FILE.TXT) failed");

    Cd::File file(handle, fid);
    snprintf(buffer, buffer_size, "File from handle: open %d, identifier %ld (expected %ld), size %ld (expected 16)",
        file.IsOpen() ? 1 : 0, (long)file.GetIdentifier(), (long)fid, (long)file.Size.Bytes);
    mu_assert(file.IsOpen() && file.GetIdentifier() == fid && file.Size.Bytes == 16, buffer);

    char text[17] = {};
    mu_assert(file.Read(16, text) == 16 && strncmp(text, "ExpectedContent", 15) == 0, "File from handle should read its content");
}

    /**
 * @brief Directories: ".." at the root stays at the root, a file in ROOT/SUB reads, and a lowercase name either is
 *        not found or finds the same file as the uppercase one (ISO 9660 names are uppercase).
 */
MU_TEST(cd_test_directory_edges)
{
    int32_t result = SRL::Cd::ChangeDir("..");
    snprintf(buffer, buffer_size, "ChangeDir(\"..\") at the root failed: %ld", (long)result);
    mu_assert(result >= 0, buffer);
    Cd::File inRoot("CD_UT.TXT");
    mu_assert(inRoot.Exists(), "After ChangeDir(\"..\") at the root, CD_UT.TXT (in the root) is not found");

    Cd::File lower("cd_ut.txt");
    snprintf(buffer, buffer_size, "Lowercase 'cd_ut.txt' found identifier %ld, uppercase is %ld", (long)lower.GetIdentifier(), (long)inRoot.GetIdentifier());
    mu_assert(!lower.Exists() || lower.GetIdentifier() == inRoot.GetIdentifier(), buffer);

    SRL::Cd::ChangeDir("ROOT");
    result = SRL::Cd::ChangeDir("SUB");
    snprintf(buffer, buffer_size, "ChangeDir(SUB) failed: %ld", (long)result);
    mu_assert(result >= 0, buffer);

    Cd::File inSub("INSUB.TXT");
    char text[16] = {};
    const bool open = inSub.Open();
    const int32_t read = open ? inSub.Read(10, text) : -1;
    snprintf(buffer, buffer_size, "ROOT/SUB/INSUB.TXT: exists %d, open %d, read %ld, content '%s' (expected 'Inside SUB')", inSub.Exists() ? 1 : 0, open ? 1 : 0, (long)read, text);
    mu_assert(read == 10 && strcmp(text, "Inside SUB") == 0, buffer);
}

    /**
 * @brief Defines the test suite for all CD-related functionality.
 */
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
    MU_RUN_TEST(cd_test_navigate_to_root_directory);
    MU_RUN_TEST(cd_test_table_of_contents);
    MU_RUN_TEST(cd_test_tray_and_table_details);
    MU_RUN_TEST(cd_test_file_sizes);
    MU_RUN_TEST(cd_test_get_sector_count);
    MU_RUN_TEST(cd_test_seek_then_read);
    MU_RUN_TEST(cd_test_read_whole_file_in_chunks);
    MU_RUN_TEST(cd_test_read_sectors_to_end);
    MU_RUN_TEST(cd_test_load_bytes_offset_and_state);
    MU_RUN_TEST(cd_test_closed_file_and_double_calls);
    MU_RUN_TEST(cd_test_two_files);
    MU_RUN_TEST(cd_test_other_files_content);
    MU_RUN_TEST(cd_test_file_not_copyable);
    MU_RUN_TEST(cd_test_initialize_twice);
    MU_RUN_TEST(cd_test_empty_file);
    MU_RUN_TEST(cd_test_large_read);
    MU_RUN_TEST(cd_test_read_after_seek_to_end);
    MU_RUN_TEST(cd_test_mix_seek_read_sectors_read);
    MU_RUN_TEST(cd_test_load_bytes_past_end);
    MU_RUN_TEST(cd_test_file_from_handle);
    MU_RUN_TEST(cd_test_directory_edges);
}
}
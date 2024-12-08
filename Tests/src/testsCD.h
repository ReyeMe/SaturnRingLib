#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C"
{

    extern const uint8_t buffer_size;
    extern char buffer[];

    void cd_test_setup(void)
    {
        // Setup for CD tests, if necessary
    }

    void cd_test_teardown(void)
    {
        // Cleanup for CD tests, if necessary
    }

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

    // Test: Directory listing
    // MU_TEST(cd_test_directory_listing) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     bool success = cdSystem.ListDirectory("root");
    //     snprintf(buffer, buffer_size, "Directory listing failed for 'root'");
    //     mu_assert(success, buffer);
    // }

    // Test: File existence
    MU_TEST(cd_test_file_exists) {
        const char * filename = "CD_UT_SMP.TXT";

        bool success = SRL::Cd::Initialize();
        snprintf(buffer, buffer_size, "CD system initialization failed");
        mu_assert(success, buffer);

        SRL::Cd::File file(filename);
        // bool exists = file.Exists();
        // snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        // mu_assert(exists, buffer);

        // bool open = file.Open();
        // snprintf(buffer, buffer_size, "File '%s' does not open but should", filename);
        // mu_assert(open, buffer);
        //
        // bool isopen = file.IsOpen();
        // snprintf(buffer, buffer_size, "File '%s' is not open but should", filename);
        // mu_assert(isopen, buffer);
        //
        // file.Close();
        // isopen = file.IsOpen();
        // snprintf(buffer, buffer_size, "File '%s' is open but should not", filename);
        // mu_assert(!isopen, buffer);
        //
        // exists = file.Exists();
        // snprintf(buffer, buffer_size, "File '%s' does not exist but should", filename);
        // mu_assert(exists, buffer);
    }

    // Test: File reading
    // MU_TEST(cd_test_read_file) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     char fileContent[1024];
    //     bool success = cdSystem.ReadFile("root/file.txt", fileContent, sizeof(fileContent));
    //     snprintf(buffer, buffer_size, "File 'root/file.txt' could not be read");
    //     mu_assert(success, buffer);
    //     snprintf(buffer, buffer_size, "Unexpected file content: %s", fileContent);
    //     mu_assert(strcmp(fileContent, "ExpectedContent") == 0, buffer);
    // }

    // Test: Handle missing file
    // MU_TEST(cd_test_missing_file) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     char fileContent[1024];
    //     bool success = cdSystem.ReadFile("root/missing.txt", fileContent, sizeof(fileContent));
    //     snprintf(buffer, buffer_size, "Reading missing file did not fail as expected");
    //     mu_assert(!success, buffer);
    // }

    // Test: Maximum files limit
    // MU_TEST(cd_test_max_files) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     for (int i = 0; i < SRL_MAX_CD_FILES; ++i) {
    //         char filename[64];
    //         snprintf(filename, sizeof(filename), "root/file%d.txt", i);
    //         bool success = cdSystem.FileExists(filename);
    //         snprintf(buffer, buffer_size, "File %s does not exist but should", filename);
    //         mu_assert(success, buffer);
    //     }
    //
    //     char overflowFile[64] = "root/overflow.txt";
    //     bool success = cdSystem.FileExists(overflowFile);
    //     snprintf(buffer, buffer_size, "File limit overflow incorrectly allowed: %s", overflowFile);
    //     mu_assert(!success, buffer);
    // }

    // Test: Background job processing
    // MU_TEST(cd_test_background_jobs) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     bool success = cdSystem.StartBackgroundJob("job1");
    //     snprintf(buffer, buffer_size, "Failed to start background job: job1");
    //     mu_assert(success, buffer);
    //
    //     success = cdSystem.StartBackgroundJob("job2");
    //     snprintf(buffer, buffer_size, "Failed to start background job: job2");
    //     mu_assert(success, buffer);
    //
    //     bool completed = cdSystem.CheckJobStatus("job1");
    //     snprintf(buffer, buffer_size, "Background job 'job1' did not complete as expected");
    //     mu_assert(completed, buffer);
    // }

    // Test: Invalid directory
    // MU_TEST(cd_test_invalid_directory) {
    //     Cd cdSystem;
    //     cdSystem.Initialize();
    //     bool success = cdSystem.ListDirectory("invalid");
    //     snprintf(buffer, buffer_size, "Listing invalid directory did not fail as expected");
    //     mu_assert(!success, buffer);
    // }

    MU_TEST_SUITE(cd_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&cd_test_setup,
                                       &cd_test_teardown,
                                       &cd_test_output_header);

        MU_RUN_TEST(cd_test_initialize);
        //MU_RUN_TEST(cd_test_directory_listing);
         MU_RUN_TEST(cd_test_file_exists);
        // MU_RUN_TEST(cd_test_read_file);
        // MU_RUN_TEST(cd_test_missing_file);
        // MU_RUN_TEST(cd_test_max_files);
        // MU_RUN_TEST(cd_test_background_jobs);
        // MU_RUN_TEST(cd_test_invalid_directory);
    }
}

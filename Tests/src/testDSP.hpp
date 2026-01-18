// Tests/src/testDSP.hpp
#pragma once

#include <srl.hpp>
#include <srl_scu.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Logger;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    /** @brief Setup routine for DSP unit tests
     */
    inline void dsp_test_setup(void)
    {
    }

    /** @brief Tear down routine for DSP unit tests
     */
    inline void dsp_test_teardown(void)
    {
    }

    /** @brief Output header for DSP test suite error reporting
     */
    inline void dsp_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_DSP****");
            }
            else
            {
                LogInfo("****UT_DSP_ERROR(S)****");
            }
        }
    }

    /** @brief Minimal DSP program used for unit tests
     * @details This program immediately ends (single ENDI instruction encoding).
     */
    static constexpr uint32_t DspProgramEnd[] =
    {
        0xE0000000U,
    };

    /** @brief Read a 32-bit MMIO register
     * @param address Address to read
     * @return Current value
     */
    inline static uint32_t ReadRegister32(uintptr_t address)
    {
        return *reinterpret_cast<volatile uint32_t *>(address);
    }

    /** @brief Write a 32-bit MMIO register
     * @param address Address to write
     * @param value Value to write
     */
    inline static void WriteRegister32(uintptr_t address, uint32_t value)
    {
        *reinterpret_cast<volatile uint32_t *>(address) = value;
    }

    /** @brief Wait for DSP end (with timeout)
     * @param maxSyncCount Maximum number of SRL sync cycles before giving up
     * @return true if DSP ended, false if timed out
     */
    inline static bool WaitForDspEnd(uint16_t maxSyncCount)
    {
        for (uint16_t i = 0; i < maxSyncCount; i++)
        {
            if (SRL::SCU::DSP::CheckEnd() == SRL::SCU::DSP::EndState::Ended)
            {
                return true;
            }

            SRL::Core::Synchronize();
        }

        return false;
    }

    /** @brief Test DSP program load/start/end flow
     */
    MU_TEST(dsp_test_load_start_and_end)
    {
        // Ensure DSP is stopped and any stale completion interrupt is consumed
        SRL::SCU::DSP::Stop();
        (void)SRL::SCU::DSP::CheckEnd();

        // Load an immediate-end program at PC=0
        SRL::SCU::DSP::LoadProgram(0x00, DspProgramEnd, (uint16_t)(sizeof(DspProgramEnd) / sizeof(DspProgramEnd[0])));
        SRL::SCU::DSP::Start(0x00);

        bool ended = WaitForDspEnd(120);
        mu_assert(ended, "DSP did not signal end within timeout");
    }

    /** @brief Test that WriteData writes into the data port and sets address
     */
    MU_TEST(dsp_test_write_read_data_roundtrip)
    {
        SRL::SCU::DSP::Stop();

        const uint8_t address = 0x00;
        const uint32_t inWords[] =
        {
            0x11223344U,
            0x55667788U,
            0xAABBCCDDU,
            0x0F0E0D0CU,
            0x10203040U,
            0xCAFEBABEU,
        };

        uint32_t outWords[sizeof(inWords) / sizeof(inWords[0])] = {};

        SRL::SCU::DSP::WriteData(address, inWords, (uint16_t)(sizeof(inWords) / sizeof(inWords[0])));
        SRL::SCU::DSP::ReadData(outWords, address, (uint16_t)(sizeof(outWords) / sizeof(outWords[0])));

        for (uint16_t i = 0; i < (uint16_t)(sizeof(outWords) / sizeof(outWords[0])); i++)
        {
            snprintf(buffer, buffer_size, "DSP RAM mismatch at %u: 0x%08lx != 0x%08lx",
                (unsigned)i,
                (unsigned long)outWords[i],
                (unsigned long)inWords[i]);
            mu_assert(outWords[i] == inWords[i], buffer);
        }
    }

    /** @brief Test that Start sets the start bit/pc and Stop clears control
     */
    MU_TEST(dsp_test_start_and_stop)
    {
        SRL::SCU::DSP::Stop();
        mu_assert_int_eq(0, (int)ReadRegister32(SRL::SCU::DSP::RegisterMap::RwCtrl));

        // Start the END program from PC=0 (should return quickly)
        SRL::SCU::DSP::LoadProgram(0x00, DspProgramEnd, (uint16_t)(sizeof(DspProgramEnd) / sizeof(DspProgramEnd[0])));
        SRL::SCU::DSP::Start(0x00);
        mu_assert(WaitForDspEnd(120), "DSP end interrupt not received");

        SRL::SCU::DSP::Stop();
        mu_assert_int_eq(0, (int)ReadRegister32(SRL::SCU::DSP::RegisterMap::RwCtrl));
    }

    /** @brief Test CheckEnd returns NotEnded when interrupt is not set
     */
    MU_TEST(dsp_test_check_end_not_ended)
    {
        SRL::SCU::DSP::Stop();

        // Consume any pending completion if present
        (void)SRL::SCU::DSP::CheckEnd();

        SRL::SCU::DSP::EndState state = SRL::SCU::DSP::CheckEnd();
        mu_assert_int_eq((int)SRL::SCU::DSP::EndState::NotEnded, (int)state);
    }

    /** @brief DSP test suite configuration and test case registration
     */
    MU_TEST_SUITE(dsp_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&dsp_test_setup, &dsp_test_teardown, &dsp_test_output_header);

        MU_RUN_TEST(dsp_test_load_start_and_end);
        MU_RUN_TEST(dsp_test_write_read_data_roundtrip);
        MU_RUN_TEST(dsp_test_start_and_stop);
        MU_RUN_TEST(dsp_test_check_end_not_ended);
    }
}

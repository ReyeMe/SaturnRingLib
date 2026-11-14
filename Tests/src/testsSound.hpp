// Tests/src/testsSound.hpp
// SRL::Sound::Pcm with the SGL sound driver (loaded from the disc by Core::Initialize): playing on free channels,
// channel limits, stopping, and loading raw PCM files. TONE.PCM holds 0x400 bytes, byte i = (i * 7) & 0xff.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_sound.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Sound::Pcm;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Number of PCM channels SRL manages. */
static constexpr uint8_t SoundChannels = 4;

/** @brief Gives the tests access to the loaded PCM data. */
class RawPcmProbe : public Pcm::RawPcm
{
public:
    using RawPcm::RawPcm;
    const int8_t *Data() const { return this->data; }
    uint32_t Size() const { return this->dataSize; }
};

/** @brief Stops every channel and waits (a few frames at most) until the driver reports them free. */
static bool sound_stop_all()
{
    for (uint8_t channel = 0; channel < SoundChannels; channel++) Pcm::StopSound(channel);

    for (int frame = 0; frame < 30; frame++)
    {
        bool allFree = true;
        for (uint8_t channel = 0; channel < SoundChannels; channel++) allFree = allFree && Pcm::IsChannelFree(channel);
        if (allFree) return true;
        Core::Synchronize();
    }

    return false;
}

void sound_test_setup(void) { sound_stop_all(); }
void sound_test_teardown(void) { sound_stop_all(); }

void sound_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_SOUND_ERROR(S)****");
    }
}

/** @brief The sound driver files are on the disc (else every other test here fails for that reason). */
MU_TEST(sound_driver_files_on_disc)
{
    mu_assert(Cd::File("SDDRVS.TSK").Exists() && Cd::File("BOOTSND.MAP").Exists(), "SDDRVS.TSK or BOOTSND.MAP is missing from the disc");
}

/** @brief A raw PCM file is loaded as it is and padded with zeros up to the 0x900 bytes slPCMOn needs. */
MU_TEST(sound_raw_pcm_load)
{
    Cd::File file("TONE.PCM");
    RawPcmProbe pcm(&file, Pcm::PcmChannels::Mono, Pcm::PcmBitDepth::Pcm8Bit, 11025);

    snprintf(buffer, buffer_size, "Size is 0x%x, expected 0x900", static_cast<unsigned>(pcm.Size()));
    mu_assert(pcm.Data() != nullptr && pcm.Size() == 0x900, buffer);

    for (uint32_t i = 0; i < pcm.Size(); i++)
    {
        const int8_t expected = i < 0x400 ? static_cast<int8_t>((i * 7) & 0xff) : 0;
        snprintf(buffer, buffer_size, "Byte 0x%x is %d, expected %d", static_cast<unsigned>(i), pcm.Data()[i], expected);
        mu_assert(pcm.Data()[i] == expected, buffer);
    }
}

/** @brief Play takes a free channel, which is then busy until StopSound frees it. */
MU_TEST(sound_play_and_stop)
{
    Cd::File file("TONE.PCM");
    RawPcmProbe pcm(&file, Pcm::PcmChannels::Mono, Pcm::PcmBitDepth::Pcm8Bit, 11025);

    const int8_t channel = Pcm::Play(pcm);
    snprintf(buffer, buffer_size, "Play returned channel %d, expected 0 to 3", channel);
    mu_assert(channel >= 0 && channel < SoundChannels, buffer);
    mu_assert(!Pcm::IsChannelFree(channel), "The channel is free while it plays");

    mu_assert(Pcm::StopSound(channel), "StopSound failed");
    mu_assert(sound_stop_all(), "The channel did not become free after StopSound");
}

/** @brief The four channels can play at once; a fifth sound gets no channel. */
MU_TEST(sound_all_channels_busy)
{
    Cd::File file("TONE.PCM");
    RawPcmProbe pcm(&file, Pcm::PcmChannels::Mono, Pcm::PcmBitDepth::Pcm8Bit, 11025);
    bool used[SoundChannels] = { false, false, false, false };

    for (int i = 0; i < SoundChannels; i++)
    {
        const int8_t channel = Pcm::Play(pcm);
        snprintf(buffer, buffer_size, "Sound %d got channel %d", i, channel);
        mu_assert(channel >= 0 && channel < SoundChannels && !used[channel], buffer);
        used[channel] = true;
    }

    mu_assert(Pcm::Play(pcm) == -1, "A fifth sound got a channel");
}

/** @brief PlayOnChannel uses the given channel, and refuses it while it is busy. */
MU_TEST(sound_play_on_channel)
{
    Cd::File file("TONE.PCM");
    RawPcmProbe pcm(&file, Pcm::PcmChannels::Mono, Pcm::PcmBitDepth::Pcm8Bit, 11025);

    mu_assert(Pcm::PlayOnChannel(pcm, 2), "PlayOnChannel(2) failed on a free channel");
    mu_assert(!Pcm::IsChannelFree(2) && Pcm::IsChannelFree(1), "Channel 2 should be busy and channel 1 free");
    mu_assert(!Pcm::PlayOnChannel(pcm, 2), "PlayOnChannel(2) worked while channel 2 was busy");
}

/** @brief Channel numbers past the last channel are refused, not used. */
MU_TEST(sound_channel_out_of_range)
{
    mu_assert(!Pcm::IsChannelFree(SoundChannels), "Channel 4 reported free");
    mu_assert(!Pcm::StopSound(SoundChannels), "StopSound(4) succeeded");
    Pcm::SetVolumePan(SoundChannels, 100, 0); // must do nothing
}

MU_TEST_SUITE(sound_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&sound_test_setup, &sound_test_teardown, &sound_test_output_header);

    MU_RUN_TEST(sound_driver_files_on_disc);
    MU_RUN_TEST(sound_raw_pcm_load);
    MU_RUN_TEST(sound_play_and_stop);
    MU_RUN_TEST(sound_all_channels_busy);
    MU_RUN_TEST(sound_play_on_channel);
    MU_RUN_TEST(sound_channel_out_of_range);
}

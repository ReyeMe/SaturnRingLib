// Tests/src/testsSound.hpp
// SRL::Sound::Pcm with the SGL sound driver (loaded from the disc by Core::Initialize): playing on free channels,
// channel limits, stopping, and loading raw PCM files. TONE.PCM holds 0x400 bytes, byte i = (i * 7) & 0xff.
// cdda_test_suite: SRL::Sound::Cdda on the disc's CD-DA tracks.
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
    MU_SUITE_CONFIGURE(&sound_test_setup, &sound_test_teardown);

    MU_RUN_TEST(sound_driver_files_on_disc);
    MU_RUN_TEST(sound_raw_pcm_load);
    MU_RUN_TEST(sound_play_and_stop);
    MU_RUN_TEST(sound_all_channels_busy);
    MU_RUN_TEST(sound_play_on_channel);
    MU_RUN_TEST(sound_channel_out_of_range);
}

// SRL::Sound::Cdda on the two CD-DA tracks of the sound campaign disc (campaigns/sound/pre.makefile): disc track 2
// (3 s) and track 3 (3 s). The drive status comes from CDC_GetCurStat.

/** @brief Drive state (CDC_ST_*), track and frame address. */
struct CddaStatus
{
    uint8_t state;
    uint8_t track;
    int32_t fad;
};

static CddaStatus cdda_status()
{
    CdcStat stat;
    CDC_GetCurStat(&stat);
    return { static_cast<uint8_t>(stat.status & 0x0f), stat.report.tno, stat.report.fad };
}

/** @brief Waits up to frames frames for the drive to play track; true when it does. */
static bool cdda_wait_playing(uint8_t track, int frames)
{
    for (int frame = 0; frame < frames; frame++)
    {
        const CddaStatus status = cdda_status();
        if (status.state == CDC_ST_PLAY && status.track == track) return true;
        Core::Synchronize();
    }

    return false;
}

/** @brief Waits up to frames frames for the drive to stop playing.
 * @return Last track seen playing (0 if none), or 0xff if it still plays
 */
static uint8_t cdda_wait_end(int frames)
{
    uint8_t lastTrack = 0;
    for (int frame = 0; frame < frames; frame++)
    {
        const CddaStatus status = cdda_status();
        if (status.state == CDC_ST_PLAY) lastTrack = status.track;
        else if (status.state == CDC_ST_PAUSE || status.state == CDC_ST_STANDBY) return lastTrack;
        Core::Synchronize();
    }

    return 0xff;
}

void cdda_test_teardown(void) { Sound::Cdda::StopPause(); }

/** @brief Track flags in the table of contents: track 1 is data, track 2 audio; neither is incremental or copyable. */
MU_TEST(cdda_toc_track_flags)
{
    Cd::TableOfContents toc = Cd::TableOfContents::GetTable();
    auto &data = toc.Tracks[0];
    auto &audio = toc.Tracks[1];

    snprintf(buffer, buffer_size, "Track 1 type %d (incremental %d, copy %d), track 2 type %d (copy %d)", static_cast<int>(data.GetType()),
        data.IsIncremental(), data.IsCopyPermitted(), static_cast<int>(audio.GetType()), audio.IsCopyPermitted());
    mu_assert(data.GetType() == Cd::TableOfContents::TrackType::Data && !data.IsIncremental() && !data.IsCopyPermitted(), buffer);
    mu_assert(audio.GetType() == Cd::TableOfContents::TrackType::Audio && !audio.IsCopyPermitted(), buffer);
}

/** @brief PlaySingle plays its track and stops at its end, without going on to the next track. */
MU_TEST(cdda_play_single_stops_at_track_end)
{
    Sound::Cdda::PlaySingle(2, false);
    mu_assert(cdda_wait_playing(2, 180), "PlaySingle(2) did not start track 2");

    const uint8_t lastTrack = cdda_wait_end(360);
    snprintf(buffer, buffer_size, "Track 2 ended on track %d (255: still playing after 6 s), expected 2", lastTrack);
    mu_assert(lastTrack == 2, buffer);
}

/** @brief Play(2, 3) plays track 2, then track 3. */
MU_TEST(cdda_play_range)
{
    Sound::Cdda::Play(2, 3, false);
    mu_assert(cdda_wait_playing(2, 180), "Play(2, 3) did not start track 2");
    mu_assert(cdda_wait_playing(3, 300), "Play(2, 3) did not go on to track 3");
}

/** @brief StopPause stops the music: the drive leaves the play state and stays out of it. */
MU_TEST(cdda_stop_pause)
{
    Sound::Cdda::PlaySingle(3, false);
    mu_assert(cdda_wait_playing(3, 180), "PlaySingle(3) did not start track 3");

    Sound::Cdda::StopPause();
    mu_assert(cdda_wait_end(60) != 0xff, "The drive still plays 1 s after StopPause");

    for (int frame = 0; frame < 30; frame++) Core::Synchronize();
    snprintf(buffer, buffer_size, "Half a second after StopPause the drive is in state %d", cdda_status().state);
    mu_assert(cdda_status().state != CDC_ST_PLAY, buffer);
}

/** @brief Resume goes on from where StopPause stopped, on the same track, and stops at that track's end. */
MU_TEST(cdda_resume_after_pause)
{
    Sound::Cdda::PlaySingle(2, false);
    mu_assert(cdda_wait_playing(2, 180), "PlaySingle(2) did not start track 2");
    for (int frame = 0; frame < 60; frame++) Core::Synchronize();

    // Position while playing: the drive reports none (0xffffff) once paused
    const int32_t pausedFad = cdda_status().fad;
    Sound::Cdda::StopPause();
    cdda_wait_end(60);

    Sound::Cdda::Resume();
    const bool playing = cdda_wait_playing(2, 120);
    const int32_t resumedFad = cdda_status().fad;
    const uint8_t lastTrack = playing ? cdda_wait_end(360) : 0;

    snprintf(buffer, buffer_size, "Resume did not play track 2 again (paused at %ld, state %d)", (long)pausedFad, cdda_status().state);
    mu_assert(playing, buffer);
    snprintf(buffer, buffer_size, "Resumed at frame address %ld, before the pause at %ld", (long)resumedFad, (long)pausedFad);
    mu_assert(resumedFad >= pausedFad, buffer);
    snprintf(buffer, buffer_size, "The resumed track ended on track %d (255: still playing after 6 s), expected 2", lastTrack);
    mu_assert(lastTrack == 2, buffer);
}

MU_TEST_SUITE(cdda_test_suite)
{
    MU_SUITE_CONFIGURE(NULL, &cdda_test_teardown);

    MU_RUN_TEST(cdda_toc_track_flags);
    MU_RUN_TEST(cdda_play_single_stops_at_track_end);
    MU_RUN_TEST(cdda_play_range);
    MU_RUN_TEST(cdda_stop_pause);
    MU_RUN_TEST(cdda_resume_after_pause);
}

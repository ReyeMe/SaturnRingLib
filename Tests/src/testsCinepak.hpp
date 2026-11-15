// Tests/src/testsCinepak.hpp
// SRL::CinepakPlayer on SKYBL.CPK, the movie of the "VDP1 - Cinepak" sample (copied to the disc by
// campaigns/sound/pre.makefile): a missing file, loading, playing a few frames, pausing and playing again, and giving
// all memory back. The player needs the SGL sound driver, so it runs in the sound campaign.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_cinepak.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Frames the player decoded (OnFrame calls) */
static int cinepak_frames = 0;

static void cinepak_count_frame(CinepakPlayer &) { cinepak_frames++; }

static size_t cinepak_used_blocks()
{
    return Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
}

/** @brief Runs frames until the player decoded `count` more frames, at most `limit` frames.
 * @return true when it did
 */
static bool cinepak_wait_frames(int count, int limit)
{
    const int target = cinepak_frames + count;
    for (int frame = 0; frame < limit && cinepak_frames < target; frame++) Core::Synchronize();
    return cinepak_frames >= target;
}

void cinepak_test_setup(void) { cinepak_frames = 0; }
void cinepak_test_teardown(void) {}

void cinepak_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_CINEPAK_ERROR(S)****");
    }
}

/** @brief A missing movie is refused, and the player keeps no memory. */
MU_TEST(cinepak_missing_file)
{
    const size_t before = cinepak_used_blocks();
    {
        CinepakPlayer player;
        mu_assert(!player.LoadMovie("NOPE.CPK"), "LoadMovie accepted a missing file");
        mu_assert(player.GetStatus() == CinepakPlayer::PlaybackStateEnum::Error && player.GetFrameData() == nullptr,
            "The player has a movie after a failed LoadMovie");
    }

    snprintf(buffer, buffer_size, "Used blocks %d -> %d", before, cinepak_used_blocks());
    mu_assert(cinepak_used_blocks() == before, buffer);
}

/** @brief The movie loads with its size, plays (OnFrame fires, time goes on), pauses at its next key frame and plays
 *         again; the player gives all its memory back when it is destroyed. */
MU_TEST(cinepak_play_pause_unload)
{
    const size_t before = cinepak_used_blocks();
    {
        CinepakPlayer player;
        player.OnFrame += cinepak_count_frame;

        mu_assert(player.LoadMovie("SKYBL.CPK"), "LoadMovie(SKYBL.CPK) failed");
        const Types::Resolution size = player.GetResolution();
        snprintf(buffer, buffer_size, "Movie is %dx%d, frame data %p", size.Width, size.Height, player.GetFrameData());
        mu_assert(size.Width > 0 && size.Height > 0 && player.GetFrameData() != nullptr, buffer);

        player.Play();
        mu_assert(cinepak_wait_frames(10, 600), "Fewer than 10 frames decoded in 10 s");
        snprintf(buffer, buffer_size, "Time is %d ms after 10 frames", (player.GetTime() * 1000).As<int32_t>());
        mu_assert(player.GetTime() > 0.0, buffer);

        // Pause waits for the next key frame: 21.8 s into SKYBL.CPK (key frames at 0, 21.8 and 27.9 s)
        player.Pause();
        for (int frame = 0; frame < 1500 && player.GetStatus() != CinepakPlayer::PlaybackStateEnum::Paused; frame++) Core::Synchronize();
        snprintf(buffer, buffer_size, "Status %d 25 s after Pause, expected Paused (1)", static_cast<int>(player.GetStatus()));
        mu_assert(player.GetStatus() == CinepakPlayer::PlaybackStateEnum::Paused, buffer);

        const int pausedFrames = cinepak_frames;
        for (int frame = 0; frame < 30; frame++) Core::Synchronize();
        mu_assert(cinepak_frames == pausedFrames, "Frames are still decoded while paused");

        player.Play();
        mu_assert(cinepak_wait_frames(1, 300), "No frame decoded 5 s after playing again");
    }

    snprintf(buffer, buffer_size, "Used blocks %d -> %d after destroying the player", before, cinepak_used_blocks());
    mu_assert(cinepak_used_blocks() == before, buffer);
}

/** @brief Loading a movie into a player that has one replaces it: the second load works and, once the player is gone,
 *         all memory is back. Last test: it may leave the disc file handle open. */
MU_TEST(cinepak_load_twice)
{
    const size_t before = cinepak_used_blocks();
    bool first = false;
    bool second = false;
    {
        CinepakPlayer player;
        first = player.LoadMovie("SKYBL.CPK");
        second = player.LoadMovie("SKYBL.CPK");
    }

    mu_assert(first, "The first LoadMovie failed");
    mu_assert(second, "The second LoadMovie failed");
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after destroying the player", before, cinepak_used_blocks());
    mu_assert(cinepak_used_blocks() == before, buffer);
}

MU_TEST_SUITE(cinepak_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&cinepak_test_setup, &cinepak_test_teardown, &cinepak_test_output_header);

    MU_RUN_TEST(cinepak_missing_file);
    MU_RUN_TEST(cinepak_play_pause_unload);
    MU_RUN_TEST(cinepak_load_twice);
}

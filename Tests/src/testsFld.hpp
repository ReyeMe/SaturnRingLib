// Tests/src/testsFld.hpp
// SRL::Fld (file loader). Two suites on one disc (written by campaigns/fld/pre.makefile, with TEXT.TXT in the root
// directory and in GAME/): fld_menu_test_suite runs in the fld campaign program (the menu), which then starts
// campaigns/fld_game with Fld::ExecGame("GAME.BIN", "GAME", 5); fld_game_test_suite runs in that game and checks what
// the loader set up. Fld::ReturnToMenu is not checked here: the BIOS clears the DDS block and work RAM on the way back,
// so the menu cannot tell its second boot from the first.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_fld.hpp>
#include <srl_sound.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Disc track the fld campaign gives as the game's first CD-DA track */
static constexpr uint8_t FldGameFirstTrack = 5;

/** @brief Reads TEXT.TXT of the current root directory into buffer. */
static const char *fld_read_text()
{
    static char text[32];
    for (char &c : text) c = '\0';
    Cd::File file("TEXT.TXT");
    if (!file.Exists()) return "(missing)";
    file.LoadBytes(0, sizeof(text) - 1, text);
    return text;
}

/** @brief The menu sees the whole disc: no DDS block, tracks are not moved. */
MU_TEST(fld_menu_is_not_started_by_a_menu)
{
    mu_assert(!Fld::IsRunningFromMenu(), "The menu thinks an FLD menu started it");
    mu_assert(Fld::FirstAudioTrack() == 2 && Sound::Cdda::DiscTrack(2) == 2, "Tracks are moved in the menu");
    snprintf(buffer, buffer_size, "Root TEXT.TXT reads '%s', expected 'root directory'", fld_read_text());
    mu_assert(strcmp(fld_read_text(), "root directory") == 0, buffer);
}

/** @brief A missing IP file or game directory, a file given as the directory, or an IP file over 8 KB (BIG.BIN,
 *         8193 bytes) fail without touching the DDS block. */
MU_TEST(fld_exec_game_missing_files)
{
    mu_assert(!Fld::ExecGame("NOPE.BIN", "GAME", FldGameFirstTrack), "ExecGame accepted a missing IP file");
    mu_assert(!Fld::ExecGame("GAME.BIN", "NODIR", FldGameFirstTrack), "ExecGame accepted a missing directory");
    mu_assert(!Fld::ExecGame("GAME.BIN", "TEXT.TXT", FldGameFirstTrack), "ExecGame accepted a file as the game directory");
    mu_assert(!Fld::ExecGame("BIG.BIN", "GAME", FldGameFirstTrack), "ExecGame accepted an IP file over 8 KB");
    mu_assert(!Fld::IsRunningFromMenu(), "A failed ExecGame wrote the DDS block");
}

/** @brief Starts campaigns/fld_game, which reports the rest. Only comes back if the game could not start. */
MU_TEST(fld_exec_game_starts)
{
    LogInfo("Starting GAME/0.BIN with the FLD kernel");
    Fld::ExecGame("GAME.BIN", "GAME", FldGameFirstTrack);
    mu_fail("ExecGame returned instead of starting the game");
}

MU_TEST_SUITE(fld_menu_test_suite)
{
    MU_RUN_TEST(fld_menu_is_not_started_by_a_menu);
    MU_RUN_TEST(fld_exec_game_missing_files);
    MU_RUN_TEST(fld_exec_game_starts);
}

/** @brief The game knows an FLD menu started it. */
MU_TEST(fld_game_started_by_menu)
{
    mu_assert(Fld::IsRunningFromMenu(), "The game does not see the DDS block");
}

/** @brief The game's directory is its root directory: it reads GAME/TEXT.TXT, and root files are out of reach. */
MU_TEST(fld_game_directory_is_root)
{
    snprintf(buffer, buffer_size, "TEXT.TXT reads '%s', expected 'GAME directory'", fld_read_text());
    mu_assert(strcmp(fld_read_text(), "GAME directory") == 0, buffer);
    mu_assert(!Cd::File("GAME.BIN").Exists(), "The game sees GAME.BIN of the disc root");
}

/** @brief Track 2 of the game is the track the menu gave; the next tracks follow it. */
MU_TEST(fld_game_audio_tracks)
{
    snprintf(buffer, buffer_size, "First track %d, track 2 -> %d, track 4 -> %d (expected %d, %d, %d)", Fld::FirstAudioTrack(),
        Sound::Cdda::DiscTrack(2), Sound::Cdda::DiscTrack(4), FldGameFirstTrack, FldGameFirstTrack, FldGameFirstTrack + 2);
    mu_assert(Fld::FirstAudioTrack() == FldGameFirstTrack && Sound::Cdda::DiscTrack(2) == FldGameFirstTrack &&
                  Sound::Cdda::DiscTrack(4) == FldGameFirstTrack + 2,
        buffer);
}

MU_TEST_SUITE(fld_game_test_suite)
{
    MU_RUN_TEST(fld_game_started_by_menu);
    MU_RUN_TEST(fld_game_directory_is_root);
    MU_RUN_TEST(fld_game_audio_tracks);
}

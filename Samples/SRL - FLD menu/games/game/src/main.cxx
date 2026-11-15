#include <srl.hpp>
#include <srl_fld.hpp>

using namespace SRL::Input;

#ifndef GAME_NUMBER
#define GAME_NUMBER 1
#endif

int main()
{
    SRL::Core::Initialize(GAME_NUMBER == 1 ? SRL::Types::HighColor(0, 60, 20) : SRL::Types::HighColor(70, 20, 0));

    SRL::Debug::Print(1, 1, "Game %d", GAME_NUMBER);
    SRL::Debug::Print(1, 3, SRL::Fld::IsRunningFromMenu() ? "Started by the FLD menu" : "Started alone");

    // GFS treats the game directory as the root directory: every game has its own TEXT.TXT
    char text[64] = { '\0' };
    SRL::Cd::File file("TEXT.TXT");
    if (file.Exists()) file.LoadBytes(0, sizeof(text) - 1, text);
    SRL::Debug::Print(1, 5, "TEXT.TXT: %s", file.Exists() ? text : "missing");

    // Track 2 is this game's first CD-DA track wherever it is on the disc
    SRL::Sound::Cdda::PlaySingle(2, true);
    SRL::Debug::Print(1, 7, "Playing CD-DA track 2 (disc track %d)", SRL::Sound::Cdda::DiscTrack(2));

    SRL::Debug::Print(1, 9, "START: back to the menu");

    Digital pad(0);

    while (true)
    {
        if (pad.WasPressed(Digital::Button::START))
        {
            SRL::Fld::ReturnToMenu();
        }

        SRL::Core::Synchronize();
    }

    return 0;
}

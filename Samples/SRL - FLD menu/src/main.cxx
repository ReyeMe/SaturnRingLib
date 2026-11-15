#include <srl.hpp>
#include <srl_fld.hpp>

using namespace SRL::Input;

/** @brief A game of the disc: its IP file and directory (see pre.makefile), and the disc track of its first CD-DA track */
struct Game
{
    const char *Name;
    const char *IpFile;
    const char *Directory;
    uint8_t FirstAudioTrack;
};

static const Game Games[] = {
    { "Game 1", "GAME1.BIN", "GAME1", 2 },
    { "Game 2", "GAME2.BIN", "GAME2", 3 },
};

static constexpr int GameCount = sizeof(Games) / sizeof(Games[0]);

int main()
{
    // The menu sees the whole disc, even after a game came back to it
    SRL::Fld::InitializeMenu();
    SRL::Core::Initialize(SRL::Types::HighColor(20, 10, 50));

    Digital pad(0);
    int selected = 0;

    SRL::Debug::Print(1, 1, "FLD menu");
    SRL::Debug::Print(1, 2, "Up/Down: choose, A or START: run");

    while (true)
    {
        if (pad.WasPressed(Digital::Button::Up)) selected = (selected + GameCount - 1) % GameCount;
        if (pad.WasPressed(Digital::Button::Down)) selected = (selected + 1) % GameCount;

        for (int i = 0; i < GameCount; i++)
        {
            SRL::Debug::Print(3, 5 + i, "%c %s (CD-DA from track %d)", i == selected ? '>' : ' ', Games[i].Name, Games[i].FirstAudioTrack);
        }

        if (pad.WasPressed(Digital::Button::A) || pad.WasPressed(Digital::Button::START))
        {
            const Game &game = Games[selected];
            SRL::Debug::PrintClearLine(9);
            SRL::Debug::Print(1, 9, "Starting %s...", game.Name);
            SRL::Core::Synchronize();

            // Does not return when the game starts
            SRL::Fld::ExecGame(game.IpFile, game.Directory, game.FirstAudioTrack);
            SRL::Debug::Print(1, 9, "Could not start %s: %s or %s is missing", game.Name, game.IpFile, game.Directory);
        }

        SRL::Core::Synchronize();
    }

    return 0;
}

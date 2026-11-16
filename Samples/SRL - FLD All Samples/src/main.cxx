#include <srl.hpp>
#include <srl_fld.hpp>

using namespace SRL::Input;

/** @brief A sample on the disc: its directory, and the disc track of its first CD-DA track (see pre.makefile) */
struct Sample
{
    const char *Name;
    const char *Directory;
    uint8_t FirstAudioTrack;
};

// Samples[], written by pre.makefile
#include "samples.gen.hpp"

/** @brief IP file of every sample */
static constexpr const char *IpFile = "SAMPLES.BIN";

static constexpr int SampleCount = sizeof(Samples) / sizeof(Samples[0]);

/** @brief Screen rows of the list, and of the selected sample's details below it */
static constexpr int ListTop = 5;
static constexpr int ListRows = 20;
static constexpr int StatusRow = ListTop + ListRows + 1;

/** @brief Shows Samples[first] to Samples[first + ListRows - 1], with a cursor on the selected one */
static void DrawList(int first, int selected)
{
    for (int row = 0; row < ListRows; row++)
    {
        const int index = first + row;
        SRL::Debug::PrintClearLine(ListTop + row);

        if (index < SampleCount)
        {
            SRL::Debug::Print(1, ListTop + row, "%c %s", index == selected ? '>' : ' ', Samples[index].Name);
        }
    }

    const Sample &sample = Samples[selected];
    SRL::Debug::PrintClearLine(StatusRow);
    SRL::Debug::Print(1, StatusRow, "%d/%d  %s  CD-DA from track %d", selected + 1, SampleCount, sample.Directory, sample.FirstAudioTrack);
}

int main()
{
    // The menu sees the whole disc
    SRL::Fld::InitializeMenu();
    SRL::Core::Initialize(SRL::Types::HighColor(20, 10, 50));

    Digital pad(0);
    int selected = 0;
    int first = 0;

    SRL::Debug::Print(1, 1, "All SRL samples");
    SRL::Debug::Print(1, 2, "Up/Down, L/R: choose   A/START: run");
    SRL::Debug::Print(1, 3, "Reset the console to come back here");
    DrawList(first, selected);

    while (true)
    {
        const int previous = selected;

        if (pad.WasPressed(Digital::Button::Up)) selected = (selected + SampleCount - 1) % SampleCount;
        if (pad.WasPressed(Digital::Button::Down)) selected = (selected + 1) % SampleCount;
        if (pad.WasPressed(Digital::Button::L)) selected = selected > ListRows ? selected - ListRows : 0;
        if (pad.WasPressed(Digital::Button::R)) selected = selected + ListRows < SampleCount ? selected + ListRows : SampleCount - 1;

        if (selected != previous)
        {
            // Scroll just enough to keep the selected sample on screen
            if (selected < first) first = selected;
            if (selected >= first + ListRows) first = selected - ListRows + 1;
            DrawList(first, selected);
        }

        if (pad.WasPressed(Digital::Button::A) || pad.WasPressed(Digital::Button::START))
        {
            const Sample &sample = Samples[selected];
            SRL::Debug::PrintClearLine(StatusRow + 1);
            SRL::Debug::Print(1, StatusRow + 1, "Starting...");
            SRL::Core::Synchronize();

            // Does not return when the sample starts
            SRL::Fld::ExecGame(IpFile, sample.Directory, sample.FirstAudioTrack);

            SRL::Debug::PrintClearLine(StatusRow + 1);
            SRL::Debug::Print(1, StatusRow + 1, "Could not start %s", sample.Directory);
        }

        SRL::Core::Synchronize();
    }

    return 0;
}

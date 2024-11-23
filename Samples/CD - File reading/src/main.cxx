#include <srl.hpp>

int main()
{
    SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black);

    // Initialize gamepad
    SRL::Input::Digital pad(0);

    // Open file
    SRL::Cd::File file("IPSUM.TXT");
    file.Open();

    // Max number of bytes on screen and page
    uint8_t page = 0;
    int32_t bytes = 40 * 25;

    // Program main loop
    bool refresh = true;
    
    while (true)
    {
        if (page > 0)
        {
            SRL::Debug::Print(0,3, "---^---");

            if (pad.WasPressed(SRL::Input::Digital::Button::Up))
            {
                page--;
                refresh = true;

                // Seek to a start of previous chunk
                file.Seek(page * bytes);
            }
        }
        else
        {
            SRL::Debug::Print(0,3, "       ");
        }

        if ((page + 1) * bytes < file.Size.Bytes)
        {
            SRL::Debug::Print(0,27, "---v---");

            if (pad.WasPressed(SRL::Input::Digital::Button::Down))
            {
                page++;
                refresh = true;
            }
        }
        else
        {
            SRL::Debug::Print(0,27, "       ");
        }
        
        if (refresh)
        {
            // Read next chunk line by line
            for (int32_t line = 5; line < 25; line++)
            {
                // Text buffer (40 bytes per line + terminator character)
                char byteBuffer[41];

                // Clear buffer
                SRL::Memory::MemSet(byteBuffer, '\0', 41);

                // Read from file into a buffer
                file.Read(40, byteBuffer);

                // Print to screen
                SRL::Debug::PrintClearLine(line);
                SRL::Debug::Print(0, line, "%s", byteBuffer);
            }
        }

        SRL::Core::Synchronize();
        refresh = false;
    }

    return 0;
}

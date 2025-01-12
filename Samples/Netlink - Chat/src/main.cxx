#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

int t = 0;
uint16_t cursorRow = 0;
uint16_t CursorColumn = 0;

void OnReceive()
{
    uint8_t byte = 0;
    
    while (SRL::Netlink::Read(&byte, 1))
    {
        if (byte != '\r')
        {
            if (byte == '\n')
            {
                CursorColumn = 0;
                
                if (++cursorRow > 14)
                {
                    cursorRow = 0;
                }

                SRL::Debug::PrintClearLine(cursorRow + 3);
            }
            else
            {
                SRL::Debug::Print(CursorColumn + 2,cursorRow + 3, "%c", byte);
                if (CursorColumn++ >= 30) CursorColumn = 0;
            }
        }
    }
}

void SendCommand(uint8_t* command)
{
    SRL::Debug::Print(1,21, "Sending: %s", command);
                
    if (SRL::Netlink::Write(command))
    {
        SRL::Debug::PrintClearLine(21);
        SRL::Debug::Print(1,21, "Sent: %s", command);
    }
    else
    {
        SRL::Debug::PrintClearLine(21);
        SRL::Debug::Print(1,21, "Timed out: %s", command);
    }
}

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "Netlink - chat");

    if (SRL::Netlink::Initialize())
    {
        SRL::Netlink::OnReceive += OnReceive;

        SRL::Input::Digital pad(0);

        size_t commandIndex = 0;
        #define len 4
        uint8_t* commands[len]
        {
            (uint8_t*)"AT&F\r",
            (uint8_t*)"ATX0\r",
            (uint8_t*)"AT&D2&K3&C1&Q0\r"
        };
        
        while(1)
        {
            if (pad.WasPressed(SRL::Input::Digital::Button::X))
            {
                SendCommand((uint8_t*)"ATA\r");
            }
            else if (pad.WasPressed(SRL::Input::Digital::Button::C))
            {
                SendCommand((uint8_t*)"ATDT9095551010\r");
            }
            else if (pad.WasPressed(SRL::Input::Digital::Button::Z))
            {
                SRL::Debug::PrintClearScreen();
            }
            else if (pad.WasPressed(SRL::Input::Digital::Button::B))
            {
                SRL::Netlink::PrintState();
            }
            else if (pad.WasPressed(SRL::Input::Digital::Button::A))
            {
                SendCommand(commands[commandIndex]);
                if (++commandIndex >= len) commandIndex = 0;
            }

            SRL::Debug::Print(1,20, "Frame: %d", t++);
            SRL::Core::Synchronize();
        }
    }

    SRL::Debug::Print(1,20, "NO MODEM");
    SRL::Core::Synchronize();
	return 0;
}

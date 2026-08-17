#include <srl.hpp>

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Input;

int main()
{
	SRL::Core::Initialize(HighColor(0,0,0));
    
    bool exitToMultiplayer = false;
    
    Digital gamepad(0);
    
	while(1)
	{

        if (SRL::Cd::IsTrayOpen())
        {
            if (exitToMultiplayer)
            {
                SYS_Exit(0);
            }
            else
            {
                SRL::Debug::Print(2, 2, "Disc Tray: Opened");
            }
        }
        else
        {
            SRL::Debug::Print(2, 2, "Disc Tray: Closed");
        }
        SRL::Debug::Print(2, 4, "Exit to Multiplayer:");
        SRL::Debug::Print(23, 4, exitToMultiplayer ? "True " : "False");
        SRL::Debug::Print(2, 5, "(Press A to change)");
        
        if (gamepad.WasPressed(Digital::Button::A))
        {
            exitToMultiplayer = !exitToMultiplayer;
        }
        
        SRL::Core::Synchronize();
	}

	return 0;
}

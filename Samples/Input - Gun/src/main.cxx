#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Using to shorten names for input
using namespace SRL::Input;

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Blue);

	VDP2_EXTEN = 0x0200;
    SRL::Core::OnVblank += Gun::VblankRefresh;

    SRL::Debug::Print(1,1, "Input gun sample");

    // Initialize light gun on port 0
    Gun port0(0);

    // Main program loop
	while(1)
	{
        // Must be called at the start of every gme loop
        Gun::Refresh();

        if (port0.IsConnected())
        {
            SRL::Debug::Print(2,4, "connected");
        }
        else
        {
            
            SRL::Debug::Print(2,4, "0x%x %d", Management::GetType(0), SynchCount);
        }

        Vector2D location = port0.GetPosition();
        SRL::Debug::Print(2,5, "%d, %d", location.X.ToInt(), location.Y.ToInt());

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

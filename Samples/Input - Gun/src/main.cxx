#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Using to shorten names for input
using namespace SRL::Input;

static const char * connected = "Connected";
static const char * disconnected = "Not Connected";

// Main program entry
int main()
{
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Blue);
    SRL::Debug::Print(1,1, "Input gun sample");

    // Initialize light gun on port 0
    Gun gun1(0);

    // Initialize light gun on port 6
    Gun gun2(6);

    // Main program loop
    while(1)
    {
        const char * p1status = nullptr;
        const char * p2status = nullptr;
        static const char * PreviousP1status = nullptr;
        static const char * PreviousP2status = nullptr;


        if (gun1.IsConnected())
        {
            p1status = connected;
        }
        else
        {
            p1status = disconnected;
        }

        if (gun2.IsConnected())
        {
            p2status = connected;
        }
        else
        {
            p2status = disconnected;
        }

        if (p1status != PreviousP1status || p2status != PreviousP2status)
        {
            PreviousP1status = p1status;
            PreviousP2status = p2status;
            SRL::Debug::PrintClearScreen();
        }

        SRL::Debug::Print(2,2, "PORT 1 :");
        SRL::Debug::Print(10,2, "%s : 0x%x 0x%x", p1status, Management::GetType(0), Management::GetRawData(0)->data);

        if (gun1.IsHeld(Gun::Trigger))
        {
            Vector2D location1 = gun1.GetPosition();
            SRL::Debug::Print(3 ,3, "X : %03d, Y : %03d", location1.X.ToInt(), location1.Y.ToInt());
        }

        if (gun2.IsHeld(Gun::Trigger))
        {
            Vector2D location2 = gun2.GetPosition();
            SRL::Debug::Print(3 ,5, "X : %03d, Y : %03d", location2.X.ToInt(), location2.Y.ToInt());
        }

        SRL::Debug::Print(2,4, "PORT 2 :");
        SRL::Debug::Print(10,4, "%s : 0x%x 0x%x", p2status, Management::GetType(6), Management::GetRawData(6)->data);


        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

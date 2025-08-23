#include <srl.hpp>

// CDDA tracks
#define LOGO_TRACK                2
#define TITLE_TRACK               3
#define SELECT_TRACK              4
#define BEGIN_GAME_TRACK          5

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Using to shorten names for input
using namespace SRL::Input;

// Main program entry 
int main()
{
    int16_t currentTrack = LOGO_TRACK;
    
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "Basic CDDA sample");
    SRL::Debug::Print(1,3, "Inputs:");
    SRL::Debug::Print(3,4, "A - Play Next Track");
    SRL::Debug::Print(3,5, "B - Play Previous Track");
    SRL::Debug::Print(3,6, "C - Stop");
    
    SRL::Sound::Cdda::PlaySingle(currentTrack, false);
    
    Digital port0(0);    

    while(1)
    {
        // Change Tracks Forward
        if (port0.WasPressed(Digital::Button::A))
        {
            currentTrack++;
            if (currentTrack > BEGIN_GAME_TRACK)
            {
                currentTrack = LOGO_TRACK;
            }
            SRL::Sound::Cdda::PlaySingle(currentTrack, false);
        }
        // Change Tracks Back
        else if (port0.WasPressed(Digital::Button::B))
        {
            currentTrack--;
            if (currentTrack < LOGO_TRACK)
            {
                currentTrack = BEGIN_GAME_TRACK;
            }
            SRL::Sound::Cdda::PlaySingle(currentTrack, false);
        }
        // Stop
        else if (port0.WasPressed(Digital::Button::C))
        {
            SRL::Sound::Cdda::StopPause();
        }
        
        SRL::Debug::Print(1,8, "Playing Track %d  ", currentTrack);
        
        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

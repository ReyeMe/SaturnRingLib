#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Using to shorten names for input
using namespace SRL::Input;

static const char * connected = "Connected";
static const char * disconnected = "Not Connected";
static const char * fire = "FIRE";
static const char * start = "START";
static const char * emptyStr = "    ";

struct GunStatus {
    const char * status;
    const char * previousStatus;
    const char * triggerStatus;
    const char * startStatus;
    const Gun & gun;

    GunStatus( const Gun & gun) : gun(gun)
    {
        Reset();
    }

    void Reset()
    {
        this->status          = nullptr;
        this->previousStatus  = nullptr;
        this->triggerStatus   = nullptr;
        this->startStatus     = nullptr;
    }

    void Update()
    {
        SetConnected();  
        SetFire(); 
        SetStart();
    }

    void SetConnected()
    {
        if (const_cast<Gun &>(this->gun).IsConnected())
        {
            this->status = connected;
        }
        else
        {
            this->status = disconnected;
        }
    }

    void SetFire()
    {
        if (const_cast<Gun &>(this->gun).IsHeld(Gun::Trigger))
        {
            this->triggerStatus = fire;
        }
        else
        {
            this->triggerStatus = emptyStr;
        }
    }

    void SetStart()
    {
        if (const_cast<Gun &>(this->gun).IsHeld(Gun::Start))
        {
            this->startStatus = startStatus;
        }
        else
        {
            this->startStatus = emptyStr;
        }
    }

    bool IsChanged()
    {
        bool bReturn = this->status != this->previousStatus;
        if (bReturn)
        {
            BackupStatus();
        }
        return bReturn;
    }

    void BackupStatus()
    {
        this->previousStatus = this->status;
    }
};

// Main program entry
int main()
{
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Blue);

    // Initialize light gun on port 0
    Gun gun1(Gun::GunPlayer1Port);

    // Initialize light gun on port 6
    Gun gun2(Gun::GunPlayer2Port);

    GunStatus gun1Status(gun1);
    GunStatus gun2Status(gun2);

    // Main program loop
    while(1)
    {
        SRL::Debug::Print(1,1, "Input gun sample");

        gun1Status.Update();
        gun2Status.Update();

        if (gun1Status.IsChanged() || gun1Status.IsChanged())
        {
            // Clear out the debug messages, avoid messages overlapping
            SRL::Debug::PrintClearScreen();
        }

        SRL::Debug::Print(2,2, "PORT 1 :");
        SRL::Debug::Print(10,2, 
                            "%s : 0x%x 0x%x",
                            gun1Status.status,
                            Management::GetType(Gun::GunPlayer1Port), Management::GetRawData(Gun::GunPlayer1Port)->data);
        Vector2D location1 = gun1.GetPosition();
        SRL::Debug::Print(3 ,3, "X : %03d, Y : %03d", location1.X.ToInt(), location1.Y.ToInt());
        SRL::Debug::Print(5 ,4, "%s : %s", gun1Status.triggerStatus, gun1Status.startStatus);

        SRL::Debug::Print(2,5, "PORT 2 :");
        Vector2D location2 = gun2.GetPosition();
        SRL::Debug::Print(3 ,6, "X : %03d, Y : %03d", location2.X.ToInt(), location2.Y.ToInt());
        SRL::Debug::Print(10,7, 
                            "%s : 0x%x 0x%x",
                            gun2Status.status,
                            Management::GetType(Gun::GunPlayer2Port), Management::GetRawData(Gun::GunPlayer2Port)->data);

        SRL::Debug::Print(5 ,8, "%s : %s", gun2Status.triggerStatus, gun2Status.startStatus);

        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

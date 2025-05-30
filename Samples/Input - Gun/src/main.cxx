#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Using to shorten names for input
using namespace SRL::Input;

// Strings to display guns status
static const char * connected = "Connected";
static const char * disconnected = "Not Connected";
static const char * emptyStr = "      ";
static const char * fire = "FIRE";
static const char * start = "START";

/** @brief Helper class to handle Gun
 */
struct GunStatus {
    const char * status;
    const char * previousStatus;
    const char * triggerStatus;
    const char * startStatus;
    const Gun & gun;

    /** @brief Constructor
     * @param gun Gun to monitor
     */
    GunStatus( const Gun & gun) : gun(gun)
    {
        Reset();
    }

    /** @brief Reset strings pointers
     */
    void Reset()
    {
        this->status          = nullptr;
        this->previousStatus  = nullptr;
        this->triggerStatus   = nullptr;
        this->startStatus     = nullptr;
    }

    /** @brief Update strings pointers
     */
    void Update()
    {
        SetConnected();  
        SetFire(); 
        SetStart();
    }

    /** @brief Update connection status string pointer 
     */
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

    /** @brief Update trigger  state string pointer 
     */
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

    /** @brief Update start button state string pointer 
     */
    void SetStart()
    {
        if (const_cast<Gun &>(this->gun).IsHeld(Gun::Start))
        {
            this->startStatus = start;
        }
        else
        {
            this->startStatus = emptyStr;
        }
    }

    /** @brief Compare previous and current Gun status
     * @returns true if change detected, false otherwise
     */
    bool IsChanged()
    {
        bool bReturn = this->status != this->previousStatus;
        if (bReturn)
        {
            BackupStatus();
        }
        return bReturn;
    }

    /** @brief Sync previous and current Ung Status 
     */
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
    Gun gun1(Gun::Player::Player1);

    // Initialize light gun on port 6
    Gun gun2(Gun::Player::Player2);

    GunStatus gun1Status(gun1);
    GunStatus gun2Status(gun2);

    int32_t cnt = 0;

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
                            Management::GetType(gun1.Port), Management::GetRawData(gun1.Port)->data);
        Vector2D location1 = gun1.GetPosition();
        SRL::Debug::Print(3 ,3, "X : %03d, Y : %03d", location1.X.As<int16_t>(), location1.Y.As<int16_t>());
        SRL::Debug::Print(5 ,4, "%s : %s", gun1Status.triggerStatus, gun1Status.startStatus);

        SRL::Debug::Print(2,5, "PORT 2 :");
        Vector2D location2 = gun2.GetPosition();
        SRL::Debug::Print(3 ,6, "X : %03d, Y : %03d", location2.X.As<int16_t>(), location2.Y.As<int16_t>());
        SRL::Debug::Print(10,7, 
                            "%s : 0x%x 0x%x",
                            gun2Status.status,
                            Management::GetType(gun2.Port), Management::GetRawData(gun2.Port)->data);

        SRL::Debug::Print(5 ,8, "%s : %s", gun2Status.triggerStatus, gun2Status.startStatus);

        SRL::Debug::Print(5, 10, "s: %d     counter: %d", SynchCount, cnt);
        cnt++;

        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

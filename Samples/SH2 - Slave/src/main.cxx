#include <srl.hpp>

// Using to shorten names for HighColor and ITask
using namespace SRL::Types;
using namespace SRL::Math::Types;

/** @brief Task Object
*/
class Task : public ITask
{
public:

    /** @brief Constructor
    */
    Task() : cpt(0) {}

    void Do()
    {
        // Print is glitching !!! FIXME !
        //SRL::Debug::Print(1,6, "%03d, %s : Line %d : %s()", cpt++, __FILE__, __LINE__ , __func__ );
        ++cpt;
    }

    uint8_t GetCounter()
    {
        return cpt;
    }

protected:
    uint8_t cpt;
};

// Main program entry
int main()
{
    Task task;

    task.ResetTask();

    SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "SH2 Slave");

    // Main program loo
    while(1)
    {
        // Clear the screen
        SRL::Debug::PrintClearScreen();

        SRL::Debug::Print(1,3, "SH2 Slave sample");

        if(!task.IsRunning())
        {
            SRL::Slave::ExecuteOnSlave(task);
        }

        // Display the counter increased by the task
        SRL::Debug::Print(1,5, "Counter increased by Slave : %d", task.GetCounter());

        SRL::Core::Synchronize();
    }

    return 0;
}
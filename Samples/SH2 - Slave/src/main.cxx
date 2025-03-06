#include <srl.hpp>

// Using to shorten names for HighColor and ITask
using namespace SRL::Types;
using namespace SRL::Math::Types;

/** @brief Task Obeject
*/
class Task : public ITask
{
public:

    /** @brief Constructor
    */
    Task() : cpt(0) {}

    void Do()
    {
        SRL::Debug::Print(1,4, "%03d, %s : Line %d : %s()", cpt++, __FILE__, __LINE__ , __func__ );
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

    SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "SH2 Slave");

    // Main program loo
    while(1)
    {
        // Clear the screen
        SRL::Debug::PrintClearScreen();

        SRL::Debug::Print(1,3, "SH2 Slave sample");

        SRL::Slave::ExecuteOnSlave(task);

        // Wait for the task to be executed
        while(!task.IsDone());

        // Display the counter increased by the task
        SRL::Debug::Print(1,5, "Counter increased by Slave : %d", task.GetCounter());

        SRL::Core::Synchronize();
    }

    return 0;
}

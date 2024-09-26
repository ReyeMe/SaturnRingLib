#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

class Task : public SRL::ITask
{
public:

	Task(bool * done) : ITask(done), cpt(0) {}

	void Do()
	{
		SRL::Debug::Print(1,4, "%d, %s", cpt++, __FUNCTION__ );
	}

	void Callback()
	{
			SRL::Debug::Print(1,5, "%d, %s", cpt++, __FUNCTION__ );
	}

protected:
	uint8_t cpt;;
};

// Main program entry
int main()
{
bool done;
	Task task(&done);



	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "SH2 Slave");

    // Main program loo
	while(1)
	{
			SRL::Debug::PrintClearScreen();
            // Print new random number
        SRL::Debug::Print(1,3, "HELLO WORLD !");

				SRL::Slave::ExecuteOnSlave(&task);

				while(!task.IsDone());

        SRL::Core::Synchronize();
	}

	return 0;
}

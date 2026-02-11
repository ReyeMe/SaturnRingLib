#include <srl.hpp>
#include <samplemodule.hpp>

using namespace SampleModule;

int main()
{
	SRL::Core::Initialize(SRL::Types::HighColor::Colors::Black);
	
    Digital port0(0);
    
    HelloWorld::ShowMessage();
    
	while(1)
	{
	    if (port0.WasPressed(Digital::Button::A))
        {
            HelloWorld::ShowMessage(true);
        }
        
        SRL::Core::Synchronize();
	}

	return 0;
}

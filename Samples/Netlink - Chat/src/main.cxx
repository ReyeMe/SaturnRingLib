#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "Netlink - chat");

    bool result = SRL::Netlink::Initialize();

    SRL::Debug::Print(1,2, "Init: %d", result);

    if (result)
    {
        SRL::Netlink::Write((uint8_t*)"AT\r\n", 4);
        SRL::Debug::Print(1,5, "AT");

        char buf[20] = { '\0' };
        size_t read = SRL::Netlink::Read((uint8_t*)buf, 20);
        SRL::Debug::PrintWithWrap(1, 6, 1, 39, "(%d)> %s", read, buf);
    }

    static int t = 0;

    // Main program loop
	while(1)
	{

        SRL::Debug::Print(1,20, "%d", t++);
        SRL::Core::Synchronize();
	}

	return 0;
}

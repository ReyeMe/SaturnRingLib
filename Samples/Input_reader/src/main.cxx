#include <srl.hpp>
using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(SaturnColor(0,100,50));
	
	Uint8* data = (Uint8*)SRL::Input::IPeripheral::GetRawData(0);

	while(1)
	{
		SRL::Debug::Print(2, 2, "ID: %x EXT: %x     ", data[0], data[1]);
		int dataId = 0;

		for (int x = 0; x < 8 && dataId < 22; x++)
		{
			SRL::Debug::Print(5 + (x * 3), 4, "%d", x + 1);
			SRL::Debug::Print(4 + (x * 3), 5, "---", x);
		}

		for (int y = 0; y < 3 && dataId < 22; y++)
		{
			SRL::Debug::Print(2, 6 + (y * 2), "%d!", y + 1);
			SRL::Debug::Print(2, 7 + (y * 2), " !");

			for (int x = 0; x < 8 && dataId < 22; x++)
			{
				SRL::Debug::Print(5 + (x * 3), 6 + (y * 2), "%x", data[dataId + 2]);
				dataId++;
			}
		}

		slSynch();
	}

	return 0;
}

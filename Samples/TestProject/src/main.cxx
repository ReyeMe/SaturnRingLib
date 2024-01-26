#include <srl.hpp>

struct TestEntity
{
	unsigned int x;
	int dx;
	int life;
	unsigned int id;
};

int main()
{
	SRL::Core::Initialize(SRL::Types::SaturnColor(200,100,50));

	TestEntity ** entities = new TestEntity*[20];

	if (entities != nullptr)
	{
		for (int index = 0; index < 20; index++)
		{
			entities[index] = nullptr;
		}
	}

	unsigned int total = 0;
	slSynch();

	while(1)
	{
		Uint32 freeMem = SRL::Memory::GetAvailableMemory();

		slPrintHex(freeMem ,slLocate(0,22));

		Uint32 blocks = SRL::Memory::GetNumberOfBlocks();

		slPrintHex(blocks ,slLocate(0,23));

		bool make = true;

		for (int index = 0; index < 20; index++)
		{
			if (entities != nullptr && entities[index] != nullptr)
			{

				entities[index]->x += entities[index]->dx;
				slPrintHex(entities[index]->id ,slLocate(0,index));

				bool hit = false;

				if (entities[index]->x == 0 || entities[index]->x > 20)
				{
					entities[index]->dx *= (entities[index]->id & 3) - 2;
					hit = true;
				}
				
				if (hit)
				{
					delete entities[index];
					entities[index] = nullptr;
				}

				make = true;
			}
			else if (make)
			{
				make = false;
				TestEntity* test = new TestEntity();

				if (test != nullptr)
				{
					if (entities != nullptr)
					{
						entities[index] = test;
						entities[index]->x = 10;
						entities[index]->dx = 1 + (index & 3);
						entities[index]->life = 10;
						entities[index]->id = total++;
					}
				}
				else
				{
					slPrintHex(total,slLocate(0,index));
					slPrint("FAIL" ,slLocate(0,index));
				}
			}
		}
		
		slSynch();
	}

	return 0;
}

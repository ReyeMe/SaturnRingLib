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
		SRL::Debug::Print(1,23, "Free memory %d", SRL::Memory::GetAvailableMemory());
		SRL::Debug::Print(1,24, "Free blocks %d", SRL::Memory::GetNumberOfBlocks());

		bool make = true;

		for (int index = 0; index < 20; index++)
		{
			if (entities != nullptr && entities[index] != nullptr)
			{

				entities[index]->x += entities[index]->dx;
				SRL::Debug::Print(1,index + 1, "%d", entities[index]->id);

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
					SRL::Debug::Assert("Memory allocation failed!");
				}
			}
		}
		
		slSynch();
	}

	return 0;
}

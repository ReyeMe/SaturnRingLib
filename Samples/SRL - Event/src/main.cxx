#include <srl.hpp>

// Using to shorten names for DateTime
using namespace SRL::Types;

class MyVblankNeedingObject
{
private:
    int counter = 0;

    void MyVblankFunction()
    {
        counter++;
    }

public:
    MyVblankNeedingObject()
    {
        //auto MyVblankFunctionProxy = MemberProxy(this, &MyVblankNeedingObject::MyVblankFunction);
        //SRL::Core::OnVblank += &MyVblankFunctionProxy;
    }

    ~MyVblankNeedingObject()
    {
        //SRL::Core::OnVblank -= BoundFunctionProxy(this, &MyVblankNeedingObject::MyVblankFunction);
    }

    void Print()
    {
        SRL::Debug::Print(5,5, "OBJ: %d    ", counter);
    }
};

uint32_t cnt = 1002;
void StaticCounter() { cnt++; }

// Main program entry
int main()
{
    SRL::Core::Initialize(HighColor(20, 10, 50));
    
    MyVblankNeedingObject obj = MyVblankNeedingObject();
    SRL::Core::OnVblank += StaticCounter;

    // Main program loop
    while (1)
    {
        obj.Print();
        SRL::Debug::Print(5,6, "ST: %d    ", cnt);
        SRL::Core::Synchronize();
    }

    return 0;
}

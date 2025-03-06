#include <srl.hpp>

// Using to shorten names for DateTime
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Main program entry
int main()
{
    SRL::Core::Initialize(HighColor(20, 10, 50));
    SRL::Debug::Print(1, 1, "Clock");

    // Main program loop
    while (1)
    {
        SRL::Types::DateTime time = SRL::Types::DateTime::Now();

        SRL::Debug::Print(1,3,
                            "%04d/%02d/%02d %02d:%02d:%02d",
                            time.Year(),
                            time.Month(),
                            time.Day(),
                            time.Hour(),
                            time.Minute(),
                            time.Second());

        SRL::Core::Synchronize();
    }

    return 0;
}

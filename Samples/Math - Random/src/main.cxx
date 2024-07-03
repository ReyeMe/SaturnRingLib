#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Debug::Print(1,1, "Random number generator sample");

    // Initialize random number generator with a specific seed
    SRL::Math::Random rnd = SRL::Math::Random(15);

    // Define variables to keep track of random data
    int32_t data[320];
    uint32_t pointer = 0;

    // Update delay counter
    uint32_t tm = 0;
    
    // Main program loo
	while(1)
	{
        // Generate new random number after a specified delay
        if (tm++ > 2)
        {
            // Generate new random number
            data[pointer] = rnd.GetNumber(-25,25);
            
            // Print new random number
            SRL::Debug::Print(1,3, "Random: %d    ", data[pointer]);

            // Move data pointer
            pointer++;

            if (pointer >= 320)
            {
                pointer = 0;
            }

            // Reset delay counter
            tm = 0;
        }

        // Draw graph
        for (uint16_t i = 0; i < 319; i++)
        {
            SRL::Scene2D::DrawLine(
                Vector2D(Fxp::FromInt(i- 160), Fxp::FromInt(data[i])),
                Vector2D(Fxp::FromInt(i + 1 - 160), Fxp::FromInt(data[i + 1])),
                HighColor::Colors::White,
                500.0);
        }
        
        SRL::Scene2D::DrawLine(
            Vector2D(Fxp::FromInt(pointer - 160), Fxp::FromInt(50)),
            Vector2D(Fxp::FromInt(pointer - 160), Fxp::FromInt(-50)),
            HighColor::Colors::Red,
            500.0);

        SRL::Core::Synchronize();
	}

	return 0;
}

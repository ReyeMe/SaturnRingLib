#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Simple line structure
struct MovingPoint
{
    /** @brief Point location
     */
    Vector2D Point;

    /** @brief Point velocity vector
     */
    Vector2D Velocity;
};

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 Distorted sprite sample");
    
    // Load texture
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");   // Loads TGA file into main RAM
    Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);       // Loads TGA into VDP1
    delete tga;                                                 // Frees main RAM

    // Get screen size
    Uint16 halfWidth = SRL::TV::Width >> 1;
    Fxp minimumWidth = -Fxp::FromInt(halfWidth);
    Fxp maximumWidth = Fxp::FromInt(halfWidth);

    Uint16 halfHeight = SRL::TV::Height >> 1;
    Fxp minimumHeight = -Fxp::FromInt(halfHeight);
    Fxp maximumHeight = Fxp::FromInt(halfHeight);

    // Initialize random number function
    SRL::Math::Random rnd = SRL::Math::Random();

    // Velocities to use
    Vector2D velocities[] =
    {
        Vector2D(-1.0, 1.0),
        Vector2D(1.0, 1.0),
        Vector2D(-1.0, -1.0),
        Vector2D(1.0, -1.0)
    };

    // Define sprite points
    MovingPoint points[] =
    {
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
        },
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
        },
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
        },
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
        },
    };

    // Main program loop
	while(1)
	{
        Vector2D spritePoints[4];

        // Move points
        for (int point = 0; point < 4; point++)
        {
            // Move line
            points[point].Point += points[point].Velocity;

            // Limit line velocity
            if (points[point].Point.X < minimumWidth || points[point].Point.X > maximumWidth)
            {
                points[point].Velocity.X *= -1.0;
            }

            if (points[point].Point.Y < minimumHeight || points[point].Point.Y > maximumHeight)
            {
                points[point].Velocity.Y *= -1.0;
            }

            spritePoints[point] = points[point].Point;
        }

        SRL::Scene2D::DrawSprite(0, spritePoints, 500.0);

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

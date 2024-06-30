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
    SRL::Debug::Print(1,1, "VDP1 Sprite sample");
    
    // Load texture
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");   // Loads TGA file into main RAM
    Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);       // Loads TGA into VDP1
    delete tga;                                                 // Frees main RAM

    Sint16 degrees = 0;

    // Main program loop
	while(1)
	{
        // Calculate sin/cos from an angle
        Angle angle = Angle::FromDegrees(degrees);
        Fxp sin = SRL::Math::Sin(angle);
        Fxp cos = SRL::Math::Cos(angle);
        degrees++;

        // Rotation center
        Vector2D location = Vector2D();

        // Rectangle size
        Vector2D size = Vector2D(
            Fxp::FromInt(100) >> 1,
            Fxp::FromInt(100) >> 1);

        // Rotated rectangle points
        Vector3D points[4] = {
            Vector3D(((cos * -size.X) - (sin * -size.Y)) + location.X, ((sin * -size.X) + (cos * -size.Y)) + location.Y, 500.0),
            Vector3D(((cos * size.X) - (sin * -size.Y)) + location.X, ((sin * size.X) + (cos * -size.Y)) + location.Y, 500.0),
            Vector3D(((cos * size.X) - (sin * size.Y)) + location.X, ((sin * size.X) + (cos * size.Y)) + location.Y, 500.0),
            Vector3D(((cos * -size.X) - (sin * size.Y)) + location.X, ((sin * -size.X) + (cos * size.Y)) + location.Y, 500.0)
        };

        // Simple sprite
        SRL::Scene2D::DrawSprite(textureIndex, points[0]);

        // Rotated sprite
        SRL::Scene2D::DrawSprite(textureIndex, points[1], -angle);

        // Scaled & rotated sprite
        SRL::Scene2D::DrawSprite(textureIndex, points[2], -angle, Vector2D(sin, cos));

        // Scaled sprite
        SRL::Scene2D::DrawSprite(textureIndex, points[3], Vector2D(sin, cos));

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

#include <srl.hpp>

// Using for shorter access to types
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Alias for better readability of the sample
using Trig = SRL::Math::Trigonometry;

// Sphere setup
static const int16_t LatitudePoints = 8;
static const int16_t Latitudes = 8;
static const Fxp Radius = 80.0;

int main()
{
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1, 1, "VDP1 - 2D Rotating sprite sphere");

    // Load ball sprite
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("BALL.TGA");
    int32_t ballTexture = SRL::VDP1::TryLoadTexture(tga);
    delete tga;

    // Prepare sphere points, we do this before main loop, for faster performance while rendering
    const Angle latStep = Angle::FromDegrees(360.0 / LatitudePoints);
    const Angle longStep = Angle::FromDegrees(180.0 / (Latitudes + 1));
    const size_t pointCount = Latitudes * LatitudePoints;
    Vector3D points[pointCount] {};

    for (int16_t longitude = 0; longitude < Latitudes; longitude++)
    {
        const Angle longRot = (longStep * (longitude + 1));

        for (int16_t latitude = 0; latitude < LatitudePoints; latitude++)
        {
            const Angle latRoll = (latStep * latitude);

            points[(longitude * Latitudes) + latitude] = Vector3D(
                Trig::Sin(longRot) * Trig::Cos(latRoll),
                Trig::Sin(longRot) * Trig::Sin(latRoll),
                Trig::Cos(longRot)
            );
        }
    }

    // Setup starting rotation
    Angle rot = 0.0;
    Angle roll = 0.0;

	while(1)
	{
        // Create rotation matrix that will rotate the sphere
        const Matrix33 matrix = Matrix33::CreateRotation(rot, 0.0, roll);

        // Rotate each sphere point and render sprite at its location
        for (size_t pointIdx = 0; pointIdx < pointCount; pointIdx++)
        {
            // Rotate point by matrix
            const Vector3D transformed = matrix * points[pointIdx];

            // Calculate sprite size
            const Fxp depth = (transformed.Z + 1.3) >> 2;

            // Render sphere point as a simple sprite
            SRL::Scene2D::DrawSprite(
                ballTexture, 
                Vector3D(
                    (Radius * transformed.X),
                    (Radius * transformed.Y),
                    (Radius * -transformed.Z) + 600.0),
                Vector2D(
                    depth,
                    depth)
                );
        }

        // Rotate the sphere a little
        rot += 0.001;
        roll += 0.001;

        SRL::Core::Synchronize();
	}

	return 0;
}

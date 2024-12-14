#include <srl.hpp>
#include "animatedObject.hpp"

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Input;

// Light shading table
HighColor shadingTable[32] =
{
	HighColor::FromRGB555(  0,  0,  0 ),
	HighColor::FromRGB555(  1,  1,  1 ),
	HighColor::FromRGB555(  2,  2,  2 ),
	HighColor::FromRGB555(  3,  3,  3 ),
	HighColor::FromRGB555(  4,  4,  4 ),
	HighColor::FromRGB555(  5,  5,  5 ),
	HighColor::FromRGB555(  6,  6,  6 ),
	HighColor::FromRGB555(  7,  7,  7 ),
	HighColor::FromRGB555(  8,  8,  8 ),
	HighColor::FromRGB555(  9,  9,  9 ),
	HighColor::FromRGB555( 10, 10, 10 ),
	HighColor::FromRGB555( 11, 11, 11 ),
	HighColor::FromRGB555( 12, 12, 12 ),
	HighColor::FromRGB555( 13, 13, 13 ),
	HighColor::FromRGB555( 14, 14, 14 ),
	HighColor::FromRGB555( 15, 15, 15 ),
	HighColor::FromRGB555( 16, 16, 16 ),
	HighColor::FromRGB555( 17, 17, 17 ),
	HighColor::FromRGB555( 18, 18, 18 ),
	HighColor::FromRGB555( 19, 19, 19 ),
	HighColor::FromRGB555( 20, 20, 20 ),
	HighColor::FromRGB555( 21, 21, 21 ),
	HighColor::FromRGB555( 22, 22, 22 ),
	HighColor::FromRGB555( 23, 23, 23 ),
	HighColor::FromRGB555( 24, 24, 24 ),
	HighColor::FromRGB555( 25, 25, 25 ),
	HighColor::FromRGB555( 26, 26, 26 ),
	HighColor::FromRGB555( 27, 27, 27 ),
	HighColor::FromRGB555( 28, 28, 28 ),
	HighColor::FromRGB555( 29, 29, 29 ),
	HighColor::FromRGB555( 30, 30, 30 ),
	HighColor::FromRGB555( 31, 31, 31 ),
};

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 3D Animation");

    // Load buddy
    // Model was converted with Obj2Nya (see https://github.com/ReyeMe/PawCraft), command parameters were: Obj2Nya "D:\frame_01.obj" "D:\frame_02.obj" "D:\SPOT.NYA" /s
    AnimatedObject bud = AnimatedObject("BUD.NYA", 0);

    // Setup camera location
    Vector3D cameraLocation = Vector3D(0.0, -30.0, -30.0);
    Vector3D cameraTarget = Vector3D(0.0, -20.0, 0.0);

    // Setup light, we can use scale of the vector to manipulate light intensity
    Vector3D lightDirection = Vector3D(0.8, 0.0, 0.8);
    
    // Set light vector
    SRL::Scene3D::SetDirectionalLight(lightDirection);

    // Initialize rotation angle
    Angle rotation = Angle::FromRaw(0.0);
    Angle rotationStep = Angle::FromDegrees(1);

    // Initialize shading table and work table
    uint32_t polygons = bud.GetFaceCount();
    SRL::Types::HighColor workTable[bud.GetFaceCount() <<2];
    uint8_t vertWork[bud.GetVertexCount()];

    // Initializes gouraud table work area
    SRL::Scene3D::LightInitGouraudTable(0, vertWork, workTable, polygons);

    // Sets current light shading table
    SRL::Scene3D::LightSetGouraudTable(shadingTable);

    // Setups copy event at each vblank
    SRL::Core::OnVblank += SRL::Scene3D::LightCopyGouraudTable;

    // Current animation time
    Fxp time = 0.0;

    // Main program loop
	while(1)
	{
        // Load identity matrix
        SRL::Scene3D::LoadIdentity();

        // Set camera location and direction
        SRL::Scene3D::LookAt(cameraLocation, cameraTarget, Angle::FromRaw(0));

        SRL::Scene3D::PushMatrix();
        {
            // Rotate buddy
            SRL::Scene3D::RotateY(rotation);
            rotation += rotationStep;

            // Draw buddy
            bud.Draw(lightDirection);

            // Process next animation frame
            bud.ProcessFrame(time += 0.1);
            if (time >= 11.0) time = 0.0;
        }
        SRL::Scene3D::PopMatrix();
        
        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

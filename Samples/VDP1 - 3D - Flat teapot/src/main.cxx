#include <srl.hpp>
#include "modelObject.hpp"

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Input;

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor(0x31, 0x14, 0x32));
    SRL::Debug::Print(1,1, "VDP1 3D Flat shaded teapot");

    // Load teapot
    // Model was converted with Obj2Nya (see https://github.com/ReyeMe/PawCraft), command parameters were: Obj2Nya "D:\teapot.obj" "D:\POT.PDT"
    ModelObject teapot = ModelObject("POT.PDT");

    // Initialize input
    Digital input(0);

    // Setup camera location
    Vector3D cameraLocation = Vector3D(0.0, -20.0, -40.0);

    // Setup light, we can use scale of the vector to manipulate light intensity
    Vector3D lightDirection = Vector3D(0.0, 20.0, 40.0);
    lightDirection.Normalize();
    lightDirection /= 4.0;
    SRL::Scene3D::SetDirectionalLight(lightDirection);

    // Initialize rotation angle
    Angle rotation = Angle::FromRaw(0.0);
    Angle rotationStep = Angle::FromDegrees(1);

    // Main program loop
	while(1)
	{
        // Load identity matrix
        SRL::Scene3D::LoadIdentity();

        // Set camera location and direction
        SRL::Scene3D::LookAt(cameraLocation, Vector3D(), Angle::FromDegrees(0.0));

        // Rotate teapot
        SRL::Scene3D::RotateY(rotation);
        rotation += rotationStep;

        // Draw teapot
        teapot.Draw();
        
        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

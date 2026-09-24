/*VDP2 Rotation Demo:
This Demo shows how to configure a rotating scroll on RBG0 and provides an interactive
showcase of the differences in rotation behavior and perspective between
1 axis, 2 axis, and 3 axis rotation modes. Since VDP2 VRAM demands
increase with the number of rotation axis used, it is recommended to select the minimum
number of axis required for your desired use case. A blue polygon is drawn in the center
of the screen alligned to the same starting axis as the RBG0 plane (facing out of the screen).
This demonstrates that 3D transforms effect both polygons and RBG0 identically when within
the valid rotation axis for the selected mode, but will not align when rotation
occurs along an invalid axis for the selected rotation mode. With 3 axis rotation selected
for RBG0, the polygon plane and RBG0 plane will always align.
*/
#include <srl.hpp>

using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Input;

//----------------------------------------------------------------------------------------------------
// Mesh data for a single polygon plane

Vector3D TestVerts[]  =
{
    Vector3D(-40.0,-140.0,0.0),
    Vector3D(-40.0,-60.0,0.0),
    Vector3D(40.0,-60.0,0.0),
    Vector3D(40.0,-140.0,0.0),
};

Polygon TestPolys[] =
{
    Polygon(Vector3D(0.0,0.0,-1.0),(uint16_t[4]){0, 1, 2, 3}),
};

Attribute TestAttr[] =
{
    Attribute(Attribute::FaceVisibility::DoubleSided,Attribute::SortMode::Maximum,
                                      No_Texture,HighColor(0,0,255),No_Gouraud,CL32KRGB,sprPolygon,No_Option),
};
Mesh TestMesh = Mesh(4,TestVerts, 1, TestPolys,TestAttr);

//-------------------------------------------------------------------------------------------------------
//Function to swap between the 3 available rotation modes

static void LoadRBG0(uint8_t config, SRL::Tilemap::ITilemap* map)
{
    SRL::VDP2::RBG0::ScrollDisable();//turn off scroll display so we dont see junk when loading to VRAM
    SRL::VDP2::ClearVRAM();//Clearing VDP2 VRAM because its use differs between Rotation Modes
    SRL::VDP2::RBG0::LoadTilemap(*map);//Transfer Tilemap to VRAM again since we cleared it

    switch (config)
    {
    case 0:
        /*Configure RBG0 with Single Axis Rotation:
        -Z axis rotation displays correctly while X and Y rotations cause uniform scaling/skewing
        -Works for 2D overhead rotation like Contra 3, or 2D backgounds that rotate with horizon */
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::OneAxis);
        SRL::Debug::Print(1,5,"Rotation Mode <L [1 Axis] R>");
        break;

    case 1:
        /*configure RBG0 with 2 Axis rotation:
        -X and Z axis rotation displays correctly while Y rotation causes scaling/skewing
        -SNES Mode 7 style floor effect like Mario Kart/F-Zero
        -Works for 3D ground/ceiling plane in games where there is no rolling around the camera's axis*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);
        SRL::Debug::Print(1,5,"Rotation Mode <L [2 Axis] R>");
        break;

    case 2:
        /*configure RBG0 with 3 Axis Rotation:
       -Full 3D rotation on all axis (SNES couldn't do this!)
       -Works for 3D flight sims or other games where camera rolling is desired*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::ThreeAxis);
        SRL::Debug::Print(1,5,"Rotation Mode <L [3 Axis] R>");
        break;
    }

    SRL::VDP2::RBG0::ScrollEnable();//Turn Scroll Display back on
}

int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
    Digital port0(0); // Initialize gamepad on port 0

    int8_t CurrentMode = 0;//variable to store which rotation mode is selected
    SRL::Tilemap::Interfaces::CubeTile* TestTilebin = new SRL::Tilemap::Interfaces::CubeTile("FOG256.BIN");//Load tilemap from cd to work RAM
    LoadRBG0(CurrentMode, TestTilebin);//Load tilemap from work RAM to VDP2 VRAM and configure it to start in Single Axis mode

    //variables to store current RBG0 position and rotation
    Vector3D RBG0position = Vector3D(0.0,100.0,160.0);
    Angle angX = Angle::FromDegrees(0.0);
    Angle angY = Angle::FromDegrees(0.1);
    Angle angZ = Angle::FromDegrees(0.0);

    //Print title and demo controls on screen
    SRL::Debug::Print(1,3,"Rotating Scroll Modes Sample");
    SRL::Debug::Print(1, 6, "<X [Rotate X] A>");
    SRL::Debug::Print(1, 7, "<Y [Rotate Y] B>");
    SRL::Debug::Print(1, 8, "<Z [Rotate Z] C>");

    //Main Game Loop
    while(1)
    {
        // Handle User Inputs
        if (port0.IsConnected())
        {
            //Switch Rotation mode based on user input
            if (port0.WasPressed(Digital::Button::R))
            {
                ++CurrentMode;
                if (CurrentMode > 2)CurrentMode = 0;
                if (CurrentMode < 0)CurrentMode = 2;
                LoadRBG0(CurrentMode, TestTilebin);

            }
            else if (port0.WasPressed(Digital::Button::L))
            {
                --CurrentMode;
                if (CurrentMode > 2)CurrentMode = 0;
                if (CurrentMode < 0)CurrentMode = 2;
                LoadRBG0(CurrentMode, TestTilebin);
            }

            //Update Rotations based on user input
            if (port0.IsHeld(Digital::Button::X)) angX += Angle::FromDegrees(0.3);
            else if (port0.IsHeld(Digital::Button::A)) angX -= Angle::FromDegrees(0.3);

            if (port0.IsHeld(Digital::Button::Y)) angY +=Angle::FromDegrees(0.3);
            else if (port0.IsHeld(Digital::Button::B)) angY -= Angle::FromDegrees(0.3);

            if (port0.IsHeld(Digital::Button::Z)) angZ += Angle::FromDegrees(0.3);
            else if (port0.IsHeld(Digital::Button::C)) angZ -=Angle::FromDegrees(0.3);

        }

        /*Rotate current matrix and Set it in rotation parameters.
        RBG0 plane gets its position, rotation, and scale based on the same Matrix that SRL::Scene3D uses.

        -NOTE: because this is Euler rotation, the order they are applied matters (Rot(XYZ) is not equal to Rot(YXZ)).
        A different order of rotations might change which axis preserve the correct perspective of RBG0 in 2 Axis mode.
        With the standard XYZ ordering below, X and Z rotations preserve perspective in 2 axis mode*/
        SRL::Scene3D::PushMatrix();
        {
            SRL::Scene3D::Translate(RBG0position);

            SRL::Scene3D::RotateX(angX);
            SRL::Scene3D::RotateY(angY);
            SRL::Scene3D::RotateZ(angZ);

            //Apply the current matrix transform to RBG0:
            SRL::VDP2::RBG0::SetCurrentTransform();
            //To demonstrate that the transform applies the same to polygons, draw a polygon plane using the same matrix:
            SRL::Scene3D::DrawMesh(TestMesh);

        }
        SRL::Scene3D::PopMatrix();

        SRL::Core::Synchronize();
    }

    return 0;
}

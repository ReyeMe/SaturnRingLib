/*VDP2 Rotation Demo:
This Demo Demonstrates how to configure a rotating scroll and provides an interactive
showcase of the differences in rotation behavior between rotation modes. Since VRAM demands
increase with the number of rotation axis used, It is recommended to select the minimum
Number of Axis Required for your desired use case.
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Input;

static void LoadRBG0(uint8_t config, SRL::Tilemap::ITilemap* map )
{
    SRL::VDP2::RBG0::ScrollDisable();//turn off scroll display so we dont see junk when loading to VRAM
    SRL::VDP2::ClearVRAM();//Clearing VDP2 VRAM because use differs between Rotation Modes
    SRL::VDP2::RBG0::LoadTilemap(*map);//Transfer Tilemap to VRAM again 
    
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
        -Snes Mode7 style effect like Maio Kart/F-Zero
        -Works for 3D games where there is no rolling around camera's axis*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);
        SRL::Debug::Print(1,5,"Rotation Mode <L [2 Axis] R>");
        break;
   
    case 2:
        /*configure RBG0 with 3 Axis Rotation:
       -Full 3D rotation
       -Works for 3D flight sims or other games where camera rolling is desired*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::ThreeAxis);
        SRL::Debug::Print(1,5,"Rotation Mode <L [3 Axis] R>");
        break;
    }
   
    SRL::VDP2::RBG0::ScrollEnable();//Turn Scroll Display back on
}
/*
//Steps to tile RBG0 with a 256x256 TGA image:

//0)Create a 256 x 256 pixel tileable image in save it as a 256 color TGA in the cd data folder 

//In main() after Core::Initialize():

//1) load the tga from the file you put in the cd data folder:
SRL::Bitmap::TGA* MyBmp = new SRL::Bitmap::TGA("IMAGE.TGA");

//2) create a 1 page tilemap from the loaded image 
SRL::Tilemap::Interfaces::Bmp2Tile* MyTilemap = new SRL::Tilemap::Interfaces::Bmp2Tile(*MyBmp,1);

//3) The easiest unit to infinitly tile a plane with is one page (512x512 pixels), but
// our image is still too small to fill it completely, so copy the image 3 more times  
// to make it a 2x2 tiled page: 
MyTilemap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(16, 0));
MyTilemap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(0, 16));
MyTilemap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(16, 16));

//4) Transfer our newly created Tilemap to VDP2 VRAM and Register on RBG0:
SRL::VDP2::RBG0::LoadTilemap(*Tile);

//4.5)Free work ram now that the transfer is complete
delete MyBmp;
delete MyTilemap;

//5) Set The rotation mode to 2 Axis:
SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);

//6) give it a default 3D translation and rotation to test its display with
SRL::Scene3D::PushMatrix();
{
        SRL::Scene3D::Translate(0.0, 10.0, 0.0);
        SRL::Scene3D::RotateX(Angle::FromDegrees(90);
        SRL::VDP2::RBG0::SetCurrentTransform();
}
SRL::Scene3D::PopMatrix();

//7) Enable RBG0 display
SRL::VDP2::RBG0::ScrollEnable();

*/
int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));

    
    Digital port0(0); // Initialize gamepad on port 0
    
    int8_t CurrentMode = 0;
   
   
    //LoadRBG0(0, TestTilebin);

    //SRL::VDP2::RBG0::SetPriority(SRL::VDP2::Priority::Layer2);//set RBG0 priority
    SRL::Bitmap::TGA* MyBmp = new SRL::Bitmap::TGA("TILE.TGA");
    SRL::Tilemap::Interfaces::Bmp2Tile* MyTileMap = new SRL::Tilemap::Interfaces::Bmp2Tile(*MyBmp, 1);
    MyTileMap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(16, 0));
    MyTileMap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(0, 16));
    MyTileMap->CopyMap(0, SRL::Tilemap::Coord(0, 0), SRL::Tilemap::Coord(15, 15), 0, SRL::Tilemap::Coord(16, 16));
    SRL::VDP2::RBG0::LoadTilemap(*MyTileMap);
    delete MyBmp;
    delete MyTileMap;
    SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);
    SRL::VDP2::RBG0::ScrollEnable();
    //SRL::VDP2::RBG0::ScrollEnable();//enable display of RBG0 
    
    SRL::Tilemap::Interfaces::CubeTile* TestTilebin = new SRL::Tilemap::Interfaces::CubeTile("FOGRGB.BIN");//Load tilemap from cd to main RAM
    //variables to store current RBG0 rotation
    int16_t angY = (int16_t)DEGtoANG(0.0);
    int16_t angZ = (int16_t)DEGtoANG(0.0);
    int16_t angX = (int16_t)DEGtoANG(100.0);
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
            if (port0.IsHeld(Digital::Button::X)) angX += DEGtoANG(0.3);
            else if (port0.IsHeld(Digital::Button::A)) angX -= DEGtoANG(0.3);

            if (port0.IsHeld(Digital::Button::Y)) angY += DEGtoANG(0.3);
            else if (port0.IsHeld(Digital::Button::B)) angY -= DEGtoANG(0.3);

            if (port0.IsHeld(Digital::Button::Z)) angZ += DEGtoANG(0.3);
            else if (port0.IsHeld(Digital::Button::C)) angZ -= DEGtoANG(0.3);
            
        }
       
        //Rotate current matrix and Set it in rotation parameters 
        slPushMatrix();
        {
            SRL::Scene3D::Translate(0.0, 100.0, 0.0);
            //slRotY(angY);
            slRotX(angX);
            //SRL::Scene3D::RotateX(Angle::FromDegrees(100));
            //slRotZ(angZ);
            SRL::VDP2::RBG0::SetCurrentTransform();
        }
        slPopMatrix();
        SRL::VDP2::RBG0::ScrollEnable();

        
        SRL::Core::Synchronize();
    }

    return 0;
}

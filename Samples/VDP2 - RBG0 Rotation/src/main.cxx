/*VDP2 Rotation Demo:
This Demo Demonstrates how to configure a rotating scroll and provides an interactive
showcase of the differences in rotation behavior between rotation modes. Since VRAM demands
increase with the number of rotation axis used, It is recommended to use the minimum
Number of Axis Required for your desired use case.
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Input;

static void LoadRBG0(uint8_t config, SRL::Tilemap::ITilemap* map )
{
    SRL::VDP2::RBG0::ScrollDisable();//turn off scroll display so we dont see junk when loading to VRAM
    slScrAutoDisp(RBG0OFF);
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
        //VDP2_RAMCTL = 0x130b;
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
       -Works for 3D flight sims or other games where carmera rolling is desired*/
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
    
    int8_t CurrentMode = 0;
   
    SRL::Tilemap::SGLb* TestTilebin = new SRL::Tilemap::SGLb("FOGRGB.BIN");//Load tilemap from cd to main RAM
   
    LoadRBG0(0, TestTilebin);

    SRL::VDP2::RBG0::LoadTilemap(*TestTilebin);
    SRL::VDP2::RBG0::SetPriority(SRL::VDP2::Priority::Layer2);//set RBG0 priority
    SRL::VDP2::RBG0::ScrollEnable();//enable display of RBG0 
    

    //variables to store current RBG0 rotation
    int16_t angY = (int16_t)DEGtoANG(0.0);
    int16_t angZ = (int16_t)DEGtoANG(0.0);
    int16_t angX = (int16_t)DEGtoANG(0.0);
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
            slTranslate(toFIXED(0.0), toFIXED(100), MsScreenDist);
            slRotY(angY);
            slRotX(angX);
            slRotZ(angZ);
            SRL::VDP2::RBG0::SetCurrentTransform();
        }
        slPopMatrix();
        
        SRL::Core::Synchronize();
    }

    return 0;
}

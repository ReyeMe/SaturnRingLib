/*VDP2 Rotation Demo:
This Demo Demonstrates how to configure a rotating scroll and provides an interactive
showcase of the differences in rotation behavior between rotation modes. Since VRAM demands
increase with the number of rotation axis used, It is recommended to use the minimum
Number of Axis Required for your desired use case.
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Input;
extern uint16_t VDP2_KTCTL;

//demonstration of Loading and Displaying on RBG0 using the 3 different rotation modes 
static void SetSGLVar(){
    VDP2_KTCTL = 1;
}
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
        //VDP2_RAMCTL = 0x130b;
        break;
     
    case 1:
        /*configure RBG0 with 2 Axis rotation:
        -X and Z axis rotation displays correctly while Y rotation causes scaling/skewing
        -Snes Mode7 style effect like Maio Kart/F-Zero
        -Works for 3D games where there is no rolling around camera's axis*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::TwoAxis);

        break;
   
    case 2:
        /*configure RBG0 with 3 Axis Rotation:
       -Full 3D rotation
       -Works for 3D flight sims or other games where carmera rolling is desired*/
        SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::ThreeAxis);
        break;
    }
    SRL::VDP2::RBG0::ScrollEnable();//Turn Scroll Display back on
}

int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
   
    // Initialize gamepad on port 0
    Digital port0(0);
    int8_t CurrentMode = 0;

    //Loading  RBG0 Tilemap
    //SRL::VDP2::RBG0::MapAddress = (void*)VDP2_VRAM_A0;
    //SRL::VDP2::RBG0::CelAddress = (void*)VDP2_VRAM_A1;
    //SRL::VDP2::RBG0::KtableAddress = (void*)VDP2_VRAM_B0;
    SRL::Tilemap::SGLb* TestTilebin = new SRL::Tilemap::SGLb("TESTN2.BIN");//Load tilemap from cd to main RAM
    //SRL::VDP2::RBG0::LoadTilemap(*TestTilebin);//Transfer Tilemap to VDP2 VRAM and register to RBG0
    /*TEST ALLOCATION WIthout Loading*/
    //SRL::Tilemap::TilemapInfo myinfo= TestTilebin->GetInfo();
    //void* my_address = SRL::VDP2::VRAM::AutoAllocateCel(myinfo, scnRBG0);
    //SRL::Debug::Print(1, 8, "VRAM ADDRESS = %d", (uint32_t)my_address-VDP2_VRAM_A0);
    /*normally you should delete ITilemap after loading but because this demo reloads to VRAM when 
    switching rotation modes, I leave it in RAM*/

    //Configuring intital RBG0 settings
    //SRL::VDP2::RBG0::KtableAddress = (void*)VDP2_VRAM_B0;
    //SRL::VDP2::RBG0::SetRotationMode(SRL::VDP2::RotationMode::ThreeAxis);//Set the rotation mode
    LoadRBG0(0, TestTilebin);
    //slMakeKtable((void*)VDP2_VRAM_B0);
    //slKtableRA((void*)VDP2_VRAM_B0, K_FIX | K_LINE | K_2WORD | K_ON);
    SRL::VDP2::RBG0::SetPriority(SRL::VDP2::Priority::Layer2);//set RBG0 priority
    SRL::VDP2::RBG0::ScrollEnable();//enable display of RBG0 
 

    //variables to store current RBG0 rotation

    int16_t angY = (int16_t)DEGtoANG(0.0);
    int16_t angZ = (int16_t)DEGtoANG(180.0);
   
    int16_t angX = (int16_t)DEGtoANG(180.0);
   
    
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
            slTranslate(toFIXED(0), toFIXED(100), MsScreenDist);
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

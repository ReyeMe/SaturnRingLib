/*VDP2 Color Calculation Demo:
This Demo Shows The use of VDP2 Color Calculation and Priority through SRL and demonstrates
its effect by creating an interactive version of color calculation diagrams found in the
 VDP2 Users Manual.

A Blue Triangle, Green Square, and Red Circle are displayed on 3 separate VDP2 ScrollScreens.
Priorities are set such that the Triangle draws on top, Square draws in middle, and Circle draws
on bottom. By changing opacities of each layer and the Color calculation mode in real time, users
can examine the behavior of VDP2 color calculation under a variety of circumstances.

A tutorial with deeper explanations of color calculation behavior can be found 
in the Readme included with this sample.
*/

#include <srl.hpp>

using namespace SRL::Types;
using namespace SRL::Input;
using namespace SRL::Math;

//Class to manage menu navigation and change VDP2 color calculation based on user input
class UiManager
{
    private:

    //Store opacity values for user to adjust
    Fxp TriangleOpacity = 0.5;
    Fxp SquareOpacity = 0.5;
    Fxp CircleOpacity = 0.99;
    
    //store user selected color mode 
    uint8_t Mode = 0;
    bool extend = false;
    
    //track which menu option user is selecting
    int CursorLine = 0;

    public:

    UiManager()
    {
        CursorLine = 0;
    }
    void UpdateUi(Digital* input)
    {
        //Update values based on user input:
        if(input->WasPressed(Digital::Button::Down)&& CursorLine<2)
        {
            CursorLine+=1;
        }
        else if(input->WasPressed(Digital::Button::Up)&& CursorLine>0)
        {
            CursorLine-=1;
        }
        //add to opacities based on user input
        else if(input->IsHeld(Digital::Button::Left))
        {
           switch(CursorLine)
           {
            case(0):
                TriangleOpacity-=Fxp(0.02);
                break;
            case(1):
                SquareOpacity-=Fxp(0.02);
                break;
            case(2): 
                CircleOpacity-=Fxp(0.02);
                break;
            default: 
                break;
            }
        }
        //subtract from opacities based on user input
        else if(input->IsHeld(Digital::Button::Right))
        {
           switch(CursorLine)
           {
            case(0):
                TriangleOpacity+=Fxp(0.02);
                break;
            case(1):
                SquareOpacity+=Fxp(0.02);
                break;
            case(2): 
                CircleOpacity+=Fxp(0.02);
                break;
            default: 
                break;
            }
        }
        //swap between 3 color calculation modes based on user input
        if(input->WasPressed(Digital::Button::R))
        {
            if(Mode<2)++Mode;
            else Mode = 0;
            switch(Mode)
            {
                case 0:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorRatiosTop,extend);
                    SRL::Debug::Print(2,10,"Color Ratios Top");
                    break;
                case 1:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorRatios2nd, extend);
                    SRL::Debug::Print(2,10,"Color Ratios 2nd");
                    break;
                case 2:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorAddition, extend);
                    SRL::Debug::Print(2,10,"Color Addition  ");
                    break;
            }

        }

        //toggle extended color calculation based on user input:
        if(input->WasPressed(Digital::Button::Z)) 
        {
            extend = !extend;
            if(extend) SRL::Debug::Print(2,13,"ON ");
            else  SRL::Debug::Print(2,13,"OFF");
            switch(Mode)
            {
                case 0:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorRatiosTop,extend);
                    break;
                case 1:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorRatios2nd, extend);
                    break;
                case 2:
                    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorAddition, extend);
                    break;
            }
        }

        //ensure Opacity is in valid range:
        TriangleOpacity = Fxp::Clamp(TriangleOpacity,0.0,1.0);
        SquareOpacity = Fxp::Clamp(SquareOpacity,0.0,1.0);
        CircleOpacity = Fxp::Clamp(CircleOpacity,0.0,1.0);
        
        //Update the opacities of the ScrollScreens corresponding to the shapes:
        SRL::VDP2::NBG0::SetOpacity(TriangleOpacity);
        SRL::VDP2::NBG1::SetOpacity(SquareOpacity);
        SRL::VDP2::NBG2::SetOpacity(CircleOpacity);

    }

    //print updated values to menu using debug functions:
    void PrintUi()
    {   
        //represent fixed point opacities as percentages in menu:
        int TriPercent = (TriangleOpacity.RawValue()*100)>>16;
        int SqrPercent = (SquareOpacity.RawValue()*100)>>16;
        int CirPercent = (CircleOpacity.RawValue()*100)>>16;
        SRL::Debug::Print(11,5,"%d %    ", TriPercent);
        SRL::Debug::Print(11,6,"%d %    ", SqrPercent);
        SRL::Debug::Print(11,7,"%d %    ", CirPercent);

        //Print a basic arrow to indicate the menu line player is selecting:
        SRL::Debug::Print(0,5,"  ");
        SRL::Debug::Print(0,6,"  ");
        SRL::Debug::Print(0,7,"  ");
        SRL::Debug::Print(0,CursorLine+5,"->");

    }

};

int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
    Digital port0(0); // Initialize gamepad on port 0

    //set initial VDP2 color calc Mode
    SRL::VDP2::SetColorCalcMode(SRL::VDP2::ColorCalcMode::UseColorRatiosTop, false);
    
    //Load Triangle image to NBG0
    SRL::Bitmap::TGA* img = new SRL::Bitmap::TGA("TRI.TGA");//Load Bitmap image to work RAM
    SRL::Tilemap::Interfaces::Bmp2Tile* TestTilebmp = new SRL::Tilemap::Interfaces::Bmp2Tile(*img);//convert bitmap to tilemap
    SRL::VDP2::NBG0::LoadTilemap(*TestTilebmp);//Transfer tilemap from work RAM to VDP2 VRAM and register with NBG2
    delete TestTilebmp;//free tilemap from work ram 
    delete img;//free original bitmap from work ram

    //Load Square imageto NBG1
    img = new SRL::Bitmap::TGA("SQR.TGA");//Load Bitmap image to work RAM
    TestTilebmp = new SRL::Tilemap::Interfaces::Bmp2Tile(*img);//convert bitmap to tilemap
    SRL::VDP2::NBG1::LoadTilemap(*TestTilebmp);//Transfer tilemap from work RAM to VDP2 VRAM and register with NBG2
    delete TestTilebmp;//free tilemap from work ram 
    delete img;//free original bitmap from work ram
    
    //Load Circle image to NBG2
    img = new SRL::Bitmap::TGA("CIR.TGA");//Load Bitmap image to work RAM
    TestTilebmp = new SRL::Tilemap::Interfaces::Bmp2Tile(*img);//convert bitmap to tilemap
    SRL::VDP2::NBG2::LoadTilemap(*TestTilebmp);//Transfer tilemap from work RAM to VDP2 VRAM and register with NBG2
    delete TestTilebmp;//free tilemap from work ram   
    delete img;//free original bitmap from work ram
    
    SRL::VDP2::NBG3::SetOpacity(1.0);
    //store XY screen positions of Background scrolls:
    Vector2D Nbg0Position(-80.0,   -45.0);
    Vector2D Nbg1Position( -135.0, -100.0);
    Vector2D Nbg2Position(-45.0, -100.0);
    
    SRL::VDP2::NBG0::SetPriority(SRL::VDP2::Priority::Layer6);//set NBG0 priority
    SRL::VDP2::NBG0::SetPosition(Nbg0Position);
    SRL::VDP2::NBG0::SetOpacity(0.5);//set opacity of NBG0
    SRL::VDP2::NBG0::ScrollEnable();//enable display of NBG0 

    SRL::VDP2::NBG1::SetPriority(SRL::VDP2::Priority::Layer4);//set NBG1 priority
    SRL::VDP2::NBG1::SetPosition(Nbg1Position);
    SRL::VDP2::NBG1::SetOpacity(0.5);//set opacity of NBG1  
    SRL::VDP2::NBG1::ScrollEnable();//enable display of NBG1 
    
    SRL::VDP2::NBG2::SetPriority(SRL::VDP2::Priority::Layer2);// Set NBG2 priority between NBG0 and NBG1
    SRL::VDP2::NBG2::SetOpacity(0.5);//set opacity of NBG2
    SRL::VDP2::NBG2::SetPosition(Nbg2Position);
    SRL::VDP2::NBG2::ScrollEnable();//enable display of NBG2

    //initialize our Ui manager class:
    UiManager MyUi = UiManager();
    //print overlay for UI:
    SRL::Debug::Print(0,2,"VDP2 Color Calculation Sample");
    SRL::Debug::Print(0,4,"Use D-pad to Select and Adust Opacities:");
    SRL::Debug::Print(2,5,"Triangle ");
    SRL::Debug::Print(2,6,"Square   ");
    SRL::Debug::Print(2,7,"Circle   ");

    SRL::Debug::Print(0,9,"Press R to switch Modes:");
    SRL::Debug::Print(2,10,"Color Ratios Top");
    SRL::Debug::Print(0,12,"Press Z to toggle Extension:");
    SRL::Debug::Print(2,13,"OFF");

    //Main Game Loop 
    while(1)
    {
        //Run Update and print functions from UiManager class to change 
        //opacity settings based on user input
        MyUi.UpdateUi(&port0);
        MyUi.PrintUi();
       
       
        //Syncronize loop update with refresh rate
        SRL::Core::Synchronize();
    }
    return 0;
}

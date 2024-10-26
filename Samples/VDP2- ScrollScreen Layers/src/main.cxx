/*VDP2 Scroll Layer Demo:
This Demo Shows how to Load Tilemaps to display on ScrollScreens and adjust their display settings
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Input;



int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
    Digital port0(0); // Initialize gamepad on port 0
    
    SRL::Tilemap::SGLb* TestTilebin = new SRL::Tilemap::SGLb("TESTN2.BIN");//Load tilemap from cd to main RAM
    SRL::VDP2::NBG0::LoadTilemap(*TestTilebin);
    delete TestTilebin;

   
    TestTilebin = new SRL::Tilemap::SGLb("FOGRGB.BIN");//Load tilemap from cd to main RAM
    SRL::VDP2::NBG1::LoadTilemap(*TestTilebin);
    delete TestTilebin;

    SRL::Debug::Print(2, 4, "%x", SRL::VDP2::NBG0::GetCelAddress());
    SRL::Debug::Print(2, 5, "%x", SRL::VDP2::NBG1::GetCelAddress());
    SRL::Debug::Print(2, 6, "%x", SRL::VDP2::NBG0::GetMapAddress());
    SRL::Debug::Print(2, 7, "%x", SRL::VDP2::NBG1::GetMapAddress());
    SRL::VDP2::NBG0::SetPriority(SRL::VDP2::Priority::Layer6);//set NBG0 priority
    SRL::VDP2::NBG0::ScrollEnable();//enable display of NBG0 
      SRL::VDP2::NBG0::SetOpacity(SRL::Types::Fxp(0.5));//set opacity of NBG0

    SRL::VDP2::NBG1::SetPriority(SRL::VDP2::Priority::Layer2);//set NBG1 priority
    SRL::VDP2::NBG1::ScrollEnable();//enable display of NBG1 
   
    SRL::Debug::Print(1,3,"VDP2 ScrollScreen Sample");
   
    Vector2D Nbg0Position(0.0,0.0);
    Vector2D Nbg1Position(0.0,0.0);
    //Main Game Loop 
    while(1)
    {
        Nbg0Position += Vector2D(1.0, 2.0);
        Nbg1Position += Vector2D(-2.0, 1.0);
        SRL::VDP2::NBG0::SetPosition(Nbg0Position);
        SRL::VDP2::NBG1::SetPosition(Nbg1Position);
               
        SRL::Core::Synchronize();
    }

    return 0;
}

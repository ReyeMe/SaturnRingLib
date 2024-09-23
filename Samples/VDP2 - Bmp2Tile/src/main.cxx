/*Conversion of Bitmap to VDP2 Tilemap:
This sample demonstrates how to convert a Bitmap Image into a Tilemap and edit the resulting map
layout using the Bmp2Tile class. 
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Input;



int main()
{
    SRL::Core::Initialize(HighColor(20, 10, 50));
    Digital port0(0); // Initialize gamepad on port 0
    SRL::VDP2::NBG0::CelAddress = (void*)VDP2_VRAM_A0;
    SRL::VDP2::NBG0::MapAddress = (void*)VDP2_VRAM_A1;
    
    //Load Bitmap from CD to Work RAM 
    SRL::Bitmap::TGA* test = new SRL::Bitmap::TGA("FOG.TGA");
    //Copy Bitmap to a Tilemap in Main Work Ram
    SRL::Tilemap::Bmp2Tile * TestBmp2Tile = new SRL::Tilemap::Bmp2Tile(*test,1);
    delete test;//No need to keep original bitmap after conversion
    
    //manipulate tilemap by copying tiled patterns between its pages:
    TestBmp2Tile->CopyMap(0, SRL::Tilemap::Co(0, 0), SRL::Tilemap::Co(7, 7), 0, SRL::Tilemap::Co(8, 8));
    
    //Load the Tilemap from Work RAM to VRAM registered with NBG0
    SRL::VDP2::NBG0::LoadTilemap(*TestBmp2Tile);
    SRL::VDP2::NBG0::SetPriority(SRL::VDP2::Priority::Layer2);//set NBG0 priority
    SRL::VDP2::NBG0::ScrollEnable();//enable display of NBG0 
    
    delete TestBmp2Tile;

    //Main Game Loop 
    while(1)
    {
        SRL::Core::Synchronize();
    }

    return 0;
}

/*Conversion of Bitmap to VDP2 Tilemap:
This sample demonstrates how to convert a Bitmap Image into a Tilemap and edit the resulting map
layout using the Bmp2Tile class. 
*/
#include <srl.hpp>
 
using namespace SRL::Types;
using namespace SRL::Input;

//example loading the weapon sprites for right hand
void LoadRightHand(char* img)
{
    //load TGA file:
    SRL::Bitmap::TGA* MyBmp = new SRL::Bitmap::TGA(img);
    //convert to tilemap with 5 pages(Source Pattern+1 page per animation frame):
    SRL::Tilemap::Bmp2Tile* Tile = new SRL::Tilemap::Bmp2Tile(*MyBmp,5);
    delete MyBmp;//no longer need original bitmap
    //copy map of each individual frame from source page to the top left corner its own dedicated page:
    //(the tiles created are 16*16 pixels , so the 128*128 pixel frames have area of 8x8 tile coordinates)
    Tile->CopyMap(0, SRL::Tilemap::Co(0, 0), SRL::Tilemap::Co(7, 7), 1, SRL::Tilemap::Co(0, 0));
    Tile->CopyMap(0, SRL::Tilemap::Co(8, 0), SRL::Tilemap::Co(15, 7), 2, SRL::Tilemap::Co(0, 0));
    Tile->CopyMap(0, SRL::Tilemap::Co(16,0), SRL::Tilemap::Co(23, 7), 3, SRL::Tilemap::Co(0, 0));
    Tile->CopyMap(0, SRL::Tilemap::Co(24,0), SRL::Tilemap::Co(31, 7), 4, SRL::Tilemap::Co(0, 0));
    
    //We will display right hand on NBG1 Layer:
    SRL::VDP2::NBG1::LoadTilemap(*Tile);
    //allign Layer to display in bottom right corner of screen:
    Vector2D pos(-200.0, -100.0);
    SRL::VDP2::NBG1::SetPosition(pos);
    //Set to display only the page of the first frame:
    SRL::VDP2::NBG1::SetMapLayout(1, 1, 1, 1);
    delete Tile;//Tilemap has been loaded to VRAM, no longer it in work ram (unless you want to edit it further)
}
//example animating right weapon 
void SetFrameRight(uint8_t frame)
{
    SRL::VDP2::NBG1::SetMapLayout(frame, frame, frame, frame);
}

int main()
{
    SRL::Core::Initialize(HighColor(20, 10, 50));
    Digital port0(0); // Initialize gamepad on port 0
    SRL::VDP2::NBG0::CelAddress = (void*)VDP2_VRAM_A0;
    SRL::VDP2::NBG0::MapAddress = (void*)VDP2_VRAM_A1;
    
    //Load Bitmap from CD to Work RAM 
    //SRL::Bitmap::TGA* test = new SRL::Bitmap::TGA("FOG.TGA");
    //Copy Bitmap to a Tilemap in Main Work Ram
    //SRL::Tilemap::Bmp2Tile * TestBmp2Tile = new SRL::Tilemap::Bmp2Tile(*test,1);
    //delete test;//No need to keep original bitmap after conversion
    
    //manipulate tilemap by copying tiled patterns between its pages:
    //TestBmp2Tile->CopyMap(0, SRL::Tilemap::Co(0, 0), SRL::Tilemap::Co(7, 7), 0, SRL::Tilemap::Co(8, 8));
    
    //Load the Tilemap from Work RAM to VRAM registered with NBG0
    //SRL::VDP2::NBG0::LoadTilemap(*TestBmp2Tile);
    LoadRightHand("WPN.TGA");
    SRL::VDP2::NBG1::SetPriority(SRL::VDP2::Priority::Layer2);//set NBG0 priority
    SRL::VDP2::NBG1::ScrollEnable();//enable display of NBG0 
    
   // delete TestBmp2Tile;
    Vector2D pos = Vector2D(0.0, 0.0);
    uint8_t frame = 1;
    //Main Game Loop 
    while(1)
    {
        if (port0.WasPressed(Digital::Button::R))
        {
            ++frame;
            if (frame > 4) frame = 1;
            SetFrameRight(frame);
        }
        //pos += Vector2D(1.0, 1.0);
        
        SRL::Core::Synchronize();

    }

    return 0;
}

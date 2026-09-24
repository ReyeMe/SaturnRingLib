/*VDP2 Scroll Layer Demo:
This Demo shows how to Load and display 2 Tilemaps on RBG0 simultaneously using the primary and secondary 
Rotation Parameters.  
*/
#include <srl.hpp>

using namespace SRL::Types;
using namespace SRL::Input;
using namespace SRL::Math;

int main()
{
    SRL::Core::Initialize(HighColor(20,10,50));
    
    // variables to store current position/ rotation of the 2 RBG0 tilemaps:
    Vector3D Rbg0PositionPrimary(0.0,80.0,80.0);//80 units below camera for ground
    Vector3D Rbg0PositionSecondary(0.0,-80.0,300.0);//80 units above camera for clouds
    Angle Rbg0RotationSecondary = Angle::FromDegrees(0.0);//Just for fun, we will also make the clouds rotate
    
    SRL::Tilemap::Interfaces::CubeTile* TestTilebin= new SRL::Tilemap::Interfaces::CubeTile("GROUND.BIN");//Load ground tilemap from cd to work RAM
    
    SRL::Tilemap::Interfaces::CubeTile *TestTilebinB= new SRL::Tilemap::Interfaces::CubeTile("CLOUDS.BIN");//Load fog tilemap from cd to work RAM
    
    //This new overload allows loading 2 tilemaps for RBG0 and initializing their rotation modes all in one go:
    SRL::VDP2::RBG0::LoadTilemap(*TestTilebin,*TestTilebinB, SRL::VDP2::RotationMode::TwoAxis,SRL::VDP2::RotationMode::TwoAxis);
    
    delete TestTilebin;//free work RAM 
    delete TestTilebinB;//free work RAM
    
    //-----------------------------------------------------------------------------------------------------------------------
    /*Example of laying out a multi plane tilemap in a 4x4 grid for the Primary Tilemap*/
    uint8_t mapLayout[4][4] = 
    {
        0,1,2,3,
        0,1,2,3,
        0,1,2,3,
        0,1,2,3,
    };

    SRL::VDP2::RBG0::SetPlanes(mapLayout,SRL::VDP2::RotationParameter::Primary);//set the multi-plane layout of Primary Tilemap 

    //------------------------------------------------------------------------------------------------------------------------
    //set initial perspective for the Primary and secondary tilemap rotations:
    SRL::Scene3D::LoadIdentity();
    
    SRL::Scene3D::PushMatrix();
    {
        SRL::Scene3D::Translate(Rbg0PositionPrimary);
        SRL::Scene3D::RotateX(Angle::FromDegrees(-90.0));
        SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Primary);
    }

    SRL::Scene3D::PushMatrix();
    {
        SRL::Scene3D::Translate(Rbg0PositionSecondary);
        SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));
        SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Secondary);
    }
    SRL::Scene3D::PopMatrix();
    
    /*Using SetParameterMode() here is not strictly necessary in our example because PerspectiveSwitch 
    is the default setting when loading 2 tilemaps, but use it if you want a different setting:*/
    SRL::VDP2::RBG0::SetParameterMode(SRL::VDP2::SwitchMode::PerspectiveSwitch);
    
    SRL::VDP2::RBG0::TransparentDisable();//Turn off transparent pixels on RBG0
    
    SRL::VDP2::RBG0::ScrollEnable();//Display RBG0 on screen
    
    SRL::Debug::Print(1,3,"VDP2 Rotation Parameters Sample");
   
    while(1)
    {    
        //Demonstrate independent manipulation of Primary and Secondary tilemaps 
        Rbg0PositionPrimary.Z-=Fxp(1.0);//scroll ground towards camera
        Rbg0PositionPrimary.X-=Fxp(1.0);//scroll ground towards camera
        Rbg0RotationSecondary+=Angle::FromDegrees(0.1);//rotate clouds
       
        //set transform of primary tilemap:
        SRL::Scene3D::LoadIdentity();
        SRL::Scene3D::PushMatrix();
        {
            SRL::Scene3D::Translate(Rbg0PositionPrimary);
            SRL::Scene3D::RotateX(Angle::FromDegrees(-90.0));//'floor' is always rotated -90 degrees X from its default
            SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Primary);
        }
        SRL::Scene3D::PopMatrix();

        //set perspective matrix of secondary tilemap:
        SRL::Scene3D::PushMatrix();
        {
            SRL::Scene3D::Translate(Rbg0PositionSecondary);
            SRL::Scene3D::RotateX(Angle::FromDegrees(90.0));//ceilng is always rotated +90 degrees X from its default
            SRL::Scene3D::RotateZ(Rbg0RotationSecondary);
            SRL::VDP2::RBG0::SetCurrentTransform(SRL::VDP2::RotationParameter::Secondary);
        }
        SRL::Scene3D::PopMatrix();
    
        SRL::Core::Synchronize();
    }
    return 0;
}

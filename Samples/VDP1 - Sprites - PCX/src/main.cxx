#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_pcx.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Main program entry
int main()
{
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1, 1, "VDP1 Sprite PCX sample");

    // Load texture
    SRL::Bitmap::PCX *pcx = new SRL::Bitmap::PCX("MAIN.PCX"); // Loads PCX file into main RAM

    if (pcx->IsFormatValid())
    {
      pcx->Dump();
    }

    int32_t textureIndex = SRL::VDP1::TryLoadTexture(pcx);    // Loads PCX into VDP1


    delete pcx; // Frees main RAM

    // Main program loop
    while (1)
    {

      // Rotation center
      Vector2D location = Vector2D();

      // Rotated rectangle points
      Vector3D points = Vector3D(location.X, location.Y, 500.0);

      // Simple sprite
      SRL::Scene2D::DrawSprite(textureIndex, points);


        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

// Main program entry
int main()
{
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 sprite effects sample");

    // Load sprite image
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
    int32_t textureIndex = SRL::VDP1::TryLoadTexture(tga);
    delete tga;

    // Initialize gouraud table entries
    HighColor * table = SRL::VDP1::GetGouraudTable();

    // 1st entry
    table[0] = HighColor::Colors::White;
    table[1] = HighColor::Colors::Red;
    table[2] = HighColor::Colors::Green;
    table[3] = HighColor::Colors::Yellow;

    // 3rd entry
    table[8] = HighColor::Colors::Magenta;
    table[9] = HighColor::Colors::Magenta;
    table[10] = HighColor::Colors::Green;
    table[11] = HighColor::Colors::Green;

    // Start with no rotation
    uint16_t angle = 0;

    // Start main program loop
	while(1)
	{

        // Draw first sprite, use first entry in gouraud table
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 0);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, -60.0, 500.0));

        // Draw second sprite, rotate it, use third gouraud table entry, enable screen doors (mesh) effect, and also scale it
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 2);
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, true);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, 0.0, 500.0), Angle::BuildRaw(angle), Vector2D(0.75, 1.5));
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, false);

        // Draw third sprite with no effect applied
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, -1);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, 60.0, 500.0));

        // Increase rotation angle
        angle+=100;

        SRL::Core::Synchronize();
	}

	return 0;
}

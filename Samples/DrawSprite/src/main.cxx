#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");   // Loads TGA file into main RAM
    int32_t textureIndex = SRL::VDP1::TryLoadTexture(tga);       // Loads TGA into VDP1
    delete tga;                                                 // Frees main RAM

    HighColor * table = SRL::VDP1::GetGouraudTable();
    table[0] = HighColor::Colors::White;
    table[1] = HighColor::Colors::Red;
    table[2] = HighColor::Colors::Green;
    table[3] = HighColor::Colors::Yellow;

    table[8] = HighColor::Colors::Magenta;
    table[9] = HighColor::Colors::Magenta;
    table[10] = HighColor::Colors::Green;
    table[11] = HighColor::Colors::Green;

    uint16_t angle = 0;

	while(1)
	{

        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 0);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, 0.0, 500.0));


        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 2);
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, true);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, -60.0, 500.0), Angle::FromRaw(angle), Vector2D(0.75, 1.5));
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, false);


        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, -1);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(0.0, 60.0, 500.0));

        angle+=100;

        SRL::Core::Synchronize();
	}

	return 0;
}

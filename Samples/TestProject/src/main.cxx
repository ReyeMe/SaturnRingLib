#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(HighColor(20,10,50));

    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
    Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
    delete tga;

    Vector2D first[4] =
    {
        Vector2D(0.0, 0.0),
        Vector2D(0.0, 20.0),
        Vector2D(30.0, 10.0),
        Vector2D(30.0, 0.0)
    };
    
    Vector2D second[4] =
    {
        Vector2D(0.0, 40.0) + first[0],
        Vector2D(0.0, 40.0) + first[1],
        Vector2D(0.0, 40.0) + first[2],
        Vector2D(0.0, 40.0) + first[3]
    };
    
    Vector2D third[4] =
    {
        Vector2D(0.0, 40.0) + second[0],
        Vector2D(0.0, 40.0) + second[1],
        Vector2D(0.0, 40.0) + second[2],
        Vector2D(0.0, 40.0) + second[3]
    };
    
    HighColor* table = SRL::VDP1::GetGouraudTable();
    table[0] = HighColor::Colors::Blue;
    table[1] = HighColor::Colors::Red;
    table[2] = HighColor::Colors::Red;
    table[3] = HighColor::Colors::Blue;
    table[4] = HighColor::Colors::Yellow;
    table[5] = HighColor::Colors::Green;
    table[6] = HighColor::Colors::Green;
    table[7] = HighColor::Colors::Yellow;

	while(1)
	{
        DateTime time = DateTime::Now();
        SRL::Debug::Print(1,1, "%d:%d:%d %d.%d.%d    ", time.Hour(), time.Minute(), time.Second(), time.Day(), time.Month(), time.Year());

        SRL::Render::DisableGouraud();
        SRL::Render::DrawPolygon(first, true, HighColor::Colors::White, 500.0);
        SRL::Render::DrawLine(first[0], Vector2D(-50.0, 0.0), HighColor::Colors::White, 500.0);
        SRL::Render::DrawSprite(textureIndex, Vector3D(-50.0, 0.0, 500.0));
        
        SRL::Render::EnableGouraud(0);
        SRL::Render::DrawPolygon(second, false, HighColor::Colors::White, 500.0);
        SRL::Render::DrawLine(second[0], Vector2D(-50.0, 40.0), HighColor::Colors::White, 500.0);
        SRL::Render::DrawSprite(textureIndex, Vector3D(-50.0, 40.0, 500.0));

        SRL::Render::EnableGouraud(1);
        SRL::Render::DrawPolygon(third, false, HighColor::Colors::White, 500.0);
        SRL::Render::DrawLine(third[0], Vector2D(-50.0, 80.0), HighColor::Colors::White, 500.0);
        SRL::Render::DrawSprite(textureIndex, Vector3D(-50.0, 80.0, 500.0));

        SRL::Core::Synchronize();
	}

	return 0;
}

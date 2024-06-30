#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 Gouraud sample");

    // Load texture
    SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
    Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
    delete tga;

    // Define test polygons
    Vector2D second[4] =
    {
        Vector2D(0.0, 0.0),
        Vector2D(0.0, 20.0),
        Vector2D(30.0, 10.0),
        Vector2D(30.0, 0.0)
    };
    
    Vector2D third[4] =
    {
        Vector2D(0.0, 50.0) + second[0],
        Vector2D(0.0, 50.0) + second[1],
        Vector2D(0.0, 50.0) + second[2],
        Vector2D(0.0, 50.0) + second[3]
    };
    
    Vector2D first[4] =
    {
        Vector2D(0.0, -50.0) + second[0],
        Vector2D(0.0, -50.0) + second[1],
        Vector2D(0.0, -50.0) + second[2],
        Vector2D(0.0, -50.0) + second[3]
    };
    
    // Get pointer to gouraud table
    HighColor* table = SRL::VDP1::GetGouraudTable();

    // First gouraud table entry
    table[0] = HighColor::Colors::Blue;
    table[1] = HighColor::Colors::Red;
    table[2] = HighColor::Colors::Red;
    table[3] = HighColor::Colors::Blue;

    // Second gouraud table entry
    table[4] = HighColor::Colors::Magenta;
    table[5] = HighColor::Colors::Green;
    table[6] = HighColor::Colors::Green;
    table[7] = HighColor::Colors::Magenta;

	while(1)
	{
        // Disable gouraud
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, -1);

        // Draw first set of shapes
        SRL::Scene2D::DrawPolygon(first, true, HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawLine(first[0], Vector2D(-50.0, -50.0), HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(-50.0, -50.0, 500.0));
        
        // Enable gouraud and use first gouraud table entry
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 0);

        // Draw second set of shapes
        SRL::Scene2D::DrawPolygon(second, false, HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawLine(second[0], Vector2D(-50.0, 0.0), HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(-50.0, 0.0, 500.0));

        // Enable gouraud and use second gouraud table entry
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 1);

        // Draw third set of shapes
        SRL::Scene2D::DrawPolygon(third, false, HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawLine(third[0], Vector2D(-50.0, 50.0), HighColor::Colors::White, 500.0);
        SRL::Scene2D::DrawSprite(textureIndex, Vector3D(-50.0, 50.0, 500.0));

        // Refresh
        SRL::Core::Synchronize();
	}

	return 0;
}

#include <srl.hpp>

using namespace SRL::Types;

int main()
{
	SRL::Core::Initialize(SaturnColor(200,100,50));

	SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
	Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
	delete tga;
	
	Vector3D location = Vector3D(0.0, 0.0, 500.0);
	Vector2D velocity = Vector2D(0.5, 0.5);

	Vector2D screenMax = Vector2D(
			Fxp::FromInt(SRL::TV::Width >> 1),
			Fxp::FromInt(SRL::TV::Height >> 1));

	Vector2D screenMin = Vector2D(
			-Fxp::FromInt(SRL::TV::Width >> 1),
			-Fxp::FromInt(SRL::TV::Height >> 1));

	while(1)
	{
		SRL::Debug::PrintWithWrap( 1, 1, 1, 40, "X: %x    \nY: %x    ", location.X.Value(), location.Y.Value());

		SRL::Render::DrawSprite(textureIndex, location);

		if (location.X < screenMin.X || location.X > screenMax.X) velocity.X *= -1.0;
		if (location.Y < screenMin.Y || location.Y > screenMax.Y) velocity.Y *= -1.0;

		location += velocity;

		slSynch();
	}

	return 0;
}

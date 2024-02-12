#include <srl.hpp>

using namespace SRL::Types;
using Buttons = SRL::Input::Gamepad::Button;

int main()
{
	SRL::Core::Initialize(SaturnColor(200,100,50));

	SRL::Bitmap::TGA* tga = new SRL::Bitmap::TGA("TEST.TGA");
	Sint32 textureIndex = SRL::VDP1::TryLoadTexture(tga);
	delete tga;
	
	Vector3D location = Vector3D(0.0, 0.0, 500.0);
	Vector2D velocity = Vector2D(0.0, 0.0);

	Vector2D screenMax = Vector2D(
			Fxp::FromInt(SRL::TV::Width >> 1),
			Fxp::FromInt(SRL::TV::Height >> 1));
	Vector2D screenMin = Vector2D(
			-Fxp::FromInt(SRL::TV::Width >> 1),
			-Fxp::FromInt(SRL::TV::Height >> 1));

	SRL::Input::Gamepad gamepad = SRL::Input::Gamepad(0);

	while(1)
	{
		if (gamepad.IsHeld(Buttons::Left)) velocity.X = -0.5;
		else if (gamepad.IsHeld(Buttons::Right)) velocity.X = 0.5;

		if (gamepad.IsHeld(Buttons::Up)) velocity.Y = -0.5;
		else if (gamepad.IsHeld(Buttons::Down)) velocity.Y = 0.5;

		// Move
		location += velocity;
		location.X = SRL::Math::Clamp(screenMin.X, screenMax.X, location.X);
		location.Y = SRL::Math::Clamp(screenMin.Y, screenMax.Y, location.Y);

		// Reset velocity
		velocity.X = 0.0;
		velocity.Y = 0.0;

		SRL::Render::DrawSprite(textureIndex, location);

		slSynch();
	}

	return 0;
}

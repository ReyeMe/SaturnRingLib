#include <srl.hpp>

using namespace SRL::Types;
using Buttons = SRL::Input::Gamepad::Button;

Event<> emptyEvent;
Event<int, Vector3D&> drawSpriteEvent;

void DrawSpriteHandler(int texture, Vector3D& loc)
{
	SRL::Render::DrawSprite(texture, loc);
}

void DoEmptyEventStuff()
{
	static long r = 0;
	SRL::Debug::Print(1, 1, "%d   ", r++);
}

int main()
{
#pragma region SRL Initialization code
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
#pragma endregion SRL Initialization code

	// Attach event
	drawSpriteEvent += DrawSpriteHandler;
	emptyEvent += DoEmptyEventStuff;

	while(1)
	{
#pragma region Movement handling
		if (gamepad.IsHeld(Buttons::Left)) velocity.X = -0.5;
		else if (gamepad.IsHeld(Buttons::Right)) velocity.X = 0.5;
		if (gamepad.IsHeld(Buttons::Up)) velocity.Y = -0.5;
		else if (gamepad.IsHeld(Buttons::Down)) velocity.Y = 0.5;
		location += velocity;
		location.X = SRL::Math::Clamp(screenMin.X, screenMax.X, location.X);
		location.Y = SRL::Math::Clamp(screenMin.Y, screenMax.Y, location.Y);
		velocity.X = 0.0;
		velocity.Y = 0.0;
#pragma endregion Movement handling

		// Invoke event
		emptyEvent.Invoke();
		drawSpriteEvent.Invoke(textureIndex, location);

		slSynch();
	}

	return 0;
}

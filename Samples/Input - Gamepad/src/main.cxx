#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Using to shorten names for input
using namespace SRL::Input;

// Graphics identifiers
#define BackgroundPad (0)
#define BackgroundWire (1)

#define InputArrow (2)
#define InputButton (3)

// Palette for background gamepad
int16_t LoadGamepadPalette(SRL::Bitmap::BitmapInfo* bitmap)
{
    // Get free CRAM bank
    int32_t id = SRL::CRAM::GetFreeBank(bitmap->ColorMode);

    if (id >= 0)
    {
        SRL::CRAM::Palette palette(bitmap->ColorMode, id);

        if (palette.Load((HighColor*)bitmap->Palette->Colors, bitmap->Palette->Count) >= 0)
        {
            // Mark bank as in use
            SRL::CRAM::SetBankUsedState(id, bitmap->ColorMode, true);
            return id;
        }
    }

    // No free bank found
    return -1;
}

// Load sample graphics
void LoadGraphics()
{
    SRL::Bitmap::TGA* background = new SRL::Bitmap::TGA("GAMEPAD.TGA");
    SRL::VDP1::TryLoadTexture(background, LoadGamepadPalette);
    delete background;

    SRL::Bitmap::TGA* wire = new SRL::Bitmap::TGA("WIRE.TGA");
    SRL::VDP1::TryLoadTexture(wire);
    delete wire;
    
    SRL::Bitmap::TGA* arrow = new SRL::Bitmap::TGA("ARROW.TGA");
    SRL::VDP1::TryLoadTexture(arrow);
    delete arrow;
    
    SRL::Bitmap::TGA* button = new SRL::Bitmap::TGA("BUTTON.TGA");
    SRL::VDP1::TryLoadTexture(button);
    delete button;
}

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "Input gamepad sample");
    
    // Load graphics
    LoadGraphics();

    // Initialize gamepad on port 0
    Digital port0(0);

    // Main program loop
	while(1)
	{
        // Draw background
        SRL::Scene2D::DrawSprite(BackgroundPad, Vector3D(0.0,0.0,500.0));
        SRL::Scene2D::DrawSprite(BackgroundWire, Vector3D(0.0,-100.0,500.0), Vector2D(1.0, 40.0));

        // Check whether gamepad is connected to port 0
        if (port0.IsConnected())
        {
            // Check D-Pad
            if (port0.IsHeld(Digital::Button::Up))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-65.0, -17.0, 500.0), Angle::FromDegrees(90));
            }

            if (port0.IsHeld(Digital::Button::Down))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-65.0, 17.0, 500.0), Angle::FromDegrees(-90));
            }

            if (port0.IsHeld(Digital::Button::Left))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-85.0, 0.0, 500.0));
            }

            if (port0.IsHeld(Digital::Button::Right))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-50.0, 0.0, 500.0), Angle::FromDegrees(180));
            }

            // Check start button
            if (port0.IsHeld(Digital::Button::START))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(0.0, 20.0, 500.0));
            }

            // Check triggers
            if (port0.IsHeld(Digital::Button::L))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-90.0, -70.0, 500.0), Angle::FromDegrees(-135));
            }

            if (port0.IsHeld(Digital::Button::R))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(90.0, -70.0, 500.0), Angle::FromDegrees(-45));
            }

            // Check other buttons
            if (port0.IsHeld(Digital::Button::A))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(45.0, 22.0, 500.0));
            }
            
            if (port0.IsHeld(Digital::Button::B))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(69.0, 5.0, 500.0));
            }
            
            if (port0.IsHeld(Digital::Button::C))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(98.0, -5.0, 500.0));
            }

            if (port0.IsHeld(Digital::Button::X))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(37.0, -7.0, 500.0));
            }
            
            if (port0.IsHeld(Digital::Button::Y))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(57.0, -20.0, 500.0));
            }
            
            if (port0.IsHeld(Digital::Button::Z))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(80.0, -30.0, 500.0));
            }
        }

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

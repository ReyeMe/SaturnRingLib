#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;

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
    using namespace SRL::Math;
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1, 1, "Input gamepad sample");

    // Load graphics
    LoadGraphics();

    // Initialize gamepad on port 0
    Digital port0(0);

    // Main program loop
    while (1)
    {
        // Draw background
        SRL::Scene2D::DrawSprite(BackgroundPad, Vector3D(0, 0, 500));
        SRL::Scene2D::DrawSprite(BackgroundWire, Vector3D(0, -100, 500), Vector2D(1, 40));

        // Check whether gamepad is connected to port 0
        if (port0.IsConnected())
        {
            // Check D-Pad
            if (port0.IsHeld(Digital::Button::Up))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-65, -17, 500), Angle::FromDegrees(90));
            }

            if (port0.IsHeld(Digital::Button::Down))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-65, 17, 500), Angle::FromDegrees(-90));
            }

            if (port0.IsHeld(Digital::Button::Left))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-85, 0, 500));
            }

            if (port0.IsHeld(Digital::Button::Right))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-50, 0, 500), Angle::FromDegrees(180));
            }

            // Check start button
            if (port0.IsHeld(Digital::Button::START))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(0, 20, 500));
            }

            // Check triggers
            if (port0.IsHeld(Digital::Button::L))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-90, -70, 500), Angle::FromDegrees(-135));
            }

            if (port0.IsHeld(Digital::Button::R))
            {
                SRL::Scene2D::DrawSprite(InputArrow, Vector3D(90, -70, 500), Angle::FromDegrees(-45));
            }

            // Check other buttons
            if (port0.IsHeld(Digital::Button::A))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(45, 22, 500));
            }

            if (port0.IsHeld(Digital::Button::B))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(69, 5, 500));
            }

            if (port0.IsHeld(Digital::Button::C))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(98, -5, 500));
            }

            if (port0.IsHeld(Digital::Button::X))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(37, -7, 500));
            }

            if (port0.IsHeld(Digital::Button::Y))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(57, -20, 500));
            }

            if (port0.IsHeld(Digital::Button::Z))
            {
                SRL::Scene2D::DrawSprite(InputButton, Vector3D(80, -30, 500));
            }
        }

        // Refresh screen
        SRL::Core::Synchronize();
    }

    return 0;
}

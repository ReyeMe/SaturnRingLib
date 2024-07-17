#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Input;

// Load color palettes here
int16_t LoadPalette(SRL::Bitmap::BitmapInfo* bitmap)
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

        return id;
    }

    // No free bank found
    return -1;
}

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 Sprite clipping");

    // Print white color is in the first bank
    SRL::CRAM::SetBankUsedState(0, SRL::CRAM::TextureColorMode::Paletted16, true);
    
    // Load texture
    SRL::Bitmap::TGA* coverOn = new SRL::Bitmap::TGA("COVERON.TGA");                     // Loads TGA file into main RAM
    int32_t coverOnTextureIndex = SRL::VDP1::TryLoadTexture(coverOn, LoadPalette);       // Loads TGA into VDP1
    delete coverOn;                                                                      // Frees main RAM

    SRL::Bitmap::TGA* coverOff = new SRL::Bitmap::TGA("COVEROFF.TGA");                   // Loads TGA file into main RAM
    int32_t coverOffTextureIndex = SRL::VDP1::TryLoadTexture(coverOff, LoadPalette);     // Loads TGA into VDP1
    delete coverOff;                                                                     // Frees main RAM

    // Get screen size
    uint16_t halfWidth = SRL::TV::Width >> 1;
    Fxp minimumWidth = -Fxp::FromInt(halfWidth);
    Fxp maximumWidth = Fxp::FromInt(halfWidth);

    uint16_t halfHeight = SRL::TV::Height >> 1;
    Fxp minimumHeight = -Fxp::FromInt(halfHeight);
    Fxp maximumHeight = Fxp::FromInt(halfHeight);
    Vector2D screenMiddlePoint = Vector2D(maximumWidth, maximumHeight);

    // Set size and position of the clipping polygon
    Vector2D clippingSize = Vector2D(60.0, 40.0);
    Vector3D clippingPosition = Vector3D(0.0, 0.0, 500.0);

    // Initialize input
    Gamepad input(0);

    // Initialize movement speed
    Fxp speed = 0.5;

    // Main program loop
	while(1)
	{
        // Move box around
        if (input.IsConnected())
        {
            if (input.IsHeld(Gamepad::Button::Left) && clippingPosition.X > minimumWidth)
            {
                clippingPosition.X -= speed;
            }
            else if (input.IsHeld(Gamepad::Button::Right) && (clippingPosition.X + clippingSize.X) < maximumWidth)
            {
                clippingPosition.X += speed;
            }
            
            if (input.IsHeld(Gamepad::Button::Up) && clippingPosition.Y > minimumHeight)
            {
                clippingPosition.Y -= speed;
            }
            else if (input.IsHeld(Gamepad::Button::Down) && (clippingPosition.Y + clippingSize.Y) < maximumHeight)
            {
                clippingPosition.Y += speed;
            }
        }

        // Set clipping rectangle
        SRL::Scene2D::SetClippingRectangle(clippingPosition + screenMiddlePoint, clippingSize);

        // This sprite is visible inside of the clipping sprite
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping, SRL::Scene2D::ClippingEffect::ClipOutside);
        SRL::Scene2D::DrawSprite(coverOffTextureIndex, Vector3D(0.0, 0.0, 500.0));

        // This sprite is visible outside of the clipping sprite
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping, SRL::Scene2D::ClippingEffect::ClipInside);
        SRL::Scene2D::DrawSprite(coverOnTextureIndex, Vector3D(0.0, 0.0, 500.0));

        // Disable clipping effect
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping, SRL::Scene2D::ClippingEffect::NoClipping);

        // Draw outline around clipping polygon
        Vector2D polygon[] = {
            Vector2D(clippingPosition),
            Vector2D(0.0, clippingSize.Y) + clippingPosition,
            clippingSize + clippingPosition,
            Vector2D(clippingSize.X, 0.0) + clippingPosition,
        };

        SRL::Scene2D::DrawPolygon(polygon, false, HighColor::Colors::Magenta, 500.0);

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

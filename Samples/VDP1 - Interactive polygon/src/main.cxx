#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Input;

// Cursors
#define CursorHand (0)
#define CursorFist (1)

// Load cursor color
void LoadCursor()
{
    // Set red color to be transparent
    SRL::Bitmap::TGA::LoaderSettings settings = SRL::Bitmap::TGA::LoaderSettings();
    settings.TransparentColor = HighColor::Colors::Red;

    // Load sprites
    SRL::Bitmap::TGA* hand = new SRL::Bitmap::TGA("HAND.TGA", settings);
    SRL::VDP1::TryLoadTexture(hand);
    delete hand;

    SRL::Bitmap::TGA* fist = new SRL::Bitmap::TGA("FIST.TGA", settings);
    SRL::VDP1::TryLoadTexture(fist);
    delete fist;
}

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 Interactive polygon");

    SRL::VDP2::SetPrintPaletteColor(1, HighColor(120,120,120));
    SRL::Debug::PrintColorSet(1);
    SRL::Debug::PrintWithWrap(1, 3, 1, 40, "D-Pad to move hand around\nA button to grab corner\nB button to switch between filled and\n  outline");
    
    // Get gamepad on port 0
    Gamepad gamepad(0);

    // Load cursor sprites
    LoadCursor();

    // Initialize gouraud entry to get a bit of shading going
    HighColor* table = SRL::VDP1::GetGouraudTable();

    // First gouraud table entry
    table[0] = HighColor(255, 255, 255);
    table[1] = HighColor(160, 160, 160);
    table[2] = HighColor(80, 80, 80);
    table[3] = HighColor(200, 200, 200);

    // Get screen size
    uint16_t halfWidth = SRL::TV::Width >> 1;
    Fxp minimumWidth = -Fxp::FromInt(halfWidth);
    Fxp maximumWidth = Fxp::FromInt(halfWidth);

    uint16_t halfHeight = SRL::TV::Height >> 1;
    Fxp minimumHeight = -Fxp::FromInt(halfHeight);
    Fxp maximumHeight = Fxp::FromInt(halfHeight);

    // Locations
    Vector3D lastCursorLocation = Vector3D(0.0, 0.0, 500.0);
    Vector3D cursorLocation = Vector3D(0.0, 0.0, 500.0);

    Vector2D polygonPoints[] =
    {
        Vector2D(-50.0, -50.0),
        Vector2D(50.0, -50.0),
        Vector2D(50.0, 50.0),
        Vector2D(-50.0, 50.0),
    };

    // Action states
    bool filledPolygon = false;
    int32_t draggingHandle = -1;

    // Main program loop
	while(1)
	{
        // Check if cursor is over any handle
        int32_t currentHandle = -1;

        for (int32_t handle = 0; handle < 4; handle++)
        {
            if (polygonPoints[handle].DistanceTo(cursorLocation) < 10.0)
            {
                currentHandle = handle;
                break;
            }
        }

        // Save last cursor location
        lastCursorLocation = cursorLocation;


        // Move cursor
        if (gamepad.IsConnected())
        {
            if (gamepad.IsHeld(Gamepad::Button::Left))
            {
                cursorLocation.X -= 0.8;
            }
            else if (gamepad.IsHeld(Gamepad::Button::Right))
            {
                cursorLocation.X += 0.8;
            }

            if (gamepad.IsHeld(Gamepad::Button::Up))
            {
                cursorLocation.Y -= 0.8;
            }
            else if (gamepad.IsHeld(Gamepad::Button::Down))
            {
                cursorLocation.Y += 0.8;
            }

            // Change polygon fill
            if (gamepad.WasPressed(Gamepad::Button::B))
            {
                filledPolygon = !filledPolygon;
            }

            // Are we dragging handles?
            if (gamepad.WasPressed(Gamepad::Button::A))
            {
                draggingHandle = currentHandle;
            }
            else if (gamepad.WasReleased(Gamepad::Button::A))
            {
                draggingHandle = -1;
            }
            
            // Clamp cursor to screen
            cursorLocation.X = SRL::Math::Clamp(minimumWidth, maximumWidth, cursorLocation.X);
            cursorLocation.Y = SRL::Math::Clamp(minimumHeight, maximumHeight, cursorLocation.Y);
        }

        // Show rectangle over a handle
        if (currentHandle >= 0)
        {
            // Move handle
            if (draggingHandle >= 0)
            {
                polygonPoints[draggingHandle] += (Vector2D)(cursorLocation - lastCursorLocation);
            }

            // Draw handle
            Vector2D handle[] = 
            {
                polygonPoints[currentHandle] + Vector2D(-5.0, -5.0),
                polygonPoints[currentHandle] + Vector2D(5.0, -5.0),
                polygonPoints[currentHandle] + Vector2D(5.0, 5.0),
                polygonPoints[currentHandle] + Vector2D(-5.0, 5.0)
            };

            SRL::Scene2D::DrawPolygon(handle, false, HighColor::Colors::White, 500.0);
        }

        // Draw polygon
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 0);
        SRL::Scene2D::DrawPolygon(polygonPoints, filledPolygon, HighColor::Colors::Magenta, 550.0);
        SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, -1);

        // Draw simple outlines
        if (filledPolygon)
        {
            SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, true);
            SRL::Scene2D::DrawPolygon(polygonPoints, false, HighColor::Colors::Blue, 520.0);
            SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, false);
        }

        // Draw cursor
        SRL::Scene2D::DrawSprite(draggingHandle >= 0 ? CursorFist : CursorHand, cursorLocation);

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

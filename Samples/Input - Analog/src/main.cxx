#include <srl.hpp>
#include <srl_log.hpp>


// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Using to shorten names for input
using namespace SRL::Input;

// Using to log debug and info messages
using namespace SRL::Logger;

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
  SRL::Bitmap::TGA* background = new SRL::Bitmap::TGA("ANALOG.TGA");
  SRL::VDP1::TryLoadTexture(background, LoadGamepadPalette);
  delete background;

  SRL::Bitmap::TGA* wire = new SRL::Bitmap::TGA("WIRE.TGA");
  SRL::VDP1::TryLoadTexture(wire, LoadGamepadPalette);
  delete wire;

  SRL::Bitmap::TGA* arrow = new SRL::Bitmap::TGA("ARROW.TGA");
  SRL::VDP1::TryLoadTexture(arrow, LoadGamepadPalette);
  delete arrow;

  SRL::Bitmap::TGA* button = new SRL::Bitmap::TGA("BUTTON.TGA");
  SRL::VDP1::TryLoadTexture(button, LoadGamepadPalette);
  delete button;
}

// Main program entry
int main()
{
   const HighColor redIsDead = HighColor(255, 0, 0);
   const HighColor notBlue = HighColor(0, 255, 0);

   const Fxp initialPadPosition[2] = { -60.0, -45.0 };

   const Fxp initialLShoulderPosition[2] = { -55.0, -80.0 };
   const Fxp initialRShoulderPosition[2] = {  55.0, -80.0 };

   const Fxp padForceVectorMaxLength = 20.0;
   const Fxp shoulderForceVectorMaxLength = 20.0;

  LogInfo("SRL::Core::Initialize");

  // Initialize library
  SRL::Core::Initialize(HighColor::Colors::Black);
  SRL::Debug::Print(1,1, "Input analogpad sample");

  LogInfo("LoadGraphics");
  // Load graphics
  LoadGraphics();

  // Initialize gamepad on port 0
  Analog port0(0);

  LogInfo("Game loop");
  // Main program loop
  while(1)
  {
    // Draw background
    SRL::Scene2D::DrawSprite(BackgroundPad, Vector3D(0.0,0.0,500.0));

    SRL::Scene2D::DrawSprite(BackgroundWire, Vector3D(0.0,-150.0,500.0), Vector2D(1.0, 40.0));

    // Check whether gamepad is connected to port 0
    if (port0.IsConnected())
    {
      // Check D-Pad
      if (port0.IsHeld(Analog::Button::Up))
      {
        SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-45.0, -10.0, 500.0), Angle::FromDegrees(90));
      	LogInfo("Analog::Button::Up PRESSED");
      }

      if (port0.IsHeld(Analog::Button::Down))
      {
        SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-45.0, 25.0, 500.0), Angle::FromDegrees(-90));
        LogDebug("Analog::Button::Down PRESSED");
      }

      if (port0.IsHeld(Analog::Button::Left))
      {
        SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-65.0, 10.0, 500.0));
        LogDebug("Analog::Button::Left PRESSED");
      }

      if (port0.IsHeld(Analog::Button::Right))
      {
        SRL::Scene2D::DrawSprite(InputArrow, Vector3D(-30.0, 10.0, 500.0), Angle::FromDegrees(180));
        LogDebug("Analog::Button::Right PRESSED");
      }

      // Check start button
      if (port0.IsHeld(Analog::Button::START))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(0.0, 30.0, 500.0));
        LogDebug("Analog::Button::START PRESSED");
      }

      // Check triggers
      if (port0.IsHeld(Analog::Button::L))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(-55.0, -80.0, 500.0));//, Angle::FromDegrees(-135));
        LogDebug("Analog::Button::L PRESSED");
      }

      if (port0.IsHeld(Analog::Button::R))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(55.0, -80.0, 500.0));//, Angle::FromDegrees(-45));
        LogDebug("Analog::Button::R PRESSED");
      }

      // Check other buttons
      if (port0.IsHeld(Analog::Button::A))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(30.0, 0.0, 500.0));
        LogDebug("Analog::Button::A PRESSED");
      }

      if (port0.IsHeld(Analog::Button::B))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(50.0, -10.0, 500.0));
        LogDebug("Analog::Button::B PRESSED");
      }

      if (port0.IsHeld(Analog::Button::C))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(75.0, -15.0, 500.0));
        LogDebug("Analog::Button::C PRESSED");
      }

      if (port0.IsHeld(Analog::Button::X))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(25.0, -20.0, 500.0));
        LogDebug("Analog::Button::X PRESSED");
      }

      if (port0.IsHeld(Analog::Button::Y))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(45.0, -30.0, 500.0));
        LogDebug("Analog::Button::Y PRESSED");
      }

      if (port0.IsHeld(Analog::Button::Z))
      {
        SRL::Scene2D::DrawSprite(InputButton, Vector3D(70.0, -35.0, 500.0));
        LogDebug("Analog::Button::Z PRESSED");
      }

      int16_t axes[4] = {
        port0.GetAxis(Analog::Axis::Axis1),
        port0.GetAxis(Analog::Axis::Axis2),
        port0.GetAxis(Analog::Axis::Axis3),
        port0.GetAxis(Analog::Axis::Axis4)
      };

      SRL::Debug::Print(1, 2, "Axis1:%03d Axis2:%03d", axes[0], axes[1]);
      SRL::Debug::Print(1, 3, "Axis3:%03d Axis4:%03d", axes[2], axes[3]);

      // Transform d-pad coordonates to [-127;128] range
      for (auto i = 0; i < 2; i++)
      {
        axes[i] -= 128;
      }

      SRL::Scene2D::DrawLine(
          Vector2D(initialPadPosition[0], initialPadPosition[1]),
          Vector2D( initialPadPosition[0] + (padForceVectorMaxLength * Fxp::FromInt(axes[0]) / 128.0),
                    initialPadPosition[1] + (padForceVectorMaxLength * Fxp::FromInt(axes[1]) / 128.0)),
          redIsDead,
          500.0);

      SRL::Scene2D::DrawLine(
          Vector2D(initialRShoulderPosition[0], initialRShoulderPosition[1]),
          Vector2D(initialRShoulderPosition[0],
                    (initialRShoulderPosition[1] - shoulderForceVectorMaxLength * Fxp::FromInt(axes[2]) / 255.0 )),
          notBlue,
          500.0);

      SRL::Scene2D::DrawLine(
          Vector2D(initialLShoulderPosition[0], initialLShoulderPosition[1]),
          Vector2D(initialLShoulderPosition[0],
                  ( initialLShoulderPosition[1] - shoulderForceVectorMaxLength * Fxp::FromInt(axes[3]) / 255.0 ) ),
          notBlue,
          500.0);
    }

    // Refresh screen
    SRL::Core::Synchronize();
  }

  return 0;
}

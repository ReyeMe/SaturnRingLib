#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;

// Simple line structure
struct LineSegment
{
    /** @brief Start of the line segment
     */
    Vector2D Start;

    /** @brief End of the line segment
     */
    Vector2D End;

    /** @brief Start point velocity vector
     */
    Vector2D StartVelocity;

    /** @brief End point velocity vector
     */
    Vector2D EndVelocity;

    /** @brief Trail of the start point
     */
    Vector2D StartTrail[32];

    /** @brief Trail of the end point
     */
    Vector2D EndTrail[32];
};

// Main program entry
int main()
{
    // Initialize library
	SRL::Core::Initialize(HighColor::Colors::Black);
    SRL::Debug::Print(1,1, "VDP1 lines sample");
    
    // Get screen size
    uint16_t halfWidth = SRL::TV::Width >> 1;
    Fxp minimumWidth = -Fxp::FromInt(halfWidth);
    Fxp maximumWidth = Fxp::FromInt(halfWidth);

    uint16_t halfHeight = SRL::TV::Height >> 1;
    Fxp minimumHeight = -Fxp::FromInt(halfHeight);
    Fxp maximumHeight = Fxp::FromInt(halfHeight);

    // Initialize random number function
    SRL::Math::Random rnd = SRL::Math::Random();

    // Velocities to use
    Vector2D velocities[] =
    {
        Vector2D(-1.0, 1.0),
        Vector2D(1.0, 1.0),
        Vector2D(-1.0, -1.0),
        Vector2D(1.0, -1.0)
    };

    // Define 2 unique line points
    LineSegment uniqueSegments[] =
    {
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
            { Vector2D() },
            { Vector2D() }
        },
        {
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(Fxp::FromInt(rnd.GetNumber(-halfWidth, halfWidth)), Fxp::FromInt(rnd.GetNumber(-halfHeight, halfHeight))),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
            Vector2D(velocities[rnd.GetNumber(0,3)]),
            { Vector2D() },
            { Vector2D() }
        }
    };

    SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, true);

    // Main program loop
	while(1)
	{
        // Draw lines
        for (int32_t line = 0; line < 2; line++)
        {
            for (int32_t trail = 31; trail >= 0; trail--)
            {
                // Keep track of where have we been
                uniqueSegments[line].StartTrail[trail] = trail - 1 < 0 ? Vector2D(uniqueSegments[line].Start) : uniqueSegments[line].StartTrail[trail - 1];
                uniqueSegments[line].EndTrail[trail] = trail - 1 < 0 ? Vector2D(uniqueSegments[line].End) : uniqueSegments[line].EndTrail[trail - 1];
                
                // Draw every 4th line
                if (trail % 4 == 0)
                {
                    // Make nice color gradient
                    uint8_t channel = 255 - (trail << 3);
                    HighColor color = HighColor(channel, channel, channel);

                    // Draw main line
                    SRL::Scene2D::DrawLine(
                        uniqueSegments[line].StartTrail[trail],
                        uniqueSegments[line].EndTrail[trail],
                        color,
                        500.0);
                    
                    // Draw connecting line
                    SRL::Scene2D::DrawLine(
                        uniqueSegments[line].StartTrail[trail],
                        uniqueSegments[(line + 1) % 2].EndTrail[trail],
                        color,
                        500.0);
                }
            }

            // Move line
            uniqueSegments[line].Start += uniqueSegments[line].StartVelocity;
            uniqueSegments[line].End += uniqueSegments[line].EndVelocity;

            // Limit line velocity
            if (uniqueSegments[line].Start.X < minimumWidth || uniqueSegments[line].Start.X > maximumWidth)
            {
                uniqueSegments[line].StartVelocity.X *= -1.0;
            }

            if (uniqueSegments[line].Start.Y < minimumHeight || uniqueSegments[line].Start.Y > maximumHeight)
            {
                uniqueSegments[line].StartVelocity.Y *= -1.0;
            }
            
            if (uniqueSegments[line].End.X < minimumWidth || uniqueSegments[line].End.X > maximumWidth)
            {
                uniqueSegments[line].EndVelocity.X *= -1.0;
            }

            if (uniqueSegments[line].End.Y < minimumHeight || uniqueSegments[line].End.Y > maximumHeight)
            {
                uniqueSegments[line].EndVelocity.Y *= -1.0;
            }
        }

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}

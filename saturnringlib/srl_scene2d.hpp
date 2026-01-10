#pragma once

#include "srl_base.hpp"
#include "srl_vdp1.hpp"

namespace SRL
{
    /** @brief Rendering of VDP1 sprites and shapes
     * @note Z/depth does not cause perspective calculation, it is used only for sorting
     */
    class Scene2D
    {
    public:
    
        /** @brief Clipping effect mode
         */
        enum ClippingEffect : uint8_t
        {
            /** @brief Disable sprite clipping
             */
            NoClipping = 0,

            /** @brief Display sprite only on the inside of the clipping rectangle
             */
            ClipOutside = 2,

            /** @brief Display sprite only on the outside of the clipping rectangle
             */
            ClipInside = 3,
        };

        /** @brief Sprite flip effect
         */
        enum FlipEffect : uint8_t
        {
            /** @brief Do not flip sprite
             */
            NoFlip = 0,

            /** @brief Flip sprite horizontally
             */
            HorizontalFlip = 1,

            /** @brief Flip sprite vertically
             */
            VerticalFlip = 2,
        };

        /** @brief List of all available sprite effects
         */
        enum class SpriteEffect : uint8_t
        {

            /** @brief Gouraud shading
             * @details Enables/disables gouraud shading for sprites.<br>Expected parameter is zero based index (**uint16_t**) to a gouraud shading table to enable shading, or negative or no value to disable it.
             * @code {.cpp}
             * // Disable shading
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, -1);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud);
             *
             * // Enable shading, use 16th entry in the table
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Gouraud, 15);
             * @endcode
             *
             */
            Gouraud = 0,

            /** @brief Screen doors effect
             * @details Enables/disables screen door effect.<br>Expects boolean value as parameter.
             * @code {.cpp}
             * // Disable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, false);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors);
             *
             * // Enable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::ScreenDoors, true);
             * @endcode
             */
            ScreenDoors = 1,

            /** @brief Half transparency effect
             * @details Enables/disables half transparency effect.<br>Expects boolean value as parameter.
             * @code {.cpp}
             * // Disable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, false);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency);
             *
             * // Enable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::HalfTransparency, true);
             * @endcode
             */
            HalfTransparency = 2,

            /** @brief Sprite clipping effect
             * @details Enables/disables clipping effect.<br>Expects SRL::Scene2D::ClippingEffect enum.<br>Use SRL::Scene2D::SetClippingRectangle() function to set the clipping rectangle.
             * @code {.cpp}
             * // Disable flip
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping, SRL::Scene2D::ClippingEffect::NoClipping);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping);
             *
             * // Enable flip
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Clipping, SRL::Scene2D::ClippingEffect::ClipInside);
             * @endcode
             */
            Clipping = 3,

            /** @brief Flip sprite effect
             * @details Allows to set sprite texture read direction (making sprite flipped).
             * @code {.cpp}
             * // Disable flipping
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Flip, SRL::Scene2D::FlipEffect::NoFlip);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Flip);
             *
             * // Enable clipping
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Flip, SRL::Scene2D::FlipEffect::HorizontalFlip);
             *
             * // Enable flip in both directions
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::Flip, SRL::Scene2D::FlipEffect::HorizontalFlip | SRL::Scene2D::FlipEffect::VerticalFlip);
             * @endcode
             */
            Flip = 4,

             /** @brief VDP2 color calculation effect
             * @details Set sprites Color Calculation Ratio to one of 8 stored opacities (Banks 0-7).
             * @code {.cpp}
             * //Set sprite to use ratio stored in opacity bank 1:
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::OpacityBank, 1);
             * @endcode
             * @note Only applies to palette color modes
             */
            OpacityBank = 5,

            /** @brief Enables textured sprite high speed shrink
             * @details When EnableHSS is specified, lines drawn with magnification of less than 1 are drawn by sampling only even or odd pixels of the original data.
             * @code {.cpp}
             * // Disable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableHSS, false);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableHSS);
             *
             * // Enable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableHSS, true);
             * @endcode
             * @note Applies only to textured polygons
             */
            EnableHSS = 6,
            
            /** @brief Enables end code character for textured sprites
             * @details Drawing in horizontal direction is terminated, when an end code is read twice.
             *  Color mode | EndCode
             * ------------|---------
             * 16 colors (CRAM)| 0xf
             * 16 colors (CLUT)| 0xf
             * 64 colors (CRAM)| 0xff
             * 128 colors (CRAM) | 0xff
             * 256 colors (CRAM) | 0xff
             * 16bit RGB | 0x7fff
             * Behaviour in combination with EnableHSS
             * HSS | ECD | Behaviour
             * ----|-----|--------------------
             * OFF | ON  | End code enabled, drawing in horizontal direction is disabled when second ECD is read and ECD becomes transparent
             * OFF | OFF | End code disabled, ECD is not processed, color of the code is drawn
             * ON & enlarge | ON | End code enabled, drawing in horizontal direction is disabled when second ECD is read and ECD becomes transparent
             * ON & reduce | ON | End code disabled, ECD is not processed, color of the code is drawn
             * ON | OFF | End code disabled, ECD is not processed, color of the code is drawn
             * @code {.cpp}
             * // Disable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableECD, false);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableECD);
             *
             * // Enable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::EnableECD, true);
             * @endcode
             * @note Applies only to textured polygons
             */
            EnableECD = 7,

            /** @brief Disables pre-clipping of textured sprite
             * @details Drawing commands comprise of group of several lines, and perspective lines compris of a number of dots.
             * Each dot is drawn based on drawing area specified by CPU. For lines completely outside or not fully inside drawing area,
             * enabled pre-clipping can improve performance. If lines are fully within drawing area, disabling pre-clipping can improve performance.
             * @image html preclip.png
             * @code {.cpp}
             * // Disable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::DisablePreClip, false);
             * // or
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::DisablePreClip);
             *
             * // Enable effect
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::DisablePreClip, true);
             * @endcode
             * @note By default, PreClip is enabled
             * @note Applies only to textured polygons
             */
            DisablePreClip = 8,
        };

        /** @brief Scaled sprite zoom point
         */
        enum ZoomPoint : uint8_t
        {
            /** @brief Upper left corner
             */
            UpperLeft = 0x5,

            /** @brief Center left side
             */
            CenterLeft = 0x9,

            /** @brief Bottom left corner
             */
            BottomLeft = 0xd,

            /** @brief Center top side
             */
            TopCenter = 0x6,

            /** @brief Sprite center
             */
            Center = 0xa,

            /** @brief Center bottom side
             */
            BottomCenter = 0xe,

            /** @brief Upper right corner
             */
            UpperRight = 0x7,

            /** @brief Center right side
             */
            CenterRight = 0xb,

            /** @brief Bottom right corner
             */
            BottomRight = 0xf
        };

        /** @brief Command control type
         */
        enum class CommandType : uint16_t
        {
            /** @brief Standard sprite command
             */
            StandardSprite = 0x0,
            
            /** @brief Rectangle sprite command
             */
            RectangleSprite = 0x1,
            
            /** @brief Textured sprite sprite command
             */
            Texture = 0x2,

            /** @brief Filled polygon command
             */
            Polygon = 0x4,
            
            /** @brief Polyline command
             */
            PolyLine = 0x5,

            /** @brief Simple line segment command
             */
            LineSegment = 0x6,
            
            /** @brief System clip change command
             */
            SystemClip = 0x9,
            
            /** @brief User clip change command
             */
            UserClip = 0x8,
            
            /** @brief Change relative coordinates for VDP1 command table
             */
            BasePosition = 0xA,

            /** @brief Draw end command
             */
            End = 0x80,
        };

    private:

        /** @brief Base address of the gouraud table
         */
        static const uint16_t GouraudTableBase = 0xe000;

        /** @brief Struct to store effect settings
         */
        struct EffectStore {

            /** @brief Gouraud table address
             */
            uint16_t Gouraud;

            /** @brief Screen doors effect state
             */
            uint16_t ScreenDoors:1;

            /** @brief Half-transparency effect state
             */
            uint16_t HalfTransparency:1;

            /** @brief Clipping mode
             */
            uint16_t Clipping:2;

            /** @brief Sprite flipping
             */
            uint16_t Flip:2;

            /** @brief Sprite opacity with VDP2 layers
            */
            uint16_t OpacityBank:3;

            /** @brief Sprite high speed shrink option
             */
            uint16_t HighSpeedShrink:1;

            /** @brief Enable or disable textured sprite end code
             */
            uint16_t EndCode:1;

            /** @brief Disable sprite pre clipping
             */
            uint16_t DisablePreClipping:1;

            /** @brief Reserved for future use
             */
            uint16_t Reserved:4;
        };

        /** @brief Stored effect state
         */
        static inline Scene2D::EffectStore Effects = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

        /** @brief Is gouraud shading enabled?
         * @return true if gouraud shading is enabled
         */
        static constexpr inline bool IsGouraudEnabled()
        {
            return Scene2D::Effects.Gouraud >= SRL::Scene2D::GouraudTableBase;
        }

    public:

        /**
         * @name Sprite/Shape command creation
         * @{
         */

        /** @brief Generates sprite command based on current effect flags
         * @param type Sprite type see SRL::Scene2D::CommandType
         * @param color Sprite color
         * @return Sprite command
         */
        static constexpr inline SPRITE GetSpriteCommand(Scene2D::CommandType type, Types::HighColor color)
        {
            uint16_t gouraudEnabled = Scene2D::IsGouraudEnabled();

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            return {
                // Control
                (int8_t)type | (gouraudEnabled << 7),

                // Link address
                0,

                // Put mode
                (uint16_t)(0x0080 |
                    ((CL32KRGB & 7) << 3) |
                    (gouraudEnabled << 2) |
                    (Scene2D::Effects.ScreenDoors << 8) |
                    (Scene2D::Effects.Clipping << 9) |
                    (Scene2D::Effects.HalfTransparency ? 0x3 : 0 )),

                // Sprite Color
                color,

                // Texture source
                0,

                // Texture size
                0,

                // X,Y coordinates as 16bit integer, repeated 4 times
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,

                // Gouraud
                Scene2D::Effects.Gouraud,

                // Dummy variable
                0
            };
            #pragma GCC diagnostic pop
        }

        /** @brief Generates sprite attributes struct based on current effect flags
         * @param texture Texture identifier
         * @param texturePalette Palette override
         * @param zoomPoint Sprite zoom (origin) point
         * @return Sprite attributes
         */
        static constexpr inline SPR_ATTR GetSpriteAttribute(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const Scene2D::ZoomPoint zoomPoint = Scene2D::ZoomPoint::Center)
        {
            uint8_t colorMode = CL32KRGB;
            uint16_t palette = No_Palet;
            CRAM::TextureColorMode colorModeType = SRL::VDP1::Metadata[texture].ColorMode;
            uint16_t paletteId = SRL::VDP1::Metadata[texture].PaletteId;

            if (texturePalette != nullptr)
            {
                colorModeType = texturePalette->GetMode();
                paletteId = texturePalette->GetId();
            }

            switch (colorModeType)
            {
            case SRL::CRAM::TextureColorMode::Paletted256:
                colorMode = CL256Bnk;
                palette = paletteId << 8| (Scene2D::Effects.OpacityBank << 11);
                break;

            case SRL::CRAM::TextureColorMode::Paletted128:
                colorMode = CL128Bnk;
                palette = paletteId << 7| (Scene2D::Effects.OpacityBank << 11);
                break;

            case SRL::CRAM::TextureColorMode::Paletted64:
                colorMode = CL64Bnk;
                palette = paletteId << 6| (Scene2D::Effects.OpacityBank << 11);
                break;

            case SRL::CRAM::TextureColorMode::Paletted16:
                colorMode = CL16Bnk;
                palette = paletteId << 4| (Scene2D::Effects.OpacityBank << 11);
                break;

            default:
                break;
            }

            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            return SPR_ATTRIBUTE(
                texture,
                palette,
                (Scene2D::IsGouraudEnabled() ? Scene2D::Effects.Gouraud : 0),

                (Scene2D::Effects.Clipping << 9) |
                (Scene2D::Effects.ScreenDoors << 8) |
                colorMode |
                (Scene2D::Effects.EndCode ? ECenb : ECdis) |
                (Scene2D::Effects.HighSpeedShrink ? HSSon : HSSoff) |
                (Scene2D::Effects.DisablePreClipping ? Pclpoff : Pclpon) |
                (Scene2D::IsGouraudEnabled() ? CL_Gouraud : 0) |
                (Scene2D::Effects.HalfTransparency ? 0x3 : 0 ),

                (Scene2D::Effects.Flip << 4) |
                FUNC_Texture |
                (zoomPoint << 8));
            #pragma GCC diagnostic pop
        }

        /** @} */

        /**
         * @name Draw functions
         * @{
         */

        /** @brief Draw sprite by using a custom command
         * @param command Custom command
         * @param depth Depth sort value
         * @return True on success
         */
        static bool Draw(SPRITE* command, const SRL::Math::Types::Fxp& depth)
        {
            return slSetSprite(command, depth.RawValue());
        }

        /** @brief Draw sprite by using custom attributes
         * @param attributes Custom sprite attributes
         * @param arguments Sprite attribute arguments
         * @return True on success
         */
        static bool Draw(SPR_ATTR* attributes, const SRL::Math::Types::Fxp* arguments)
        {
            // We cannot use slDispSprite, as that seems to be bugged and is drawing images 1px wider than it should
            return slDispSpriteHV((FIXED*)arguments, attributes, 0) != 0;
        }

        /** @brief Draw sprite by using custom attributes and 4 points
         * @param attributes Custom sprite attributes
         * @param points Corners of the sprite in screen coordinates
         * @param depth Depth sort value
         * @return True on success
         */
        static bool Draw(SPR_ATTR* attributes, const SRL::Math::Types::Vector2D points[4], const SRL::Math::Types::Fxp depth)
        {
            // We cannot use slDispSprite, as that seems to be bugged and is drawing images 1px wider than it should
            return slDispSprite4P((FIXED*)points, depth.RawValue(), attributes) != 0;
        }

        /** @brief Draw sprite from 4 points
         * @param texture Sprite texture
         * @param texturePalette Sprite texture color palette override
         * @param points Corners of the sprite in screen coordinates
         * @param depth Depth sort value
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const SRL::Math::Types::Vector2D points[4],
            const SRL::Math::Types::Fxp depth)
        {
            // Sprite attributes and command points
            SPR_ATTR attr = Scene2D::GetSpriteAttribute(texture, texturePalette);
            return slDispSprite4P((FIXED*)points, depth.RawValue(), &attr);
        }

        /** @brief Draw sprite from 4 points
         * @param texture Sprite texture
         * @param points Corners of the sprite in screen coordinates
         * @param depth Depth sort value
         * @return True on success
         */
        static bool DrawSprite(const uint16_t texture, const SRL::Math::Types::Vector2D points[4], const SRL::Math::Types::Fxp depth)
        {
            return Scene2D::DrawSprite(texture, nullptr, points, depth);
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param texturePalette Sprite texture color palette override
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param angle Sprite rotation angle
         * @param scale Scale of the sprite
         * @param zoomPoint Zoom point of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const SRL::Math::Types::Vector3D& location,
            const SRL::Math::Types::Angle& angle = SRL::Math::Types::Angle(),
            const SRL::Math::Types::Vector2D& scale = SRL::Math::Types::Vector2D(1.0, 1.0),
            const Scene2D::ZoomPoint zoomPoint = Scene2D::ZoomPoint::Center)
        {
            if (angle.RawValue() != 0)
            {
                // Due to bug in SGL we can't use slDispSpriteHV or slDispSpriteSZ with angles
                const SRL::Math::Types::Fxp sin = Math::Trigonometry::Sin(angle);
                const SRL::Math::Types::Fxp cos = Math::Trigonometry::Cos(angle);

                const SRL::Math::Types::Vector2D size = SRL::Math::Types::Vector2D(
                    SRL::Math::Types::Fxp((int16_t)VDP1::Textures[texture].Width) * scale.X,
                    SRL::Math::Types::Fxp((int16_t)VDP1::Textures[texture].Height) * scale.Y);

                const SRL::Math::Types::Vector2D halfSize = SRL::Math::Types::Vector2D(
                    (SRL::Math::Types::Fxp((int16_t)VDP1::Textures[texture].Width) * scale.X) >> 1,
                    (SRL::Math::Types::Fxp((int16_t)VDP1::Textures[texture].Height) * scale.Y) >> 1);

                // Prepare rotation matrix
                SRL::Math::Matrix43 rotMat = SRL::Math::Matrix43::Identity();
                rotMat.RotateZ(angle);

                // Set zoom
                SRL::Math::Types::Vector3D points[4]
                {
                    SRL::Math::Types::Vector3D(-halfSize.X, -halfSize.Y, 0.0),
                    SRL::Math::Types::Vector3D(halfSize.X, -halfSize.Y, 0.0),
                    SRL::Math::Types::Vector3D(halfSize.X, halfSize.Y, 0.0),
                    SRL::Math::Types::Vector3D(-halfSize.X, halfSize.Y, 0.0)
                };

                SRL::Math::Types::Vector3D offsetLocation = SRL::Math::Types::Vector3D();

                switch (zoomPoint)
                {
                    // Corners
                    // Corners
                    case Scene2D::ZoomPoint::UpperLeft:
                        offsetLocation.X -= halfSize.X;
                        offsetLocation.Y -= halfSize.Y;
                        break;
                
                    case Scene2D::ZoomPoint::UpperRight:
                        offsetLocation.X += halfSize.X;
                        offsetLocation.Y -= halfSize.Y;
                        break;

                    case Scene2D::ZoomPoint::BottomRight:
                        offsetLocation.X += halfSize.X;
                        offsetLocation.Y += halfSize.Y;
                        break;

                    case Scene2D::ZoomPoint::BottomLeft:
                        offsetLocation.X -= halfSize.X;
                        offsetLocation.Y += halfSize.Y;
                        break;

                    // Center sides
                    case Scene2D::ZoomPoint::CenterLeft:
                        offsetLocation.X -= halfSize.X;
                        break;
                
                    case Scene2D::ZoomPoint::TopCenter:
                        offsetLocation.Y -= halfSize.Y;
                        break;

                    case Scene2D::ZoomPoint::CenterRight:
                        offsetLocation.X += halfSize.X;
                        break;

                    case Scene2D::ZoomPoint::BottomCenter:
                        offsetLocation.Y += halfSize.Y;
                        break;

                    // Sprite center
                    default:
                        break;
                }

                const SRL::Math::Types::Vector2D realPoints[4] = {
                    SRL::Math::Types::Vector2D(location + (rotMat.TransformPoint(points[0] + offsetLocation))),
                    SRL::Math::Types::Vector2D(location + (rotMat.TransformPoint(points[1] + offsetLocation))),
                    SRL::Math::Types::Vector2D(location + (rotMat.TransformPoint(points[2] + offsetLocation))),
                    SRL::Math::Types::Vector2D(location + (rotMat.TransformPoint(points[3] + offsetLocation)))
                };

                // Calculate new 4 corners
                return Scene2D::DrawSprite(texture, texturePalette, realPoints, location.Z);
            }
            else
            {
                // Sprite attributes and command points
                SPR_ATTR attr = Scene2D::GetSpriteAttribute(texture, texturePalette, zoomPoint);

                FIXED sgl_pos[XYZSS];
                sgl_pos[X] = location.X.RawValue();
                sgl_pos[Y] = location.Y.RawValue();
                sgl_pos[Z] = location.Z.RawValue();
                sgl_pos[Sh] = scale.X.RawValue();
                sgl_pos[Sv] = scale.Y.RawValue();

                // We cannot use slDispSprite, as that seems to be bugged and is drawing images 1px wider than it should
                return slDispSpriteHV(sgl_pos, &attr, 0) != 0;
            }
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param angle Sprite rotation angle
         * @param scale Scale of the sprite
         * @param zoomPoint Zoom point of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            const SRL::Math::Types::Vector3D& location,
            const SRL::Math::Types::Angle& angle = SRL::Math::Types::Angle::Zero(),
            const SRL::Math::Types::Vector2D& scale = SRL::Math::Types::Vector2D(1.0, 1.0),
            const Scene2D::ZoomPoint zoomPoint = Scene2D::ZoomPoint::Center)
        {
            return Scene2D::DrawSprite(texture, nullptr, location, angle, scale, zoomPoint);
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param scale Scale of the sprite
         * @param zoomPoint Zoom point of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            const SRL::Math::Types::Vector3D& location,
            const SRL::Math::Types::Vector2D& scale,
            const Scene2D::ZoomPoint zoomPoint = Scene2D::ZoomPoint::Center)
        {
            return Scene2D::DrawSprite(texture, nullptr, location, SRL::Math::Types::Angle(), scale, zoomPoint);
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param texturePalette Sprite texture color palette override
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param scale Scale of the sprite
         * @param zoomPoint Zoom point of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const SRL::Math::Types::Vector3D& location,
            const SRL::Math::Types::Vector2D& scale,
            const Scene2D::ZoomPoint zoomPoint = Scene2D::ZoomPoint::Center)
        {
            return Scene2D::DrawSprite(texture, texturePalette, location, SRL::Math::Types::Angle(), scale, zoomPoint);
        }

        /** @brief Draws a Line
        * @param start start point
        * @param end end point
        * @param color color of the line
        * @param sort Z order
        */
        static bool DrawLine(const SRL::Math::Types::Vector2D& start,const SRL::Math::Types::Vector2D& end, const Types::HighColor& color, const SRL::Math::Types::Fxp sort)
        {
            SPRITE line = Scene2D::GetSpriteCommand(Scene2D::CommandType::LineSegment, color);
            line.XA = start.X.As<int16_t>();
            line.YA = start.Y.As<int16_t>();
            line.XB = end.X.As<int16_t>();
            line.YB = end.Y.As<int16_t>();
            return slSetSprite(&line, sort.RawValue()) != 0;
        }

        /** @brief Draws a generic polygon
         * @param points Points of the polygon
         * @param fill Indicates whether polygon is filled or if it is just a poly-line
         * @param color Polygon color
         * @param sort Z order
         */
        static bool DrawPolygon(SRL::Math::Types::Vector2D points[4], const bool fill, const Types::HighColor& color, const SRL::Math::Types::Fxp sort)
        {
            SPRITE polygon = Scene2D::GetSpriteCommand(fill ? Scene2D::CommandType::Polygon : Scene2D::CommandType::PolyLine, color);
            polygon.XA = points[0].X.As<int16_t>();
            polygon.YA = points[0].Y.As<int16_t>();
            polygon.XB = points[1].X.As<int16_t>();
            polygon.YB = points[1].Y.As<int16_t>();
            polygon.XC = points[2].X.As<int16_t>();
            polygon.YC = points[2].Y.As<int16_t>();
            polygon.XD = points[3].X.As<int16_t>();
            polygon.YD = points[3].Y.As<int16_t>();
            return slSetSprite(&polygon, sort.RawValue());
        }

        /** @} */

        /**
         * @name Sprite effect functions
         * @{
         */

        /** @brief Set the Clipping rectangle
         * @param location Rectangle top left corner location in screen coordinates, where top left corner of the screen is (0,0)
         * @param size Rectangle size
         * @return true on success
         */
        static inline bool SetClippingRectangle(const SRL::Math::Types::Vector3D& location, const SRL::Math::Types::Vector2D& size)
        {
            SPRITE sprite;
            sprite.CTRL = FUNC_UserClip;
            sprite.XA = location.X.As<int16_t>();
            sprite.YA = location.Y.As<int16_t>();
            sprite.XC = (location.X + size.X).As<int16_t>();
            sprite.YC = (location.Y + size.Y).As<int16_t>();
            return slSetSprite(&sprite, location.Z.RawValue());
        }

        /** @brief Set sprite effect
         * @details See @ref SRL::Scene2D::SpriteEffect for valid effect data
         * @param effect Effect id
         * @param data Effect parameter
         */
        static inline void SetEffect(const SpriteEffect effect, const int32_t data = -1) {
            switch (effect)
            {
            case SpriteEffect::Gouraud:
                Scene2D::Effects.Gouraud = data >= 0 ? (SRL::Scene2D::GouraudTableBase + ((uint16_t)data & 0xffff)) : 0;
                break;

            case SpriteEffect::ScreenDoors:
                Scene2D::Effects.ScreenDoors = data == 1;
                break;

            case SpriteEffect::HalfTransparency:
                Scene2D::Effects.HalfTransparency = data == 1;
                break;

            case SpriteEffect::Clipping:
                Scene2D::Effects.Clipping = data < 0 ? 0 : data & 0x3;
                break;

            case SpriteEffect::Flip:
                Scene2D::Effects.Flip = data < 0 ? 0 : data & 0x3;
                break;

            case SpriteEffect::OpacityBank:
                Scene2D::Effects.OpacityBank = data < 0 ? 0 : data & 0x7;
                break;

            case SpriteEffect::EnableHSS:
                Scene2D::Effects.HighSpeedShrink = data == 1;
                break;

            case SpriteEffect::EnableECD:
                Scene2D::Effects.EndCode = data == 1;
                break;

            case SpriteEffect::DisablePreClip:
                Scene2D::Effects.DisablePreClipping = data == 1;
                break;

            default:
                break;
            }
        }

        /** @brief Get currently set effect value
         * @details See @ref SRL::Scene2D::SpriteEffect for valid effect data
         * @param effect Effect id
         * @return Currently set effect value, see @ref SRL::Scene2D::SpriteEffect for expected return values
         */
        static inline int32_t GetEffect(const SpriteEffect effect) {
            switch (effect)
            {
            case SpriteEffect::Gouraud:
                return Scene2D::Effects.Gouraud > 0 ? Scene2D::Effects.Gouraud - SRL::Scene2D::GouraudTableBase : 0;

            case SpriteEffect::ScreenDoors:
                return Scene2D::Effects.ScreenDoors;

            case SpriteEffect::HalfTransparency:
                return Scene2D::Effects.HalfTransparency;

            case SpriteEffect::Clipping:
                return Scene2D::Effects.Clipping;

            case SpriteEffect::Flip:
                return Scene2D::Effects.Flip;

            case SpriteEffect::OpacityBank:
                return Scene2D::Effects.OpacityBank;

            case SpriteEffect::EnableHSS:
                return Scene2D::Effects.HighSpeedShrink;

            case SpriteEffect::EnableECD:
                return Scene2D::Effects.EndCode;

            case SpriteEffect::DisablePreClip:
                return Scene2D::Effects.DisablePreClipping;

            default:
                return -1;
            }
        }

        /** @} */

    };
}

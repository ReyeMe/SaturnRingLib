#pragma once

#include "srl_base.hpp"
#include "srl_vdp1.hpp"
#include "srl_geo.hpp"
#include "srl_angle.hpp"

namespace SRL
{
    /** @brief Rendering of VDP1 sprites and shapes
     */
    class Scene2D
    {
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

            /** @brief Reserved for future use
             */
            uint16_t Reserved:7;
        };

        /** @brief Stored effect state
         */
        static inline Scene2D::EffectStore Effects = { 0, 0, 0, 0, 0, 0, 0 };

        /** @brief Is gouraud shading enabled?
         * @return true if gouraud shading is enabled
         */
        static constexpr inline bool IsGouraudEnabled()
        {
            return Scene2D::Effects.Gouraud >= SRL::Scene2D::GouraudTableBase;
        }

        /** @brief Generates base shape command
         * @param type Sprite type
         * @param color Sprite color
         * @return Sprite command
         */
        static constexpr inline SPRITE GetShapeCommand(uint16_t type, Types::HighColor color) 
        {
            SPRITE sprite;
            sprite.COLR = color;
            sprite.CTRL = type | (Scene2D::IsGouraudEnabled() ? UseGouraud : 0);

            sprite.PMOD = 0x0080 |
                ((CL32KRGB & 7) << 3) |
                (Scene2D::IsGouraudEnabled() ? CL_Gouraud : 0) |
                (Scene2D::Effects.ScreenDoors << 8) |
                (Scene2D::Effects.Clipping << 9) |
                (Scene2D::Effects.HalfTransparency ? 0x3 : 0 );

            sprite.GRDA = (Scene2D::IsGouraudEnabled() ? Scene2D::Effects.Gouraud : 0);
            return sprite;
        }

        /** @brief Generates sprite attributes struct
         * @param texture Texture identifier
         * @param texturePalette Palette override
         * @return Sprite attributes
         */
        static constexpr inline SPR_ATTR GetSpriteAttribute(const uint16_t texture, SRL::CRAM::Palette* texturePalette)
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
                ECdis | 
                (Scene2D::IsGouraudEnabled() ? CL_Gouraud : 0) |
                (Scene2D::Effects.HalfTransparency ? 0x3 : 0 ),

                (Scene2D::Effects.Flip << 4) |
                FUNC_Texture |
                _ZmCC);
            #pragma GCC diagnostic pop
        }

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
        enum SpriteEffect : uint8_t
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
             * @details Allows to set sprite texture read direction (making sprite flipped)
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
             * @details Set sprites Color Calculation Ratio to one of 8 stored opacities (Banks 0-7)
             * @note Only applies to palette color modes
             * @code {.cpp}
             * //Set sprite to use ratio stored in opacity bank 1: 
             * SRL::Scene2D::SetEffect(SRL::Scene2D::SpriteEffect::OpacityBank, 1);
             */
            OpacityBank = 5,
        };

        /** @brief Set the Clipping rectangle
         * @param location Rectangle top left corner location in screen coordinates, where top left corner of the screen is (0,0)
         * @param size Rectangle size
         * @return true on success
         */
        static inline bool SetClippingRectangle(const Types::Vector3D& location, const Types::Vector2D& size)
        {
            SPRITE sprite;
            sprite.CTRL = FUNC_UserClip;
            sprite.XA = location.X.ToInt();
            sprite.YA = location.Y.ToInt();
            sprite.XC = (location.X + size.X).ToInt();
            sprite.YC = (location.Y + size.Y).ToInt();
            return slSetSprite(&sprite, location.Z.Value());
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
                
            default:
                break;
            }
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
            const Types::Vector2D location[4],
            const Types::Fxp depth)
        {
            // Sprite attributes and command points
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            SPR_ATTR attr = Scene2D::GetSpriteAttribute(texture, texturePalette);

            return slDispSprite4P((FIXED*)location, depth.Value(), &attr);
        }

        /** @brief Draw sprite from 4 points
         * @param texture Sprite texture
         * @param points Corners of the sprite in screen coordinates
         * @param depth Depth sort value
         * @return True on success
         */
        static bool DrawSprite(const uint16_t texture, const Types::Vector2D location[4], const Types::Fxp depth)
        {
            return Scene2D::DrawSprite(texture, nullptr, location, depth);
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param texturePalette Sprite texture color palette override
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param angle Sprite rotation angle
         * @param scale Scale of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const Types::Vector3D& location,
            const Types::Angle& angle = Types::Angle::FromRaw(0),
            const Types::Vector2D& scale = Types::Vector2D(1.0, 1.0))
        {
            if (scale.X != scale.Y || angle.Value() != 0)
            {
                // Due to bug in SGL we can't use slDispSpriteHV or slDispSpriteSZ
                Types::Fxp sin = Math::Sin(angle);
                Types::Fxp cos = Math::Cos(angle);

                Types::Vector2D size = Types::Vector2D(
                    (Types::Fxp::FromInt(VDP1::Textures[texture].Width) * scale.X) >> 1,
                    (Types::Fxp::FromInt(VDP1::Textures[texture].Height) * scale.Y) >> 1);

                Types::Vector2D points[4] = {
                    Types::Vector2D(((cos * -size.X) - (sin * -size.Y)) + location.X, ((sin * -size.X) + (cos * -size.Y)) + location.Y),
                    Types::Vector2D(((cos * size.X) - (sin * -size.Y)) + location.X, ((sin * size.X) + (cos * -size.Y)) + location.Y),
                    Types::Vector2D(((cos * size.X) - (sin * size.Y)) + location.X, ((sin * size.X) + (cos * size.Y)) + location.Y),
                    Types::Vector2D(((cos * -size.X) - (sin * size.Y)) + location.X, ((sin * -size.X) + (cos * size.Y)) + location.Y)
                };

                // Calculate new 4 corners
                return Scene2D::DrawSprite(texture, texturePalette, points, location.Z);
            }
            else
            {
                // Sprite attributes and command points
                SPR_ATTR attr = Scene2D::GetSpriteAttribute(texture, texturePalette);

                FIXED sgl_pos[5];
                sgl_pos[X] = location.X.Value();
                sgl_pos[Y] = location.Y.Value();
                sgl_pos[Z] = location.Z.Value();
                sgl_pos[Sh] = scale.X.Value();
                sgl_pos[Sv] = scale.Y.Value();
 
                return slDispSprite(sgl_pos, &attr, angle.Value()) != 0;
            }
        }

        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param angle Sprite rotation angle
         * @param scale Scale of the sprite
         * @return True on success
         */
        static bool DrawSprite(const uint16_t texture, const Types::Vector3D& location, const Types::Angle& angle = Types::Angle::FromRaw(0), const Types::Vector2D& scale = Types::Vector2D(1.0, 1.0))
        {
            return Scene2D::DrawSprite(texture, nullptr, location, angle, scale);
        }
        
        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param scale Scale of the sprite
         * @return True on success
         */
        static bool DrawSprite(const uint16_t texture, const Types::Vector3D& location, const Types::Vector2D& scale)
        {
            return Scene2D::DrawSprite(texture, nullptr, location, Types::Angle::FromRaw(0), scale);
        }
        
        /** @brief Draw simple sprite
         * @param texture Sprite texture
         * @param texturePalette Sprite texture color palette override
         * @param location Location of the sprite (Z coordinate is used for sorting)
         * @param scale Scale of the sprite
         * @return True on success
         */
        static bool DrawSprite(
            const uint16_t texture,
            SRL::CRAM::Palette* texturePalette,
            const Types::Vector3D& location,
            const Types::Vector2D& scale)
        {
            return Scene2D::DrawSprite(texture, texturePalette, location, Types::Angle::FromRaw(0), scale);
        }
        
        /** @brief Draws a Line
        * @param start start point
        * @param end end point
        * @param color color of the line
        * @param sort Z order
        */
        static bool DrawLine(const Types::Vector2D& start, const Types::Vector2D& end, const Types::HighColor& color, const Types::Fxp sort)
        {
            SPRITE line = Scene2D::GetShapeCommand(FUNC_Line, color);
            line.XA = start.X.ToInt();
            line.YA = start.Y.ToInt();
            line.XB = end.X.ToInt();
            line.YB = end.Y.ToInt();
            return slSetSprite(&line,sort.Value()) != 0;
        }

        /** @brief Draws a generic polygon
         * @param points Points of the polygon
         * @param fill Indicates whether polygon is filled or if it is just a poly-line
         * @param color Polygon color
         * @param sort Z order
         */
        static bool DrawPolygon(const Types::Vector2D points[4], const bool fill, const Types::HighColor& color, const Types::Fxp sort)
        {
            SPRITE polygon = Scene2D::GetShapeCommand(fill ? FUNC_Polygon : FUNC_PolyLine, color);
            polygon.XA = points[0].X.ToInt();
            polygon.YA = points[0].Y.ToInt();
            polygon.XB = points[1].X.ToInt();
            polygon.YB = points[1].Y.ToInt();
            polygon.XC = points[2].X.ToInt();
            polygon.YC = points[2].Y.ToInt();
            polygon.XD = points[3].X.ToInt();
            polygon.YD = points[3].Y.ToInt();
            return slSetSprite(&polygon, sort.Value());
        }
    };
}


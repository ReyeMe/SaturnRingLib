#pragma once

#include "srl_base.hpp"
#include "srl_vdp1.hpp"
#include "srl_geo.hpp"
#include "srl_mesh.hpp"
#include "srl_angle.hpp"

namespace SRL
{
    /** @brief Rendering of VDP1 objects
     */
    class Render
    {
    private:

        /** @brief Base address of the gouraud table
         */
        static const Uint16 GouraudTableBase = 0xe000;

        /** @brief Address to the gouraud table
         */
        static inline Uint16 GouraudTableEntry = SRL::Render::GouraudTableBase - 1;

        /** @brief Is gouraud shading enabled?
         * @return true if gouraud shading is enabled
         */
        static constexpr inline bool IsGouraudEnabled()
        {
            return Render::GouraudTableEntry >= SRL::Render::GouraudTableBase;
        }

    public:

        /** @brief Set reference to the chunk of gouraud table to be used when drawing 2D object
         * @details This function sets reference to the first color in 4 color sequence that will be used for the gouraud shading of the 2D objects.
         * To set the colors inside  the table use @ref SRL::VDP1::GetGouraudTable() function to get pointer to the gouraud table array
         * @note This function does not apply to @ref SRL::Types::SmoothMesh and @ref SRL::Types::Mesh
         * @param index Index of the first color in the table
         */
        static inline void EnableGouraud(Uint16 index)
        {
            SRL::Render::GouraudTableEntry = SRL::Render::GouraudTableBase | index;
        }

        /** @brief This will disable gouraud shading when drawing 2D objects
         * @note This function does not apply to @ref SRL::Types::SmoothMesh and @ref SRL::Types::Mesh
         */
        static inline void DisableGouraud()
        {
            SRL::Render::GouraudTableEntry = SRL::Render::GouraudTableBase - 1;
        }

        /** @brief Draw mesh
         * @param mesh Mesh to draw
         * @param light Light direction
         * @return True on success
         */
        static void DrawSmoothMesh(Types::SmoothMesh& mesh, Types::Vector3D& light)
        {
            slPutPolygonX(mesh.SglPtr(), (FIXED*)&light);
        }

        /** @brief Draw mesh
         * @param mesh Mesh to draw
         * @return True on success
         */
        static bool DrawMesh(Types::Mesh& mesh)
        {
            return slPutPolygonS(mesh.SglPtr());
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
            const Uint16 texture,
            SRL::CRAM::Palette* texturePalette,
            const Types::Vector3D& location,
            const Types::Angle& angle = Types::Angle::FromRaw(0),
            const Types::Vector2D& scale = Types::Vector2D(1.0, 1.0))
        {
            Uint8 colorMode = CL32KRGB;
            Uint16 palette = No_Palet;
            CRAM::TextureColorMode colorModeType = SRL::VDP1::Metadata[texture].ColorMode;
            Uint16 paletteId = SRL::VDP1::Metadata[texture].PaletteId;

            if (texturePalette != nullptr)
            {
                colorModeType = texturePalette->GetMode();
                paletteId = texturePalette->GetId();
            }

            switch (colorModeType)
            {
            case SRL::CRAM::TextureColorMode::Paletted256:
                colorMode = CL256Bnk;
                palette = paletteId << 8;
                break;
            
            case SRL::CRAM::TextureColorMode::Paletted128:
                colorMode = CL128Bnk;
                palette = paletteId << 7;
                break;
            
            case SRL::CRAM::TextureColorMode::Paletted64:
                colorMode = CL64Bnk;
                palette = paletteId << 6;
                break;
            
            case SRL::CRAM::TextureColorMode::Paletted16:
                colorMode = CL16Bnk;
                palette = paletteId << 4;
                break;

            default:
                break;
            }

            // Sprite attributes and command points
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            SPR_ATTR attr = SPR_ATTRIBUTE(
                texture,
                palette,
                (Render::IsGouraudEnabled() ? Render::GouraudTableEntry : 0),
                MESHoff | colorMode | No_Window | ECdis | (Render::IsGouraudEnabled() ? CL_Gouraud : 0),
                sprVflip | FUNC_Sprite);
            #pragma GCC diagnostic pop

            FIXED sgl_pos[5];
            sgl_pos[X] = location.X.Value();
            sgl_pos[Y] = location.Y.Value();
            sgl_pos[Z] = location.Z.Value();
            sgl_pos[3] = scale.X.Value();
            sgl_pos[4] = scale.Y.Value();

            if (scale.X != scale.Y)
            {
                return slDispSpriteHV(sgl_pos, &attr, angle.Value()) != 0;
            }
            else
            {
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
        static bool DrawSprite(const Uint16 texture, const Types::Vector3D& location, const Types::Angle& angle = Types::Angle::FromRaw(0), const Types::Vector2D& scale = Types::Vector2D(1.0, 1.0))
        {
            return Render::DrawSprite(texture, nullptr, location, angle, scale);
        }
        
        /** @brief Draws a Line
        * @param start start point
        * @param end end point
        * @param color color of the line
        * @param sort Z order
        */
        static bool DrawLine(const Types::Vector2D& start, const Types::Vector2D& end, const Types::HighColor& color, const Types::Fxp sort)
        {
            SPRITE line;
            line.XA = start.X.ToInt();
            line.YA = start.Y.ToInt();
            line.XB = end.X.ToInt();
            line.YB = end.Y.ToInt();
            line.COLR = color;
            line.CTRL = FUNC_Line | (Render::IsGouraudEnabled() ? UseGouraud : 0);
            line.GRDA = (Render::IsGouraudEnabled() ? Render::GouraudTableEntry : 0);
            line.PMOD =  0x0080 |((CL32KRGB & 7) << 3) | (Render::IsGouraudEnabled() ? CL_Gouraud : 0);
            return slSetSprite(&line,sort.Value()) != 0;
        }

        /** @brief Draws a generic polygon
         * @param points Points of the polygon
         * @param fill Indicates whether polygon is filled or if it is just a polyline
         * @param color Polygon color
         * @param sort Z order
         */
        static void DrawPolygon(const Types::Vector2D points[4], const bool fill, const Types::HighColor& color, const Types::Fxp sort)
        {
            SPRITE line;
            line.COLR = color;
            line.CTRL = (fill ? FUNC_Polygon : FUNC_PolyLine) | (Render::IsGouraudEnabled() ? UseGouraud : 0);
            line.PMOD = 0x0080 | ((CL32KRGB & 7) << 3) | (Render::IsGouraudEnabled() ? CL_Gouraud : 0);
            line.GRDA = (Render::IsGouraudEnabled() ? SRL::Render::GouraudTableEntry : 0);
            line.XA = points[0].X.ToInt();
            line.YA = points[0].Y.ToInt();
            line.XB = points[1].X.ToInt();
            line.YB = points[1].Y.ToInt();
            line.XC = points[2].X.ToInt();
            line.YC = points[2].Y.ToInt();
            line.XD = points[3].X.ToInt();
            line.YD = points[3].Y.ToInt();
            slSetSprite(&line, sort.Value());
        }
    };
}


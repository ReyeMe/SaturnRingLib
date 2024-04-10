#pragma once

#include "srl_base.hpp"
#include "srl_vdp1.hpp"
#include "srl_geo.hpp"
#include "srl_mesh.hpp"

namespace SRL
{
	class Render
	{
	private:
	

	public:

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
		 * @param location Location of the sprite (Z coordinate is used for sorting)
		 * @param scale Scale of the sprite
		 * @return True on success
		 */
		static bool DrawSprite(const Uint16 texture, const Types::Vector3D& location, const Types::Vector2D& scale = Types::Vector2D(1.0, 1.0))
		{
			Uint8 colorMode = CL32KRGB;
			Uint16 palette = No_Palet;

			switch (SRL::VDP1::Metadata[texture].ColorMode)
			{
			case SRL::VDP1::TextureColorMode::Paletted:
				colorMode = CL256Bnk;
				palette = SRL::VDP1::Metadata[texture].PaletteId;
				break;
			
			case SRL::VDP1::TextureColorMode::Paletted16:
				colorMode = CL16Bnk;
				palette = SRL::VDP1::Metadata[texture].PaletteId;
				break;

			default:
				break;
			}

			// Sprite attributes and command points
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wnarrowing"
			SPR_ATTR attr = SPR_ATTRIBUTE(texture, palette, No_Gouraud, MESHoff | colorMode | No_Window | ECdis, sprVflip | FUNC_Sprite);
			#pragma GCC diagnostic pop

    		FIXED sgl_pos[XYZSS];
			sgl_pos[X] = location.X.Value();
			sgl_pos[Y] = location.Y.Value();
			sgl_pos[Z] = location.Z.Value();
			sgl_pos[Sh] = scale.X.Value();
			sgl_pos[Sv] = scale.Y.Value();

			if (scale.X != scale.Y)
			{
				return slDispSpriteHV(sgl_pos, &attr, 0) != 0;
			}
			else
			{
				return slDispSprite(sgl_pos, &attr, 0) != 0;
			}
		}

		static bool DrawLine(const Types::Vector2D& A, const Types::Vector2D& B, const Types::SaturnColor& color, const FIXED sort_nr)
		{
			SRL::Debug::Print(1, 3, "Drawline");
			//set attr
			SPRITE line;
			line.XA = A.X.Value();
			line.YA = A.Y.Value();
			line.XB = B.X.Value();
			line.YB = B.Y.Value();
			line.COLR = color;
			line.CTRL = FUNC_Line;
			line.GRDA = 0;
			line.PMOD =  0x0080 |((CL32KRGB & 7) << 3);
			return slSetSprite(&line,sort_nr) != 0;
		}
	};
}


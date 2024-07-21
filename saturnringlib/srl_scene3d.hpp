#pragma once

#include "srl_base.hpp"
#include "srl_mesh.hpp"

namespace SRL
{
    /** @brief Rendering of 3D objects
     */
    class Scene3D
    {
    public:

        /** @brief Draw SRL::Types::SmoothMesh
         * @param mesh SRL::Types::SmoothMesh to draw
         * @param light Light direction unit vector (This is independent of the SRL::Scene3D::SetDirectionalLight)
         */
        static void DrawSmoothMesh(Types::SmoothMesh& mesh, Types::Vector3D& light)
        {
            slPutPolygonX(mesh.SglPtr(), (FIXED*)&light);
        }

        /** @brief Draw SRL::Types::Mesh
         * @param mesh SRL::Types::Mesh to draw
         * @param slaveOnly Value indicates whether processing of the SRL::Types::Mesh should be handled only on the slave CPU
         * @return True on success
         */
        static bool DrawMesh(Types::Mesh& mesh, const bool slaveOnly)
        {
            if (slaveOnly)
            {
                return slPutPolygonS(mesh.SglPtr());
            }
            
            return slPutPolygon(mesh.SglPtr());
        }
        
        /** @brief Draw SRL::Types::Mesh with orthographic projection
         * @note Light source calculations and clipping cannot be performed with this function.
         * @param mesh SRL::Types::Mesh to draw
         * @param attribute Indicates an attribute in the SRL::Types::Mesh that will be shared by all polygons.<br>If set to 0, each polygon is displayed using the data at the beginning of the attribute table, otherwise specified attribute data will be displayed.
         * @return True On success
         */
        static bool DrawOrthographicMesh(Types::Mesh& mesh, uint16_t attribute)
        {
            return slDispPolygon(mesh.SglPtr(), attribute);
        }

        /** @brief Set value indicating how far in front of the projection surface to actually project.
         * @details Function specifies the distance from the forward boundary surface to the rear boundary surface.
         * @image html slZdspLevel.png width=1024
         * @param level Display level. See table below for valid parameter values and image above for explanation.
         * parameter value | real value
         * ----------------|------------
         * 1               | 1/2
         * 2               | 1/4
         * 3               | 1/8
         * 4               | 1/16
         * 5               | 1/32
         * 6               | 1/64
         * 7               | 1/128
         */
        static void SetDepthDisplayLevel(uint16_t level)
        {
            slZdspLevel(level);
        }

        /** @brief Set directional light source
         * @note If scaling operation is being performed on current matrix, normal vector of the polygon is also being affected,thus brightness will change accordingly.  
         * @param direction Light direction unit vector
         */
        static void SetDirectionalLight(Types::Vector3D& direction)
        {
            slLight((FIXED*)&direction);
        }

        /** @brief Initialize gouraud table for light calculation with SRL::Scene3D::DrawSmoothMesh
         * @code {.cpp}
         * // When the total number of polygons is 500 and the maximum number of vertices per model is 100.
         * #define MAX_POLYGON 500
         * #define MAX_MODEL_VERT 100
         * SRL::Types::HighColor workTable[MAX_POLYGON << 2];
         * uint8 vertWork[MAX_MODEL_VERT];
         * 
         * SRL::Scene3D::LightInitGouraudTable(0, vertWork, workTable, MAX_POLYGON);
         * @endcode
         * @param gouraudRamOffset Relative address to the first entry from which to write light gouraud data in SRL::VDP1::GetGouraudTable(). Using 0 here would mean first entry, 2 is second entry in the table, where each entry is 4 color long.
         * @param vertexCalculationBuffer Vertex arithmetic work buffer
         * @param tableStorage Work gouraud table with size of maxPolygons
         * @param maxPolygons Maximum number of polygons that can be processed by the light calculation
         */
        static void LightInitGouraudTable(uint32_t gouraudRamOffset, uint8_t* vertexCalculationBuffer, Types::HighColor* tableStorage, uint32_t maxPolygons)
        {
            slInitGouraud((GOURAUDTBL*)tableStorage, maxPolygons, 0xe000 + (gouraudRamOffset << 2), vertexCalculationBuffer);
        }

        /** @brief Set custom light gouraud table.
         * @details Table must contain 32 color entries from the darkest color to the brightest.
         * @param table custom light table
         */
        static void LightSetGouraudTable(Types::HighColor table[32])
        {
            slSetGouraudTbl((uint16_t*)table);
        }

        /** @brief Copies gouraud table calculated by the library to VRAM.
         * @code {.cpp}
         * // Attach the function to VBlank
         * SRL::Core::OnVblank += SRL::Scene3D::LightCopyGouraudTable;
         * @endcode
         * @note This function must be always called in vblank when using SRL::Scene3D::DrawSmoothMesh()
         */
        static void LightCopyGouraudTable()
        {
            slGouraudTblCopy();
        }

        /** @brief Set angle of the perspective projection.
         * @details Perspective angle determines the angle corresponding to the width of the screen. In combination with SRL::Scene3D::SetDepthDisplayLevel and SRL::Scene3D::SetWindow projection volume can be completely determined.
         * @image html slPerspective.png width=448
         * @note Because this function also sets parameter for rotating scroll, also execute slRpasaIntSet() before calling this if using rotating scroll
         * @param angle Perspective angle (Range 10-160 degrees)
         */
        static void SetPerspective(Types::Angle angle)
        {
            slPerspective(angle.Value());
        }

        /** @brief Set window limiting the display of sprites and polygons.
         * @details There can be two windows on screen at once, sprite clipping can be used to decide whether sprites are displayed inside the window or not.<br>Depth limit indicates maximal distance to the near projection plane.<br>Center point can be used to set projection center.
         * @image html slWindow.png width=1024
         * @note Be careful! This function also affects sprites rendered with SRL::Scene2D.
         * @param topLeft Top left point on the screen
         * @param bottomRight Bottom right point on the screen
         * @param center 3D projection center (vanishing point)
         * @param depthLimit Maximal allowed distance from the near projection plane
         * @return True on success
         */
        static bool SetWindow(Types::Vector2D& topLeft, Types::Vector2D& bottomRight, Types::Vector2D& center, Types::Fxp& depthLimit)
        {
            return slWindow(
                topLeft.X.ToInt(),
                topLeft.Y.ToInt(),
                bottomRight.X.ToInt(),
                bottomRight.Y.ToInt(),
                depthLimit.ToInt(),
                center.X.ToInt(),
                center.Y.ToInt()) != 0;
        }

    };
}


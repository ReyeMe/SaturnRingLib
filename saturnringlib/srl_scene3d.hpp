#pragma once

#include "srl_base.hpp"
#include "srl_mesh.hpp"
#include "srl_matrix.hpp"

namespace SRL
{
    /** @brief Rendering of 3D objects
     */
    class Scene3D
    {
        /** @brief Disable constructor
         */
        Scene3D() = delete;

        /** @brief Disable destructor
         */
        ~Scene3D() = delete;

    public:

        /**
         * @name Draw functions
         * @{
         */

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
        static bool DrawMesh(Types::Mesh& mesh, const bool slaveOnly = false)
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

        /** @} */

        /**
         * @name Light handling functions
         * @{
         */

        /** @brief Set directional light source
         * @note If scaling operation is being performed on current matrix, normal vector of the polygon is also being affected,thus brightness will change accordingly.  
         * @param direction Light direction unit vector
         */
        static void SetDirectionalLight(const Types::Vector3D& direction)
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

        /** @} */

        /**
         * @name Camera functions
         * @{
         */

        /** @brief Check if point/circle area is on screen
         * @param point Point to test
         * @param size Size of the point
         * @return true if point is on screen
         */
        static bool IsOnScreen(const SRL::Types::Vector3D& point, const SRL::Types::Fxp size)
        {
            return slCheckOnScreen((FIXED*)&point, size.Value()) >= 0;
        }

        /** @brief Make camera look at certain point in the 3D scene
         * @param camera Camera location
         * @param target Target point
         * @param roll Rotation around the line of sight vector
         */
        static void LookAt(const SRL::Types::Vector3D& camera, const SRL::Types::Vector3D& target, const SRL::Types::Angle roll)
        {
            slLookAt((FIXED*)&camera, (FIXED*)&target, (ANGLE)roll.Value());
        }

        /** @brief Projects 3D point onto a screen from current transformation matrix
         * @param position Position in world space
         * @param result Position on screen
         * @return Fxp Distance from world space to screen space
         */
        static SRL::Types::Fxp ProjectToScreen(const SRL::Types::Vector3D& position, SRL::Types::Vector2D* result)
        {
            return SRL::Types::Fxp::FromRaw(slConvert3Dto2DFX((FIXED*)&position, (FIXED*)result));
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
        static void SetDepthDisplayLevel(const uint16_t level)
        {
            slZdspLevel(level);
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

        /** @} */

        /**
         * @name Transformation matrix operations
         * @{
         */

        /** @brief Push current matrix onto the matrix stack
         */
        inline static void PushMatrix()
        {
            slPushMatrix();
        }

        /** @brief Push current matrix onto the matrix stack and set identity matrix as current
         */
        inline static void PushIdentityMatrix()
        {
            slPushUnitMatrix();
        }

        /** @brief Pop matrix from top of the stack and set it as current 
         */
        inline static void PopMatrix()
        {
            slPopMatrix();
        }

        /** @brief Replaces current matrix with identity matrix
         */
        inline static void LoadIdentity()
        {
            slInitMatrix();
        }

        /** @brief Set current matrix
         * @param matrix Transformation matrix
         */
        inline static void SetMatrix(SRL::Types::Matrix& matrix)
        {
            slLoadMatrix((FIXED(*)[3])matrix.SglPtr());
        }

        /** @brief Get current matrix
         * @note This will copy current matrix to specified address
         * @param result Transformation matrix
         */
        inline static void GetMatrix(SRL::Types::Matrix* result)
        {
            slGetMatrix((FIXED(*)[3])result);
        }

        /** @brief Inverts current matrix
         */
        inline static void InvertMatrix()
        {
            slInversMatrix();
        }

        /** @brief Transpose current matrix (zero movement in parallel direction)
         */
        inline static void TransposeMatrix()
        {
            slTransposeMatrix();
        }

        /** @brief Multiply current matrix by specified matrix
         * @param matrix Matrix to multiply current matrix with
         */
        inline static void MultiplyMatrix(SRL::Types::Matrix& matrix)
        {
            slMultiMatrix((FIXED(*)[3])matrix.SglPtr());
        }

        /** @brief Will convert current matrix into an unit matrix
         */
        inline static void NormalizeMatrix()
        {
            slUnitMatrix(CURRENT);
        }

        /** @brief Will convert translation component of the current matrix into an unit vector
         */
        inline static void NormalizeTranslationMatrix()
        {
            slUnitTranslate(CURRENT);
        }

        /** @brief Will convert rotation component of the current matrix into an unit rotation
         */
        inline static void NormalizeRotationMatrix()
        {
            slUnitAngle(CURRENT);
        }

        /** @brief Rotate current matrix around arbitrary axis by specific angle
         * @param axis Rotation axis
         * @param angle Rotation angle
         */
        inline static void Rotate(const SRL::Types::Vector3D& axis, SRL::Types::Angle angle)
        {
            slRotAX(axis.X.Value(), axis.Y.Value(), axis.Z.Value(), (ANGLE)angle.Value());
        }

        /** @brief Rotate current matrix around X axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateX(SRL::Types::Angle angle)
        {
            slRotX((ANGLE)angle.Value());
        }

        /** @brief Rotate current matrix around X axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateX(SRL::Types::Fxp sin, SRL::Types::Fxp cos)
        {
            slRotXSC(sin.Value(), cos.Value());
        }
        
        /** @brief Rotate current matrix around Y axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateY(SRL::Types::Angle angle)
        {
            slRotY((ANGLE)angle.Value());
        }

        /** @brief Rotate current matrix around Y axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateY(SRL::Types::Fxp sin, SRL::Types::Fxp cos)
        {
            slRotYSC(sin.Value(), cos.Value());
        }
        
        /** @brief Rotate current matrix around Z axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateZ(SRL::Types::Angle angle)
        {
            slRotZ((ANGLE)angle.Value());
        }

        /** @brief Rotate current matrix around X axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateZ(SRL::Types::Fxp sin, SRL::Types::Fxp cos)
        {
            slRotZSC(sin.Value(), cos.Value());
        }
        
        /** @brief Scale current transformation matrix
         * @param x Scale factor on X axis
         * @param y Scale factor on Y axis
         * @param z Scale factor on Z axis
         */
        inline static void Scale(SRL::Types::Fxp x, SRL::Types::Fxp y, SRL::Types::Fxp z)
        {
            slScale(x.Value(), y.Value(), z.Value());
        }

        /** @brief Scale current transformation matrix
         * @param scale Scale factor
         */
        inline static void Scale(const SRL::Types::Vector3D& scale)
        {
            slScale(scale.X.Value(), scale.Y.Value(), scale.Z.Value());
        }

        /** @brief Scale current transformation matrix
         * @param scale Scale factor
         */
        inline static void Scale(SRL::Types::Fxp scale)
        {
            slScale(scale.Value(), scale.Value(), scale.Value());
        }

        /** @brief Translate current transformation matrix
         * @param x Translation delta on X axis 
         * @param y Translation delta on Y axis
         * @param z Translation delta on Z axis
         */
        inline static void Translate(SRL::Types::Fxp x, SRL::Types::Fxp y, SRL::Types::Fxp z)
        {
            slTranslate(x.Value(), y.Value(), z.Value());
        }

        /** @brief Translate current transformation matrix
         * @param delta Translation delta
         */
        inline static void Translate(const SRL::Types::Vector3D& delta)
        {
            slTranslate(delta.X.Value(), delta.Y.Value(), delta.Z.Value());
        }

        /** @} */
    };
}


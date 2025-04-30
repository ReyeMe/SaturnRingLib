#pragma once

#include "srl_base.hpp"
#include "srl_mesh.hpp"

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
        static void DrawSmoothMesh(Types::SmoothMesh& mesh, SRL::Math::Types::Vector3D& light)
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
         * @name Gouraud light handling functions
         * @brief Affects quads with SRL::Types::Attribute::DisplayOption::EnableGouraud option set
         * @note Light direction of the gouraud light is set when drawing the mesh using SRL::Scene3D::DrawSmoothMesh()
         * @{
         */

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

        /** @brief Set custom light gouraud table. Affects quads with SRL::Types::Attribute::DisplayOption::EnableGouraud option set
         * @details Table must contain 32 color entries from the darkest color to the brightest.
         * @param table custom light table
         */
        static void LightSetGouraudTable(Types::HighColor table[32])
        {
            slSetGouraudTbl((uint16_t*)table);
        }

        /** @brief Set gouraud color of light source
         * @note This option will override SRL::Types::LightSetGouraudTable()
         * @param color Light source color
         */
        static void LightSetGouraudColor(const SRL::Types::HighColor color)
        {
            slSetGouraudColor(color);
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
         * @name Flat light handling functions
         * @brief Affects quads with SRL::Types::Attribute::DisplayOption::EnableFlatLight option set
         * @{
         */

        /** @brief Set Ambient color of the light, this the darkest color a light can produce
         * @param color Ambient color
         * @note Light color should be set after calling this function
         * @note This function is not valid for SRL::Scene3D::LightSetGouraudColor()
         */
        static void LightSetAmbient(const SRL::Types::HighColor color)
        {
            slSetAmbient(color);
        }

        /** @brief Set color of the flat light source
         * @param color Light source color
         */
        static void LightSetColor(const SRL::Types::HighColor color)
        {
            slSetFlatColor(color);
        }

        /** @brief Set directional light source
         * @note If scaling operation is being performed on current matrix, normal vector of the polygon is also being affected,thus brightness will change accordingly.  
         * @param direction Light direction unit vector
         */
        static void SetDirectionalLight(const SRL::Math::Types::Vector3D& direction)
        {
            slLight((FIXED*)&direction);
        }

        /** @} */

        /**
         * @name Depth shading handling functions
         * @brief Affects quads with SRL::Types::Attribute::DisplayOption::EnableDepthShading option set
         * @{
         */

        /** @brief Set the gouraud table used for depth shading
         * @param gouraudRamOffset Relative address to the first entry from which to write light gouraud data in SRL::VDP1::GetGouraudTable(). Using 0 here would mean first entry, 2 is second entry in the table, where each entry is 4 color long.
         * @param table Custom depth shading table
         */
        static void SetDepthShadingTable(const uint32_t gouraudRamOffset, Types::HighColor table[32])
        {
            slSetDepthTbl((uint16_t*)table, 0xe000 + (gouraudRamOffset << 2), 32);
        }
        
        /** @brief Set the range from the near distance to the depth in steps
         * @param near Near distance
         * @param depth Depth value as power of two (eg: setting it to 5 will result in 32)
         * @param step Step between near distance and depth as power of two
         */
        static void SetDepthShadingLimits(const SRL::Math::Types::Fxp& near, const uint16_t depth, const uint16_t step)
        {
            slSetDepthLimit(near.RawValue(), depth, step);
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
        static bool IsOnScreen(const SRL::Math::Types::Vector3D& point, const SRL::Math::Types::Fxp size)
        {
            return slCheckOnScreen((FIXED*)&point, size.RawValue()) >= 0;
        }

        /** @brief Make camera look at certain point in the 3D scene
         * @param camera Camera location
         * @param target Target point
         * @param roll Rotation around the line of sight vector
         */
        static void LookAt(const SRL::Math::Types::Vector3D& camera, const SRL::Math::Types::Vector3D& target, const SRL::Math::Types::Angle roll)
        {
            slLookAt((FIXED*)&camera, (FIXED*)&target, (ANGLE)roll.RawValue());
        }

        /** @brief Projects 3D point onto a screen from current transformation matrix
         * @param position Position in world space
         * @param result Position on screen
         * @return Fxp Distance from world space to screen space
         */
        static SRL::Math::Types::Fxp ProjectToScreen(const SRL::Math::Types::Vector3D& position, SRL::Math::Types::Vector2D* result)
        {
            return SRL::Math::Types::Fxp::BuildRaw(slConvert3Dto2DFX((FIXED*)&position, (FIXED*)result));
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
        static void SetPerspective(SRL::Math::Types::Angle angle)
        {
            slPerspective(angle.RawValue());
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
        static bool SetWindow(const SRL::Math::Types::Vector2D& topLeft, const SRL::Math::Types::Vector2D& bottomRight, const SRL::Math::Types::Vector2D& center, const SRL::Math::Types::Fxp& depthLimit)
        {
            return slWindow(
                topLeft.X.As<int16_t>(),
                topLeft.Y.As<int16_t>(),
                bottomRight.X.As<int16_t>(),
                bottomRight.Y.As<int16_t>(),
                depthLimit.As<int16_t>(),
                center.X.As<int16_t>(),
                center.Y.As<int16_t>()) != 0;
        }

        /** @brief Sets a value indicating whether to preform near clip correction
         * @param enabled If set to true, Near clip correction will be preformed (default), otherwise none and polygon will be clipped if not all 4 points are on screen
         * @note If game does not require near clip correction (Polygons not passing near plane of the camera), setting this to false, can improve performance
         */
        static void SetNearClipCorrection(bool enabled)
        {
            slNearClipFlag(enabled);
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
            slUnitMatrix(CURRENT);
        }

        /** @brief Sets identity translation matrix
         */
        inline static void IdentityTranslationMatrix()
        {
            slUnitTranslate(CURRENT);
        }

        /** @brief Sets identity rotation matrix
         */
        inline static void IdentityRotationMatrix()
        {
            slUnitAngle(CURRENT);
        }

        /** @brief Set current matrix
         * @param matrix Transformation matrix
         */
        inline static void SetMatrix(SRL::Math::Matrix43& matrix)
        {
            slLoadMatrix((FIXED(*)[3])&matrix);
        }

        /** @brief Get current matrix
         * @note This will copy current matrix to specified address
         * @param result Transformation matrix
         */
        inline static void GetMatrix(SRL::Math::Matrix43* result)
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
        inline static void MultiplyMatrix(SRL::Math::Matrix43& matrix)
        {
            slMultiMatrix((FIXED(*)[3])&matrix);
        }

        /** @brief Rotate current matrix around arbitrary axis by specific angle
         * @param axis Rotation axis
         * @param angle Rotation angle
         */
        inline static void Rotate(const SRL::Math::Types::Vector3D& axis, const SRL::Math::Types::Angle angle)
        {
            slRotAX(axis.X.RawValue(), axis.Y.RawValue(), axis.Z.RawValue(), (ANGLE)angle.RawValue());
        }

        /** @brief Rotate current matrix around X axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateX(const SRL::Math::Types::Angle angle)
        {
            slRotX((ANGLE)angle.RawValue());
        }

        /** @brief Rotate current matrix around X axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateX(const SRL::Math::Types::Fxp sin, const SRL::Math::Types::Fxp cos)
        {
            slRotXSC(sin.RawValue(), cos.RawValue());
        }
        
        /** @brief Rotate current matrix around Y axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateY(const SRL::Math::Types::Angle angle)
        {
            slRotY((ANGLE)angle.RawValue());
        }

        /** @brief Rotate current matrix around Y axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateY(const SRL::Math::Types::Fxp sin, const SRL::Math::Types::Fxp cos)
        {
            slRotYSC(sin.RawValue(), cos.RawValue());
        }
        
        /** @brief Rotate current matrix around Z axis by specific angle
         * @param angle Rotation angle
         */
        inline static void RotateZ(const SRL::Math::Types::Angle angle)
        {
            slRotZ((ANGLE)angle.RawValue());
        }

        /** @brief Rotate current matrix around X axis by specific sinus and cosine values
         * @param sin Sinus value
         * @param cos Cosine value
         */
        inline static void RotateZ(const SRL::Math::Types::Fxp sin, const SRL::Math::Types::Fxp cos)
        {
            slRotZSC(sin.RawValue(), cos.RawValue());
        }
        
        /** @brief Scale current transformation matrix
         * @param x Scale factor on X axis
         * @param y Scale factor on Y axis
         * @param z Scale factor on Z axis
         */
        inline static void Scale(const SRL::Math::Types::Fxp x, const SRL::Math::Types::Fxp y, const SRL::Math::Types::Fxp z)
        {
            slScale(x.RawValue(), y.RawValue(), z.RawValue());
        }

        /** @brief Scale current transformation matrix
         * @param scale Scale factor
         */
        inline static void Scale(const SRL::Math::Types::Vector3D& scale)
        {
            slScale(scale.X.RawValue(), scale.Y.RawValue(), scale.Z.RawValue());
        }

        /** @brief Scale current transformation matrix
         * @param scale Scale factor
         */
        inline static void Scale(const SRL::Math::Types::Fxp scale)
        {
            slScale(scale.RawValue(), scale.RawValue(), scale.RawValue());
        }

        /** @brief Translate current transformation matrix
         * @param x Translation delta on X axis 
         * @param y Translation delta on Y axis
         * @param z Translation delta on Z axis
         */
        inline static void Translate(const SRL::Math::Types::Fxp x, const SRL::Math::Types::Fxp y, const SRL::Math::Types::Fxp z)
        {
            slTranslate(x.RawValue(), y.RawValue(), z.RawValue());
        }

        /** @brief Translate current transformation matrix
         * @param delta Translation delta
         */
        inline static void Translate(const SRL::Math::Types::Vector3D& delta)
        {
            slTranslate(delta.X.RawValue(), delta.Y.RawValue(), delta.Z.RawValue());
        }

        /** @brief Transforms point by current transformation matrix
         * @param point Point to transform
         * @return Transformed point
         */
        static SRL::Math::Types::Vector3D TransformPoint(const SRL::Math::Types::Vector3D& point)
        {
            SRL::Math::Types::Vector3D result = SRL::Math::Types::Vector3D();
            slCalcPoint(point.X.RawValue(), point.Y.RawValue(), point.Z.RawValue(), (FIXED*)&result);
            return result;
        }

        /** @brief Transforms direction vector by current transformation matrix
         * @param point Direction vector to transform
         * @return Transformed direction vector
         */
        static SRL::Math::Types::Vector3D TransformVector(const SRL::Math::Types::Vector3D& point)
        {
            SRL::Math::Types::Vector3D result = SRL::Math::Types::Vector3D();
            slCalcVector((FIXED*)&point, (FIXED*)&result);
            return result;
        }

        /** @} */
    };
}


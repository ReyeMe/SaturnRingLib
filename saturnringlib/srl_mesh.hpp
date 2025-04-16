#pragma once

#include "srl_base.hpp"

namespace SRL::Types
{
    /** @brief Face attributes
     */
    struct Attribute : public SRL::SGL::SglType<Attribute, ATTR>
    {
        /** @brief Face visibility type
         */
        enum FaceVisibility : uint8_t
        {
            /** @brief Normal vector is used to determine whether face is visible or not
             */
            SingleSided = 0,
    
            /** @brief Face is visible at all times
             */
            DoubleSided = 1
        };

        /** @brief Sorting mode for the face
         */
        enum SortMode : uint8_t
        {
            /** @brief Draw polygon before last drawn polygon
             */
            BeforeLast = SORT_BFR,

            /** @brief Use the foremost point out of 4 points
             */
            Minimum = SORT_MIN,

            /** @brief Use the farthest of the four points
             */
            Maximum = SORT_MAX,

            /** @brief Use the average position of 4 points
             */
            Center = SORT_CEN
        };

        /** @brief Quad display mode
         */
        enum DisplayMode : uint16_t
        {
            /** @brief No special mode (default)
             */
            NoMode = CL_Replace,

            /** @brief Shadow mode
             */
            Shadow = CL_Shadow,

            /** @brief Half brightness mode
             */
            HalfBright = CL_Half,

            /** @brief Half transparency mode
             * @note Half transparency does not mix with VDP2 layers
             */
            HalfTransparent = CL_Trans,

            /** @brief Mesh (screen doors) mode
             */
            Mesh = MESHon,

            /** @brief Clip area outside of user window
             */
            ClippingIn = Window_In,

            /** @brief Clip area inside of user window
             */
            ClippingOut = Window_Out,

            /** @brief Use high speed shrink when drawing scaled quad
             */
            HighSpeedShrink = HSSon
        };

        /** @brief Quad display type
         * @note Polygon, PolyLine and Segment only use solid color, and ignore texture parameter.
         * @note All Sprite options use color parameter either as palette index, or ignore it, depending on colorMode setting
         */
        enum DisplayType : uint32_t
        {
            /** @brief Displays quad as filled polygon
             */
            Polygon = (uint32_t)sprPolygon,

            /** @brief Displays quad as polygon contour (wireframe)
             */
            PolyLine = (uint32_t)sprPolyLine,

            /** @brief Displays quad as single line (only first and second point are used, rest is ignored)
             */
            Segment = (uint32_t)sprLine,

            /** @brief Draws a regular textured quad
             */
            Sprite = (uint32_t)sprNoflip,

            /** @brief Draws a textured quad mirrored on its horizontal axis
             */
            SpriteHFlip = (uint32_t)sprHflip,

            /** @brief Draws a textured quad mirrored on its vertical axis
             */
            SpriteVFlip = (uint32_t)sprVflip,

            /** @brief Draws a textured quad mirrored on both of its axes
             */
            SpriteHVFlip = (uint32_t)sprHVflip
        };

        /** @brief Additional quad display options
         */
        enum DisplayOption : uint16_t
        {
            /** @brief No option
             */
            NoOption = No_Option,

            /** @brief Use flat light shading
             */
            EnableFlatLight = UseLight,

            /** @brief Use depth shading
             */
            EnableDepthShading = UseDepth,

            /** @brief Clip quad by user window clipping
             */
            EnableClipping = UseClip,

            /** @brief Use gouraud shading
             */
            EnableGouraud = UseGouraud
        };

        /** @brief Construct a new empty Attribute
         */
        Attribute() :
            Visibility(FaceVisibility::SingleSided),
            Sort(0),
            Texture(0),
            Display(0),
            ColorMode(0),
            Gouraud(0),
            Direction(0) { }

        /** @brief Construct a new custom quad attribute
         *  @param visibility Indicates whether to cull quads facing away from camera
         *  @param sort Indicates sorting option for the drawing of the quad
         *  @param texture Texture index or 0 for no texture
         *  @param color Quad color (Not in use if quad is textured), or texture palette index
         *  @param gouraud Gouraud table address
         *  @param mode Display mode (display effects and color mode)
         *  @param type Display type (sprite, polygon, etc)
         *  @param options Display options (light, gouraud, depth shading)
         */
        Attribute(const FaceVisibility visibility, const SortMode sort, const uint16_t texture, uint16_t color, uint16_t gouraud, uint16_t mode, uint32_t type, uint16_t options) :
            Visibility(visibility),
            Sort(sort | (((type) >> 16) & 0x1c) | options),
            Texture(texture),
            Display(mode | (((type) >> 24) & 0xc0)),
            ColorMode(color),
            Gouraud(gouraud),
            Direction(type & 0x3f) { }

        /** @brief Construct a new custom quad attribute
         * @param visibility Indicates whether to cull quads facing away from camera
         * @param sort Indicates sorting option for the drawing of the quad
         * @param texture Texture index or 0 for no texture
         * @param color Quad color (Not in use if quad is textured), or texture palette index
         * @param gouraudRamOffset Relative address to the first entry from which to write light gouraud data in SRL::VDP1::GetGouraudTable(). Using 0 here would mean first entry, 2 is second entry in the table, where each entry is 4 color long.
         * @param colorMode Color mode of the quad (RGB555, 256, 128, 64, 16 colors)
         * @param mode Display mode of the quad
         * @param type Display type of the quad
         * @param option Display options
         */
        Attribute(
            const FaceVisibility visibility,
            const SortMode sort,
            const uint16_t texture,
            const uint16_t color,
            const uint16_t gouraudRamOffset,
            const CRAM::TextureColorMode colorMode,
            const DisplayMode mode,
            const DisplayType type,
            const DisplayOption option)
            : Visibility(visibility), Texture(texture), ColorMode(color), Gouraud(0xe000 + (gouraudRamOffset << 2)), Direction(type & 0x3f)
        {
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wnarrowing"
            uint16_t displayMode = CL32KRGB;

            switch (colorMode)
            {
                case CRAM::TextureColorMode::Paletted16:
                    displayMode = CL16Bnk | ECdis;
                    this->Sort |= UsePalette;
                    break;

                case CRAM::TextureColorMode::Paletted64:
                    displayMode = CL64Bnk | ECdis;
                    this->Sort |= UsePalette;
                    break;

                case CRAM::TextureColorMode::Paletted128:
                    displayMode = CL128Bnk | ECdis;
                    this->Sort |= UsePalette;
                    break;

                case CRAM::TextureColorMode::Paletted256:
                    displayMode = CL256Bnk | ECdis;
                    this->Sort |= UsePalette;
                    break;
                
                default:
                    break;
            }
        
            displayMode |= mode;
            this->Sort = sort | ((type >> 16) & 0x1c) | option;
            this->Display = displayMode | ((type >> 24) & 0xc0);
            #pragma GCC diagnostic pop
        }
        
        /** @brief Indicates whether face is visible at all times or only when its normal vector is pointing towards camera
         */
        FaceVisibility Visibility;

        /** @brief Sort reference position and optional settings
         */
        uint8_t Sort;

        /** @brief Texture number
         */
        uint16_t Texture;

        /** @brief Display mode
         */
        uint16_t Display;

        /** @brief Color mode
         */
        uint16_t ColorMode;

        /** @brief Gouraud shading table
         */
        uint16_t Gouraud;

        /** @brief Texture inversion and function number
         */
        uint16_t Direction;
    };

    /** @brief Mesh polygon
     */
    struct Polygon : public SRL::SGL::SglType<Polygon, POLYGON>
    {
        /** @brief Construct a new Polygon object
         */
        Polygon() : Normal(), Vertices { 0, 0, 0, 0 } { }

        /** @brief Construct a new Polygon object
         * @param normal Normal vector
         * @param vertices Polygon vertex indicies
         */
        Polygon(const SRL::Math::Types::Vector3D& normal, const uint16_t vertices[4]) : Normal(normal), Vertices { vertices[0], vertices[1], vertices[2], vertices[3] } { }

        /** @brief Normal vector of the polygon
         */
        SRL::Math::Types::Vector3D Normal;

        /** @brief Vertices of the polygon
         */
        uint16_t Vertices[4];
    };

    /** @brief 3D mesh
     */
    struct Mesh : public SRL::SGL::SglType<Mesh, PDATA>
    {
        /** @brief Vertices of the mesh
         */
        SRL::Math::Types::Vector3D *Vertices;

        /** @brief Number of vertices of the mesh
         */
        size_t VertexCount;

        /** @brief Mesh faces
         */
        Polygon *Faces;

        /** @brief Number of faces
         */
        size_t FaceCount;

        /** @brief Face attributes
         */
        Attribute *Attributes;

        /** @brief Construct a new empty mesh object
         */
        Mesh() : Attributes(nullptr), FaceCount(0), Faces(nullptr), VertexCount(0), Vertices(nullptr) { }

        /** @brief Construct a new empty mesh object and initialize its arrays
         * @param vertexCount Number of vertices in the mesh
         * @param polygonCount Number of polygons in the mesh
         */
        Mesh(const size_t& vertexCount, const size_t& polygonCount) : FaceCount(polygonCount), VertexCount(vertexCount)
        {
            this->Vertices = autonew SRL::Math::Types::Vector3D[vertexCount];
            this->Faces = autonew Polygon[polygonCount];
            this->Attributes = autonew Attribute[polygonCount];
        }

        /** @brief Construct a new mesh object from existing data
         * @param vertexCount Number of points
         * @param vertices Vertex data
         * @param faceCount Number of faces
         * @param faces Face polygons
         * @param attributes Face attributes
         */
        Mesh(const size_t& vertexCount,
            SRL::Math::Types::Vector3D* vertices,
            const size_t& faceCount,
            Polygon* faces,
            Attribute* attributes) : FaceCount(faceCount), VertexCount(vertexCount)
        {
            this->Vertices = vertices;
            this->Faces = faces;
            this->Attributes = attributes;
        }
        
        /** @brief Move assignment operator
         * @param other Mesh to assign
         * @return Mesh3D& Assigned mesh data
         */
        Mesh& operator=(Mesh&& other) noexcept
        {
            if (this != &other)
            {
                // Steal resources from the source object
                delete[] this->Vertices;
                this->Vertices = other.Vertices;
                this->VertexCount = other.VertexCount;

                delete[] this->Faces;
                this->Faces = other.Faces;
                this->FaceCount = other.FaceCount;

                delete[] this->Attributes;
                this->Attributes = other.Attributes;

                // Reset the source object
                other.Vertices = nullptr;
                other.VertexCount = 0;
                other.Faces = nullptr;
                other.FaceCount = 0;
                other.Attributes = nullptr;
            }

            return *this;
        }

        /** @brief Destroy the Mesh object
         */
        ~Mesh()
        {
            // Release resources
            delete[] this->Vertices;
            delete[] this->Faces;
            delete[] this->Attributes;
        }
    };

    /** @brief 3D smooths mesh
     */
    struct SmoothMesh : public SRL::SGL::SglType<SmoothMesh, XPDATA>
    {
        /** @brief Vertices of the mesh
         */
        SRL::Math::Types::Vector3D *Vertices;

        /** @brief Number of vertices of the mesh
         */
        size_t VertexCount;

        /** @brief Mesh faces
         */
        Polygon *Faces;

        /** @brief Number of faces
         */
        size_t FaceCount;

        /** @brief Face attributes
         */
        Attribute *Attributes;

        /** @brief Normal vector data for vertices
         */
        SRL::Math::Types::Vector3D* Normals;

        /** @brief Construct a new empty mesh object
         */
        SmoothMesh() : Normals(nullptr), Attributes(nullptr), FaceCount(0), Faces(nullptr), VertexCount(0), Vertices(nullptr)  { }
        
        /** @brief Construct a new empty mesh object and initialize its arrays
         * @param vertexCount Number of vertices in the mesh
         * @param faceCount Number of polygons in the mesh
         */
        SmoothMesh(const size_t& vertexCount, const size_t& faceCount) : FaceCount(faceCount), VertexCount(vertexCount)
        {
            this->Vertices = autonew SRL::Math::Types::Vector3D[vertexCount];
            this->Faces = autonew Polygon[faceCount];
            this->Attributes = autonew Attribute[faceCount];
            this->Normals = autonew SRL::Math::Types::Vector3D[vertexCount];
        }

        /** @brief Construct a new mesh object from existing data
         * @param vertexCount Number of points
         * @param vertices Vertex data
         * @param faceCount Number of faces
         * @param faces Face polygons
         * @param attributes Face attributes
         * @param normals Vertex normals
         */
        SmoothMesh(const size_t& vertexCount,
            SRL::Math::Types::Vector3D* vertices,
            const size_t& faceCount,
            Polygon* faces,
            Attribute* attributes,
            SRL::Math::Types::Vector3D* normals) : FaceCount(faceCount), VertexCount(vertexCount)
        {
            this->Vertices = vertices;
            this->Faces = faces;
            this->Attributes = attributes;
            this->Normals = normals;
        }
        
        /** @brief Move assignment operator
         * @param other Mesh to assign
         * @return Mesh3D& Assigned mesh data
         */
        SmoothMesh& operator=(SmoothMesh&& other) noexcept
        {
            if (this != &other)
            {
                // Steal resources from the source object
                delete[] this->Vertices;
                this->Vertices = other.Vertices;
                this->VertexCount = other.VertexCount;

                delete[] this->Normals;
                this->Normals = other.Normals;

                delete[] this->Faces;
                this->Faces = other.Faces;
                this->FaceCount = other.FaceCount;

                delete[] this->Attributes;
                this->Attributes = other.Attributes;

                // Reset the source object
                other.Vertices = nullptr;
                other.VertexCount = 0;
                other.Faces = nullptr;
                other.FaceCount = 0;
                other.Attributes = nullptr;
                other.Normals = nullptr;
            }

            return *this;
        }

        /** @brief Destroy the Mesh object
         */
        ~SmoothMesh()
        {
            // Release resources
            delete[] this->Vertices;
            delete[] this->Faces;
            delete[] this->Attributes;
            delete[] this->Normals;
        }
    };
}

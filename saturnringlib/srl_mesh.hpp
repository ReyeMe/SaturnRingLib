#pragma once

#include "srl_base.hpp"
#include "srl_vector.hpp"

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
            /** @brief Use the position of the polygon displayed immediately before
             */
            UseLast = 0,

            /** @brief Use the foremost point out of 4 points
             */
            Minimum = 1,

            /** @brief Use the farthest of the four points
             */
            Maximum = 2,

            /** @brief Use the average position of 4 points
             */
            Center = 3
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

        /** @brief Construct a new Attribute object
         *  @note TODO: Needs documenting and better constructor!
         *  @param visibility 
         *  @param sort 
         *  @param texture 
         *  @param color 
         *  @param gouraud 
         *  @param mode 
         *  @param direction 
         *  @param options 
         */
        Attribute(const FaceVisibility visibility, const SortMode sort, const uint16_t texture, uint16_t color, uint16_t gouraud, uint16_t mode, uint32_t direction, uint16_t options) :
            Visibility(visibility),
            Sort(sort | (((direction) >> 16) & 0x1c) | options),
            Texture(texture),
            Display(mode | (((direction) >> 24) & 0xc0)),
            ColorMode(color),
            Gouraud(gouraud),
            Direction(direction & 0x3f) { }

        /** @brief Inidcates whether face is visible at all times or only when its normal vector is pointing towards camera
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
        Polygon(const Vector3D& normal, const uint16_t vertices[4]) : Normal(normal), Vertices { vertices[0], vertices[1], vertices[2], vertices[3] } { }

        /** @brief Normal vector of the polygon
         */
        Vector3D Normal;

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
        Vector3D *Vertices;

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
            this->Vertices = new Vector3D[vertexCount];
            this->Faces = new Polygon[polygonCount];
            this->Attributes = new Attribute[polygonCount];
        }

        /** @brief Construct a new mesh object from existing data
         * @param vertexCount Number of points
         * @param vertices Vertex data
         * @param faceCount Number of faces
         * @param faces Face polygons
         * @param attributes Face attributes
         */
        Mesh(const size_t& vertexCount,
            Vector3D* vertices,
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
        Vector3D *Vertices;

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
        Vector3D* Normals;

        /** @brief Construct a new empty mesh object
         */
        SmoothMesh() : Normals(nullptr), Attributes(nullptr), FaceCount(0), Faces(nullptr), VertexCount(0), Vertices(nullptr)  { }
        
        /** @brief Construct a new empty mesh object and initialize its arrays
         * @param vertexCount Number of vertices in the mesh
         * @param faceCount Number of polygons in the mesh
         */
        SmoothMesh(const size_t& vertexCount, const size_t& faceCount) : FaceCount(faceCount), VertexCount(vertexCount)
        {
            this->Vertices = new Vector3D[vertexCount];
            this->Faces = new Polygon[faceCount];
            this->Attributes = new Attribute[faceCount];
            this->Normals = new Vector3D[vertexCount];
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
            Vector3D* vertices,
            const size_t& faceCount,
            Polygon* faces,
            Attribute* attributes,
            Vector3D* normals) : FaceCount(faceCount), VertexCount(vertexCount)
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

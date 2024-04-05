#pragma once

#include "srl_base.hpp"
#include "srl_geo.hpp"

namespace SRL::Types
{
    /** @brief Face attributes
     */
    struct Attribute : public SRL::SGL::SglType<Attribute, ATTR>
    {
        /** @brief Face visibility type
         */
        enum FaceVisibility : Uint8
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
        enum SortMode : Uint8
        {
            /** @brief Use the position of the polygon displayed immediately before
             */
            UseLast = 0,

            /** @brief Use the foremost point out of 4 points
             */
            Minimum,

            /** @brief Use the farthest of the four points
             */
            Maximum,

            /** @brief Use the average position of 4 points
             */
            Center
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
         *  @note TODO: Needs documenting and better contructor!
         *  @param visibility 
         *  @param sort 
         *  @param texture 
         *  @param color 
         *  @param gouraud 
         *  @param mode 
         *  @param direction 
         *  @param options 
         */
        Attribute(const FaceVisibility visibility, const SortMode sort, const Uint16 texture, Uint16 color, Uint16 gouraud, Uint16 mode, Uint32 direction, Uint16 options) :
            Visibility(visibility),
            Texture(texture),
            ColorMode(color),
            Gouraud(gouraud),
            Sort(sort | (((direction) >> 16) & 0x1c) | options),
            Display(mode | (((direction) >> 24) & 0xc0)),
            Direction(direction & 0x3f) { }

        /** @brief Inidcates whether face is visible at all times or only when its normal vector is pointing towards camera
         */
        FaceVisibility Visibility;

        /** @brief Sort reference position and optional settings
         */
        Uint8 Sort;

        /** @brief Texture number
         */
        Uint16 Texture;

        /** @brief Display mode
         */
        Uint16 Display;

        /** @brief Color mode
         */
        Uint16 ColorMode;

        /** @brief Gouraud shading table
         */
        Uint16 Gouraud;

        /** @brief Texture inversion and function number
         */
        Uint16 Direction;
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
        Polygon(const Vector3D& normal, const Uint16 vertices[4]) : Normal(normal), Vertices { vertices[0], vertices[1], vertices[2], vertices[3] } { }

        /** @brief Normal vector of the polygon
         */
        Vector3D Normal;

        /** @brief Vertices of the polygon
         */
        Uint16 Vertices[4];
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
        size_t VerticesCount;

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
        Mesh()
        {
            this->Vertices = nullptr;
            this->VerticesCount = 0;
            this->Faces = nullptr;
            this->FaceCount = 0;
            this->Attributes = nullptr;
        }

        /** @brief Construct a new empty mesh object and initialize its arrays
         * @param vertexCount Number of vertices in the mesh
         * @param polygonCount Number of polygons in the mesh
         */
        Mesh(const size_t& vertexCount, const size_t& polygonCount)
        {
            this->Vertices = new Vector3D[vertexCount];
            this->VerticesCount = vertexCount;
            this->Faces = new Polygon[polygonCount];
            this->FaceCount = polygonCount;
            this->Attributes = new Attribute[polygonCount];
        }

        /** @brief Construct a new mesh object from existing data
         * @param vertexCount Number of points
         * @param vertices 
         * @param faceCount 
         * @param faces 
         * @param attributes
         */
        Mesh(const size_t& vertexCount,
            Vector3D* vertices,
            const size_t& faceCount,
            Polygon* faces,
            Attribute* attributes) : Mesh(vertexCount, faceCount)
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
                this->VerticesCount = other.VerticesCount;

                delete[] this->Faces;
                this->Faces = other.Faces;
                this->FaceCount = other.FaceCount;

                delete[] this->Attributes;
                this->Attributes = other.Attributes;

                // Reset the source object
                other.Vertices = nullptr;
                other.VerticesCount = 0;
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
}

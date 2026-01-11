#pragma once

#include "srl_base.hpp"
#include "srl_color.hpp"
#include "srl_mesh.hpp"

namespace SRL::Types
{
    class Primitive3D
    {
        Primitive3D() = delete;
        ~Primitive3D() = delete;

    public:

        /** @brief Create a cube mesh with specified size
         * @param size Half-size of the cube (distance from center to each face)
         * @param color Color for all faces
         * @return Mesh representing the cube
         */
        static Mesh CreateCube(const SRL::Math::Types::Fxp& size, const SRL::Types::HighColor& color)
        {
            Mesh mesh(8, 6);

            mesh.Vertices[0] = SRL::Math::Types::Vector3D(-size, -size, -size);
            mesh.Vertices[1] = SRL::Math::Types::Vector3D( size, -size, -size);
            mesh.Vertices[2] = SRL::Math::Types::Vector3D( size,  size, -size);
            mesh.Vertices[3] = SRL::Math::Types::Vector3D(-size,  size, -size);
            mesh.Vertices[4] = SRL::Math::Types::Vector3D(-size, -size,  size);
            mesh.Vertices[5] = SRL::Math::Types::Vector3D( size, -size,  size);
            mesh.Vertices[6] = SRL::Math::Types::Vector3D( size,  size,  size);
            mesh.Vertices[7] = SRL::Math::Types::Vector3D(-size,  size,  size);

            uint16_t faceIndices[6][4] = {
                { 3, 2, 1, 0 }, // Front face  (-Z)
                { 4, 5, 6, 7 }, // Back face   (+Z)
                { 0, 1, 5, 4 }, // Bottom face (-Y)
                { 7, 6, 2, 3 }, // Top face    (+Y)
                { 0, 4, 7, 3 }, // Left face   (-X)
                { 5, 1, 2, 6 }  // Right face  (+X)
            };

            SRL::Math::Types::Vector3D normals[6] = {
                SRL::Math::Types::Vector3D( 0,  0, -1), // Front
                SRL::Math::Types::Vector3D( 0,  0,  1), // Back
                SRL::Math::Types::Vector3D( 0, -1,  0), // Bottom
                SRL::Math::Types::Vector3D( 0,  1,  0), // Top
                SRL::Math::Types::Vector3D(-1,  0,  0), // Left
                SRL::Math::Types::Vector3D( 1,  0,  0)  // Right
            };

            for (size_t i = 0; i < 6; i++)
            {
                mesh.Faces[i] = Polygon(normals[i], faceIndices[i]);
                mesh.Attributes[i] = Attribute(
                    Attribute::FaceVisibility::SingleSided,
                    Attribute::SortMode::Center,
                    0,
                    color,
                    0,
                    (uint16_t)(CL32KRGB | ECdis),
                    (uint32_t)Attribute::DisplayType::Polygon,
                    Attribute::DisplayOption::NoOption);
            }

            return mesh;
        }

        /** @brief Create a plane mesh on the XZ plane
         * @param width Half-width of the plane (X axis)
         * @param depth Half-depth of the plane (Z axis)
         * @param color Color for the plane
         * @param doubleSided Whether the plane should be visible from both sides
         * @return Mesh representing the plane
         */
        static Mesh CreatePlane(const SRL::Math::Types::Fxp& width, const SRL::Math::Types::Fxp& depth, const SRL::Types::HighColor& color, bool doubleSided = false)
        {
            Mesh mesh(4, 1);

            mesh.Vertices[0] = SRL::Math::Types::Vector3D(-width, 0, -depth);
            mesh.Vertices[1] = SRL::Math::Types::Vector3D( width, 0, -depth);
            mesh.Vertices[2] = SRL::Math::Types::Vector3D( width, 0,  depth);
            mesh.Vertices[3] = SRL::Math::Types::Vector3D(-width, 0,  depth);

            uint16_t indices[4] = { 0, 1, 2, 3 };
            SRL::Math::Types::Vector3D normal(0, 1, 0);

            mesh.Faces[0] = Polygon(normal, indices);
            mesh.Attributes[0] = Attribute(
                doubleSided ? Attribute::FaceVisibility::DoubleSided : Attribute::FaceVisibility::SingleSided,
                Attribute::SortMode::Center,
                0,
                color,
                0,
                (uint16_t)(CL32KRGB | ECdis),
                (uint32_t)Attribute::DisplayType::Polygon,
                Attribute::DisplayOption::NoOption);

            return mesh;
        }

        /** @brief Create a UV sphere mesh
         * @param radius Radius of the sphere
         * @param segments Number of horizontal segments (longitude divisions, minimum 4)
         * @param rings Number of vertical rings (latitude divisions, minimum 2)
         * @param color Color for all faces
         * @return Mesh representing the sphere
         */
        static Mesh CreateSphere(const SRL::Math::Types::Fxp& radius, uint16_t segments, uint16_t rings, const SRL::Types::HighColor& color)
        {
            if (segments < 4) segments = 4;
            if (rings < 2) rings = 2;

            size_t vertexCount = (rings + 1) * segments;
            size_t faceCount = rings * segments;

            Mesh mesh(vertexCount, faceCount);

            SRL::Math::Types::Angle segmentStep = SRL::Math::Types::Angle::FromDegrees(360) / segments;
            SRL::Math::Types::Angle ringStep = SRL::Math::Types::Angle::FromDegrees(180) / rings;

            size_t vertexIndex = 0;
            for (uint16_t ring = 0; ring <= rings; ring++)
            {
                SRL::Math::Types::Angle phi = SRL::Math::Types::Angle::FromDegrees(-90) + ringStep * ring;
                SRL::Math::Types::Fxp y = radius * phi.Sin();
                SRL::Math::Types::Fxp ringRadius = radius * phi.Cos();

                for (uint16_t seg = 0; seg < segments; seg++)
                {
                    SRL::Math::Types::Angle theta = segmentStep * seg;
                    SRL::Math::Types::Fxp x = ringRadius * theta.Cos();
                    SRL::Math::Types::Fxp z = ringRadius * theta.Sin();

                    mesh.Vertices[vertexIndex++] = SRL::Math::Types::Vector3D(x, y, z);
                }
            }

            size_t faceIndex = 0;
            for (uint16_t ring = 0; ring < rings; ring++)
            {
                for (uint16_t seg = 0; seg < segments; seg++)
                {
                    uint16_t current = ring * segments + seg;
                    uint16_t next = ring * segments + ((seg + 1) % segments);
                    uint16_t currentUp = current + segments;
                    uint16_t nextUp = next + segments;

                    uint16_t indices[4] = { current, next, nextUp, currentUp };

                    SRL::Math::Types::Vector3D center = (
                        mesh.Vertices[current] +
                        mesh.Vertices[next] +
                        mesh.Vertices[nextUp] +
                        mesh.Vertices[currentUp]
                    ) / SRL::Math::Types::Fxp(4);

                    SRL::Math::Types::Vector3D normal = center.Normalize();

                    mesh.Faces[faceIndex] = Polygon(normal, indices);
                    mesh.Attributes[faceIndex] = Attribute(
                        Attribute::FaceVisibility::SingleSided,
                        Attribute::SortMode::Center,
                        0,
                        color,
                        0,
                        (uint16_t)(CL32KRGB | ECdis),
                        (uint32_t)Attribute::DisplayType::Polygon,
                        Attribute::DisplayOption::NoOption);

                    faceIndex++;
                }
            }

            return mesh;
        }

        /** @brief Create a cylinder mesh
         * @param radius Radius of the cylinder
         * @param height Half-height of the cylinder (distance from center to top/bottom)
         * @param segments Number of segments around the circumference (minimum 4)
         * @param color Color for all faces
         * @param capped Whether to include top and bottom cap faces
         * @return Mesh representing the cylinder
         */
        static Mesh CreateCylinder(const SRL::Math::Types::Fxp& radius, const SRL::Math::Types::Fxp& height, uint16_t segments, const SRL::Types::HighColor& color, bool capped = true)
        {
            if (segments < 4) segments = 4;

            size_t vertexCount = segments * 2;
            size_t faceCount = segments;

            if (capped)
            {
                vertexCount += 2;
                faceCount += segments * 2;
            }

            Mesh mesh(vertexCount, faceCount);

            SRL::Math::Types::Angle segmentStep = SRL::Math::Types::Angle::FromDegrees(360) / segments;

            for (uint16_t seg = 0; seg < segments; seg++)
            {
                SRL::Math::Types::Angle theta = segmentStep * seg;
                SRL::Math::Types::Fxp x = radius * theta.Cos();
                SRL::Math::Types::Fxp z = radius * theta.Sin();

                mesh.Vertices[seg] = SRL::Math::Types::Vector3D(x, -height, z);
                mesh.Vertices[seg + segments] = SRL::Math::Types::Vector3D(x, height, z);
            }

            if (capped)
            {
                mesh.Vertices[segments * 2] = SRL::Math::Types::Vector3D(0, -height, 0);
                mesh.Vertices[segments * 2 + 1] = SRL::Math::Types::Vector3D(0, height, 0);
            }

            size_t faceIndex = 0;

            for (uint16_t seg = 0; seg < segments; seg++)
            {
                uint16_t current = seg;
                uint16_t next = (seg + 1) % segments;
                uint16_t currentUp = current + segments;
                uint16_t nextUp = next + segments;

                uint16_t indices[4] = { current, next, nextUp, currentUp };

                SRL::Math::Types::Angle theta = segmentStep * seg + segmentStep / 2;
                SRL::Math::Types::Vector3D normal(theta.Cos(), 0, theta.Sin());

                mesh.Faces[faceIndex] = Polygon(normal, indices);
                mesh.Attributes[faceIndex] = Attribute(
                    Attribute::FaceVisibility::SingleSided,
                    Attribute::SortMode::Center,
                    0,
                    color,
                    0,
                    (uint16_t)(CL32KRGB | ECdis),
                    (uint32_t)Attribute::DisplayType::Polygon,
                    Attribute::DisplayOption::NoOption);

                faceIndex++;
            }

            if (capped)
            {
                uint16_t bottomCenter = segments * 2;
                uint16_t topCenter = segments * 2 + 1;

                for (uint16_t seg = 0; seg < segments; seg++)
                {
                    uint16_t current = seg;
                    uint16_t next = (seg + 1) % segments;

                    uint16_t bottomIndices[4] = { bottomCenter, next, current, bottomCenter };
                    SRL::Math::Types::Vector3D bottomNormal(0, -1, 0);

                    mesh.Faces[faceIndex] = Polygon(bottomNormal, bottomIndices);
                    mesh.Attributes[faceIndex] = Attribute(
                        Attribute::FaceVisibility::SingleSided,
                        Attribute::SortMode::Center,
                        0,
                        color,
                        0,
                        (uint16_t)(CL32KRGB | ECdis),
                        (uint32_t)Attribute::DisplayType::Polygon,
                        Attribute::DisplayOption::NoOption);

                    faceIndex++;
                }

                for (uint16_t seg = 0; seg < segments; seg++)
                {
                    uint16_t current = seg + segments;
                    uint16_t next = ((seg + 1) % segments) + segments;

                    uint16_t topIndices[4] = { topCenter, current, next, topCenter };
                    SRL::Math::Types::Vector3D topNormal(0, 1, 0);

                    mesh.Faces[faceIndex] = Polygon(topNormal, topIndices);
                    mesh.Attributes[faceIndex] = Attribute(
                        Attribute::FaceVisibility::SingleSided,
                        Attribute::SortMode::Center,
                        0,
                        color,
                        0,
                        (uint16_t)(CL32KRGB | ECdis),
                        (uint32_t)Attribute::DisplayType::Polygon,
                        Attribute::DisplayOption::NoOption);

                    faceIndex++;
                }
            }

            return mesh;
        }

        /** @brief Create a cone mesh
         * @param radius Radius of the cone base
         * @param height Half-height of the cone (distance from center to apex/base)
         * @param segments Number of segments around the circumference (minimum 4)
         * @param color Color for all faces
         * @param capped Whether to include bottom cap face
         * @return Mesh representing the cone
         */
        static Mesh CreateCone(const SRL::Math::Types::Fxp& radius, const SRL::Math::Types::Fxp& height, uint16_t segments, const SRL::Types::HighColor& color, bool capped = true)
        {
            if (segments < 4) segments = 4;

            size_t vertexCount = segments + 1;
            size_t faceCount = segments;

            if (capped)
            {
                vertexCount += 1;
                faceCount += segments;
            }

            Mesh mesh(vertexCount, faceCount);

            SRL::Math::Types::Angle segmentStep = SRL::Math::Types::Angle::FromDegrees(360) / segments;

            for (uint16_t seg = 0; seg < segments; seg++)
            {
                SRL::Math::Types::Angle theta = segmentStep * seg;
                SRL::Math::Types::Fxp x = radius * theta.Cos();
                SRL::Math::Types::Fxp z = radius * theta.Sin();

                mesh.Vertices[seg] = SRL::Math::Types::Vector3D(x, -height, z);
            }

            mesh.Vertices[segments] = SRL::Math::Types::Vector3D(0, height, 0);

            if (capped)
            {
                mesh.Vertices[segments + 1] = SRL::Math::Types::Vector3D(0, -height, 0);
            }

            size_t faceIndex = 0;

            for (uint16_t seg = 0; seg < segments; seg++)
            {
                uint16_t current = seg;
                uint16_t next = (seg + 1) % segments;
                uint16_t apex = segments;

                uint16_t indices[4] = { current, next, apex, apex };

                SRL::Math::Types::Angle theta = segmentStep * seg + segmentStep / 2;
                SRL::Math::Types::Fxp nx = theta.Cos();
                SRL::Math::Types::Fxp nz = theta.Sin();
                SRL::Math::Types::Vector3D normal = SRL::Math::Types::Vector3D(nx, radius / (height * 2), nz).Normalize();

                mesh.Faces[faceIndex] = Polygon(normal, indices);
                mesh.Attributes[faceIndex] = Attribute(
                    Attribute::FaceVisibility::SingleSided,
                    Attribute::SortMode::Center,
                    0,
                    color,
                    0,
                    (uint16_t)(CL32KRGB | ECdis),
                    (uint32_t)Attribute::DisplayType::Polygon,
                    Attribute::DisplayOption::NoOption);

                faceIndex++;
            }

            if (capped)
            {
                uint16_t bottomCenter = segments + 1;

                for (uint16_t seg = 0; seg < segments; seg++)
                {
                    uint16_t current = seg;
                    uint16_t next = (seg + 1) % segments;

                    uint16_t bottomIndices[4] = { bottomCenter, next, current, bottomCenter };
                    SRL::Math::Types::Vector3D bottomNormal(0, -1, 0);

                    mesh.Faces[faceIndex] = Polygon(bottomNormal, bottomIndices);
                    mesh.Attributes[faceIndex] = Attribute(
                        Attribute::FaceVisibility::SingleSided,
                        Attribute::SortMode::Center,
                        0,
                        color,
                        0,
                        (uint16_t)(CL32KRGB | ECdis),
                        (uint32_t)Attribute::DisplayType::Polygon,
                        Attribute::DisplayOption::NoOption);

                    faceIndex++;
                }
            }

            return mesh;
        }
    };
}

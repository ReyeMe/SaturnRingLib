#pragma once

#include "srl_base.hpp"
#include "srl_mesh.hpp"

namespace SRL::Types
{
    /** @brief Simple collision detection helpers
     */
    class Collision
    {
        Collision() = delete;
        ~Collision() = delete;

    public:

        /** @brief Result of a collision test
         */
        struct Result
        {
            bool Collided;
            SRL::Math::Types::Fxp Depth;
            SRL::Math::Types::Vector3D Normal;

            Result() : Collided(false), Depth(0), Normal() {}
            Result(bool collided) : Collided(collided), Depth(0), Normal() {}
            Result(bool collided, SRL::Math::Types::Fxp depth, SRL::Math::Types::Vector3D normal)
                : Collided(collided), Depth(depth), Normal(normal) {}

            operator bool() const { return Collided; }
        };

        /** @brief Create an AABB from center position and half-size
         * @param center Center position of the box
         * @param halfSize Half-size in each axis
         * @return AABB collision shape
         */
        static SRL::Math::Types::AABB CreateBox(const SRL::Math::Types::Vector3D& center, const SRL::Math::Types::Vector3D& halfSize)
        {
            return SRL::Math::Types::AABB(center, halfSize);
        }

        /** @brief Create an AABB from center position and uniform half-size
         * @param center Center position of the box
         * @param halfSize Half-size (same for all axes)
         * @return AABB collision shape
         */
        static SRL::Math::Types::AABB CreateBox(const SRL::Math::Types::Vector3D& center, const SRL::Math::Types::Fxp& halfSize)
        {
            return SRL::Math::Types::AABB(center, halfSize);
        }

        /** @brief Create a sphere from center position and radius
         * @param center Center position of the sphere
         * @param radius Radius of the sphere
         * @return Sphere collision shape
         */
        static SRL::Math::Types::Sphere CreateSphere(const SRL::Math::Types::Vector3D& center, const SRL::Math::Types::Fxp& radius)
        {
            return SRL::Math::Types::Sphere(center, radius);
        }

        /** @brief Test collision between two AABBs
         * @param a First box
         * @param b Second box
         * @return Collision result with penetration depth and normal
         */
        static Result TestBoxBox(const SRL::Math::Types::AABB& a, const SRL::Math::Types::AABB& b)
        {
            if (!a.IntersectsAABB(b))
            {
                return Result(false);
            }

            SRL::Math::Types::Vector3D aMin = a.GetMin();
            SRL::Math::Types::Vector3D aMax = a.GetMax();
            SRL::Math::Types::Vector3D bMin = b.GetMin();
            SRL::Math::Types::Vector3D bMax = b.GetMax();

            SRL::Math::Types::Fxp overlapX = SRL::Math::Types::Fxp::Min(aMax.X, bMax.X) - SRL::Math::Types::Fxp::Max(aMin.X, bMin.X);
            SRL::Math::Types::Fxp overlapY = SRL::Math::Types::Fxp::Min(aMax.Y, bMax.Y) - SRL::Math::Types::Fxp::Max(aMin.Y, bMin.Y);
            SRL::Math::Types::Fxp overlapZ = SRL::Math::Types::Fxp::Min(aMax.Z, bMax.Z) - SRL::Math::Types::Fxp::Max(aMin.Z, bMin.Z);

            SRL::Math::Types::Vector3D centerA = a.GetPosition();
            SRL::Math::Types::Vector3D centerB = b.GetPosition();

            if (overlapX <= overlapY && overlapX <= overlapZ)
            {
                SRL::Math::Types::Fxp sign = (centerA.X < centerB.X) ? SRL::Math::Types::Fxp(-1) : SRL::Math::Types::Fxp(1);
                return Result(true, overlapX, SRL::Math::Types::Vector3D(sign, 0, 0));
            }
            else if (overlapY <= overlapX && overlapY <= overlapZ)
            {
                SRL::Math::Types::Fxp sign = (centerA.Y < centerB.Y) ? SRL::Math::Types::Fxp(-1) : SRL::Math::Types::Fxp(1);
                return Result(true, overlapY, SRL::Math::Types::Vector3D(0, sign, 0));
            }
            else
            {
                SRL::Math::Types::Fxp sign = (centerA.Z < centerB.Z) ? SRL::Math::Types::Fxp(-1) : SRL::Math::Types::Fxp(1);
                return Result(true, overlapZ, SRL::Math::Types::Vector3D(0, 0, sign));
            }
        }

        /** @brief Test collision between two spheres
         * @param a First sphere
         * @param b Second sphere
         * @return Collision result with penetration depth and normal
         */
        static Result TestSphereSphere(const SRL::Math::Types::Sphere& a, const SRL::Math::Types::Sphere& b)
        {
            SRL::Math::Types::Vector3D delta = b.GetPosition() - a.GetPosition();
            SRL::Math::Types::Fxp distSq = delta.LengthSquared();
            SRL::Math::Types::Fxp radiusSum = a.GetRadius() + b.GetRadius();

            if (distSq > radiusSum * radiusSum)
            {
                return Result(false);
            }

            SRL::Math::Types::Fxp dist = delta.Length();

            if (dist > SRL::Math::Types::Fxp(0))
            {
                SRL::Math::Types::Vector3D normal = delta / dist;
                SRL::Math::Types::Fxp depth = radiusSum - dist;
                return Result(true, depth, normal);
            }
            else
            {
                return Result(true, radiusSum, SRL::Math::Types::Vector3D(1, 0, 0));
            }
        }

        /** @brief Test collision between a sphere and an AABB
         * @param sphere The sphere
         * @param box The box
         * @return Collision result with penetration depth and normal
         */
        static Result TestSphereBox(const SRL::Math::Types::Sphere& sphere, const SRL::Math::Types::AABB& box)
        {
            SRL::Math::Types::Vector3D closest = box.GetClosestPoint(sphere.GetPosition());
            SRL::Math::Types::Vector3D delta = sphere.GetPosition() - closest;
            SRL::Math::Types::Fxp distSq = delta.LengthSquared();
            SRL::Math::Types::Fxp radius = sphere.GetRadius();

            if (distSq > radius * radius)
            {
                return Result(false);
            }

            SRL::Math::Types::Fxp dist = delta.Length();

            if (dist > SRL::Math::Types::Fxp(0))
            {
                SRL::Math::Types::Vector3D normal = delta / dist;
                SRL::Math::Types::Fxp depth = radius - dist;
                return Result(true, depth, normal);
            }
            else
            {
                SRL::Math::Types::Vector3D boxCenter = box.GetPosition();
                SRL::Math::Types::Vector3D sphereCenter = sphere.GetPosition();
                SRL::Math::Types::Vector3D size = box.GetSize();

                SRL::Math::Types::Vector3D diff = sphereCenter - boxCenter;
                SRL::Math::Types::Fxp distX = size.X - diff.X.Abs();
                SRL::Math::Types::Fxp distY = size.Y - diff.Y.Abs();
                SRL::Math::Types::Fxp distZ = size.Z - diff.Z.Abs();

                if (distX <= distY && distX <= distZ)
                {
                    SRL::Math::Types::Fxp sign = (diff.X >= SRL::Math::Types::Fxp(0)) ? SRL::Math::Types::Fxp(1) : SRL::Math::Types::Fxp(-1);
                    return Result(true, distX + radius, SRL::Math::Types::Vector3D(sign, 0, 0));
                }
                else if (distY <= distX && distY <= distZ)
                {
                    SRL::Math::Types::Fxp sign = (diff.Y >= SRL::Math::Types::Fxp(0)) ? SRL::Math::Types::Fxp(1) : SRL::Math::Types::Fxp(-1);
                    return Result(true, distY + radius, SRL::Math::Types::Vector3D(0, sign, 0));
                }
                else
                {
                    SRL::Math::Types::Fxp sign = (diff.Z >= SRL::Math::Types::Fxp(0)) ? SRL::Math::Types::Fxp(1) : SRL::Math::Types::Fxp(-1);
                    return Result(true, distZ + radius, SRL::Math::Types::Vector3D(0, 0, sign));
                }
            }
        }

        /** @brief Test if a point is inside an AABB
         * @param point The point to test
         * @param box The box
         * @return True if point is inside the box
         */
        static bool TestPointBox(const SRL::Math::Types::Vector3D& point, const SRL::Math::Types::AABB& box)
        {
            return box.ContainsPoint(point);
        }

        /** @brief Test if a point is inside a sphere
         * @param point The point to test
         * @param sphere The sphere
         * @return True if point is inside the sphere
         */
        static bool TestPointSphere(const SRL::Math::Types::Vector3D& point, const SRL::Math::Types::Sphere& sphere)
        {
            return sphere.Contains(point);
        }
    };
}

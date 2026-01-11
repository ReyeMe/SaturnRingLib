#pragma once

#include "srl_base.hpp"
#include "srl_collision.hpp"

namespace SRL::Types
{
    /** @brief Simple rigidbody for physics simulation
     */
    class Rigidbody
    {
    public:
        /** @brief Position in world space
         */
        SRL::Math::Types::Vector3D Position;

        /** @brief Linear velocity
         */
        SRL::Math::Types::Vector3D Velocity;

        /** @brief Linear acceleration (gravity, etc.)
         */
        SRL::Math::Types::Vector3D Acceleration;

        /** @brief Mass of the body (0 = static/infinite mass)
         */
        SRL::Math::Types::Fxp Mass;

        /** @brief Inverse mass (cached for efficiency, 0 = static)
         */
        SRL::Math::Types::Fxp InverseMass;

        /** @brief Drag/friction coefficient (0-1, applied each frame)
         */
        SRL::Math::Types::Fxp Drag;

        /** @brief Bounciness/restitution coefficient (0 = no bounce, 1 = full bounce)
         */
        SRL::Math::Types::Fxp Restitution;

        /** @brief Whether this body is affected by physics
         */
        bool IsKinematic;

        /** @brief Construct a new Rigidbody
         * @param position Initial position
         * @param mass Mass of the body (0 = static)
         */
        Rigidbody(const SRL::Math::Types::Vector3D& position = SRL::Math::Types::Vector3D(),
                  const SRL::Math::Types::Fxp& mass = SRL::Math::Types::Fxp(1))
            : Position(position)
            , Velocity()
            , Acceleration()
            , Mass(mass)
            , Drag(SRL::Math::Types::Fxp(0.01))
            , Restitution(SRL::Math::Types::Fxp(0.5))
            , IsKinematic(false)
            , accumulatedForce()
        {
            if (mass > SRL::Math::Types::Fxp(0))
            {
                InverseMass = SRL::Math::Types::Fxp(1) / mass;
            }
            else
            {
                InverseMass = SRL::Math::Types::Fxp(0);
            }
        }

        /** @brief Set the mass of the body
         * @param mass New mass (0 = static/infinite mass)
         */
        void SetMass(const SRL::Math::Types::Fxp& mass)
        {
            Mass = mass;
            if (mass > SRL::Math::Types::Fxp(0))
            {
                InverseMass = SRL::Math::Types::Fxp(1) / mass;
            }
            else
            {
                InverseMass = SRL::Math::Types::Fxp(0);
            }
        }

        /** @brief Apply a force to the body (accumulated until next update)
         * @param force Force vector to apply
         */
        void ApplyForce(const SRL::Math::Types::Vector3D& force)
        {
            accumulatedForce = accumulatedForce + force;
        }

        /** @brief Apply an impulse to the body (immediate velocity change)
         * @param impulse Impulse vector to apply
         */
        void ApplyImpulse(const SRL::Math::Types::Vector3D& impulse)
        {
            Velocity = Velocity + impulse * InverseMass;
        }

        /** @brief Update the rigidbody physics
         * @param deltaTime Time step (typically use fixed value like 1/60)
         */
        void Update(const SRL::Math::Types::Fxp& deltaTime)
        {
            if (IsKinematic || InverseMass == SRL::Math::Types::Fxp(0))
            {
                accumulatedForce = SRL::Math::Types::Vector3D();
                return;
            }

            SRL::Math::Types::Vector3D forceAcceleration = accumulatedForce * InverseMass;
            SRL::Math::Types::Vector3D totalAcceleration = Acceleration + forceAcceleration;

            Velocity = Velocity + totalAcceleration * deltaTime;

            Velocity = Velocity * (SRL::Math::Types::Fxp(1) - Drag);

            Position = Position + Velocity * deltaTime;

            accumulatedForce = SRL::Math::Types::Vector3D();
        }

        /** @brief Get the kinetic energy of the body
         * @return Kinetic energy (0.5 * m * v^2)
         */
        SRL::Math::Types::Fxp GetKineticEnergy() const
        {
            return Mass * Velocity.LengthSquared() / SRL::Math::Types::Fxp(2);
        }

        /** @brief Get the momentum of the body
         * @return Momentum vector (m * v)
         */
        SRL::Math::Types::Vector3D GetMomentum() const
        {
            return Velocity * Mass;
        }

        /** @brief Check if the body is at rest (velocity near zero)
         * @param threshold Velocity threshold to consider at rest
         * @return True if velocity magnitude is below threshold
         */
        bool IsAtRest(const SRL::Math::Types::Fxp& threshold = SRL::Math::Types::Fxp(0.01)) const
        {
            return Velocity.LengthSquared() < threshold * threshold;
        }

        /** @brief Stop all movement
         */
        void Stop()
        {
            Velocity = SRL::Math::Types::Vector3D();
            accumulatedForce = SRL::Math::Types::Vector3D();
        }

        /** @brief Create an AABB collider at the rigidbody position
         * @param halfSize Half-size of the box
         * @return AABB at current position
         */
        SRL::Math::Types::AABB GetBoxCollider(const SRL::Math::Types::Vector3D& halfSize) const
        {
            return SRL::Math::Types::AABB(Position, halfSize);
        }

        /** @brief Create an AABB collider at the rigidbody position
         * @param halfSize Uniform half-size of the box
         * @return AABB at current position
         */
        SRL::Math::Types::AABB GetBoxCollider(const SRL::Math::Types::Fxp& halfSize) const
        {
            return SRL::Math::Types::AABB(Position, halfSize);
        }

        /** @brief Create a sphere collider at the rigidbody position
         * @param radius Radius of the sphere
         * @return Sphere at current position
         */
        SRL::Math::Types::Sphere GetSphereCollider(const SRL::Math::Types::Fxp& radius) const
        {
            return SRL::Math::Types::Sphere(Position, radius);
        }

    private:
        SRL::Math::Types::Vector3D accumulatedForce;
    };

    /** @brief Physics helper functions
     */
    class Physics
    {
        Physics() = delete;
        ~Physics() = delete;

    public:
        /** @brief Default gravity vector (negative Y)
         */
        static constexpr SRL::Math::Types::Fxp DefaultGravity = SRL::Math::Types::Fxp(-9.8);

        /** @brief Resolve collision between two rigidbodies
         * @param a First rigidbody
         * @param b Second rigidbody
         * @param collision Collision result from Collision::Test* functions
         */
        static void ResolveCollision(Rigidbody& a, Rigidbody& b, const Collision::Result& collision)
        {
            if (!collision.Collided)
            {
                return;
            }

            SRL::Math::Types::Fxp totalInverseMass = a.InverseMass + b.InverseMass;

            if (totalInverseMass == SRL::Math::Types::Fxp(0))
            {
                return;
            }

            SRL::Math::Types::Vector3D separation = collision.Normal * collision.Depth;
            a.Position = a.Position + separation * (a.InverseMass / totalInverseMass);
            b.Position = b.Position - separation * (b.InverseMass / totalInverseMass);

            SRL::Math::Types::Vector3D relativeVelocity = b.Velocity - a.Velocity;
            SRL::Math::Types::Fxp velocityAlongNormal = relativeVelocity.Dot(collision.Normal);

            if (velocityAlongNormal > SRL::Math::Types::Fxp(0))
            {
                return;
            }

            SRL::Math::Types::Fxp restitution = SRL::Math::Types::Fxp::Min(a.Restitution, b.Restitution);
            SRL::Math::Types::Fxp impulseMagnitude = -(SRL::Math::Types::Fxp(1) + restitution) * velocityAlongNormal;
            impulseMagnitude = impulseMagnitude / totalInverseMass;

            SRL::Math::Types::Vector3D impulse = collision.Normal * impulseMagnitude;
            a.Velocity = a.Velocity - impulse * a.InverseMass;
            b.Velocity = b.Velocity + impulse * b.InverseMass;
        }

        /** @brief Resolve collision between a rigidbody and a static surface
         * @param body The rigidbody
         * @param collision Collision result from Collision::Test* functions
         */
        static void ResolveCollisionStatic(Rigidbody& body, const Collision::Result& collision)
        {
            if (!collision.Collided || body.InverseMass == SRL::Math::Types::Fxp(0))
            {
                return;
            }

            body.Position = body.Position + collision.Normal * collision.Depth;

            SRL::Math::Types::Fxp velocityAlongNormal = body.Velocity.Dot(collision.Normal);

            if (velocityAlongNormal < SRL::Math::Types::Fxp(0))
            {
                SRL::Math::Types::Vector3D normalVelocity = collision.Normal * velocityAlongNormal;
                body.Velocity = body.Velocity - normalVelocity * (SRL::Math::Types::Fxp(1) + body.Restitution);
            }
        }

        /** @brief Apply gravity to a rigidbody
         * @param body The rigidbody
         * @param gravity Gravity acceleration (default: -9.8 on Y axis)
         */
        static void ApplyGravity(Rigidbody& body, const SRL::Math::Types::Fxp& gravity = DefaultGravity)
        {
            body.Acceleration.Y = gravity;
        }

        /** @brief Apply gravity to a rigidbody with custom direction
         * @param body The rigidbody
         * @param gravityVector Gravity acceleration vector
         */
        static void ApplyGravity(Rigidbody& body, const SRL::Math::Types::Vector3D& gravityVector)
        {
            body.Acceleration = gravityVector;
        }
    };
}

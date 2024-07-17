#pragma once

#include "srl_math.hpp"
#include "srl_vector.hpp"
#include "srl_angle.hpp"

namespace SRL
{
    /** @brief Geometry functions
     */
    class Geo
    {
    public:

        /** @brief PI value
         */
        static constexpr const SRL::Types::Fxp PI = 3.1415927;

        /** @brief Projects 3D point onto a screen from current transformation matrix
         * @param position Position in world space
         * @param result Position on screen
         * @return Fxp Distance from world space to screen space
         */
        static SRL::Types::Fxp ProjectToScreen(const SRL::Types::Vector3D& position, SRL::Types::Vector2D* result)
        {
            return SRL::Types::Fxp::FromRaw(slConvert3Dto2DFX((FIXED*)&position, (FIXED*)result));
        }

        /** @brief Check if point is on screen
         * @param point Point to test
         * @param size Size of the point
         * @return true if point is on screen
         */
        static bool IsOnScreen(const SRL::Types::Vector3D& point, const SRL::Types::Fxp size)
        {
            return slCheckOnScreen((FIXED*)&point, size.Value()) >= 0;
        }
    };
}

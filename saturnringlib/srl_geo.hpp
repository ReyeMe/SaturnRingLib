#pragma once

#include "srl_math.hpp"
#include "srl_vector.hpp"

namespace SRL
{
	/** @brief Geometry functions
	 */
	class Geo
	{
	public:
		
        /** @brief Projects 3D point onto a screen from current transformation matrix
         * @param position Position in world space
         * @param result Position on screen
         * @return Fxp Distance from world space to screen space
         */
        static SRL::Types::Fxp ProjectToScreen(const SRL::Types::Vector3D& position, SRL::Types::Vector2D* result)
        {
            return SRL::Types::Fxp::FromRaw(slConvert3Dto2DFX((FIXED*)&position, (FIXED*)result));
        }
	};
}

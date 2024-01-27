#pragma once

#include "srl_base.hpp"

namespace SRL::Types
{
	/** @brief Saturn color struct
	 */
	struct SaturnColor
	{
		/** @brief Color data
		 */
		Uint16 ARGB;

		/** @brief Defines new transparent color
		 */
		constexpr SaturnColor() : ARGB(0)
		{
			// Do nothing
		}
		
		/** @brief Defines new color from ARGB value
		 */
		constexpr SaturnColor(const Uint16& argb) : ARGB(argb)
		{
			// Do nothing
		}
		
		/** @brief Defines new color, components are in rage of 0-255 but are divided by 8
		 * @param r Red component
		 * @param g Red component
		 * @param b Red component
		 */
		constexpr SaturnColor(const Uint8& r, const Uint8& g, const Uint8& b) : ARGB(0x8000 | (((b >> 3) & 0x1f) << 10) | (((g >> 3) & 0x1f) << 5) | ((r >> 3) & 0x1f))
		{
			// Do nothing
		}
		
		/** @brief Is color transparent
		 * @return true 
		 * @return false 
		 */
		constexpr inline bool IsTransparent()
		{
			return this->ARGB & 0x8000 == 0;
		}
		
		/** @brief Is color transparent
		 * @return true 
		 * @return false 
		 */
		constexpr inline bool Red()
		{
			return this->ARGB & 0x1f;
		}
		
		/** @brief Is color transparent
		 * @return true 
		 * @return false 
		 */
		constexpr inline bool Green()
		{
			return (this->ARGB >> 5) & 0x1f;
		}
		
		/** @brief Is color transparent
		 * @return true 
		 * @return false 
		 */
		constexpr inline bool Blue()
		{
			return (this->ARGB >> 10) & 0x1f;
		}
		
	};
		
	/** @brief Predefined colors
	 */
	class Colors
	{
	public:
		/** @brief White color
		 */
		static constexpr Types::SaturnColor White = Types::SaturnColor(0xffff);
		
		/** @brief White color
		 */
		static constexpr Types::SaturnColor Black = Types::SaturnColor(0x8000);

		/** @brief Red color
		 */
		static constexpr Types::SaturnColor Red = Types::SaturnColor(255, 0, 0);
		
		/** @brief Green color
		 */
		static constexpr Types::SaturnColor Green = Types::SaturnColor(0, 255, 0);
		
		/** @brief Blue color
		 */
		static constexpr Types::SaturnColor Blue = Types::SaturnColor(0, 0, 255);
		
		/** @brief Yellow color
		 */
		static constexpr Types::SaturnColor Yellow = Types::SaturnColor(255, 255, 0);
		
		/** @brief Magenta color
		 */
		static constexpr Types::SaturnColor Magenta = Types::SaturnColor(255, 0, 255);
	};
	
}

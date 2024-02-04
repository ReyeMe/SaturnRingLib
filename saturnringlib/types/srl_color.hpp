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
		Uint16 Opaque : 1;
		Uint16 Blue : 5;
		Uint16 Green : 5;
		Uint16 Red : 5;

		/** @brief Defines new transparent color
		 */
		constexpr SaturnColor() : Opaque(0), Blue(0), Green(0), Red(0)
		{
			// Do nothing
		}
		
		/** @brief Defines new color from saturn ABGR555 value
		 * @param value VAlue in ABGR555
		 */
		constexpr SaturnColor(const Uint16& value) :
			Opaque((value >> 15) & 0x1),
			Blue((value >> 10) & 0x1f),
			Green((value >> 5) & 0x1f),
			Red(value & 0x1f)
		{
			// Do nothing
		}
		
		/** @brief Defines new color (components are in rage of 0-255 but are divided by 8)
		 * @param r Red component
		 * @param g Ggreen component
		 * @param b Blue component
		 */
		constexpr SaturnColor(const Uint8& r, const Uint8& g, const Uint8& b) :
			Opaque(1),
			Red((r >> 3) & 0x1f),
			Green((g >> 3) & 0x1f),
			Blue((b >> 3) & 0x1f)
		{
			// Do nothing
		}

		/** @brief Get color from RGB555 (components are in range o 0-31)
		 * @param r Red component
		 * @param g Ggreen component
		 * @param b Blue component
		 * @return constexpr SaturnColor 
		 */
		static SaturnColor FromRGB555(const Uint8& r, const Uint8& g, const Uint8& b)
		{
			return SaturnColor(r & 0x1f, g & 0x1f, b & 0x1f);
		}

		/** @brief Get from RGB555
		 * @param value Value in ARGB555 format
		 * @return SaturnColor color
		 */
		static SaturnColor FromARGB15(const Uint16& value)
		{
			return ((value >> 15) & 0x1) != 0 ? SaturnColor::FromRGB555((value >> 10) & 0x1f, (value >> 5) & 0x1f, value & 0x1f) : SaturnColor();
		}
		
		/** @brief Get from RGB888
		 * @param value Value in ARGB888 format
		 * @return SaturnColor color
		 */
		static SaturnColor FromRGB24(const Uint32& value)
		{
			return SaturnColor::FromRGB555(
				(((value >> 16) & 0xff) >> 3) & 0x1f,
				(((value >> 8) & 0xff) >> 3) & 0x1f,
				((value & 0xff) >> 3) & 0x1f);
		}
		
		/** @brief Get from ARGB888
		 * @param value Value in ARGB888 format
		 * @return SaturnColor color
		 */
		static SaturnColor FromARGB32(const Uint32 value)
		{
			if (value & 0xff000000 > 0)
			{
				return SaturnColor::FromRGB555(
					(((value >> 16) & 0xff) >> 3) & 0x1f,
					(((value >> 8) & 0xff) >> 3) & 0x1f,
					((value & 0xff) >> 3) & 0x1f);
			}

			return SaturnColor();
		}
		
		/** @brief Get color represented as ABGR555 value
		 * @return Single ABGR555 value
		 */
		constexpr inline Uint16 GetABGR()
		{
			return *((Uint16*)this);
		}
		
		// This is automatically called 
		// when '=' operator is 
		// used between C1 and C2. 
	
		constexpr inline void operator=(const Uint16& value) 
		{ 
			this->Opaque = (value >> 15) & 0x1;
			this->Red = value & 0x1f;
			this->Green = (value >> 5) & 0x1f;
			this->Blue = (value >> 10) & 0x1f; 
		} 
  
		/** @brief Cast to ABGR555
		 * @return ABGR value
		 */
    	constexpr inline operator Uint16&() { return *((Uint16*)this); }
		
		/** @brief Cast to ABGR555
		 * @return ABGR value
		 */
    	constexpr inline operator Uint16() const { return *((Uint16*)this); }
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

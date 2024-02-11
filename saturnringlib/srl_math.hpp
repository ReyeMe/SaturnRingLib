#pragma once

#include "srl_base.hpp"
#include "srl_fxp.hpp"

namespace SRL
{
	/** @brief Math functions
	 */
	class Math
	{
	public:

		/// @brief Get absolute value
		/// @tparam ValueType Type of the value
		/// @param value Numberic value
		/// @return Absolute value
		template<typename ValueType>
		constexpr static ValueType Abs(const ValueType& value)
		{
			return value < 0 ? -value : value;
		}

		/// @brief Get maximum value of two values
		/// @tparam ValueType Type of the value
		/// @param first First value
		/// @param second Second value
		/// @return Maximum value
		template<typename ValueType>
		constexpr static ValueType Max(const ValueType& first, const ValueType& second)
		{
			return first > second ? first : second;
		}

		/// @brief Get minimum value of two values
		/// @tparam ValueType Type of the value
		/// @param first First value
		/// @param second Second value
		/// @return Minimum value
		template<typename ValueType>
		constexpr static ValueType Min(const ValueType& first, const ValueType& second)
		{
			return first < second ? first : second;
		}

		/// @brief Clamp value in the range
		/// @tparam ValueType Type of the value
		/// @param minimum Minimum of the range
		/// @param maximum Maximum of the range
		/// @param value Value to clamp
		/// @return Clamped value
		template<typename ValueType>
		constexpr static ValueType Clamp(const ValueType& minimum, const ValueType& maximum, const ValueType& value)
		{
			return Math::Min(maximum, Math::Max(minimum, value));
		}
	};
}
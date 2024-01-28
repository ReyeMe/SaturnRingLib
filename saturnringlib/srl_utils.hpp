#pragma once

#include "srl_base.hpp"
#include "types/srl_color.hpp"
#include "srl_vdp2.hpp"
#include "srl_memory.hpp"
#include <std/string.h>

/** @brief Utility functions
 */
namespace SRL::Utils
{
	/** @brief Endianity helpers
	 */
	namespace Endianity
	{
		/** @brief Swap endianity
		 * @tparam ValueType Type of the value
		 * @param value Value to swap
		 * @return ValueType 
		 */
		template<typename ValueType>
		inline static ValueType Swap(const ValueType& value)
		{
			ValueType result = 0;
			ValueType original = value;
			Sint32 size = sizeof(ValueType);

			while (size-- > 0)
			{
				result <<= 8;
				result = result | (original & 0xff);
				original >>= 8;
			}
			
			return result;
		}
	};

	/** @brief Structure data loader by dannyduarte
	 */
	namespace StructLoader
	{
		/** @brief Check whether struct contains size function
		 * @tparam T Struct type
		 */
		template<typename T>
		concept HasLoadSizeFunction = requires {
			{ std::declval<T>().LoadSize() } -> std::same_as<size_t>;
		};

		/** @brief Get struct pointer and iterate buffer pointer
		 * @tparam T Struct type
		 * @param iterator Data buffer
		 * @param count Struct size
		 * @return T* Struct pointer
		 */
		template<typename T>
		T* GetAndIterate(char*& iterator, size_t count = 1)
		{
			T* ptr = reinterpret_cast<T*>(iterator);

			if constexpr (HasLoadSizeFunction<T>)
			{
				iterator += ptr->LoadSize() * count;
			}
			else
			{
				iterator += (sizeof(T) * count);
			}

			return ptr;
		}
	}
}
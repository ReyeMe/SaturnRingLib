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
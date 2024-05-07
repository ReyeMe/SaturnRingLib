#pragma once

#include "srl_base.hpp"

extern "C" {
	extern char _heap_start;
	extern char _heap_end;
}

#include <tlsf.h>
#include <stdlib.h>

namespace hwram
{
	namespace internal
	{
		// Dont mess with this directly!
		inline tlsf_t pool;// = tlsf_pool_create((void*)&_heap_start, reinterpret_cast<size_t>(&_heap_end) - reinterpret_cast<size_t>(&_heap_start));

		inline bool in_range(void* ptr)
		{
			return (ptr >= (void*)&_heap_start && ptr <= (char*)&_heap_end);
		}
	}

	inline void free(void* ptr)
	{
		tlsf_free(internal::pool, ptr);
	}

	inline void* malloc(size_t size)
	{
		return tlsf_malloc(internal::pool, size);
	}

	inline void* realloc(void* ptr, size_t size)
	{
		return tlsf_realloc(internal::pool, ptr, size);
	}
}

namespace lwram
{
	namespace internal
	{
		// Dont mess with this directly!
		constexpr size_t lwram_start = 0x00200000;
		constexpr size_t lwram_size = 0x100000;
		inline tlsf_t const pool = tlsf_pool_create((void*)lwram_start, lwram_size);

		inline bool in_range(void* ptr)
		{
			return (ptr >= (void*)lwram_start && ptr <= (char*)lwram_start + lwram_size);
		}
	}

	inline void free(void* ptr)
	{
		tlsf_free(internal::pool, ptr);
	}

	inline void* malloc(size_t size)
	{
		return tlsf_malloc(internal::pool, size);
	}

	inline void* realloc(void* ptr, size_t size)
	{
		return tlsf_realloc(internal::pool, ptr, size);
	}
}

namespace SRL
{
	/** @brief Dynamic memory management
	 */
	class Memory
	{
	public:

		/** @brief Gets all available free memory
		 * @return Number of bytes free
		 */
		inline static size_t GetAvailableMemory()
		{
			/* todo */
			return 0;
		}

		/** @brief Initialize memory
		 */
		inline static void Initialize()
		{
			hwram::internal::pool = tlsf_pool_create((void*)&_heap_start, reinterpret_cast<size_t>(&_heap_end) - reinterpret_cast<size_t>(&_heap_start));
		}
	};
}

inline void* operator new(size_t size)
{
	return hwram::malloc(size);
}

inline void operator delete(void* ptr)
{
	hwram::free(ptr);
}

inline void operator delete(void* ptr, unsigned int)
{
	hwram::free(ptr);
}

inline void* operator new(size_t, void* ptr)
{
	return ptr;
}

// Add overload for array form of new operator
inline void* operator new[](size_t size)
{
	return hwram::malloc(size);
}

// Add overload for array form of delete operator
inline void operator delete[](void* ptr)
{
	hwram::free(ptr);
}

// Add overload for array form of delete operator with size parameter
inline void operator delete[](void* ptr, size_t size)
{
	hwram::free(ptr);
}

// Add overload for array form of placement new
inline void* operator new[](size_t, void* ptr)
{
	return ptr;
}


#pragma once

#include "srl_base.hpp"

extern "C" {
    extern char _heap_start;
    extern char _heap_end;
}

#include <tlsf.h>
#include <stdlib.h>

namespace SRL
{
    /** @brief Dynamic memory management
     * @details This class wraps implementation of TLSF by Matthew Conte http://tlsf.baisoku.org
     */
    class Memory
    {
    private:
        /** @brief Main system memory region size
         * @note This is assigned on system initialization
         */
        inline static size_t mainWorkRamSize;

        /** @brief Main system memory region start address
         * @note This is assigned on system initialization
         */
        inline static tlsf_t mainWorkRam;

    public:

        /** @brief Memory zone codes
         */
        enum Zone
        {
            /** @brief High (main) system RAM
             */
            HWRam = 0,

            /** @brief Low system RAM
             */
            LWRam = 1,

            /** @brief Expansion cart RAM
             */
            CartRam = 2
        };

        /** @brief Malloc for main system RAM
         */
        class HighWorkRam
        {
        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(void* ptr)
            {
                return (ptr >= (void*)&_heap_start && ptr <= (char*)&_heap_end);
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            inline static void Free(void* ptr)
            {
                tlsf_free(Memory::mainWorkRam, ptr);
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            inline static void* Malloc(size_t size)
            {
                return tlsf_malloc(Memory::mainWorkRam, size);
            }

            /** @brief Reallocate existing memory
             * @param ptr Pointer to the existing allocated memory
             * @param size New size in number of bytes that should be allocated
             * @return Pointer to the allocated space in memory
             */
            inline static void* Realloc(void* ptr, size_t size)
            {
                return tlsf_realloc(Memory::mainWorkRam, ptr, size);
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetUsedSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetFreeSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            inline static size_t GetSize()
            {
                return Memory::mainWorkRamSize;
            }
        };

        /** @brief Malloc for slower system RAM
         */
        class LowWorkRam
        {
        private:
            
            /** @brief Zone start
             */
            constexpr inline static size_t Start = 0x00200000;

            /** @brief Zone size
             */
            constexpr inline static size_t Size = 0x100000;

            /** @brief Memory zone
             */
            inline static tlsf_t const zone = tlsf_create_with_pool((void*)LowWorkRam::Start, LowWorkRam::Size);

        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(void* ptr)
            {
                return (ptr >= (void*)LowWorkRam::Start && ptr <= (char*)LowWorkRam::Start + LowWorkRam::Size);
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            inline static void Free(void* ptr)
            {
                tlsf_free(LowWorkRam::zone, ptr);
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            inline static void* Malloc(size_t size)
            {
                return tlsf_malloc(LowWorkRam::zone, size);
            }

            /** @brief Reallocate existing memory
             * @param ptr Pointer to the existing allocated memory
             * @param size New size in number of bytes that should be allocated
             * @return Pointer to the allocated space in memory
             */
            inline static void* Realloc(void* ptr, size_t size)
            {
                return tlsf_realloc(LowWorkRam::zone, ptr, size);
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetUsedSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetFreeSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            inline static size_t GetSize()
            {
                return LowWorkRam::Size;
            }
        };

        /** @brief Malloc for expansion cart RAM
         */
        class CartRam
        {
        public:

            /** @brief Check whether pointer is in range of the memory zone
             * @param ptr Pointer to check
             * @return true if pointer belongs to the current memory zone
             */
            inline static bool InRange(void* ptr)
            {
                return false;
            }

            /** @brief Free allocated memory
             * @param ptr Pointer to allocated memory
             */
            inline static void Free(void* ptr)
            {
                // TODO implement
            }

            /** @brief Allocate some memory
             * @param size Number of bytes to allocate
             * @return Pointer to the allocated space in memory
             */
            inline static void* Malloc(size_t size)
            {
                return nullptr;
            }

            /** @brief Reallocate existing memory
             * @param ptr Pointer to the existing allocated memory
             * @param size New size in number of bytes that should be allocated
             * @return Pointer to the allocated space in memory
             */
            inline static void* Realloc(void* ptr, size_t size)
            {
                return nullptr;
            }
            
            /** @brief Gets total size of the used space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetUsedSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the free space in the memory zone
             * @return Number of bytes
             */
            inline static size_t GetFreeSpace()
            {
                return 0;
            }

            /** @brief Gets total size of the memory zone
             * @return Number of bytes
             */
            inline static size_t GetSize()
            {
                return 0;
            }
        };

        /** @brief Initialize memory
         */
        inline static void Initialize()
        {
            Memory::mainWorkRamSize = reinterpret_cast<size_t>(&_heap_end) - reinterpret_cast<size_t>(&_heap_start);
            Memory::mainWorkRam = tlsf_create_with_pool((void*)&_heap_start, Memory::mainWorkRamSize);

            // TODO: Initialize Cart RAM here
        }

        /** @brief Gets total size of the used space in the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetUsedSpace(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return 0;
            
            case Zone::LWRam:
                return 0;
            
            case Zone::CartRam:
                return CartRam::GetUsedSpace();
            
            default:
                return 0;
            }
        }

        /** @brief Gets total size of the free space in the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetFreeSpace(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return 0;
            
            case Zone::LWRam:
                return 0;
            
            case Zone::CartRam:
                return CartRam::GetFreeSpace();
            
            default:
                return 0;
            }
        }

        /** @brief Gets total size of the memory zone
         * @param zone Memory zone
         * @return Number of bytes
         */
        inline static size_t GetSize(const Zone zone)
        {
            switch (zone)
            {
            case Zone::HWRam:
                return 0;
            
            case Zone::LWRam:
                return 0;
            
            case Zone::CartRam:
                return CartRam::GetSize();
            
            default:
                return 0;
            }
        }

        /** @brief Free allocated memory from any memory zone
         * @param ptr Pointer to allocated memory
         */
        inline static void Free(void* ptr)
        {
            if (HighWorkRam::InRange(ptr))
            {
                HighWorkRam::Free(ptr);
            }
            else if (LowWorkRam::InRange(ptr))
            {
                LowWorkRam::Free(ptr);
            }
            else if (CartRam::InRange(ptr))
            {
                CartRam::Free(ptr);
            } 
        }
    };
}

/** @brief New keyword for expansion cartridge RAM
 */
#define cartnew new (SRL::Memory::Zone::CartRam)

/** @brief New keyword for low work RAM
 */
#define lwnew new (SRL::Memory::Zone::LWRam)

/** @brief New keyword for high work RAM 
 */
#define hwnew new

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new(size_t size)
{
    return SRL::Memory::HighWorkRam::Malloc(size);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @param tone Memory zone
 * @return Pointer to the allocated space in memory
 */
inline void* operator new(size_t size, const SRL::Memory::Zone zone)
{
    switch (zone)
    {
    case SRL::Memory::Zone::CartRam:
        return SRL::Memory::CartRam::Malloc(size);
    
    case SRL::Memory::Zone::LWRam:
        return SRL::Memory::LowWorkRam::Malloc(size);
    
    default:
        return SRL::Memory::HighWorkRam::Malloc(size);
    }  
}

/** @brief Free allocated memory
 * @param ptr Pointer to allocated memory
 */
inline void operator delete(void* ptr)
{
    SRL::Memory::Free(ptr);
}

/** @brief Free allocated memory
 * @param ptr Pointer to allocated memory
 * @param size Number of bytes to free (Not used)
 */
inline void operator delete(void* ptr, size_t size)
{
    SRL::Memory::Free(ptr);
}

/** @brief Calls appropriate constructor on a specific pre-allocated region
 * @param size Size of the object in that location
 * @param ptr Target location
 * @return New pointer to specified location
 * @section Example
 * @code
 * char buffer[100];
 * int* p = new (buffer) int(42); // placement new
 * @endcode
 */
inline void* operator new(size_t size, void* ptr)
{
    return ptr;
}

/** @brief Allocate some memory from array
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new[](size_t size)
{
    return SRL::Memory::HighWorkRam::Malloc(size);
}

/** @brief Allocate some memory
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated space in memory
 */
inline void* operator new[](size_t size, const SRL::Memory::Zone zone)
{
    switch (zone)
    {
    case SRL::Memory::Zone::CartRam:
        return SRL::Memory::CartRam::Malloc(size);
    
    case SRL::Memory::Zone::LWRam:
        return SRL::Memory::LowWorkRam::Malloc(size);
    
    default:
        return SRL::Memory::HighWorkRam::Malloc(size);
    }  
}

/** @brief Free allocated array from memory
 * @param ptr Pointer to allocated memory
 */
inline void operator delete[](void* ptr)
{
    SRL::Memory::Free(ptr);
}

/** @brief Free allocated array from memory
 * @param ptr Pointer to allocated memory
 * @param size Number of bytes to free (Not used)
 */
inline void operator delete[](void* ptr, size_t size)
{
    SRL::Memory::Free(ptr);
}

/** @brief Create new pointer to a specific location
 * @param size Number of bytes to allocate
 * @param ptr Target location
 * @return New pointer to specified location
 */
inline void* operator new[](size_t size, void* ptr)
{
    return ptr;
}


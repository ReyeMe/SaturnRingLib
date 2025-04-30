#pragma once
/**
 * @file srl_cpu.hpp
 * @brief SH-2 CPU Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn SH-2 CPU hardware registers
 * and cache management functionality. The SH-2 CPU is the main processor of the
 * Sega Saturn and provides various hardware registers for controlling cache,
 * interrupts, and other CPU functions.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"

namespace SRL::Hardware::CPU
{
    using namespace MemoryRegions;

    //----------------------------------------------------------------------
    // CPU Base Address
    //----------------------------------------------------------------------
    static inline constexpr std::uintptr_t CpuBase = 0xFFFFFF00UL;

    //----------------------------------------------------------------------
    // Cache Control Registers
    //----------------------------------------------------------------------
    static inline constexpr Reg8 CacheControl { CpuBase, 0xE92UL };  // Cache Control Register (CCR)

    /**
     * @brief Cache control register bit definitions
     */
    namespace CacheControlBits
    {
        static inline constexpr std::uint8_t CacheEnable = 0x01;               // Bit 0: Cache enable
        static inline constexpr std::uint8_t CacheDisable = 0x00;              // Bit 0: Cache disable
        static inline constexpr std::uint8_t InstructionCacheDisable = 0x02;   // Bit 1: Instruction cache disable
        static inline constexpr std::uint8_t DataCacheDisable = 0x04;          // Bit 2: Data cache disable
        static inline constexpr std::uint8_t HybridModeBit = 0x04;             // Bit 2: Hybrid mode bit
        static inline constexpr std::uint8_t TwoWayBit = 0x08;                 // Bit 3: Two-way mode bit

        // Bit mask constants for cache control
        static inline constexpr std::uint8_t CacheEnableMask = 0xFE;           // Mask to preserve all bits except cache enable (bit 0)
        static inline constexpr std::uint8_t InstructionCacheMask = 0xFD;      // Mask to preserve all bits except instruction cache bit (bit 1)
        static inline constexpr std::uint8_t DataCacheMask = 0xFB;             // Mask to preserve all bits except data cache bit (bit 2)
        static inline constexpr std::uint8_t HybridModeMask = 0xFB;            // Mask to preserve all bits except hybrid mode bit (bit 2)
        static inline constexpr std::uint8_t TwoWayMask = 0xF7;                // Mask to preserve all bits except two-way mode bit (bit 3)
        static inline constexpr std::uint8_t ModeMask = 0xF3;                  // Mask to preserve all bits except mode bits (bits 2-3)
    }

    /**
     * @brief Cache aliasing options
     * 
     * These constants define the SH-2 CPU cache alias areas that control how memory is accessed
     * in relation to the cache. Each area has specific behavior regarding cache usage.
     */
    namespace CacheAlias
    {
        static inline constexpr std::uint32_t Cached = 0x00000000;       // Cache area - Cache is used when the CE bit in CCR is 1
        static inline constexpr std::uint32_t CacheThrough = 0x20000000; // Cache-through area - Cache is not used
        static inline constexpr std::uint32_t Purge = 0x40000000;        // Associative purge area - Cache line of the specified address is purged
        static inline constexpr std::uint32_t AddressArray = 0x60000000; // Address array read/write area - Cache address array is accessed directly
        static inline constexpr std::uint32_t DataArray = 0xC0000000;    // Data array read/write area - Cache data array is accessed directly
        static inline constexpr std::uint32_t IO = 0xE0000000;           // I/O area - Cache is not used
    }

    /**
     * @brief Cache organization modes
     */
    namespace CacheModes
    {
        static inline constexpr std::uint8_t FourWay = 0x00;  // 4-way associative cache (64 sets × 4 ways × 16 bytes)
        static inline constexpr std::uint8_t Hybrid = 0x04;   // Hybrid mode: 2KB 2-way cache + 2KB scratch pad
        static inline constexpr std::uint8_t TwoWay = 0x08;   // 2-way associative cache (128 sets × 2 ways × 16 bytes)
    }

    // Cache line size in bytes
    static inline constexpr size_t CacheLineSize = 16;

    /*****************************************************************************
     * Cache Management
     ****************************************************************************/

     /**
      * @brief Class to manage the SH-2 CPU cache control.
      *
      * This class provides functionality to initialize the cache, purge cache lines,
      * clear cache, manipulate the cache control register, and manage cache aliasing.
      * It supports both instruction and data caches, and allows control over cache modes.
      */
    class Cache
    {
    private:
        /**
         * @brief Apply a mask to the cache control register.
         *
         * This is a utility function to simplify bit manipulation operations
         * on the cache control register. It applies the mask to preserve bits
         * and then applies the value to set specific bits.
         *
         * @param mask The bit mask to apply (1 = preserve bit, 0 = apply new value).
         * @param value The new value to apply where mask bits are 0.
         */
        static inline void ApplyMask(std::uint8_t mask, std::uint8_t value)
        {
            CacheControl = (CacheControl & mask) | value;
        }

        /**
         * @brief Helper method to clear all cache lines across all ways.
         *
         * This is an internal method used by PurgeAll and Initialize
         * to avoid code duplication.
         */
        static inline void ClearCacheLines()
        {
            bool cacheWasEnabled = IsCacheEnabled();
            
            // Disable cache before purging
            if (cacheWasEnabled) {
                SetCacheEnabled(false);
            }
            
            // In full scratch pad mode, no cache to clear
            if (IsFullScratchpadMode()) {
                if (cacheWasEnabled) {
                    SetCacheEnabled(true);
                }
                return;
            }

            CacheMode currentMode = GetCacheMode();
            size_t wayCount = GetWayCount();
            size_t setCount = GetSetCount();
            
            // Access the cache through direct data array access
            for (std::uint32_t set = 0; set < setCount; ++set) {
                for (std::uint32_t way = 0; way < wayCount; ++way) {
                    // Calculate address for this set
                    // Use the data array access to access the cache directly
                    std::uint32_t* addr = Alias<std::uint32_t>(
                        reinterpret_cast<void*>(set * CacheLineSize), 
                        CacheAliasType::DataArray);
                    
                    // Clear the entire line (4 words of 4 bytes each = 16 bytes)
                    for (std::uint32_t i = 0; i < (CacheLineSize / sizeof(std::uint32_t)); ++i) {
                        addr[i] = 0;
                    }
                }
            }
            
            // Restore cache state if it was enabled
            if (cacheWasEnabled) {
                SetCacheEnabled(true);
            }
        }

    public:
        /**
         * @brief Enum for Cache Organization modes.
         *
         * These modes define the cache organization (associativity and line count).
         */
        enum class CacheMode : std::uint8_t
        {
            FourWay = CacheModes::FourWay,  /**< 4-way associative cache (64 sets × 4 ways × 16 bytes) */
            Hybrid = CacheModes::Hybrid,     /**< Hybrid mode: 2KB 2-way cache + 2KB scratch pad */
            TwoWay = CacheModes::TwoWay      /**< 2-way associative cache (128 sets × 2 ways × 16 bytes) */
        };

        /**
         * @brief Enum for cache aliasing options.
         * 
         * These constants define the SH-2 CPU cache alias areas that control how memory is accessed
         * in relation to the cache. Each area has specific behavior regarding cache usage.
         */
        enum class CacheAliasType : std::uint32_t
        {
            Cached = CacheAlias::Cached,            /**< Cache area - Cache is used when the CE bit in CCR is 1 */
            CacheThrough = CacheAlias::CacheThrough, /**< Cache-through area - Cache is not used */
            Purge = CacheAlias::Purge,              /**< Associative purge area - Cache line of the specified address is purged */
            AddressArray = CacheAlias::AddressArray, /**< Address array read/write area - Cache address array is accessed directly */
            DataArray = CacheAlias::DataArray,      /**< Data array read/write area - Cache data array is accessed directly */
            IO = CacheAlias::IO                     /**< I/O area - Cache is not used */
        };

        /**
         * @brief Get the current cache organization mode.
         */
        static inline CacheMode GetCacheMode()
        {
            std::uint8_t modeBits = CacheControl & (CacheControlBits::HybridModeBit | CacheControlBits::TwoWayBit);
            
            if (modeBits == CacheControlBits::HybridModeBit) {
                return CacheMode::Hybrid;
            } else if (modeBits == CacheControlBits::TwoWayBit) {
                return CacheMode::TwoWay;
            } else {
                return CacheMode::FourWay;
            }
        }

        /**
         * @brief Check if the cache is enabled.
         */
        static inline bool IsCacheEnabled()
        {
            return (CacheControl & CacheControlBits::CacheEnable) != 0;
        }

        /**
         * @brief Enable or disable the cache.
         * 
         * @param enable True to enable the cache, false to disable it.
         */
        static inline void SetCacheEnabled(bool enable)
        {
            ApplyMask(CacheControlBits::CacheEnableMask, 
                    enable ? CacheControlBits::CacheEnable : CacheControlBits::CacheDisable);
        }

        /**
         * @brief Check if the cache is in full scratchpad mode.
         * 
         * In full scratchpad mode, the entire unified cache RAM is used as a general-purpose RAM
         * rather than as a cache.
         */
        static inline bool IsFullScratchpadMode()
        {
            // Full scratchpad mode: Two-way bit is set (bit 3) and cache is disabled (bit 0)
            return ((CacheControl & CacheControlBits::TwoWayBit) != 0) && !IsCacheEnabled();
        }

        /**
         * @brief Check if the cache is in hybrid mode.
         * 
         * In hybrid mode, half of the cache (2KB) is used as a cache and the other half (2KB)
         * is used as scratchpad RAM.
         */
        static inline bool IsHybridMode()
        {
            return GetCacheMode() == CacheMode::Hybrid;
        }

        /**
         * @brief Set the cache to operate in full scratchpad mode.
         * 
         * In full scratchpad mode, the entire unified cache RAM is used as a general-purpose RAM
         * rather than as a cache.
         * 
         * @param enable True to enable scratchpad mode, false to disable it.
         */
        static inline void SetFullScratchpadMode(bool enable)
        {
            if (enable) {
                // To enter scratchpad mode: Set two-way bit and disable cache
                ApplyMask(CacheControlBits::TwoWayMask, CacheControlBits::TwoWayBit);
                ApplyMask(CacheControlBits::CacheEnableMask, CacheControlBits::CacheDisable);
            } else {
                // To exit scratchpad mode: Just enable the cache
                ApplyMask(CacheControlBits::CacheEnableMask, CacheControlBits::CacheEnable);
            }
        }

        /**
         * @brief Get the number of cache lines available for the current mode.
         */
        static inline size_t GetCacheLineCount()
        {
            CacheMode currentMode = GetCacheMode();
            
            switch (currentMode) {
                case CacheMode::FourWay:
                    return 256; // 64 sets × 4 ways
                case CacheMode::TwoWay:
                    return 256; // 128 sets × 2 ways
                case CacheMode::Hybrid:
                    return 128; // Half is cache: 64 sets × 2 ways
                default:
                    return 0;
            }
        }

        /**
         * @brief Get the number of sets for the current mode.
         */
        static inline size_t GetSetCount()
        {
            CacheMode currentMode = GetCacheMode();
            
            switch (currentMode) {
                case CacheMode::FourWay:
                    return 64;  // 4-way has 64 sets
                case CacheMode::TwoWay:
                    return 128; // 2-way has 128 sets
                case CacheMode::Hybrid:
                    return 64;  // Hybrid has 64 sets
                default:
                    return 0;
            }
        }

        /**
         * @brief Get the number of ways for the current mode.
         */
        static inline size_t GetWayCount()
        {
            CacheMode currentMode = GetCacheMode();
            
            switch (currentMode) {
                case CacheMode::FourWay:
                    return 4;  // 4-way associative
                case CacheMode::TwoWay:
                    return 2;  // 2-way associative
                case CacheMode::Hybrid:
                    return 2;  // Hybrid is 2-way
                default:
                    return 0;
            }
        }

        /**
         * @brief Set the cache organization mode.
         * 
         * @param mode The cache organization mode to set.
         */
        static inline void SetCacheMode(CacheMode mode)
        {
            // Cache mode change requires disabling the cache first
            bool wasEnabled = IsCacheEnabled();
            if (wasEnabled) {
                SetCacheEnabled(false);
            }
            
            // Clear all mode bits first
            std::uint8_t value = CacheControl & CacheControlBits::ModeMask;
            
            // Set the appropriate mode bits
            switch (mode) {
                case CacheMode::FourWay:
                    // Four-way mode is all bits clear
                    break;
                    
                case CacheMode::Hybrid:
                    value |= CacheControlBits::HybridModeBit;
                    break;
                    
                case CacheMode::TwoWay:
                    value |= CacheControlBits::TwoWayBit;
                    break;
            }
            
            // Apply the new mode
            CacheControl = value;
            
            // Clear all cache lines after mode change
            ClearCacheLines();
            
            // Restore cache enabled state
            if (wasEnabled) {
                SetCacheEnabled(true);
            }
        }

        /**
         * @brief Initialize the cache with the specified mode.
         * 
         * This method initializes the cache with the specified organization mode
         * and enables or disables the cache as requested.
         * 
         * @param mode The cache organization mode to set.
         * @param enable Whether to enable the cache after initialization.
         */
        static inline void Initialize(CacheMode mode = CacheMode::FourWay, bool enable = true)
        {
            // Disable cache during initialization
            SetCacheEnabled(false);
            
            // Set the cache mode
            SetCacheMode(mode);
            
            // Clear all cache lines
            ClearCacheLines();
            
            // Enable cache if requested
            if (enable) {
                SetCacheEnabled(true);
            }
        }

        /**
         * @brief Purge a specific cache line.
         * 
         * This method purges the cache line that contains the specified address.
         * 
         * @param address The address whose cache line should be purged.
         */
        static inline void PurgeLine(void* address)
        {
            // Use the purge area to purge the cache line
            volatile std::uint32_t* purgeAddr = Alias<std::uint32_t>(address, CacheAliasType::Purge);
            *purgeAddr; // Read to trigger purge
        }

        /**
         * @brief Purge all cache lines.
         * 
         * This method purges all cache lines by clearing them directly.
         */
        static inline void PurgeAll()
        {
            ClearCacheLines();
        }

        /**
         * @brief Check if an address is eligible for instruction cache.
         * 
         * This method checks if the specified address can be cached in the instruction cache.
         * 
         * @param address The address to check.
         * @return True if the address can be cached in the instruction cache.
         */
        static inline bool IsInstructionCacheEligible(void* address)
        {
            // Check if instruction cache is enabled
            if ((CacheControl & CacheControlBits::InstructionCacheDisable) != 0) {
                return false;
            }
            
            // Check if address is in a cacheable region
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(address);
            std::uint32_t region = addr & 0xE0000000;
            
            return region == CacheAlias::Cached;
        }

        /**
         * @brief Check if an address is eligible for data cache.
         * 
         * This method checks if the specified address can be cached in the data cache.
         * 
         * @param address The address to check.
         * @return True if the address can be cached in the data cache.
         */
        static inline bool IsDataCacheEligible(void* address)
        {
            // Check if data cache is enabled
            if ((CacheControl & CacheControlBits::DataCacheDisable) != 0) {
                return false;
            }
            
            // Check if address is in a cacheable region
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(address);
            std::uint32_t region = addr & 0xE0000000;
            
            return region == CacheAlias::Cached;
        }

        /**
         * @brief Create an alias pointer for the specified address.
         * 
         * This method creates an alias pointer for the specified address using the
         * specified cache alias area.
         * 
         * @tparam T The type of the pointer.
         * @param address The address to alias.
         * @param aliasType The cache alias area to use.
         * @return A pointer to the aliased address.
         */
        template<typename T>
        static inline T* Alias(void* address, CacheAliasType aliasType)
        {
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(address);
            std::uint32_t offset = addr & 0x1FFFFFFF; // Keep only the offset bits
            std::uint32_t alias = static_cast<std::uint32_t>(aliasType);
            
            return reinterpret_cast<T*>(offset | alias);
        }
    }; // class Cache
}

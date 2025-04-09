// saturnringlib/srl_cpu.hpp
#pragma once

#include <cstdint>
#include <cstddef>

namespace SRL::CPU
{
    /*****************************************************************************
     * Cache Class
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
         * @brief Direct access to the SH-2 CPU cache control register.
         * Kept private to encapsulate hardware details from users.
         *
         * This register controls the behavior of the SH-2 CPU cache, including enabling/disabling
         * the cache, selecting cache modes, and managing instruction and data cache functionality.
         * It is located at memory address 0xFFFFFE92.
         */
        inline static volatile std::uint8_t& CacheControlRegister = *reinterpret_cast<volatile std::uint8_t*>(0xFFFFFE92);

        /**
         * @brief Cache control register bit definitions.
         * 
         * These are defined according to the SH7604 Hardware Manual section 8.2
         */
        static constexpr std::uint8_t CacheEnable = 0x01;               // Bit 0: Cache enable
        static constexpr std::uint8_t CacheDisable = 0x00;              // Bit 0: Cache disable
        static constexpr std::uint8_t InstructionCacheDisable = 0x02;   // Bit 1: Instruction cache disable
        static constexpr std::uint8_t DataCacheDisable = 0x04;          // Bit 2: Data cache disable
        static constexpr std::uint8_t HybridModeBit = 0x04;             // Bit 2: Hybrid mode bit
        static constexpr std::uint8_t TwoWayBit = 0x08;                 // Bit 3: Two-way mode bit

        /**
         * @brief Bit mask constants for cache control.
         */
        static constexpr std::uint8_t CacheEnableMask = 0xFE;           // Mask to preserve all bits except cache enable (bit 0)
        static constexpr std::uint8_t InstructionCacheMask = 0xFD;      // Mask to preserve all bits except instruction cache bit (bit 1)
        static constexpr std::uint8_t DataCacheMask = 0xFB;             // Mask to preserve all bits except data cache bit (bit 2)
        static constexpr std::uint8_t HybridModeMask = 0xFB;            // Mask to preserve all bits except hybrid mode bit (bit 2)
        static constexpr std::uint8_t TwoWayMask = 0xF7;                // Mask to preserve all bits except two-way mode bit (bit 3)
        static constexpr std::uint8_t ModeMask = 0xF3;                  // Mask to preserve all bits except mode bits (bits 2-3)

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
                        reinterpret_cast<void*>(set * LineSize), 
                        CacheAlias::DataArray);
                    
                    // Clear the entire line (4 words of 4 bytes each = 16 bytes)
                    for (std::uint32_t i = 0; i < (LineSize / sizeof(std::uint32_t)); ++i) {
                        addr[i] = 0;
                    }
                }
            }
            
            // Restore cache state if it was enabled
            if (cacheWasEnabled) {
                SetCacheEnabled(true);
            }
        }

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
            CacheControlRegister = (CacheControlRegister & mask) | value;
        }

    public:
        /**
         * @brief Enum for Cache Organization modes.
         *
         * These modes define the cache organization (associativity and line count).
         */
        enum class CacheMode : std::uint8_t
        {
            FourWay = 0x00,    /**< 4-way associative cache (64 sets × 4 ways × 16 bytes) */
            Hybrid = 0x04,     /**< Hybrid mode: 2KB 2-way cache + 2KB scratch pad */
            TwoWay = 0x08      /**< 2-way associative cache (128 sets × 2 ways × 16 bytes) */
        };

        /**
         * @brief Enum for cache aliasing options.
         * 
         * These constants define the SH-2 CPU cache alias areas that control how memory is accessed
         * in relation to the cache. Each area has specific behavior regarding cache usage.
         */
        enum class CacheAlias : std::uint32_t
        {
            Cached = 0x00000000,      /**< Cache area - Cache is used when the CE bit in CCR is 1 */
            CacheThrough = 0x20000000, /**< Cache-through area - Cache is not used */
            Purge = 0x40000000,       /**< Associative purge area - Cache line of the specified address is purged */
            AddressArray = 0x60000000, /**< Address array read/write area - Cache address array is accessed directly */
            DataArray = 0xC0000000,   /**< Data array read/write area - Cache data array is accessed directly */
            IO = 0xE0000000           /**< I/O area - Cache is not used */
        };

        /**
         * @brief Cache line size in bytes.
         *
         * This constant defines the size of a single cache line in the SH-2 CPU cache.
         */
        static constexpr size_t LineSize = 16;

        /**
         * @brief Get the current cache organization mode.
         */
        static inline CacheMode GetCacheMode()
        {
            std::uint8_t modeBits = CacheControlRegister & (HybridModeBit | TwoWayBit);
            
            if (modeBits == HybridModeBit) {
                return CacheMode::Hybrid;
            } else if (modeBits == TwoWayBit) {
                return CacheMode::TwoWay;
            } else {
                return CacheMode::FourWay;
            }
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
            return ((CacheControlRegister & TwoWayBit) != 0) && !IsCacheEnabled();
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
                ApplyMask(TwoWayMask, TwoWayBit);
                ApplyMask(CacheEnableMask, CacheDisable);
            } else {
                // To exit scratchpad mode: Just enable the cache
                ApplyMask(CacheEnableMask, CacheEnable);
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
                    return 64;
                case CacheMode::TwoWay:
                    return 128;
                case CacheMode::Hybrid:
                    return 64;  // Half of full capacity
                default:
                    return 0;
            }
        }

        /**
         * @brief Get the number of cache ways for the current mode.
         */
        static inline size_t GetWayCount()
        {
            CacheMode currentMode = GetCacheMode();
            
            switch (currentMode) {
                case CacheMode::FourWay:
                    return 4;
                case CacheMode::TwoWay:
                case CacheMode::Hybrid:
                    return 2;
                default:
                    return 0;
            }
        }

        /**
         * @brief Get the size of the scratchpad portion in bytes, if any.
         */
        static inline size_t GetScratchpadSize()
        {
            if (IsFullScratchpadMode()) {
                return 4 * 1024;  // Full 4KB
            } else if (IsHybridMode()) {
                return 2 * 1024;  // Half of memory, 2KB
            } else {
                return 0;  // No scratchpad
            }
        }

        /**
         * @brief Set the cache organization mode.
         * 
         * @param mode The cache mode (FourWay, TwoWay, or Hybrid).
         */
        static inline void SetCacheMode(CacheMode mode)
        {
            // First disable cache to avoid inconsistent state
            bool cacheWasEnabled = IsCacheEnabled();
            if (cacheWasEnabled) {
                SetCacheEnabled(false);
            }
            
            // Clear the mode bits and set the new mode
            ApplyMask(ModeMask, static_cast<std::uint8_t>(mode));
            
            // Clear all cache lines when changing mode to avoid inconsistency
            ClearCacheLines();
            
            // Restore cache state if it was enabled
            if (cacheWasEnabled) {
                SetCacheEnabled(true);
            }
        }

        /**
         * @brief Check if instructions are eligible for caching.
         *
         * This function checks if instructions are allowed to be cached. When false,
         * instructions will not be cached even if the global cache is enabled.
         * This is independent of the global cache enable state.
         *
         * @return `true` if instructions are eligible for caching, `false` otherwise.
         */
        static inline bool IsInstructionCacheEligible()
        {
            return !(CacheControlRegister & InstructionCacheDisable);
        }

        /**
         * @brief Set instruction cache eligibility.
         *
         * @param eligible True to make instructions eligible for caching, false to disable.
         */
        static inline void SetInstructionCacheEligible(bool eligible)
        {
            ApplyMask(InstructionCacheMask, eligible ? 0 : InstructionCacheDisable);
        }

        /**
         * @brief Check if data is eligible for caching.
         *
         * This function checks if data loads/stores are allowed to be cached.
         * When false, data operations will not be cached even if the global cache is enabled.
         * This is independent of the global cache enable state.
         *
         * Note: In hybrid mode, the data cache disable bit (bit 2) is also used to enable
         * hybrid mode, so data cache eligibility cannot be separately controlled in hybrid mode.
         *
         * @return `true` if data is eligible for caching, `false` otherwise.
         */
        static inline bool IsDataCacheEligible()
        {
            CacheMode currentMode = GetCacheMode();
            
            // If in hybrid mode, the data cache bit has a different meaning
            if (currentMode == CacheMode::Hybrid) {
                return true;  // In hybrid mode, data caching is always eligible in the cache portion
            }
            
            return !(CacheControlRegister & DataCacheDisable);
        }

        /**
         * @brief Set data cache eligibility.
         *
         * @param eligible True to make data eligible for caching, false to disable.
         * 
         * Note: In hybrid mode, this operation is not allowed as it would change the cache mode.
         * The function will not make changes if in hybrid mode.
         */
        static inline void SetDataCacheEligible(bool eligible)
        {
            CacheMode currentMode = GetCacheMode();
            
            // If in hybrid mode, do not change the data cache bit as it would change the mode
            if (currentMode != CacheMode::Hybrid) {
                ApplyMask(DataCacheMask, eligible ? 0 : DataCacheDisable);
            }
        }

        /**
         * @brief Check if cache is enabled.
         *
         * This function checks if the cache system is enabled. When disabled,
         * neither instructions nor data will be cached, regardless of their
         * individual eligibility settings.
         *
         * @return `true` if cache is enabled, `false` otherwise.
         */
        static inline bool IsCacheEnabled()
        {
            return (CacheControlRegister & CacheEnable) != 0;
        }

        /**
         * @brief Enable or disable the cache.
         *
         * @param enable True to enable the cache, false to disable.
         */
        static inline void SetCacheEnabled(bool enable)
        {
            ApplyMask(CacheEnableMask, enable ? CacheEnable : CacheDisable);
        }

        /**
         * @brief Purge a specific cache line.
         *
         * This function purges a specific cache line by accessing the address
         * through the purge area.
         *
         * @param address The address to purge from cache.
         */
        static inline void PurgeLine(void* address)
        {
            // Access through purge area to invalidate the line
            volatile void* purgeAddr = Alias<void>(address, CacheAlias::Purge);
            *reinterpret_cast<volatile std::uint8_t*>(purgeAddr);
        }

        /**
         * @brief Purge all cache lines.
         *
         * This function purges all cache lines by clearing them directly.
         */
        static inline void PurgeAll()
        {
            ClearCacheLines();
        }

        /**
         * @brief Initialize the cache to a known good state.
         *
         * This function initializes the cache with default settings. It clears
         * all cache lines, sets the specified mode, and enables the cache.
         *
         * @param mode The cache mode to set (default is FourWay).
         * @param enableInstructionCache Whether to enable instruction caching (default is true).
         * @param enableDataCache Whether to enable data caching (default is true).
         * @param enableCache Whether to enable the cache overall (default is true).
         */
        static inline void Initialize(
            CacheMode mode = CacheMode::FourWay,
            bool enableInstructionCache = true,
            bool enableDataCache = true,
            bool enableCache = true)
        {
            // Disable cache first
            SetCacheEnabled(false);
            
            // Set mode
            ApplyMask(ModeMask, static_cast<std::uint8_t>(mode));
            
            // Clear all cache lines
            ClearCacheLines();
            
            // Set instruction cache eligibility
            SetInstructionCacheEligible(enableInstructionCache);
            
            // Set data cache eligibility (only if not in hybrid mode)
            if (mode != CacheMode::Hybrid) {
                SetDataCacheEligible(enableDataCache);
            }
            
            // Enable cache if requested
            if (enableCache) {
                SetCacheEnabled(true);
            }
        }

        /**
         * @brief Convert a pointer to the specified cache alias address.
         *
         * This function allows conversion of a regular memory pointer to a cache alias address.
         * Depending on the alias mode, this function ensures the pointer accesses memory
         * with the desired behavior, such as cache-through or non-cached access.
         *
         * @tparam T Type of the pointer to return.
         * @param ptr The original memory pointer.
         * @param mode The cache alias mode (e.g., CacheThrough, Purge).
         * @return A pointer with the desired cache alias.
         *
         * @example
         * ```cpp
         * void* ptr = some_pointer;
         * auto cacheThroughPtr = Cache::Alias<void>(ptr, Cache::CacheAlias::CacheThrough);
         * ```
         */
        template<typename T>
        static constexpr T* Alias(const void* ptr, Cache::CacheAlias mode)
        {
            return reinterpret_cast<T*>((reinterpret_cast<std::uintptr_t>(ptr) & 0x1FFFFFFF) | static_cast<std::uint32_t>(mode));
        }

        /**
         * @brief Structure representing the complete state of the cache.
         *
         * This structure encapsulates all aspects of the cache state, including
         * mode, instruction cache state, data cache state, and whether the
         * cache is enabled. It provides a clean way to save and restore cache state.
         */
        class CacheState
        {
        public:
            /**
             * @brief Construct a CacheState with explicit component values.
             *
             * This constructor allows creating a CacheState with specific configuration values.
             *
             * @param mode The cache mode.
             * @param instructionEligible Whether instructions are eligible for caching.
             * @param dataEligible Whether data is eligible for caching.
             * @param enabled Whether the entire cache is enabled.
             */
            explicit CacheState(CacheMode mode, bool instructionEligible, bool dataEligible, bool enabled) :
                mode(mode),
                instructionCacheEligible(instructionEligible),
                dataCacheEligible(dataEligible),
                cacheEnabled(enabled)
            {
            }

            /**
             * @brief Get the current cache mode.
             * @return The current mode.
             */
            CacheMode GetMode() const { return mode; }

            /**
             * @brief Check if instruction cache is eligible for caching.
             * @return true if instruction cache is eligible for caching, false otherwise.
             */
            bool IsInstructionCacheEligible() const { return instructionCacheEligible; }

            /**
             * @brief Check if data cache is eligible for caching.
             * @return true if data cache is eligible for caching, false otherwise.
             */
            bool IsDataCacheEligible() const { return dataCacheEligible; }

            /**
             * @brief Check if the entire cache is enabled.
             * @return true if cache is enabled, false otherwise.
             */
            bool IsCacheEnabled() const { return cacheEnabled; }

        private:
            const CacheMode mode; /**< Current cache mode */
            const bool instructionCacheEligible; /**< Whether instruction cache is eligible for caching */
            const bool dataCacheEligible; /**< Whether data cache is eligible for caching */
            const bool cacheEnabled; /**< Whether the entire cache is enabled */
        };

        /**
         * @brief Get the current state of the cache.
         *
         * Returns a CacheState structure representing the current state of all
         * cache settings, including mode, instruction cache state, data cache state, and enabled state.
         *
         * @return The current cache state.
         */
        static inline CacheState GetState()
        {
            return CacheState(
                GetCacheMode(),
                IsInstructionCacheEligible(),
                IsDataCacheEligible(),
                IsCacheEnabled());
        }

        /**
         * @brief Apply a cache state to restore a previous configuration.
         *
         * This function applies a previously saved CacheState to restore
         * the cache to that specific configuration.
         *
         * @param state The CacheState to apply.
         */
        static inline void ApplyState(const CacheState& state)
        {
            // First disable cache to avoid inconsistent state
            SetCacheEnabled(false);
            
            // Set mode
            SetCacheMode(state.GetMode());
            
            // Set instruction cache eligibility
            SetInstructionCacheEligible(state.IsInstructionCacheEligible());
            
            // Set data cache eligibility (only if not in hybrid mode)
            if (state.GetMode() != CacheMode::Hybrid) {
                SetDataCacheEligible(state.IsDataCacheEligible());
            }
            
            // Restore cache enabled state
            SetCacheEnabled(state.IsCacheEnabled());
        }

        /**
         * @brief RAII class to temporarily change cache mode.
         *
         * This class automatically changes the cache mode (FourWay/TwoWay/Hybrid) 
         * when constructed and restores its original mode when destructed.
         * It handles proper cache disabling and clearing to maintain consistency.
         *
         * @example
         * ```cpp
         * {
         *     Cache::ScopedCacheMode scopedMode(Cache::CacheMode::TwoWay);
         *     // Cache is now in two-way mode within this scope
         *     // Do some operations that benefit from two-way mode
         * } // Original cache mode is automatically restored when leaving scope
         * ```
         */
        class ScopedCacheMode
        {
        public:
            /**
             * @brief Construct a ScopedCacheMode object with a specific mode.
             *
             * The constructor saves the original mode and applies the new mode.
             *
             * @param newMode The cache mode to apply during the scope.
             */
            explicit ScopedCacheMode(CacheMode newMode) : 
                originalState(GetState())
            {
                SetCacheMode(newMode);
                
                // Restore cache enabled state if it was enabled
                if (originalState.IsCacheEnabled()) {
                    SetCacheEnabled(true);
                }
            }

            /**
             * @brief Destruct the ScopedCacheMode object.
             *
             * The destructor restores the original cache mode.
             */
            ~ScopedCacheMode()
            {
                ApplyState(originalState);
            }

            // Deleted copy/move operations to prevent misuse
            ScopedCacheMode(const ScopedCacheMode&) = delete;
            ScopedCacheMode& operator=(const ScopedCacheMode&) = delete;
            ScopedCacheMode(ScopedCacheMode&&) = delete;
            ScopedCacheMode& operator=(ScopedCacheMode&&) = delete;

        private:
            const CacheState originalState; /**< The original cache state before construction */
        };

        /**
         * @brief RAII class to temporarily disable the cache.
         *
         * This class automatically disables the cache when constructed and restores
         * its original state when destructed. This is useful for operations that
         * require the cache to be temporarily disabled without having to manually
         * restore the state.
         *
         * @example
         * ```cpp
         * {
         *     Cache::ScopedDisable scopedDisable;
         *     // Cache is disabled within this scope
         *     // Do some operations that require cache to be disabled
         * } // Cache state is automatically restored when leaving scope
         * ```
         */
        class ScopedDisable
        {
        public:
            /**
             * @brief Construct a ScopedDisable object.
             *
             * The constructor disables the cache and saves the original state
             * so it can be restored when the object is destructed.
             */
            ScopedDisable() : wasEnabled(IsCacheEnabled())
            {
                SetCacheEnabled(false);
            }

            /**
             * @brief Destruct the ScopedDisable object.
             *
             * The destructor restores the cache to its original state when
             * the ScopedDisable object was constructed.
             */
            ~ScopedDisable()
            {
                SetCacheEnabled(wasEnabled);
            }

            // Deleted copy/move operations to prevent misuse
            ScopedDisable(const ScopedDisable&) = delete;
            ScopedDisable& operator=(const ScopedDisable&) = delete;
            ScopedDisable(ScopedDisable&&) = delete;
            ScopedDisable& operator=(ScopedDisable&&) = delete;

        private:
            const bool wasEnabled;  /**< Whether the cache was enabled before construction */
        };
    };
}
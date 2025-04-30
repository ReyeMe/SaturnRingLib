#pragma once
/**
 * @file srl_ram_cartridge.hpp
 * @brief DRAM Cartridge Hardware Interface
 * 
 * This file provides direct access to the Sega Saturn DRAM cartridge hardware.
 * The DRAM cartridge is an extension module that can be inserted into the Saturn's
 * cartridge slot to provide additional RAM.
 * 
 * @note This provides a low-level direct hardware access alternative to SGL functions
 */

#include "srl_hardware_common.hpp"
#include "srl_scu.hpp"

namespace SRL::Hardware::Memory
{
    using namespace MemoryRegions;

    //----------------------------------------------------------------------
    // Cartridge registers - Hardware interface
    //----------------------------------------------------------------------
    // Cartridge identification register
    static inline constexpr Reg8 CartridgeId { Cs1Base, 0x00FFFFFFUL };

    //----------------------------------------------------------------------
    // Memory mapping constants - Address calculations
    //----------------------------------------------------------------------
    static inline constexpr std::uintptr_t Bank1Offset = 0x02000000UL;  // Base address offset for Bank 1
    static inline constexpr std::uint32_t BankCount = 4;                // Number of sub-banks per primary DRAM bank
    static inline constexpr std::uintptr_t Address1MiB = 0x22580000UL;  // Recommended base address for 1MiB cartridge
    static inline constexpr std::uintptr_t Address4MiB = 0x22400000UL;  // Base address for 4MiB cartridge

    //----------------------------------------------------------------------
    // Memory size constants - Memory management
    //----------------------------------------------------------------------
    static inline constexpr std::size_t BankSize = 0x100000UL;          // 1MiB bank size
    static inline constexpr std::size_t SubBankSize = 0x40000UL;        // 256KiB sub-bank size

    /**
     * @brief DRAM cartridge identifier values
     */
    namespace CartridgeIdValues
    {
        static inline constexpr std::uint8_t Invalid = 0x00;  // No cartridge or invalid cartridge
        static inline constexpr std::uint8_t Size1MiB = 0x5A;  // 1 Megabyte DRAM cartridge
        static inline constexpr std::uint8_t Size4MiB = 0x5C;  // 4 Megabyte DRAM cartridge
    }

    /**
     * @brief DRAM Cartridge hardware interface with detection and memory management
     * 
     * This class provides access to the extended RAM cartridge that can be
     * inserted into the Saturn's cartridge slot. It handles cartridge detection,
     * initialization, memory mapping, and provides methods for accessing cartridge memory.
     * 
     * Memory Map Overview:
     * - 1MiB Cartridge: Base address at 0x22580000 (for contiguous access)
     * - 4MiB Cartridge: Base address at 0x22400000
     * 
     * Address Range: 0x2240'0000 to 0x227F'FFFF
     */
    class DramCartridge
    {
    public:
        // Prevent instantiation - this is a static-only interface
        DramCartridge() = delete;
        DramCartridge(const DramCartridge&) = delete;
        DramCartridge(DramCartridge&&) = delete;
        DramCartridge& operator=(const DramCartridge&) = delete;
        DramCartridge& operator=(DramCartridge&&) = delete;

        /**
         * @brief DRAM cartridge identifier
         *
         * Identifies the type and capacity of the installed DRAM cartridge
         */
        enum class IdType : std::uint8_t
        {
            Invalid = CartridgeIdValues::Invalid, /**< No valid cartridge detected */
            Size1MiB = CartridgeIdValues::Size1MiB, /**< 1MiB (8-Mbit) cartridge */
            Size4MiB = CartridgeIdValues::Size4MiB  /**< 4MiB (32-Mbit) cartridge */
        };

        /**
         * @brief Initializes and detects the DRAM cartridge
         *
         * Configures the necessary hardware settings and detects the cartridge type.
         * Must be called before using any other DRAM cartridge functions.
         *
         * @return true if a valid cartridge was detected and initialized
         * @return false if no valid cartridge was found
         */
        static bool Initialize() noexcept
        {
            // Store original register values
            const auto asr0_backup = SCU::AddrSpaceCtrl0;
            const auto aref_backup = SCU::AbusRefresh;

            // Set the SCU wait
            SCU::AddrSpaceCtrl0 = 0x23301FF0;

            // Write to A-Bus refresh
            SCU::AbusRefresh = 0x00000013;

            // Determine ID and base address
            bool result = Detect();
            if (!result)
            {
                // Restore values in case we can't detect DRAM cartridge
                SCU::AddrSpaceCtrl0 = asr0_backup;
                SCU::AbusRefresh = aref_backup;
            }

            return result;
        }

        /**
         * @brief Gets the recommended base memory address for memory allocation
         *
         * Returns the appropriate address based on cartridge type:
         * - For 1MiB cartridge: Uses 0x22580000 to provide contiguous memory
         * - For 4MiB cartridge: Uses 0x22400000 (start of DRAM area)
         *
         * @return Base address for memory allocation or nullptr if no cartridge
         */
        static void* BaseAddress() noexcept
        {
            if (!IsAvailable())
            {
                return nullptr;
            }

            return (Type() == IdType::Size1MiB) ?
                reinterpret_cast<void*>(Address1MiB) :
                reinterpret_cast<void*>(Address4MiB);
        }

        /**
         * @brief Gets the type of the detected DRAM cartridge
         *
         * @return The cartridge identifier (1MiB, 4MiB, or Invalid)
         */
        static IdType Type() noexcept
        {
            return s_id;
        }

        /**
         * @brief Gets the total usable memory size of the cartridge in bytes
         *
         * @return Memory size (1MiB, 4MiB, or 0 if no cartridge)
         */
        static std::size_t Size() noexcept
        {
            switch (s_id)
            {
            case IdType::Size1MiB: return 0x00100000; // 1 MiB
            case IdType::Size4MiB: return 0x00400000; // 4 MiB
            default: return 0;
            }
        }

        /**
         * @brief Checks if a valid DRAM cartridge is available for use
         *
         * @return true if cartridge is initialized and ready for use
         * @return false if no cartridge is available
         */
        static bool IsAvailable() noexcept
        {
            return s_id != IdType::Invalid && s_base != nullptr;
        }

        /**
         * @brief Computes the physical memory address for a DRAM location
         *
         * @param bank Primary bank index (0-1)
         * @param subBank Sub-bank within the primary bank (0-3)
         * @param offset Byte offset within the sub-bank
         * @return Physical memory address in the Saturn address space
         */
        static constexpr std::uintptr_t CalculateDramAddress(
            std::uint32_t bank,
            std::uint32_t subBank,
            std::uintptr_t offset) noexcept
        {
            return Cs0Base + offset |
                ((bank & 0x01) + 2) << 21 |
                (subBank & 0x03) << 19;
        }

        /**
         * @brief Get a pointer to DRAM memory at specified bank and offset
         *
         * @param bank Primary bank (0-1)
         * @param subBank Sub-bank within primary bank (0-3)
         * @param offset Byte offset within sub-bank
         * @return void* Pointer to the specified DRAM location
         */
        static void* GetPointerAt(std::uint32_t bank, std::uint32_t subBank, std::uintptr_t offset) noexcept
        {
            return reinterpret_cast<void*>(CalculateDramAddress(bank, subBank, offset));
        }

        /**
         * @brief Get reference to a 32-bit value at specified DRAM location
         *
         * @param bank Primary bank (0-1)
         * @param subBank Sub-bank within primary bank (0-3)
         * @param offset Byte offset within sub-bank
         * @return volatile uint32_t& Reference to the 32-bit value
         */
        static inline volatile std::uint32_t& GetRef32At(std::uint32_t bank, std::uint32_t subBank, std::uintptr_t offset) noexcept
        {
            return *reinterpret_cast<volatile std::uint32_t*>(CalculateDramAddress(bank, subBank, offset));
        }

    private:
        // Internal state
        inline static IdType s_id = IdType::Invalid; /**< Detected cartridge type */
        inline static void* s_base = nullptr;        /**< Base pointer to DRAM memory */

        /**
         * @brief Detects the DRAM cartridge type and configuration
         *
         * Performs cartridge detection by reading the ID register and testing memory
         * to determine the specific cartridge type and memory layout.
         *
         * @return true if a valid cartridge was detected
         * @return false if no valid cartridge could be found
         */
        static bool Detect() noexcept
        {
            // Check the ID using direct memory access
            s_id = static_cast<IdType>(CartridgeId & 0xFF);

            if ((s_id != IdType::Size1MiB) && (s_id != IdType::Size4MiB))
            {
                s_id = IdType::Invalid;
                s_base = nullptr;
                return false;
            }

            // Initialize DRAM using direct access
            for (std::uint32_t b = 0; b < BankCount; b++)
            {
                // Access bank 0 and 1, sub-bank b
                volatile auto& dram0 = *reinterpret_cast<volatile std::uint32_t*>(CalculateDramAddress(0, b, 0));
                volatile auto& dram1 = *reinterpret_cast<volatile std::uint32_t*>(CalculateDramAddress(1, b, 0));
                
                // Write test pattern
                dram0 = 0x55AA1122;
                dram1 = 0xAA55CCDD;
                
                // Verify pattern matches
                if ((dram0 != 0x55AA1122) || (dram1 != 0xAA55CCDD))
                {
                    s_id = IdType::Invalid;
                    s_base = nullptr;
                    return false;
                }
            }
            
            // Setup base address based on size
            s_base = (s_id == IdType::Size1MiB) ? 
                reinterpret_cast<void*>(Address1MiB) : 
                reinterpret_cast<void*>(Address4MiB);
                
            return true;
        }
    };
}

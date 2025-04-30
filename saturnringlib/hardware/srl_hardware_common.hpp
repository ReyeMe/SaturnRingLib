#pragma once
/**
 * @file srl_hardware_common.hpp
 * @brief Common Hardware Definitions for Saturn Hardware Access
 * 
 * This file provides the base templates and common definitions for accessing
 * Saturn hardware registers. It implements a modern C++ approach for hardware
 * register access with type-safe wrappers.
 *
 * Key components:
 * - Reg<T> template for register access with proper volatile semantics
 * - Common memory region base addresses
 * - Helper types for hardware register access
 * 
 * @note All register addresses are defined as compile-time constants
 */

#include <cstdint>
#include <cstddef>

namespace SRL::Hardware
{
    /**
     * @brief Template wrapper for hardware registers that provides a more concise syntax
     *
     * This template allows declaring hardware registers with their comments in a single line
     * and provides type-safe access to the underlying volatile memory.
     */
    template<typename T>
    class Reg
    {
    private:
        std::uintptr_t _address;

    public:
        // Constructor that combines offset and base address
        constexpr Reg(std::uintptr_t base, std::uintptr_t offset) : _address(base + offset) {}

        // Access operators
        volatile T& operator*() const  { return *reinterpret_cast<volatile T*>(_address); }
        volatile T* operator->() const  { return reinterpret_cast<volatile T*>(_address); }

        // Direct assignment and access
        volatile T& operator=(T value) const
        {
            *reinterpret_cast<volatile T*>(_address) = value;
            return *reinterpret_cast<volatile T*>(_address);
        }

        // Implicit conversion to value
        operator T() const  { return *reinterpret_cast<volatile T*>(_address); }

        // Get raw address
        std::uintptr_t address() const  { return _address; }
    };

    // Helper aliases for common register types
    using Reg8 = Reg<std::uint8_t>;
    using Reg16 = Reg<std::uint16_t>;
    using Reg32 = Reg<std::uint32_t>;

    /**
     * @brief Memory region base addresses for Saturn hardware
     * 
     * These constants define the base addresses for various hardware components
     * of the Sega Saturn system.
     */
    namespace MemoryRegions
    {
        static inline constexpr std::uintptr_t Cs0Base   = 0x22000000UL;  // Cartridge/CS0
        static inline constexpr std::uintptr_t Cs1Base   = 0x24000000UL;  // Cartridge/CS1
        static inline constexpr std::uintptr_t Cs2Base   = 0x25000000UL;  // A-Bus CS2
        static inline constexpr std::uintptr_t CsaaBase  = 0x25800000UL;  // A-Bus (area after CS2)
        static inline constexpr std::uintptr_t DummyBase = 0x25000000UL;  // A-Bus dummy
        static inline constexpr std::uintptr_t Vdp1Base  = 0x25D00000UL;  // VDP1
        static inline constexpr std::uintptr_t Vdp2Base  = 0x25F80000UL;  // VDP2
        static inline constexpr std::uintptr_t ScuBase   = 0x25FE0000UL;  // SCU
        static inline constexpr std::uintptr_t LwramBase = 0x00200000UL;  // LWRAM
        static inline constexpr std::uintptr_t SmpcBase  = 0x20100000UL;  // SMPC
        static inline constexpr std::uintptr_t CdBlockBase = 0x25890000UL; // CD Block
    }
}

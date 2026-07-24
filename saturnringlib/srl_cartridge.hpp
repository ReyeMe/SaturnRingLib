#pragma once

/** @file srl_cartridge.hpp
 *  @brief Sega Saturn Cartridge Hardware Detection and Memory Mapping API.
 *  @details Provides identification routines for RAM expansion cartridges (1 MiB / 4 MiB),
 *           USB Development cartridges, and Data cartridges, along with memory base address and size queries.
 */

#include "srl_devcart.hpp" // for USB dev cart detection
#include "srl_string.hpp"  // Provides strncmp (from <cstring> equivalent in SRL namespace?)

#include <cstdint> // For uint32_t, uint8_t, uintptr_t, size_t

namespace SRL
{
/** @brief Cartridge hardware detection and memory mapping utilities.
 */
namespace Cartridge
{

    /** @brief Address of the Interrupt Status Register in the SCU (System Control Unit).
     *
     *  This register is used to mask/unmask interrupts temporarily during cartridge detection
     *  to prevent interference while accessing cartridge memory spaces.
     */
    static constexpr uint32_t InterruptStatusRegister = 0x25fe00a4UL; /* Interrupt status register */

    /** @brief Cartridge ID values.
     *
     *  These enums represent possible cartridge types detected on the system.
     */
    enum CartridgeId
    {
        /** @brief No cartridge detected.
         */
        None = 0,

        /** @brief 1 MiB RAM cartridge (ID value read from hardware).
         */
        Cart1MiB = 0x5A,

        /** @brief 4 MiB RAM cartridge (ID value read from hardware).
         */
        Cart4MiB = 0x5C,

        /** @brief USB development cartridge (detected via DevCart API).
         */
        USBDevCart,

        /** @brief Data cartridge (detected via string signature in memory).
         */
        DataCart,

    };

    /** @brief Namespace for 1 MiB cartridge specifics.
     */
    namespace Cartridge1MiB
    {
        /** @brief Cartridge type identifier enum value.
         */
        static constexpr CartridgeId Id = Cart1MiB;

        /** @brief Base address for contiguous access to the 1 MiB cartridge memory.
         *
         *  Note: This uses a remapped address (0x02400000) for linear access, differing from standard CS0/CS1 spaces.
         */
        static constexpr uintptr_t Address = 0x02400000UL;

        /** @brief Size of the cartridge in bytes.
         */
        static constexpr size_t Size = 1024 * 1024; // 1 MiB

        /** @brief Human-readable name.
         */
        static constexpr const char* Name = "1 MiB Cartridge";
    } // namespace Cartridge1MiB

    /** @brief Namespace for 4 MiB cartridge specifics.
     */
    namespace Cartridge4MiB
    {
        /** @brief Cartridge type identifier enum value.
         */
        static constexpr CartridgeId Id = Cart4MiB;

        /** @brief Base address for contiguous access to the 4 MiB cartridge memory.
         */
        static constexpr uintptr_t Address = 0x24000000UL;

        /** @brief Size of the cartridge in bytes.
         */
        static constexpr size_t Size = 4 * 1024 * 1024; // 4 MiB

        /** @brief Human-readable name.
         */
        static constexpr const char* Name = "4 MiB Cartridge";
    } // namespace Cartridge4MiB

    /** @brief Namespace for USB Dev cartridge specifics.
     */
    namespace CartridgeUSBDev
    {
        /** @brief Cartridge type identifier enum value.
         */
        static constexpr CartridgeId Id = USBDevCart;

        /** @brief Human-readable name.
         */
        static constexpr const char* Name = "USB Dev Cartridge";
    } // namespace CartridgeUSBDev

    /** @brief Namespace for Data cartridge specifics.
     */
    namespace CartridgeData
    {
        /** @brief Cartridge type identifier enum value.
         */
        static constexpr CartridgeId Id = DataCart;

        /** @brief Base address for contiguous access to the data cartridge memory.
         */
        static constexpr uintptr_t Address = 0x22000000UL;

        /** @brief Human-readable name.
         */
        static constexpr const char* Name = "Data Cartridge";

        /** @brief Hardware ID string used for signature-based detection.
         *
         *  This string ("SEGA SEGASA") is checked at the base address to identify the cartridge.
         *  Note: Array size is 12 bytes (including null terminator; strncmp handles comparison).
         */
        static constexpr char HwId[] = "SEGA SEGASA";

        /** @brief Size of the cartridge in bytes (placeholder).
         *
         *  TODO: Determine and set actual size if known (e.g., via hardware specs).
         */
        static constexpr size_t Size = 0; // TBD
    } // namespace CartridgeData

    /** @brief Convert a raw cartridge ID byte (read from hardware) to a CartridgeId enum.
     *
     *  Currently supports only RAM cartridges; other types return None.
     *
     * @param rawId Raw cartridge ID byte (8-bit value from register).
     * @return Corresponding CartridgeId (or None if unknown).
     */
    inline static CartridgeId RawIdToCartridgeId(uint8_t rawId)
    {
        switch (rawId)
        {
        case Cart1MiB:
            return CartridgeId::Cart1MiB;
        case Cart4MiB:
            return CartridgeId::Cart4MiB;
        default:
            return CartridgeId::None;
        }
    }

    /** @brief Address of the Cartridge ID register in memory-mapped I/O.
     *
     *  Reading this 8-bit register returns the raw ID for RAM cartridges.
     */
    static constexpr uintptr_t CartridgeIdRegister = 0x24FFFFFFUL;

    /** @brief Detect memory (RAM) cartridge by reading the ID register.
     *
     *  This function temporarily disables interrupts via the SCU to safely access the cartridge space,
     *  reads the ID byte, and restores the SCU state. This prevents bus conflicts or interrupts during probe.
     *
     *  Note: Only detects RAM carts (1MiB/4MiB); returns raw ID (0 for none/unknown).
     *
     * @return uint8_t The raw cartridge ID read from the CartridgeIdRegister.
     */
    inline static uint8_t DetectMemoryCartridge()
    {
        // Save current SCU interrupt mask
        uint32_t scuMask = *((volatile uint32_t*)InterruptStatusRegister);

        // Clear interrupt mask to configure for cartridge access (disables interrupts temporarily)
        *((volatile uint32_t*)InterruptStatusRegister) = 0x00000000;

        // Read cartridge ID (volatile to ensure MMIO access)
        uint8_t cartId = *((volatile uint8_t*)CartridgeIdRegister);

        // Restore original SCU interrupt mask
        *((volatile uint32_t*)InterruptStatusRegister) = scuMask;

        return cartId;
    }

    /** @brief Detect Data cartridge by checking for a known hardware ID string in memory.
     *
     *  Similar to DetectMemoryCartridge, it disables interrupts via SCU, then compares a fixed string
     *  ("SEGA SEGASA") at the cartridge's base address. This is a signature-based detection.
     *
     *  Note: strncmp is used (from srl_string.hpp); comparison length is size of HwId (11 chars + null).
     *
     * @return bool True if the signature matches, false otherwise.
     */
    inline static bool DetectDataCartridge()
    {
        bool bReturn = false;

        // Save current SCU interrupt mask
        uint32_t scuMask = *((volatile uint32_t*)InterruptStatusRegister);

        // Clear interrupt mask for safe access
        *((volatile uint32_t*)InterruptStatusRegister) = 0x00000000;

        // Compare HwId string at the cartridge address (cast to char* for string access)
        bReturn = (0 == strncmp((char*)CartridgeData::HwId, (char*)CartridgeData::Address, sizeof(CartridgeData::HwId)));

        // Restore SCU mask
        *((volatile uint32_t*)InterruptStatusRegister) = scuMask;

        return bReturn;
    }

    /** @brief Comprehensive cartridge type detection.
     *
     *  Prioritizes detection order:
     *  1. RAM cartridges via ID register.
     *  2. USB Dev cartridge via SRL::DevCart::CS0::IsConnected().
     *  3. Data cartridge via string signature.
     *
     *  Returns CartridgeId::None if no recognized cartridge is found.
     *
     * @return CartridgeId The detected cartridge type (or None).
     */
    inline static CartridgeId DetectCartridgeType()
    {
        // 1- Try to identify a RAM cartridge
        CartridgeId ramCartId = RawIdToCartridgeId(DetectMemoryCartridge());

        if (ramCartId != CartridgeId::None)
        {
            return ramCartId;
        }

        // 2- Try to identify a USB dev cartridge
        if (SRL::DevCart::CS0::IsConnected())
        {
            return CartridgeId::USBDevCart;
        }

        // 3- Try to identify a data cartridge
        if (DetectDataCartridge())
        {
            return CartridgeId::DataCart;
        }

        // Fallback to RAM result (None) or extend for other types
        return ramCartId;
    }

    /** @brief Get the base memory address for a given cartridge type.
     *
     *  Returns nullptr for unsupported or unknown types (e.g., USBDevCart, which does not have a fixed mmap).
     *
     * @param type Cartridge type.
     * @return void* Base address (or nullptr if unavailable).
     */
    inline static void* GetBaseAddressForType(SRL::Cartridge::CartridgeId type)
    {
        switch (type)
        {
        case CartridgeId::Cart1MiB:
            return (void*)Cartridge1MiB::Address; // Use special address for 1MiB for contiguous access
        case CartridgeId::Cart4MiB:
            return (void*)Cartridge4MiB::Address;
        case CartridgeId::DataCart:
            return (void*)CartridgeData::Address;
        case CartridgeId::USBDevCart:
        default:
            return nullptr;
        }
    }

    /** @brief Get the size in bytes for a given cartridge type.
     *
     * @param type Cartridge type.
     * @return size_t Size in bytes (0 for unknown or unspecified cartridges).
     */
    inline static size_t GetSizeForType(CartridgeId type)
    {
        switch (type)
        {
        case CartridgeId::Cart1MiB:
            return Cartridge1MiB::Size;
        case CartridgeId::Cart4MiB:
            return Cartridge4MiB::Size;
        case CartridgeId::DataCart:
            return CartridgeData::Size;
        default:
            return 0;
        }
    }

    /** @brief Get a human-readable string name for a cartridge type.
     *
     * @param type Cartridge type.
     * @return const char* Name string (or "Unknown Cartridge" for unrecognized types).
     */
    inline static const char* GetStringFromType(CartridgeId type)
    {
        switch (type)
        {
        case CartridgeId::Cart1MiB:
            return Cartridge1MiB::Name;
        case CartridgeId::Cart4MiB:
            return Cartridge4MiB::Name;
        case CartridgeId::USBDevCart:
            return CartridgeUSBDev::Name;
        case CartridgeId::DataCart:
            return CartridgeData::Name;
        default:
            return "Unknown Cartridge";
        }
    }

    /** @brief Check if a specific cartridge type is currently inserted/detected.
     *
     *  Performs full hardware probing each call; cache the result if called frequently in hot loops.
     *
     * @param type The CartridgeId to check for.
     * @return true If the specified cartridge type is detected, false otherwise.
     */
    inline static bool IsCartridgeTypePresent(CartridgeId type)
    {
        return DetectCartridgeType() == type;
    }

} // namespace Cartridge
} // namespace SRL
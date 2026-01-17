#pragma once

#include <srl_string.hpp> // For memcpy
#include <cstdint>        // For uintptr_t, size_t, uint8_t
#include <type_traits>    // For std::enable_if_t

namespace SRL::Types
{
    /**
     * @brief Access mode for a Register.
     */
    enum class AccessMode : uint8_t
    {
        Read,     //!< Read-only
        Write,    //!< Write-only
        ReadWrite //!< Read/Write
    };

    /**
     * @name AccessMode helpers
     * Free helper utilities for testing/printing AccessMode values.
     */
    constexpr inline bool isReadable(AccessMode m) noexcept
    {
        return (m == AccessMode::Read) || (m == AccessMode::ReadWrite);
    }

    constexpr inline bool isWritable(AccessMode m) noexcept
    {
        return (m == AccessMode::Write) || (m == AccessMode::ReadWrite);
    }

    /**
     * @brief Simple POD describing a memory region on a register.
     *
     * The AccessMode is a compile-time template parameter. The type therefore
     * exposes the access mode as a static constexpr member and the runtime
     * constructor only accepts address and size.
     */
    template <uintptr_t Address, size_t Size, AccessMode Mode, typename... Args>
    struct Register
    {
        static constexpr AccessMode access = Mode; ///< Compile-time access mode

        const uintptr_t address = Address; ///< Base address of the region
        const size_t size = Size;       ///< Size of the region in bytes

        /**
         * @brief Checks if the register is readable.
         * @return true if the register has read access.
         */
        constexpr bool isReadable() const noexcept
        {
            return (Mode == AccessMode::Read) || (Mode == AccessMode::ReadWrite);
        }

        /**
         * @brief Checks if the register is writable.
         * @return true if the register has write access.
         */
        constexpr bool isWritable() const noexcept
        {
            return (Mode == AccessMode::Write) || (Mode == AccessMode::ReadWrite);
        }

        // Only constructor allowed: address, size
        constexpr explicit Register(uintptr_t adr, size_t sz) noexcept
            : address(adr), size(sz)
        {
        }

        /**
         * @brief Get the base address of the region.
         */
        constexpr uintptr_t getAddress() const noexcept { return address; }

        /**
         * @brief Get the size of the region in bytes.
         */
        constexpr size_t getSize() const noexcept { return size; }

        /**
         * @brief Get the compile-time access mode for this Register instance.
         */
        constexpr AccessMode getAccess() const noexcept { return access; }

        /**
         * @brief If this Register was instantiated with AccessMode::Read,
         *        provide a pointer to the readable memory so callers can copy it.
         *
         * This member only participates in overload resolution when Mode == AccessMode::Read.
         * It returns a pointer to const uint8_t at the memory-mapped address.
         */
        template <AccessMode M = Mode, typename std::enable_if_t<(M == AccessMode::Read) || (M == AccessMode::ReadWrite), int> = 0>
        inline size_t data(void *dest) const noexcept
        {
            const uint8_t *src = reinterpret_cast<const uint8_t *>(address);
            memcpy(dest, src, size);
            return size; // number of bytes copied (region size)
        }

        /**
         * @brief If this Register was instantiated with AccessMode::Write,
         *        copy the provided buffer into the memory-mapped region.
         *
         * Enabled only when Mode == AccessMode::Write.
         */
        template <AccessMode M = Mode, typename std::enable_if_t<(M == AccessMode::Write) || (M == AccessMode::ReadWrite), int> = 0>
        inline size_t set(const void *src) const noexcept
        {
            // Copy from src into the region address
            memcpy(reinterpret_cast<void *>(address), src, size);
            return size; // number of bytes written
        }

        // Disallow default construction to enforce the 2-parameter ctor
        Register() = delete;
    };
} // namespace SRL::Types
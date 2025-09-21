#pragma once

/** @brief Bitfield manipulation utilities
 */
namespace SRL::Bitfields
{

    

    /** @brief A class to manage a set of bit flags defined by an enumeration.
     *
     * This class provides a type-safe way to manipulate bit flags using an enumeration.
     * It supports common bitwise operations such as OR, AND, and NOT, as well as checking
     * if specific flags are set. The underlying storage type for the flags is determined
     * by the enum_type of the provided enumeration.
     *
     * @tparam Flag An enumeration type that defines the flags. It must have an inner type `enum_type`
     *              that specifies the underlying integral type for the flags.
     * @tparam Type The underlying integral type for the flags. Defaults to `Flag::enum_type`.
     *
     * @requires The `Flag` type must be an enumeration and `Type` must be an integral type.
     */
    template <typename Flag, typename Type = Flag::enum_type>
    requires std::is_enum_v<typename Flag::Enum> && std::is_integral_v<Type>
    class BitField
    {
    private:
        Type bits_; // Internal storage for the flags

    public:
        // Constructors
        BitField() : bits_(0) {}                               // Default constructor: no flags set
        explicit BitField(Type bits) : bits_(bits) {}          // Constructor: initialize with raw bits
        BitField(std::initializer_list<Type> flags) : bits_(0) // Constructor: initialize with a list of flags
        {
            for (auto f : flags)
                bits_ |= f; // Set each flag provided in the initializer list
        }

        // Conversion to bool
        explicit operator bool() const { return bits_ != 0; } // Operator to check if any flag is set

        // Bitwise OR
        BitField operator|(BitField other) const { return BitField(bits_ | other.bits_); } // OR operator: combines flags
        BitField &operator|=(BitField other)
        {
            bits_ |= other.bits_; // OR assignment: adds flags
            return *this;
        }

        // Bitwise AND
        BitField operator&(BitField other) const { return BitField(bits_ & other.bits_); } // AND operator: keeps common flags
        BitField &operator&=(BitField other)
        {
            bits_ &= other.bits_; // AND assignment: removes flags that are not common
            return *this;
        }

        // Bitwise NOT
        BitField operator~() const { return BitField(static_cast<Type>(~bits_)); } // NOT operator: inverts all flags

        // Test if a flag is set
        bool has(Type flag) const { return (bits_ & flag) != 0; } // Checks if a specific flag is set

        // Get raw bits
        Type bits() const { return bits_; } // Returns the raw bits representing the flags
    };

}

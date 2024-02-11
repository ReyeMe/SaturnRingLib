#pragma once

#include "srl_base.hpp"
#include "std/utils.h"

namespace SRL::Types
{
	/** @brief Fixed point matype and math by https://github.com/robertoduarte
	 */
	class Fxp
	{
	private:

		/** @brief CPU address
		 */
		static inline constexpr size_t CpuAddress = 0xFFFFF000UL;

		/** @brief Divisor register
		 */
		static inline auto& DivisorRegister = *reinterpret_cast<volatile Uint32*>(CpuAddress + 0x0F00UL);

		/** @brief Dividend register
		 */
		static inline auto& DividendRegister = *reinterpret_cast<volatile Uint32*>(CpuAddress + 0x0F10UL);

		/** @brief Result of the division register
		 */
		static inline auto& DivisionResultRegister = *reinterpret_cast<volatile Uint32*>(CpuAddress + 0x0F14UL);

		/** @brief Fixed point value
		 */
		Sint32 value;

		/** @brief Private constructor for creating Fxp objects from an Sint32 value.
		 * @param inValue The Sint32 value to store.
		 */
		constexpr Fxp(const Sint32& inValue) : value(inValue) {}

		/** @brief Private constructor for creating Fxp objects from an Sint32 value.
		 * @param inValue The Sint32 value to store.
		 */
		constexpr Fxp(const Uint32& inValue) : value(inValue) {}

	public:

		/** @brief Default constructor, initializes the value to 0.
		 */
		constexpr Fxp() : value(0) {}

		/** @brief Copy constructor.
		 * @param fxp The Fxp object to copy.
		 */
		constexpr Fxp(const Fxp& fxp) : value(fxp.value) {}

		/** @brief Constructor to create an Fxp object from a floating-point value.
		 * @param f The floating-point value to convert to fixed-point.
		 */
		consteval Fxp(const double& f) : value(f * 65536.0) {}

		/** @brief Constructor to create an Fxp object from a floating-point value.
		 * @param f The floating-point value to convert to fixed-point.
		 */
		consteval Fxp(const float& f) : value(f * 65536.0f) {}

		/** @brief Cast integer as fixed point value
		 * @param value Integer fixed point value
		 * @return Fixed point value
		 */
		static constexpr Fxp FromRaw(const Sint32& value)
		{
			return Fxp(value);
		}

		/** @brief Convert integer to fixed point by multipliing with 65536
		 * @param value Integer value
		 * @return Fixed point value
		 */
		static constexpr Fxp FromInt(const Sint32& value)
		{
			return Fxp(value << 16);
		}

		/** @brief Starts division of two fixed point numbers
		 * @param dividend The dividend
		 * @param divisor The divisor
		 */
		static void AsyncDivisionStart(const Fxp& dividend, const Fxp& divisor)
		{
			Uint32 dividendh;
			__asm__ volatile("swap.w %[in], %[out]\n"
				: [out] "=&r"(dividendh)
				: [in] "r"(dividend.value));
			__asm__ volatile("exts.w %[in], %[out]"
				: [out] "=&r"(dividendh)
				: [in] "r"(dividendh));

			Fxp::DividendRegister = dividendh;
			Fxp::DivisorRegister = divisor.value;
			Fxp::DivisionResultRegister = dividend.value << 16;
		}

		/** @brief Retires result of async division
		 * @note If division was not completed yet, this function block further execution until the division is done
		 * @return Division result
		 */
		static Fxp AsyncDivisionGet()
		{
			return static_cast<Sint32>(Fxp::DivisionResultRegister);
		}

		/** @brief Faster and slightly less accurate version of Sqrt.
		 * @note Due to interpolation, it has a maximum error of around 6.08%.
		 * @return The square root as an Fxp object.
		 */
		constexpr Fxp FastSqrt() const
		{
			Sint32 baseEstimation = 0;
			Sint32 estimation = this->value;

			if (estimation > 0)
			{
				if (estimation < 65536)
				{
					baseEstimation = 1 << 7;
					estimation <<= 7;

					Sint32 iterationValue = this->value >> 1;
					while (iterationValue)
					{
						estimation >>= 1;
						baseEstimation <<= 1;
						iterationValue >>= 2;
					}
				}
				else
				{
					baseEstimation = (1 << 14);

					while (baseEstimation < estimation)
					{
						estimation >>= 1;
						baseEstimation <<= 1;
					}
				}
			}

			return baseEstimation + estimation;
		}
		
		/** @brief Calculate the square root of the value.
		 * @return The square root as an Fxp object.
		 */
		constexpr Fxp Sqrt() const
		{
			Uint32 remainder = static_cast<Uint32>(value);
			Uint32 root = 0;
			Uint32 bit = 0x40000000; // Set the initial bit to the highest bit for 32-bit integers

			while (bit > 0x40)
			{
				// Try the current bit in the root calculation
				Uint32 trial = root + bit;

				if (remainder >= trial)
				{
					remainder -= trial;
					root = trial + bit;
				}

				// Shift the remainder and decrease the bit for the next iteration
				remainder <<= 1;
				bit >>= 1;
			}

			root >>= 8; // Adjust the result

			return static_cast<Uint32>(root);
		}

		/** @brief Convert fixed point value to integer by dividing with 65536
		 * @return Integer value 
		 */
		consteval Sint32 ToInt()
		{
			return this->value >> 16;
		}

		/** @brief Gets raw value
		 * @return Raw fixed point value 
		 */
		constexpr Sint32 Value() const
		{
			return this->value;
		}

		/** @brief Add and assign another Fxp object to this object.
		 * @param fxp The Fxp object to add.
		 * @return A reference to this object.
		 */
		constexpr Fxp& operator+=(const Fxp& fxp)
		{
			value += fxp.value;
			return *this;
		}

		/** @brief Subtract and assign another Fxp object from this object.
		 * @param fxp The Fxp object to subtract.
		 * @return A reference to this object.
		 */
		constexpr Fxp& operator-=(const Fxp& fxp)
		{
			value -= fxp.value;
			return *this;
		}

		/** @brief Multiply and assign another Fxp object to this object.
		 * @param fxp The Fxp object to multiply.
		 * @return A reference to this object.
		 */
		constexpr Fxp& operator*=(const Fxp& fxp)
		{
			if (std::is_constant_evaluated())
			{
				double a = this->value / 65536.0;
				double b = fxp.value / 65536.0;
				this->value = (a * b) * 65536.0;;
			}
			else
			{
				Uint32 mach;
				__asm__ volatile(
					"\tdmuls.l %[a], %[b]\n"
					"\tsts mach, %[mach]\n"
					"\tsts macl, %[out]\n"
					"\txtrct %[mach], %[out]\n"
					/* Output */
					: [mach] "=&r" (mach),
					[out] "=&r" (this->value)
					/* Input */
					: [a] "r" (this->value),
					[b] "r" (fxp.value)
					: "mach", "macl");
			}

			return *this;
		}

		/** @brief Multiply two Fxp objects.
		 * @param fxp The multiplier.
		 * @return The product as an Fxp object.
		 */
		constexpr Fxp operator*(const Fxp& fxp) const
		{
			return Fxp(*this) *= fxp;
		}

		/** @brief Divide two Fxp objects.
		 * @param fxp The divisor.
		 * @return The quotient as an Fxp object.
		 */
		constexpr Fxp operator/(const Fxp& fxp) const
		{
			return Fxp(*this) /= fxp;
		}

		/** @brief Divide and assign another Fxp object to this object.
		 * @param fxp The Fxp object to divide by.
		 * @return A reference to this object.
		 */
		constexpr Fxp& operator/=(const Fxp& fxp)
		{
			if (std::is_constant_evaluated())
			{
				double a = this->value / 65536.0;
				double b = fxp.value / 65536.0;
				this->value = (a / b) * 65536.0;;
			}
			else
			{
				Fxp::AsyncDivisionStart(*this, fxp);
				this->value = Fxp::AsyncDivisionGet().value;
			}
			
			return *this;
		}

		/** @brief Copy assignment operator.
		 * @param fxp The Fxp object to copy.
		 * @return A reference to this object.
		 */
		constexpr Fxp& operator=(const Fxp&) = default;

		/**  @brief Negate the value.
		 * @return The negated value as an Fxp object.
		 */
		constexpr Fxp operator-() const
		{
			return -this->value;
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if bigger
		 */
		constexpr bool operator>(const Sint32& integer) const
		{
			return this->value > (integer << 16);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same or bigger
		 */
		constexpr bool operator>=(const Sint32& integer) const
		{
			return this->value >= (integer << 16);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if smaller
		 */
		constexpr bool operator<(const Sint32& integer) const
		{
			return this->value < (integer << 16);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same or smaller
		 */
		constexpr bool operator<=(const Sint32& integer) const
		{
			return this->value <= (integer << 16);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same
		 */
		constexpr bool operator==(const Sint32& integer) const
		{
			return this->value == (integer << 16);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if not same
		 */
		constexpr bool operator!=(const Sint32& integer) const
		{
			return this->value != (integer << 16);
		}

		/** @brief Add another Fxp object to this object.
		 * @param fxp The Fxp object to add.
		 * @return The sum as an Fxp object.
		 */
		constexpr Fxp operator+(const Fxp& fxp) const
		{
			return this->value + fxp.value;
		}

		/** @brief Subtract another Fxp object from this object.
		 * @param fxp The Fxp object to subtract.
		 * @return The difference as an Fxp object.
		 */
		constexpr Fxp operator-(const Fxp& fxp) const
		{
			return this->value - fxp.value;
		}

		/** @brief Compare two Fxp objects for greater than.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is greater than the other; otherwise, `false`.
		 */
		constexpr bool operator>(const Fxp& fxp) const
		{
			return this->value > fxp.value;
		}

		/** @brief Compare two Fxp objects for greater than or equal.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is greater than or equal to the other; otherwise, `false`.
		 */
		constexpr bool operator>=(const Fxp& fxp) const
		{
			return this->value >= fxp.value;
		}

		/** @brief Compare two Fxp objects for less than.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is less than the other; otherwise, `false`.
		 */
		constexpr bool operator<(const Fxp& fxp) const
		{
			return this->value < fxp.value;
		}

		/** @brief Compare two Fxp objects for less than or equal.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is less than or equal the other; otherwise, `false`.
		 */
		constexpr bool operator<=(const Fxp& fxp) const
		{
			return this->value <= fxp.value;
		}

		/** @brief Compare two Fxp objects for equality.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is equal to the other; otherwise, `false`.
		 */
		constexpr bool operator==(const Fxp& fxp) const
		{
			return this->value == fxp.value;
		}

		/** @brief Compare two Fxp objects for inequality.
		 * @param fxp The Fxp object to compare with.
		 * @return `true` if this object is not equal to the other; otherwise, `false`.
		 */
		constexpr bool operator!=(const Fxp& fxp) const
		{
			return this->value != fxp.value;
		}

		/** @brief Right shift operator for logical right shift.
		 * @param shiftAmount The number of bits to shift.
		 * @return The result of the logical right shift as an Fxp object.
		 */
		constexpr Fxp operator>>(const size_t& shiftAmount) const
		{
			return this->value >> shiftAmount;
		}

		/**
		 * @brief Right shift and assign operator for logical right shift.
		 * @param shiftAmount The number of bits to shift.
		 * @return A reference to this object after the logical right shift.
		 */
		constexpr Fxp& operator>>=(const size_t& shiftAmount)
		{
			this->value >>= shiftAmount; return *this;
		}

		/**
		 * @brief Left shift operator for shifting the internal value by a specified number of bits.
		 * @param shiftAmount The number of bits to shift the internal value to the left.
		 * @return A new Fxp object with the internal value left-shifted by the specified amount.
		 */
		constexpr Fxp operator<<(const size_t& shiftAmount) const
		{
			return this->value << shiftAmount;
		}

		/**
		 * @brief In-place left shift operator for shifting the internal value by a specified number of bits.
		 * @param shiftAmount The number of bits to shift the internal value to the left.
		 * @return A reference to this Fxp object after left-shifting the internal value in place.
		 */
		constexpr Fxp& operator<<=(const size_t& shiftAmount)
		{
			this->value <<= shiftAmount;
			return *this;
		}
	};
}
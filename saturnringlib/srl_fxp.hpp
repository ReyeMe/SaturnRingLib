#pragma once

#include "srl_base.hpp"

namespace SRL::Types
{
	/** @brief Fixed point type and math by https://github.com/robertoduarte
	 */
	class Fxp
	{
	private:

		/** @brief CPU address
		 */
		static inline constexpr size_t CpuAddress = 0xFFFFF000UL;

		/** @brief Divisor register
		 */
		static inline auto& DivisorRegister = *reinterpret_cast<volatile uint32_t*>(CpuAddress + 0x0F00UL);

		/** @brief Dividend register
		 */
		static inline auto& DividendRegister = *reinterpret_cast<volatile uint32_t*>(CpuAddress + 0x0F10UL);

		/** @brief Result of the division register
		 */
		static inline auto& DivisionResultRegister = *reinterpret_cast<volatile uint32_t*>(CpuAddress + 0x0F14UL);

        /** @brief Total number of bits
		 */
        static constexpr int TotalBits = sizeof(int32_t) << 3;

        /** @brief Number of bits for the fractional part
		 */
        static constexpr int FractionBits = 16;

		/** @brief Fixed point value
		 */
		int32_t value;

		/** @brief Private constructor for creating Fxp objects from an int32_t value.
		 * @param inValue The int32_t value to store.
		 */
		constexpr Fxp(const int32_t& inValue) : value(inValue) {}

		/** @brief Private constructor for creating Fxp objects from an int32_t value.
		 * @param inValue The int32_t value to store.
		 */
		constexpr Fxp(const uint32_t& inValue) : value(inValue) {}

	public:

        /** @brief MAX valie
         */
        static inline constexpr uint32_t FxpMax = static_cast<uint32_t>((1 << (TotalBits - FractionBits)) - 1);

        /** @brief MIN valie
         */
        static inline constexpr uint32_t FxpMin = static_cast<uint32_t>(-(1 << (TotalBits - FractionBits - 1)));

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

        /**
         * @brief Returns the maximum representable value for the fixed-point type.
         * @return Fxp The maximum value.
         */
        static constexpr Fxp MaxValue()
        {
            return Fxp(FxpMax); // Maximum integer value
        }

        /**
         * @brief Returns the minimum representable value for the fixed-point type.
         * @return Fxp The minimum value.
         */
        static constexpr Fxp MinValue()
        {
            return Fxp(FxpMin); // Minimum integer value
        }

        /**
         * @brief ...
         * @return ...
         */
        static constexpr Fxp Mod(const Fxp& a, const Fxp& b)
        {
            // b should not be 0-
            return Fxp::FromRaw(a.Value() % b.Value());
        }

        /**
         * @brief ...
         * @return ...
         */
        static constexpr Fxp Abs(const Fxp& a)
        {
            return Fxp::FromRaw(a.Value() & ~0x80000000);
        }


		/** @brief Cast integer as fixed point value
		 * @param value Integer fixed point value
		 * @return Fixed point value
		 */
		static constexpr Fxp FromRaw(const int32_t& value)
		{
			return Fxp(value);
		}

		/** @brief Convert integer to fixed point by multipliing with 65536
		 * @param value Integer value
		 * @return Fixed point value
		 */
		static constexpr Fxp FromInt(const int32_t& value)
		{
			return Fxp(value << FractionBits);
		}

		/** @brief Starts division of two fixed point numbers
		 * @param dividend The dividend
		 * @param divisor The divisor
		 */
		static void AsyncDivisionStart(const Fxp& dividend, const Fxp& divisor)
		{
			uint32_t dividendh;
			__asm__ volatile("swap.w %[in], %[out]\n"
				: [out] "=&r"(dividendh)
				: [in] "r"(dividend.value));
			__asm__ volatile("exts.w %[in], %[out]"
				: [out] "=&r"(dividendh)
				: [in] "r"(dividendh));

			Fxp::DividendRegister = dividendh;
			Fxp::DivisorRegister = divisor.value;
			Fxp::DivisionResultRegister = dividend.value << FractionBits;
		}

		/** @brief Retires result of async division
		 * @note If division was not completed yet, this function block further execution until the division is done
		 * @return Division result
		 */
		static Fxp AsyncDivisionGet()
		{
			return static_cast<int32_t>(Fxp::DivisionResultRegister);
		}

		/** @brief Faster and slightly less accurate version of Sqrt.
		 * @note Due to interpolation, it has a maximum error of around 6.08%.
		 * @return The square root as an Fxp object.
		 */
		constexpr Fxp FastSqrt() const
		{
			int32_t baseEstimation = 0;
			int32_t estimation = this->value;

			if (estimation > 0)
			{
				if (estimation < 65536)
				{
					baseEstimation = 1 << 7;
					estimation <<= 7;

					int32_t iterationValue = this->value >> 1;
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
			uint32_t remainder = static_cast<uint32_t>(value);
			uint32_t root = 0;
			uint32_t bit = 0x40000000; // Set the initial bit to the highest bit for 32-bit integers

			while (bit > 0x40)
			{
				// Try the current bit in the root calculation
				uint32_t trial = root + bit;

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

			return static_cast<uint32_t>(root);
		}

		/** @brief Convert fixed point value to integer by dividing with 65536
		 * @return Integer value
		 */
		constexpr int32_t ToInt() const
		{
			return this->value >> FractionBits;
		}

		/** @brief Convert fixed point value to float by dividing with 65536
		 * @return float value
		 */
		constexpr float ToFloat() const
		{
			return this->value / 65536.0f;
		}

		/** @brief Convert fixed point value to double by dividing with 65536
		 * @return double value
		 */
		constexpr double ToDouble() const
		{
			return this->value / 65536.0;
		}

		/** @brief Gets raw value
		 * @return Raw fixed point value
		 */
		constexpr int32_t Value() const
		{
			return this->value;
		}

    /** @name Operators
     * @{
     */

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
				uint32_t mach;
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
		constexpr bool operator>(const int32_t& integer) const
		{
			return this->value > (integer << FractionBits);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same or bigger
		 */
		constexpr bool operator>=(const int32_t& integer) const
		{
			return this->value >= (integer << FractionBits);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if smaller
		 */
		constexpr bool operator<(const int32_t& integer) const
		{
			return this->value < (integer << FractionBits);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same or smaller
		 */
		constexpr bool operator<=(const int32_t& integer) const
		{
			return this->value <= (integer << FractionBits);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if same
		 */
		constexpr bool operator==(const int32_t& integer) const
		{
			return this->value == (integer << FractionBits);
		}

		/** @brief Compare fixed point value to the integer value
		 * @note Integer value gets multiplied by 65536 before conversion
		 * @param integer Integer value
		 * @return True if not same
		 */
		constexpr bool operator!=(const int32_t& integer) const
		{
			return this->value != (integer << FractionBits);
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

    /** @}
     */
	};
}

#pragma once

#include "srl_base.hpp"
#include "srl_fxp.hpp"
#include "srl_angle.hpp"
#include "srl_datetime.hpp"

namespace SRL
{
	/** @brief Math functions
	 */
	class Math
	{
	public:

        /** @brief Sinus operation
         * @param angle Angle value 
         * @return Sinus value
         */
        const static Types::Fxp Sin(Types::Angle angle)
        {
            return Types::Fxp::FromRaw(slSin(angle.Value()));
        }

        /** @brief Cosine operation
         * @param angle Angle value 
         * @return Cosine value
         */
        const static Types::Fxp Cos(Types::Angle angle)
        {
            return Types::Fxp::FromRaw(slCos(angle.Value()));
        }

		/// @brief Get absolute value
		/// @tparam ValueType Type of the value
		/// @param value Numeric value
		/// @return Absolute value
		template<typename ValueType>
		constexpr static ValueType Abs(const ValueType& value)
		{
			return value < 0 ? -value : value;
		}

		/// @brief Get maximum value of two values
		/// @tparam ValueType Type of the value
		/// @param first First value
		/// @param second Second value
		/// @return Maximum value
		template<typename ValueType>
		constexpr static ValueType Max(const ValueType& first, const ValueType& second)
		{
			return first > second ? first : second;
		}

		/// @brief Get minimum value of two values
		/// @tparam ValueType Type of the value
		/// @param first First value
		/// @param second Second value
		/// @return Minimum value
		template<typename ValueType>
		constexpr static ValueType Min(const ValueType& first, const ValueType& second)
		{
			return first < second ? first : second;
		}

		/// @brief Clamp value in the range
		/// @tparam ValueType Type of the value
		/// @param minimum Minimum of the range
		/// @param maximum Maximum of the range
		/// @param value Value to clamp
		/// @return Clamped value
		template<typename ValueType>
		constexpr static ValueType Clamp(const ValueType& minimum, const ValueType& maximum, const ValueType& value)
		{
			return Math::Min(maximum, Math::Max(minimum, value));
		}

        /** @brief Pseudo-Random number generator
         * @note This generator uses Xorshift (see https://en.wikipedia.org/wiki/Xorshift)
         */
        class Random
        {
        private:

            /** @brief Pseudo-Random number generator seed
             */
            uint32_t startState;

            /** @brief Get next pseudo-random number
             * @return Generated number
             */
            uint32_t GetNextPeriod()
            {
                // thx to thepuristofgreed from SX discord for showing this and comparing against my slower and less random version from last commit
                this->startState ^= this->startState << 13;
                this->startState ^= this->startState >> 17;
                this->startState ^= this->startState << 5;
                return this->startState;
            }

        public:
        
            /** @brief Construct a new pseudo-random number generator
             */
            Random()
            {
                Types::DateTime time = Types::DateTime::Now();
                this->startState = (time.Year() << 9) +
                    (time.Month() << 5) +
                    time.Day() +
                    time.Hour() +
                    time.Minute() +
                    time.Second();
            }

            /** @brief Construct a new pseudo-random number generator
             * @param seed Starting seed
             */
            Random(uint32_t seed)
            {
                this->startState = seed;
            }

            /** @brief Destroy the pseudo-random number generator object
             */
            ~Random() { }

            /** @brief Get next pseudo-random number
             * @return Generated number in a full range
             */
            uint32_t GetNumber()
            {
                return this->GetNextPeriod();
            }
            
            /** @brief Get next pseudo-random number
             * @param from Inclusive start of the range
             * @param to Inclusive end of the range
             * @return Generated number in a range
             */
            int32_t GetNumber(int32_t from, int32_t to)
            {
                uint32_t number = this->GetNumber();
                return from + (number % Math::Abs(to - from));
            }
        };
	};
}
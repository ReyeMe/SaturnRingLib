#pragma once

#include "srl_base.hpp"
#include "srl_fxp.hpp"
#include "srl_datetime.hpp"

namespace SRL
{
	/** @brief Math functions
	 */
	class Math
	{
	public:

		/// @brief Get absolute value
		/// @tparam ValueType Type of the value
		/// @param value Numberic value
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
         * @note This pseudo number generator is done by using LFSR (see https://en.wikipedia.org/wiki/Linear-feedback_shift_register)
         */
        class Random
        {
        private:

            /** @brief Fallback number choosen by different random number generator I stumbled on online
             */
            inline static const Uint32 FallbackRandomNumber = 0x7c1ec949u;

            /** @brief Pseudo-Random number generator seed
             */
            Uint32 startState;

            /** @brief Pseudo-Random number generator seed
             */
            Uint32 seed;

            /** @brief Get next pseudo-random number
             * @return Generated number
             */
            Uint32 GetNextPeriod()
            {
                Uint32 lfsr = this->startState;

                // Start state must not be 0
                if (lfsr == 0)
                {
                    // Seed is 0, just fall back
                    if (this->seed == 0)
                    {
                        lfsr = Random::FallbackRandomNumber;
                    }
                    else
                    {
                        lfsr = this->seed;
                    }
                }

                Uint32 bit;
                Uint32 period = 0;

                do
                {
                    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5) ^ (lfsr >> 16) ^ (lfsr >> 17) ^ (lfsr >> 18) ^ (lfsr >> 19)) & 1u;
                    lfsr = (lfsr >> 1) | (bit << 31);
                    ++period;
                }
                while (lfsr == this->startState);

                this->startState = lfsr;
                return lfsr;
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
                
                this->seed = this->startState;
                this->startState = this->GetNextPeriod();
            }

            /** @brief Construct a new pseudo-random number generator
             * @param seed Starting seed
             */
            Random(Uint32 seed)
            {
                this->startState = seed;
                this->seed = seed;
                this->startState = this->GetNextPeriod();
            }

            /** @brief Destroy the pseudo-random number generator object
             */
            ~Random() { }

            /** @brief Get next pseudo-random number
             * @return Generated number in a full range
             */
            Uint32 GetNumber()
            {
                return this->GetNextPeriod();
            }
            
            /** @brief Get next pseudo-random number
             * @param from Inclusive start of the range
             * @param to Inclusive end of the range
             * @return Generated number in a range
             */
            Sint32 GetNumber(Sint32 from, Sint32 to)
            {
                Uint32 number = this->GetNumber();
                return from + (number % Math::Abs(to - from));
            }
        };
	};
}
#pragma once

#include "srl_base.hpp"
#include "srl_fxp.hpp"

namespace SRL::Types
{
    /** @brief Angle value
     */
    struct Angle : SRL::SGL::SglType<Angle, ANGLE>
    {
    private:

        /** @brief Angle value
         */
        Sint16 value;

        /** @brief Construct a new instance from raw value
         */
		constexpr Angle(const Sint16& inValue) : value(inValue) {}
        
        /** @brief Construct a new empty instance
         */
		constexpr Angle() : value(0) {}

    public:

        /** @brief Construct a new instance from existing instance
         */
		constexpr Angle(const Angle& angle) : value(angle.value) {}

        /** @brief Construct a new instance from raw value
         * @param value Raw value
         * @return new Angle instance
         */
		static constexpr Angle FromRaw(const Sint16& value)
		{
			return Angle(value);
		}

        /** @brief Construct a new instance from degrees value
         * @param value Angle in degrees
         * @return new Angle instance
         */
		static constexpr Angle FromDegrees(const Sint16& value)
		{
			return Angle((Sint16)(Fxp::FromInt(value) / 360.0).Value());
		}

        /** @brief Construct a new instance from degrees value
         * @param value Angle in degrees
         * @return new Angle instance
         */
		static constexpr Angle FromDegrees(const Fxp& value)
		{
			return Angle((Sint16)(value / 360.0).Value());
		}

        /** @brief Construct a new instance from radians value
         * @param value Angle in radians
         * @return new Angle instance
         */
		static constexpr Angle FromRadians(const Fxp& value)
		{
			return Angle((Sint16)((value * 57.2958) / 360.0).Value());
		}

        /** @brief Convert angle to degrees
         * @return Angle in degrees
         */
		constexpr Fxp AsDegrees() const
		{
			return Fxp::FromRaw(this->value) * 360.0;
		}

        /** @brief Convert angle to radians
         * @return Angle in radians
         */
		constexpr Fxp AsRadians() const
		{
			return this->AsDegrees() * 0.0174533;
		}

        /** @brief Get raw value
         * @return Raw value 
         */
		constexpr Sint16 Value() const
		{
			return this->value;
		}

        /** @name Operators
         * @{
         */

        /** @brief Add two angles together
         * @param angle Other angle
         * @return Computation result 
         */
		constexpr Angle operator+(const Angle& angle) const
		{
			return Angle((Sint16)(this->value + angle.value));
		}

        /** @brief Subtract two angles from each other
         * @param angle Other angle
         * @return Computation result 
         */
		constexpr Angle operator-(const Angle& angle) const
		{
			return Angle((Sint16)(this->value - angle.value));
		}

        /** @brief Add two angles together and set the result to current instance
         * @param angle Other angle
         * @return Computation result 
         */
		constexpr Angle& operator+=(const Angle& angle)
		{
			value += angle.value;
			return *this;
		}

        /** @brief Subtract two angles from each other and set the result to current instance
         * @param angle Other angle
         * @return Computation result 
         */
		constexpr Angle& operator-=(const Angle& angle)
		{
			value -= angle.value;
			return *this;
		}

        /** @brief Set value of another instance to current instance
         * @param angle Other angle
         * @return Computation result 
         */
		constexpr Angle& operator=(const Angle&) = default;

        /** @brief Invert current angle value
         * @return Inverted angle
         */
		constexpr Angle operator-() const
		{
			return (Sint16)-this->value;
		}

        /** @brief Compare whether current angle is bigger than other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator>(const Angle& angle) const
		{
			return this->value > angle.value;
		}

        /** @brief Compare whether current angle is same or bigger than other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator>=(const Angle& angle) const
		{
			return this->value >= angle.value;
		}

        /** @brief Compare whether current angle is smaller than other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator<(const Angle& angle) const
		{
			return this->value < angle.value;
		}

        /** @brief Compare whether current angle is same or smaller than other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator<=(const Angle& angle) const
		{
			return this->value <= angle.value;
		}

        /** @brief Compare whether current angle is same as other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator==(const Angle& angle) const
		{
			return this->value == angle.value;
		}

        /** @brief Compare whether current angle is not same as other angle
         * @param angle Other angle value
         * @return Comparison result
         */
		constexpr bool operator!=(const Angle& angle) const
		{
			return this->value != angle.value;
		}
        
        /** @}
         */
    };
}
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

        /** @brief Single turn in degrees
         */
        inline static constexpr const Fxp SingleTurn = 360.0;

        /** @brief Conversion constant from radians to degrees
         */
        inline static constexpr const Fxp ToDegreesConversion = 57.2958;

        /** @brief Conversion constant from degrees to radians
         */
        inline static constexpr const Fxp ToRadiansConversion = 0.0174533;

        /** @brief Angle value
         */
        int16_t value;

        /** @brief Construct a new instance from existing instance
         */
        constexpr Angle(const int16_t& inValue) : value(inValue) {}

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
        static constexpr Angle FromRaw(const int16_t& value)
        {
            return Angle(value);
        }

        /** @brief Construct a new instance from degrees value
         * @param value Angle in degrees
         * @return new Angle instance
         */
        static constexpr Angle FromDegrees(const int16_t& value)
        {
            return Angle((int16_t)(Fxp::FromInt(value) / Angle::SingleTurn).Value());
        }

        /** @brief Construct a new instance from degrees value
         * @param value Angle in degrees
         * @return new Angle instance
         */
        static constexpr Angle FromDegrees(const Fxp& value)
        {
            return Angle((int16_t)(value / Angle::SingleTurn).Value());
        }

        /** @brief Construct a new instance from radians value
         * @param value Angle in radians
         * @return new Angle instance
         */
        static constexpr Angle FromRadians(const Fxp& value)
        {
            return Angle((int16_t)((value * Angle::ToDegreesConversion) / Angle::SingleTurn).Value());
        }

        /** @brief Convert angle to degrees
         * @return Angle in degrees
         */
        constexpr Fxp AsDegrees() const
        {
            return Fxp::FromRaw(this->value) * Angle::SingleTurn;
        }

        /** @brief Convert angle to radians
         * @return Angle in radians
         */
        constexpr Fxp AsRadians() const
        {
            return this->AsDegrees() * Angle::ToRadiansConversion;
        }

        /** @brief Get raw value
         * @return Raw value
         */
        constexpr int16_t Value() const
        {
            return this->value;
        }

        // Normalize the angle to the range [0, 360)
        Angle NormalizeDegree() const
        {
            Fxp normalizedValue = Fxp::Mod(this->AsDegrees(), Fxp(360.0)); // Compute remainder
            if (normalizedValue < Fxp(0.0)) {
                normalizedValue += Fxp(360.0); // Adjust to be positive
            }
            return Angle::FromDegrees(normalizedValue);
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
            return Angle(static_cast<int16_t>(this->value + angle.value));
        }

        /** @brief Subtract two angles from each other
         * @param angle Other angle
         * @return Computation result
         */
        constexpr Angle operator-(const Angle& angle) const
        {
            return Angle(static_cast<int16_t>(this->value - angle.value));
        }

        /** @brief Multiplies two angles together
         * @param angle Other angle
         * @return Computation result
         */
        constexpr Angle operator*(const Angle& angle) const
        {
            return Angle(static_cast<int16_t>(this->value * angle.value));
        }

        /** @brief Divides two angles from each other
         * @param angle Other angle
         * @return Computation result
         */
        constexpr Angle operator/(const Angle& angle) const
        {
            return Angle(static_cast<int16_t>(this->value / angle.value));
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
         * @return Computation result
         */
        constexpr Angle& operator=(const Angle&) = default;

        /** @brief Invert current angle value
         * @return Inverted angle
         */
        constexpr Angle operator-() const
        {
            return this->value ^ 0x8000;
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

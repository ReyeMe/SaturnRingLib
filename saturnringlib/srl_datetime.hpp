# pragma once

#include "srl_base.hpp"
#include "srl_debug.hpp"

namespace SRL::Types
{
    /** @brief Time management
     */
    struct DateTime
    {
    public:

        /** @brief Base year to count time from
         */
        static constexpr const uint16_t BaseYear = 1980;

    private:

        /** @brief Number of seconds to start of each month in normal year
         */
        static constexpr const uint32_t FromMonth[] = { 0, 2678400, 5097600, 777600, 10368000, 13046400, 15638400, 18316800, 20995200, 23587200, 26265600, 28857600 };

        /** @brief Number of seconds in a minute
         */
        static constexpr const uint16_t FromMinute = 60;

        /** @brief Number of seconds in a hour
         */
        static constexpr const uint16_t FromHour = 3600;

        /** @brief Number of seconds in a day
         */
        static constexpr const uint32_t FromDay = 86400;

        /** @brief Current year
         */
        uint16_t year;

        /** @brief Current month
         */
        uint8_t month;

        /** @brief Current week
         */
        uint8_t week;

        /** @brief Current day
         */
        uint8_t day;

        /** @brief Current hour
         */
        uint8_t hour;

        /** @brief Current number of minutes
         */
        uint8_t minute;

        /** @brief Current number of seconds
         */
        uint8_t second;

        /** @brief Does the year number points to a leap year
         * @return true if is leap year
         */
        static constexpr bool IsLeapYear(uint16_t year)
        {
            return ((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
        }

        /** @brief Construct a new time object
         */
        DateTime() : year(DateTime::BaseYear), month(1), week(0), day(1), hour(0), minute(0), second(0) { }

        /** @brief Gets number of seconds elapsed since start of the year
         * @return uint32_t
         */
        static uint32_t GetSecondsThisYear(const DateTime& date)
        {
            uint32_t result = date.second +
                (date.minute * DateTime::FromMinute) +
                (date.hour * DateTime::FromHour) +
                ((date.day - 1) * DateTime::FromDay) +
                DateTime::FromMonth[date.month - 1];

            if (date.month > 2 && DateTime::IsLeapYear(date.year))
            {
                result += DateTime::FromDay;
            }

            return result;
        }

    public:

        /** @brief Construct a new time object
         */
        DateTime(uint8_t second, uint8_t minute, uint8_t hour, uint8_t day, uint8_t week, uint8_t month, uint16_t year) :
            year(year),
            month(month),
            week(week),
            day(day),
            hour(hour),
            minute(minute),
            second(second)
        {
            if (year < DateTime::BaseYear)
            {
                SRL::Debug::Assert("Year value is out of range!\nMinimal possible year is 1980");
                year = DateTime::BaseYear;
            }

            if (month < 1)
            {
                SRL::Debug::Assert("Month value is out of range!\nMinimal possible month is 1");
                month = 1;
            }
            else if (month > 12)
            {
                SRL::Debug::Assert("Month value is out of range!\nMaximal possible month is 12");
                month = 1;
            }

            if (day < 1)
            {
                SRL::Debug::Assert("Day value is out of range!\nMinimal possible day is 1");
                day = 1;
            }
        }

        /** @brief Construct a new time object from backup unit date
         * @param date Backup unit date
         */
        DateTime(const BupDate* date) : DateTime(0, date->min, date->time, date->day, date->week, date->month, date->year + DateTime::BaseYear) { }

        /** @brief Gets current RTC value
         * @return Current time
         */
        static DateTime Now()
        {
            SmpcDateTime* currentState = &(Smpc_Status->rtc);

            return DateTime(
                slDec2Hex(currentState->second),
                slDec2Hex(currentState->minute),
                slDec2Hex(currentState->hour),
                slDec2Hex(currentState->date),
                currentState->month >> 4,
                currentState->month & 0x0f,
                slDec2Hex(currentState->year)
            );
        }

        /** @brief Gets date year
         * @return Date year
         */
        constexpr uint16_t Year()
        {
            return this->year;
        }

        /** @brief Gets date month
         * @return Date month
         */
        constexpr uint8_t Month()
        {
            return this->month;
        }

        /** @brief Gets date week
         * @return Date week
         */
        constexpr uint8_t Week()
        {
            return this->week;
        }

        /** @brief Gets date day
         * @return Date day
         */
        constexpr uint8_t Day()
        {
            return this->day;
        }

        /** @brief Gets clock hours
         * @return Clock hours
         */
        constexpr uint8_t Hour()
        {
            return this->hour;
        }

        /** @brief Gets clock minutes
         * @return Clock minutes
         */
        constexpr uint8_t Minute()
        {
            return this->minute;
        }

        /** @brief Gets clock seconds
         * @return Clock seconds
         */
        constexpr uint8_t Second()
        {
            return this->second;
        }

        /** @brief Convert date into backup unit format
         * @return Backup unit date
         */
        constexpr BupDate ToBackupUnitDate()
        {
            return BupDate
            {
                (uint8_t)(this->year - DateTime::BaseYear),
                this->month,
                this->day,
                this->hour,
                this->minute,
                this->week
            };
        }

        /** @name Operators
         * @{
         */

        /** @brief Check if current date comes after another date
         * @param date The date
         * @return true if comes after current date
         */
        constexpr bool operator>(const DateTime& date) const
        {
            return this->year > date.year ||
                (this->year == date.year && DateTime::GetSecondsThisYear(*this) > DateTime::GetSecondsThisYear(date));
        }

        /** @brief Check if current date comes after or is same as another date
         * @param date The date
         * @return true if comes after or is same as current date
         */
        constexpr bool operator>=(const DateTime& date) const
        {
            return this->year > date.year ||
                (this->year == date.year && DateTime::GetSecondsThisYear(*this) >= DateTime::GetSecondsThisYear(date));
        }

        /** @brief Check if current date comes before another date
         * @param date The date
         * @return true if comes before current date
         */
        constexpr bool operator<(const DateTime& date) const
        {
            return this->year < date.year ||
                (this->year == date.year && DateTime::GetSecondsThisYear(*this) < DateTime::GetSecondsThisYear(date));
        }

        /** @brief Check if current date comes before or is same as another date
         * @param date The date
         * @return true if comes before or is same as current date
         */
        constexpr bool operator<=(const DateTime& date) const
        {
            return this->year < date.year ||
                (this->year == date.year && DateTime::GetSecondsThisYear(*this) <= DateTime::GetSecondsThisYear(date));
        }

        /** @brief Check if current date is same as another date
         * @param date The date
         * @return true if current date is same as another date
         */
        constexpr bool operator==(const DateTime& date) const
        {
            return this->year == date.year &&
                this->month == date.month &&
                this->week == date.week &&
                this->day == date.day &&
                this->hour == date.hour &&
                this->minute == date.minute &&
                this->second == date.second;
        }

        /** @brief Check if current date is not the same as another date
         * @param date The date
         * @return true if current date is not the same as another date
         */
        constexpr bool operator!=(const DateTime& date) const
        {
            return this->year != date.year ||
                (this->year == date.year && DateTime::GetSecondsThisYear(*this) != DateTime::GetSecondsThisYear(date));
        }

        /** @}
         */
    };
}

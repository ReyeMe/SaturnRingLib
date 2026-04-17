#pragma once

#include <stdint.h>
#include "srl_base.hpp"
#include "srl_interrupt.hpp"
#include "srl_system.hpp"

namespace SRL
{
    /** @brief High-precision 48-bit timestamp with DVU hardware acceleration.
     *  @details Stores timer values as a 48-bit composite (32-bit overflow counter + 16-bit FRT).
     *  The internal representation uses 64-bit storage (high=Timer32, low=FRT<<16) to
     *  enable hardware-accelerated division via the SH-2 DVU.
     *
     *  @par Architecture & PHI_128 Mode:
     *  This class is designed specifically for the Sega Saturn's FRT (Free Running Timer) running
     *  in **PHI_128 mode** (~222 kHz, ~4.47μs precision). This matches SGL's default FRT
     *  configuration. Changing the FRT clock mode after initialization will break all
     *  timing calculations.
     *
     *  - FRT (Free Running Timer): 16-bit hardware counter at PHI_128 (~222 kHz)
     *  - timer32: 32-bit overflow counter incremented each time FRT wraps (~295ms)
     *  - 48-bit total range: 0 to 2^48-1 ticks (~673 days at PHI_128)
     *
     *  @par Divider Strategy:
     *  The DVU performs 64-bit / 32-bit division: (ticks << 16) / divisor
     *  where divisor = frequency / 128, matching the PHI_128 tick rate directly.
     *  This gives correct Fxp 16.16 results without any tick normalization shifts.
     *
     *  Example: 1 second at 26.6875 MHz
     *  - Ticks = 26,687,500 / 128 = 208,496
     *  - Dividend = 208,496 << 16 = 13,667,041,280
     *  - Divisor = 26,687,500 / 128 = 208,496
     *  - Result = 65,536 = 1.0 in Fxp 16.16 ✓
     *
     *  @par Fxp Conversion Limits:
     *  While the Tickstamp can store up to ~673 days, conversion functions return
     *  Fxp (16.16 format) which has a hard limit of 32767 in the integer part:
     *  - ToMilliseconds(): Maximum ~32767 ms (~32.8 seconds)
     *  - ToSeconds():      Maximum ~32767 seconds (~9.1 hours)
     *  - ToMinutes():      Maximum ~32767 minutes (~22.7 days)
     *  - ToClock():        Maximum ~546 hours (~22.7 days), returns ClockTime struct
     *  For durations exceeding Fxp limits, use ToClock() which returns individual components.
     *
     *  @par Usage Example:
     *  @code
     *  Tickstamp start = Timer::Capture();          // Capture start time
     *  // ... do some work ...
     *  Tickstamp end = Timer::Capture();            // Capture end time
     *  Tickstamp elapsed = end - start;             // Calculate elapsed (64-bit subtraction)
     *  Fxp seconds = elapsed.ToSeconds();           // Convert to seconds (Fxp 16.16)
     *  @endcode
     *
     *  @warning This class assumes FRT is configured in PHI_128 mode (SGL default).
     *  Behavior is undefined if the FRT clock mode is changed after initialization.
     */
    class Tickstamp final
    {
    public:
        /** @brief Immutable clock display format (HH:MM:SS.mmm).
         *  @details Returned by Tickstamp::ToClock(). Uses only 1 DVU division
         *  internally (minutes-based), then integer arithmetic for split.
         *  Intended for GUI display only — not for precise calculations
         *  or comparisons. Uses pure truncation throughout the chain
         *  (minutes → seconds → milliseconds) for consistent display.
         *
         *  @par Precision chain:
         *  - Minutes: 16-bit Fxp fraction → ~0.9ms precision per second
         *  - Seconds: derived from fractional minutes × 60 → 0–59
         *  - Milliseconds: derived from fractional seconds × 1000 → 0–999 (~0.015ms precision)
         *
         *  @par Range:
         *  - Maximum: ~546 hours (~22.7 days), limited by ToMinutes() Fxp range
         *  - Minimum: 0 hours, 0 minutes, 0 seconds, 0 milliseconds
         *
         *  @par Immutability:
         *  Fields are read-only. Only Tickstamp::ToClock() can construct instances.
         *  This prevents misuse in time comparisons where Fxp methods should be used.
         */
        struct ClockTime
        {
        private:
            /// @cond Internal
            // Grant access to Tickstamp for private constructor
            friend struct Tickstamp;
            /// @endcond
            uint16_t hours;
            uint16_t milliseconds;
            uint8_t  minutes;
            uint8_t  seconds;

            /** @brief Private constructor, only accessible by Tickstamp::ToClock(). */
            constexpr ClockTime(uint16_t h, uint8_t m, uint8_t s, uint16_t ms) noexcept
                : hours(h), milliseconds(ms), minutes(m), seconds(s)
            {
            }

        public:
            /** @brief Default constructor. Initializes to 00:00:00.000. */
            constexpr ClockTime() noexcept : hours(0), milliseconds(0), minutes(0), seconds(0) {}

            uint16_t Hours()        const noexcept { return hours; }         ///< Hours component (0–546)
            uint8_t  Minutes()      const noexcept { return minutes; }       ///< Minutes component (0–59)
            uint8_t  Seconds()      const noexcept { return seconds; }       ///< Seconds component (0–59)
            uint16_t Milliseconds() const noexcept { return milliseconds; }  ///< Milliseconds component (0–999)
        };
    private:
        /// @cond Internal
        // Grant access to TimerTest for testing
        friend class TimerTest;
        /// @endcond
#ifdef SRL_MODE_PAL
        static constexpr float Base26MhzCPUFrequency = 26874100;  ///< PAL 26MHz base frequency
        static constexpr float Base28MhzCPUFrequency = 28636360;  ///< PAL 28MHz base frequency
#else
        static constexpr float Base26MhzCPUFrequency = 26687500;  ///< NTSC 26.6875 MHz actual
        static constexpr float Base28MhzCPUFrequency = 28437500;  ///< NTSC 28.4375 MHz actual
#endif

        /** @brief Divider configuration for DVU 64/32 division.
         *  @details Stores pre-calculated divisors for time-to-tick conversions.
         *  Divisors use frequency/128 to match PHI_128 tick rate directly.
         *
         *  @par DVU Division:
         *  The DVU performs (ticks << 16) / divisor, where:
         *  1. Ticks at PHI_128 rate: seconds * (frequency / 128)
         *  2. Dividend = ticks << 16 (stored as High:Low in Tickstamp)
         *  3. Divisor = frequency / 128 (matches tick rate)
         *  4. Result = ticks * 65536 / (frequency / 128) = seconds * 65536 (Fxp 16.16)
         */
        struct DividerConfig
        {
            const uint32_t SecondsDivider;       ///< For ToSeconds(): frequency / 128
            const uint32_t MillisecondsDivider;  ///< For ToMilliseconds(): frequency / 128 / 1000
            const uint32_t MinutesDivider;       ///< For ToMinutes(): frequency / 128 * 60
        };

        /** @brief 26MHz divider configuration (NTSC: 26.6875 MHz, PAL: 26.8741 MHz) */
        static inline constexpr DividerConfig DividerConfig26Mhz = {
            .SecondsDivider = static_cast<uint32_t>(Base26MhzCPUFrequency / 128),
            .MillisecondsDivider = static_cast<uint32_t>(Base26MhzCPUFrequency / 128 / 1000),
            .MinutesDivider = static_cast<uint32_t>(Base26MhzCPUFrequency / 128 * 60)
        };

        /** @brief 28MHz divider configuration (NTSC: 28.4375 MHz, PAL: 28.63636 MHz) */
        static inline constexpr DividerConfig DividerConfig28Mhz = {
            .SecondsDivider = static_cast<uint32_t>(Base28MhzCPUFrequency / 128),
            .MillisecondsDivider = static_cast<uint32_t>(Base28MhzCPUFrequency / 128 / 1000),
            .MinutesDivider = static_cast<uint32_t>(Base28MhzCPUFrequency / 128 * 60)
        };

        static inline const DividerConfig* dividerConfig = &DividerConfig26Mhz;
        /// @cond Internal
        // Grant access to Timer class for private configuration
        friend class Timer;
        /// @endcond

        /** @brief Construct from raw internal values (no shift).
         *  @internal Used by operator- to return pre-shifted results.
         */
        static Tickstamp FromRaw(uint32_t high, uint32_t low) noexcept
        {
            Tickstamp ts;
            ts.High = high;
            ts.Low = low;
            return ts;
        }

        /** @brief Initializes divider configuration based on current system clock mode.
         *  @details Selects appropriate divider configuration (26MHz or 28MHz)
         *  based on the detected system clock mode.
         */
        static void InitDivider()
        {
            dividerConfig = (System::GetClockMode() == System::ClockMode::Mode26MHz) ? &DividerConfig26Mhz : &DividerConfig28Mhz;
        }

        /** @brief Returns the active seconds divisor for current clock mode.
         *  @details Returns the divisor value used for converting ticks to seconds.
         *  The divisor depends on the current system clock mode (26MHz or 28MHz).
         *  Primarily useful for debugging or validation of timing calculations.
         *  @return The seconds divisor value (frequency / 128).
         */
        static uint32_t GetSecondsDivider() { return dividerConfig->SecondsDivider; }

        /** @brief Returns the active milliseconds divisor for current clock mode.
         *  @details Returns the divisor value used for converting ticks to milliseconds.
         *  The divisor depends on the current system clock mode (26MHz or 28MHz).
         *  Primarily useful for debugging or validation of timing calculations.
         *  @return The milliseconds divisor value (frequency / 128 / 1000).
         */
        static uint32_t GetMillisecondsDivider() { return dividerConfig->MillisecondsDivider; }

        /** @brief Overrides automatic divider selection with manual choice.
         *  @param use26Mhz If true, uses 26MHz divider configuration; otherwise uses 28MHz.
         *  @details Allows manual override of the automatic clock mode detection
         *  for testing or specific hardware requirements.
         */
        static void OverrideDivider(bool use26Mhz)
        {
            dividerConfig = use26Mhz ? &DividerConfig26Mhz : &DividerConfig28Mhz;
        }

        /** @brief Reusable DVU 64/32-bit division helper.
         *  @param divisor 32-bit divisor for the division
         *  @param high High 32 bits of 48-bit dividend
         *  @param low Low 16 bits of 48-bit dividend (in upper 16 bits of uint32_t)
         *  @return 32-bit result from DVU (lower 32 bits of 64/32 result)
         *  @details Performs 64-bit division using SH-2 DVU hardware.
         *  Critical: DVSR must be written before DVDNTL to trigger operation.
         */
        static inline uint32_t Division64By32(uint32_t divisor, uint32_t high, uint32_t low) noexcept
        {
            // Use the same register access as Timer class
            constexpr uintptr_t dvuBase = 0xFFFFF000;  ///< DVU hardware register base address
            volatile uint32_t& DivisorRegister = *reinterpret_cast<volatile uint32_t*>(dvuBase + 0x0F00);
            volatile uint32_t& DividendHighRegister = *reinterpret_cast<volatile uint32_t*>(dvuBase + 0x0F10);
            volatile uint32_t& DividendLowRegister = *reinterpret_cast<volatile uint32_t*>(dvuBase + 0x0F14);

            DivisorRegister = divisor;
            DividendHighRegister = high;
            DividendLowRegister = low;
            return DividendLowRegister;
        }

        /** @brief Direct high/FRT constructor.
         *  @param h High 32 bits (Timer32 overflow counter)
         *  @param frt_val FRT register value (16-bit)
         *  @details Stores values directly. The FRT is placed in the upper 16 bits of low
         *  (big-endian layout) for DVU 64-bit division compatibility.
         *
         *  @par Example:
         *  @code
         *  Tickstamp ts(5, 12345);  // 5 overflows, FRT=12345
         *  @endcode
         */
        constexpr Tickstamp(uint32_t overflowCounter, uint16_t frtValue) : High(overflowCounter), Low(static_cast<uint32_t>(frtValue) << 16) {}
        //@}

    public:

        /** @brief High 32 bits of the 48-bit timestamp.
         *  @details Contains the Timer32 overflow counter, incremented each time
         *  the 16-bit FRT wraps around. Combined with Low, forms a 48-bit timestamp
         *  with ~673 days of range at PHI_128 tick rate.
         */
        uint32_t High;  ///< High 32 bits (overflow counter)

        /** @brief Low 32 bits formatted for DVU division.
         *  @details Contains the 16-bit FRT value shifted left by 16 bits (FRT << 16).
         *  This layout enables efficient 64/32-bit division using the SH-2's DVU
         *  hardware unit, where the actual tick value is (Low >> 16).
         */
        uint32_t Low;   ///< Low 32 bits for DVU (FRT << 16)

        /** @brief Default constructor. Initializes to zero. */
        Tickstamp() = default;

        /** @name Compile-Time Builders
         * Create Tickstamps at compile time with no runtime conversion overhead.
         * These builders calculate both 26MHz and 28MHz tick counts at compile time,
         * storing two complete Tickstamps, then return a reference to the appropriate
         * one at runtime based on the active divider.
         */
         //@{
         /** @brief Create Tickstamp from raw tick count at compile time.
         *  @param ticks Total tick count as 48-bit value
         *  @return Tickstamp initialized with the specified tick count (frequency-independent)
         *
         *  @par Example:
         *  @code
         *  constexpr auto ts = Tickstamp::FromTicks(1000000);  // 1M ticks
         *  @endcode
         */
        constexpr static Tickstamp FromTicks(uint64_t ticks)
        {
            return Tickstamp(
                static_cast<uint32_t>(ticks >> 16),
                static_cast<uint16_t>(ticks & 0xFFFF)
            );
        }

        /** @brief Create Tickstamp from seconds at compile time (dual-frequency).
         *  @tparam Seconds Time value in seconds as float template parameter
         *  @return const reference to the appropriate Tickstamp for current frequency
         *  @details Calculates two complete Tickstamps at compile time (26MHz and 28MHz),
         *  stores them in static constexpr variables, then returns a reference
         *  to the correct one at runtime.
         *
         *  @par Example:
         *  @code
         *  const auto& oneSecond = Tickstamp::FromSeconds<1.0f>();  // Runtime selection
         *  Fxp secs = oneSecond.ToSeconds();  // Already correct for current frequency
         *  @endcode
         */
        template<float Seconds>
        static const Tickstamp& FromSeconds()
        {
            static constexpr Tickstamp ts26 = FromTicks(static_cast<uint64_t>(Seconds * (Base26MhzCPUFrequency / 128.0f)));
            static constexpr Tickstamp ts28 = FromTicks(static_cast<uint64_t>(Seconds * (Base28MhzCPUFrequency / 128.0f)));
            return (dividerConfig == &DividerConfig26Mhz) ? ts26 : ts28;
        }

        /** @brief Create Tickstamp from milliseconds at compile time (dual-frequency).
         *  @tparam Milliseconds Time value in milliseconds as float template parameter
         *  @return const reference to the appropriate Tickstamp for current frequency
         *  @details Calculates two complete Tickstamps at compile time (26MHz and 28MHz),
         *  stores them in static constexpr variables, then returns a reference
         *  to the correct one at runtime.
         *
         *  @par Example:
         *  @code
         *  const auto& frameTime = Tickstamp::FromMilliseconds<16.667f>();
         *  Fxp ms = frameTime.ToMilliseconds();  // Already correct for current frequency
         *  @endcode
         */
        template<float Milliseconds>
        static const Tickstamp& FromMilliseconds()
        {
            static constexpr Tickstamp ts26 = FromTicks(static_cast<uint64_t>(Milliseconds * (Base26MhzCPUFrequency / 128.0f / 1000.0f)));
            static constexpr Tickstamp ts28 = FromTicks(static_cast<uint64_t>(Milliseconds * (Base28MhzCPUFrequency / 128.0f / 1000.0f)));
            return (dividerConfig == &DividerConfig26Mhz) ? ts26 : ts28;
        }

        /** @brief Create Tickstamp from minutes at compile time (dual-frequency).
         *  @tparam Minutes Time value in minutes as float template parameter
         *  @return const reference to the appropriate Tickstamp for current frequency
         *  @details Calculates two complete Tickstamps at compile time (26MHz and 28MHz),
         *  stores them in static constexpr variables, then returns a reference
         *  to the correct one at runtime.
         *
         *  @par Example:
         *  @code
         *  const auto& fiveMins = Tickstamp::FromMinutes<5.0f>();
         *  Fxp mins = fiveMins.ToMinutes();  // Already correct for current frequency
         *  @endcode
         */
        template<float Minutes>
        static const Tickstamp& FromMinutes()
        {
            static constexpr Tickstamp ts26 = FromTicks(static_cast<uint64_t>(Minutes * (Base26MhzCPUFrequency / 128.0f * 60.0f)));
            static constexpr Tickstamp ts28 = FromTicks(static_cast<uint64_t>(Minutes * (Base28MhzCPUFrequency / 128.0f * 60.0f)));
            return (dividerConfig == &DividerConfig26Mhz) ? ts26 : ts28;
        }
        //@}

        /** @name Core Operations
         * Essential timer operations for time calculations.
         */
         //@{
        /** @brief 64-bit subtraction with borrow (hardware-accelerated).
         *  @param other Tickstamp to subtract from this one (this - other)
         *  @return New Tickstamp containing the difference
         *  @details Performs atomic 64-bit subtraction using inline SH-2 assembly (subc).
         *  The operation handles overflow/borrow correctly across the 64-bit range.
         *
         *  @par Operation:
         *  @code
         *  result = this - other;  // 64-bit subtraction with carry/borrow
         *  @endcode
         *
         *  @par Example:
         *  @code
         *  Tickstamp later = Timer::Capture();
         *  Tickstamp earlier = Timer::Capture();
         *  Tickstamp diff = later - earlier;  // Time elapsed between captures
         *  @endcode
         *
         *  @note This is the primary method for calculating time deltas. The result
         *  maintains full 48-bit precision for subsequent conversion.
         */
        Tickstamp operator-(const Tickstamp& other) const noexcept
        {
            uint32_t temp_high = this->High;
            uint32_t temp_low = this->Low;
            __asm__ volatile (
                "clrt\n\t"
                "subc %[other_low], %[temp_low]\n\t"
                "subc %[other_high], %[temp_high]"
                : [temp_high] "+&r"(temp_high), [temp_low] "+&r"(temp_low)
                : [other_high] "r"(other.High), [other_low] "r"(other.Low)
                : "t"
                );
            return FromRaw(temp_high, temp_low);
        }

        /** @brief 64-bit addition with carry (hardware-accelerated).
         *  @param other Tickstamp to add to this one
         *  @return New Tickstamp containing the sum
         *  @details Performs atomic 64-bit addition using inline SH-2 assembly (addc).
         *
         *  @par Example:
         *  @code
         *  Tickstamp now = Timer::Capture();
         *  Tickstamp delay = Tickstamp::FromSeconds<2.0f>();
         *  Tickstamp deadline = now + delay;  // 2 seconds from now
         *  @endcode
         */
        Tickstamp operator+(const Tickstamp& other) const noexcept
        {
            uint32_t temp_high = this->High;
            uint32_t temp_low = this->Low;
            __asm__ volatile (
                "clrt\n\t"
                "addc %[other_low], %[temp_low]\n\t"
                "addc %[other_high], %[temp_high]"
                : [temp_high] "+&r"(temp_high), [temp_low] "+&r"(temp_low)
                : [other_high] "r"(other.High), [other_low] "r"(other.Low)
                : "t"
                );
            return FromRaw(temp_high, temp_low);
        }

        /** @brief Equality comparison.
         *  @param other Tickstamp to compare against
         *  @return True if both timestamps represent the same point in time
         */
        bool operator==(const Tickstamp& other) const noexcept
        {
            return this->High == other.High && this->Low == other.Low;
        }

        /** @brief Inequality comparison.
         *  @param other Tickstamp to compare against
         *  @return True if timestamps differ
         */
        bool operator!=(const Tickstamp& other) const noexcept
        {
            return !(*this == other);
        }

        /** @brief Less-than comparison.
         *  @param other Tickstamp to compare against
         *  @return True if this timestamp is earlier than other
         */
        bool operator<(const Tickstamp& other) const noexcept
        {
            return this->High < other.High || (this->High == other.High && this->Low < other.Low);
        }

        /** @brief Greater-than comparison.
         *  @param other Tickstamp to compare against
         *  @return True if this timestamp is later than other
         */
        bool operator>(const Tickstamp& other) const noexcept
        {
            return other < *this;
        }

        /** @brief Less-than-or-equal comparison.
         *  @param other Tickstamp to compare against
         *  @return True if this timestamp is earlier than or equal to other
         */
        bool operator<=(const Tickstamp& other) const noexcept
        {
            return !(other < *this);
        }

        /** @brief Greater-than-or-equal comparison.
         *  @param other Tickstamp to compare against
         *  @return True if this timestamp is later than or equal to other
         */
        bool operator>=(const Tickstamp& other) const noexcept
        {
            return !(*this < other);
        }

        /** @brief Converts ticks to milliseconds using DVU hardware acceleration.
         *  @return Time in milliseconds as fixed-point number (Fxp 16.16 format).
         *  @details Uses SH-2 DVU for 64-bit division: (ticks << 16) / (frequency / 1000).
         *  Provides millisecond resolution but with reduced maximum range.
         *
         *  @par Range:
         *  - Minimum: ~0.00447 milliseconds (1 tick at PHI_128)
         *  - **Maximum: 32767 milliseconds (~32.8 seconds)**
         *  - Overflow: Values above 32767 will wrap in Fxp 16.16 format
         *
         *  @par When to Use:
         *  Use this method when you need millisecond precision for short durations
         *  (e.g., animation timing, input delays, frame timing). For longer durations
         *  (>30 seconds), use ToSeconds() instead to avoid overflow.
         *
         *  @warning **CRITICAL**: The 32767 ms limit (~32.8 seconds) is much smaller
         *  than ToSeconds(). If you need to measure durations longer than
         *  30 seconds, always use ToSeconds().
         *
         *  @par Example:
         *  @code
         *  Tickstamp elapsed = Timer::DeltaTicks();
         *  Fxp ms = elapsed.ToMilliseconds();
         *  if (ms > 16.0) {  // More than 16ms (>60fps)
         *      // Handle slow frame
         *  }
         *  @endcode
         *
         *  @see ToSeconds() for longer range (up to 9.1 hours)
         */
        Math::Types::Fxp ToMilliseconds() const noexcept
        {
            return Math::Types::Fxp::BuildRaw(Division64By32(dividerConfig->MillisecondsDivider, High, Low));
        }

        /** @brief Converts ticks to seconds using DVU hardware acceleration.
         *  @return Time in seconds as fixed-point number (Fxp 16.16 format).
         *  @details Uses SH-2 DVU for 64-bit division: (ticks << 16) / frequency.
         *  The result is in 16.16 fixed-point format with hardware precision.
         *
         *  @par Range:
         *  - Minimum: ~0.00000447 seconds (1 tick at PHI_128)
         *  - **Maximum: 32767 seconds (~9.1 hours)**
         *  - Overflow: Values above 32767 will wrap in Fxp 16.16 format
         *
         *  @par Accuracy:
         *  Typical accuracy within ±0.1% for normal game timing (1ms to 1 hour).
         *
         *  @warning **CRITICAL**: The return type Fxp 16.16 has a hard limit of 32767.
         *  If your ticks represent more than 32767 seconds, the result will
         *  overflow and produce incorrect values.
         *
         *  @par Example:
         *  @code
         *  Tickstamp elapsed = Timer::DeltaTicks();
         *  Fxp seconds = elapsed.ToSeconds();
         *  @endcode
         *
         *  @see ToMilliseconds() for millisecond precision (shorter range)
         */
        Math::Types::Fxp ToSeconds() const noexcept
        {
            return Math::Types::Fxp::BuildRaw(Division64By32(dividerConfig->SecondsDivider, High, Low));
        }

        /** @brief Converts ticks to minutes using DVU hardware acceleration.
         *  @return Time in minutes as fixed-point number (Fxp 16.16 format).
         *  @details Uses SH-2 DVU for 64-bit division: (ticks << 16) / (frequency * 60).
         *  Provides the longest Fxp range of all conversion methods.
         *
         *  @par Range:
         *  - **Maximum: 32767 minutes (~22.7 days)**
         *  - Minimum: ~0.000000075 minutes (1 tick at PHI_128)
         *
         *  @par When to Use:
         *  Use for long-duration timing such as play session length, in-game
         *  clocks, or cooldown timers that span multiple minutes or hours.
         *
         *  @par Example:
         *  @code
         *  Tickstamp elapsed = end - start;
         *  Fxp mins = elapsed.ToMinutes();
         *  if (mins > 5.0) {  // More than 5 minutes
         *      // Auto-save
         *  }
         *  @endcode
         *
         *  @see ToSeconds() for second-precision timing
         */
        Math::Types::Fxp ToMinutes() const noexcept
        {
            return Math::Types::Fxp::BuildRaw(Division64By32(dividerConfig->MinutesDivider, High, Low));
        }

        /** @brief Converts ticks to clock display format (HH:MM:SS.mmm) using 1 DVU division.
         *  @return Immutable ClockTime with hours, minutes, seconds, and milliseconds.
         *  @details Performs a single DVU division via minutes divisor, then extracts
         *  seconds and milliseconds from fractional parts using pure truncation.
         *  Much cheaper than calling ToSeconds() + ToMinutes() separately.
         *
         *  @par How it works:
         *  1. DVU divides ticks by minutes divisor → Fxp 16.16 total minutes
         *  2. Integer part → total minutes → split into hours and minutes
         *  3. Fractional minutes × 60 → seconds + sub-second fraction
         *  4. Sub-second fraction × 1000 → milliseconds (0–999)
         *
         *  @par Range:
         *  - Maximum: ~546 hours (~22.7 days), limited by ToMinutes() Fxp range
         *  - Milliseconds precision: ~0.015ms (~15μs per step)
         *  - Updates visibly every frame at 60fps (~16ms steps)
         *
         *  @par Intended Use:
         *  This method is designed **exclusively for GUI display**. Use ClockTime to show
         *  elapsed time in HH:MM:SS.mmm format. Do NOT use for timing comparisons or
         *  game logic — use ToSeconds(), ToMilliseconds(), or direct Tickstamp
         *  subtraction for precise timing operations.
         *
         *  @par Example:
         *  @code
         *  Tickstamp elapsed = end - start;
         *  ClockTime ct = elapsed.ToClock();
         *  // Display as "02:15:30.500"
         *  sprintf(buf, "%02u:%02u:%02u.%03u",
         *      ct.Hours(), ct.Minutes(), ct.Seconds(), ct.Milliseconds());
         *  @endcode
         *
         *  @note ClockTime is immutable — use ToSeconds()/ToMinutes() for comparisons.
         *  @note Pure truncation means exact second boundaries may show as SS-1:999
         *  instead of SS:000. This is consistent and visually correct for display.
         *  @see ToSeconds(), ToMinutes(), ToMilliseconds() for Fxp precision
         */
        ClockTime ToClock() const noexcept
        {
            // Single DVU division: get total minutes as Fxp 16.16
            // Range: up to 32767 minutes (~546 hours / ~22.7 days)
            uint32_t rawMinutes = Division64By32(dividerConfig->MinutesDivider, High, Low);

            // Integer part = total minutes, fractional part = sub-minute
            uint32_t totalMinutes = rawMinutes >> 16;
            uint32_t minFrac = rawMinutes & 0xFFFF;

            // Pure truncation chain: minutes → seconds → milliseconds
            // Step 1: fractional minutes × 60 = seconds + sub-second fraction
            uint32_t scaledSec = minFrac * 60;
            uint8_t  secs = static_cast<uint8_t>(scaledSec >> 16);
            uint32_t secFrac = scaledSec & 0xFFFF;

            // Step 2: fractional seconds × 1000 = milliseconds (0–999)
            // 16-bit fraction → ~0.015ms precision, updates every frame
            uint16_t ms = static_cast<uint16_t>((secFrac * 1000) >> 16);

            // Split total minutes into hours and minutes
            uint16_t hrs = static_cast<uint16_t>(totalMinutes / 60);
            uint8_t  mins = static_cast<uint8_t>(totalMinutes % 60);

            return ClockTime(hrs, mins, secs, ms);
        }
        //@}
    };

    /** @brief High-precision hardware timer with DVU-accelerated conversions.
     *
     *  @details The Timer class provides Sega Saturn game developers with precise,
     *  hardware-accelerated timing capabilities. It combines the SH-2's
     *  FRT (Free Running Timer) for counting and DVU (Division Unit) for
     *  fast fixed-point conversions.
     *
     *  @par Key Features:
     *  - **48-bit timer range**: Up to ~673 days of continuous timing
     *  - **Hardware acceleration**: DVU provides single-cycle division
     *  - **Fixed-point output**: Native Fxp 16.16 compatibility (no float conversion needed)
     *  - **Frame-rate independent**: Delta time calculations for smooth animation
     *  - **Regional support**: Automatic NTSC/PAL frequency handling
     *
     *  @par Architecture:
     *  The timer uses a two-tier counting system:
     *  1. FRT (16-bit): Hardware counter running at ~222 kHz (PHI_128)
     *  2. Timer32 (32-bit): Software overflow counter incremented via interrupt
     *  3. Combined: 48-bit range with ~4.47μs precision (PHI_128)
     *
     *  @par Precision (PHI_128 mode, SGL default):
     *  | Tick Rate | Precision | Hardware Max |
     *  |-----------|-----------|--------------|
     *  | ~222 kHz  | ~4.47 μs  | ~673 days    |
     *
     *  @par Basic Usage:
     *  @code
     *  // Use delta time for animation (called automatically by Core::Synchronize)
     *  Fxp delta = Timer::DeltaSeconds();
     *  position = position + velocity * delta;
     *  @endcode
     *
     *  @par Manual Timing:
     *  @code
     *  Tickstamp start = Timer::Capture();
     *  // ... operation to measure ...
     *  Tickstamp end = Timer::Capture();
     *  Tickstamp elapsed = end - start;
     *  Fxp seconds = elapsed.ToSeconds();
     *  @endcode
     *
     *  @warning The hardware can track up to ~673 days, but the Fxp 16.16 format limits
     *  conversions: ToMilliseconds() to ~32.8s, ToSeconds() to ~9.1h,
     *  ToMinutes() to ~22.7 days. For GUI display use ToClock() which returns
     *  a ClockTime struct with no Fxp limitation (up to ~546 hours).
     *
     *  @see Tickstamp
     */
    class Timer final
    {
        /// @cond Internal
        // Grant access to Core and TimerTest for testing and core access (hidden from Doxygen)
        friend class Core;
        friend class TimerTest;
        /// @endcond
        /** @name Hardware Configuration
         * FRT hardware register addresses and configuration constants.
         */
         //@{
        /** @brief FRT hardware register base address. */
        static constexpr uintptr_t frtBase = 0xfffffe10;

        /** @brief Timer Interrupt Enable Register offset. */
        static constexpr uint8_t tierOffset = 0x00;

        /** @brief Timer Control/Status Register offset. */
        static constexpr uint8_t statusOffset = 0x01;

        /** @brief Timer Control Register offset. */
        static constexpr uint8_t controlOffset = 0x06;

        /** @brief TIER overflow interrupt enable bit. */
        static constexpr uint8_t tierOverflowIrq = 0x02;

        /** @brief TCR clock selection mask. */
        static constexpr uint8_t tcrClockMask = 0x03;

        /** @brief VCRD: FRT overflow vector number register address. */
        static constexpr uintptr_t vcrdAddr = 0xFFFFFE68;

        /** @brief IPRB: Interrupt priority register B address (SCI + FRT). */
        static constexpr uintptr_t iprbAddr = 0xFFFFFE60;

        /** @brief FRT overflow interrupt vector number. */
        static constexpr uint8_t frtFoviVector = 0x66;

        /** @brief Maximum priority level for FRT interrupt. */
        static constexpr uint8_t frtPriorityLevel = 0x0F;
        //@}

        /** @name Hardware Register References
         * Volatile references to FRT hardware registers.
         */
         //@{
        /** @brief Timer Interrupt Enable Register reference.
         *  @details Volatile reference to the FRT TIER register at frtBase + tierOffset.
         *  Controls which FRT interrupts are enabled (overflow, compare A/B).
         */
        static inline volatile uint8_t& tierReg = *reinterpret_cast<volatile uint8_t*>(frtBase + tierOffset);

        /** @brief Timer Control/Status Register reference.
         *  @details Volatile reference to the FRT TCSR register at frtBase + statusOffset.
         *  Contains status flags and control bits for the FRT.
         */
        static inline volatile uint8_t& statusReg = *reinterpret_cast<volatile uint8_t*>(frtBase + statusOffset);

        /** @brief Timer Control Register reference.
         *  @details Volatile reference to the FRT TCR register at frtBase + controlOffset.
         *  Controls the FRT clock source and counter operation.
         */
        static inline volatile uint8_t& controlReg = *reinterpret_cast<volatile uint8_t*>(frtBase + controlOffset);
        //@}

        /** @name Internal State
         * Private state variables for timer operation.
         */
         //@{
        /** @brief 32-bit overflow counter (incremented by frtHandler on each FRT overflow).
         *  @details Public for diagnostic purposes in tests.
         */
        static inline volatile uint32_t timer32 = 0;

        /** @brief Frame-level timestamp for delta time calculations.
         *  @details Updated by Update(). Used internally for frame delta calculation.
         */
        static inline Tickstamp frameSnapshot = Tickstamp(0, 0);

        /** @brief Frame delta ticks (raw elapsed ticks between frames).
         *  @details Private storage for delta timing values. Modified only by Update().
         */
        static inline Tickstamp deltaTicks = Tickstamp(0, 0);

        /** @brief Frame delta time in seconds (fixed-point 16.16).
         *  @details Private storage for delta timing values. Modified only by Update().
         */
        static inline Math::Types::Fxp deltaSeconds = 0;

        /** @brief Frame delta time in milliseconds (fixed-point 16.16).
         *  @details Private storage for delta timing values. Modified only by Update().
         */
        static inline Math::Types::Fxp deltaMilliseconds = 0;

        /** @brief Frame delta time in minutes (fixed-point 16.16).
         *  @details Private storage for delta timing values. Modified only by Update().
         */
        static inline Math::Types::Fxp deltaMinutes = 0;
        //@}

        /** @brief Initializes timer system for SRL usage.
         *  @details Uses SGL's default PHI_128 FRT configuration (~222 kHz, ~4.47μs precision).
         *  - Disables FRT interrupts during setup
         *  - Clears FRT counter and status
         *  - Sets up VCRD/IPRB for overflow interrupt routing
         *  - Installs overflow handler and enables interrupt
         *  Region (NTSC/PAL) is determined at compile time via SRL_MODE.
         *  System clock mode (26/28 MHz) is auto-detected at runtime.
         *  @note Called automatically by Core::Initialize.
         */
        static void Init()
        {
            Tickstamp::InitDivider();

            // Step 1: Disable all FRT interrupts during configuration
            tierReg = 0x00;

            // Step 2: Clear FRT counter and status (leave TCR at SGL default PHI_128)
            *reinterpret_cast<volatile uint16_t*>(frtBase + 0x02) = 0;  // FRC = 0
            statusReg = 0;  // Clear TCSR flags

            // Step 3: Configure VCRD - map FRT overflow interrupt to vector 0x66
            volatile uint16_t& vcrd = *reinterpret_cast<volatile uint16_t*>(vcrdAddr);
            vcrd = static_cast<uint16_t>(frtFoviVector) << 8;

            // Step 4: Configure IPRB - set FRT interrupt priority (bits 11-8)
            volatile uint16_t& iprb = *reinterpret_cast<volatile uint16_t*>(iprbAddr);
            uint16_t iprbVal = iprb;
            iprbVal &= 0xF0FF;  // Clear FRT priority bits (11-8)
            iprbVal |= static_cast<uint16_t>(frtPriorityLevel) << 8;
            iprb = iprbVal;

            // Step 5: Install FRT overflow handler at vector 0x66 (FRT_FOVI)
            Interrupt::SetHandler(Interrupt::Vector::TrapV, Timer::frtHandler);

            // Step 6: Initialize software state
            timer32 = 0;

            // Step 7: Enable FRT overflow interrupt (FRT_OVIE = 0x02 in TIER)
            tierReg = tierOverflowIrq;

            // Initialize frame snapshot to current time to prevent large delta on first Update()
            frameSnapshot = Capture();
        }

        /** @brief Updates frame-level timing state.
         *  @details Captures current time, calculates delta from previous frame, and
         *  updates all timing globals (deltaTicks, deltaSeconds, deltaMilliseconds, deltaMinutes).
         *  This is the primary method for frame-rate independent timing.
         *
         *  @par What It Updates:
         *  - frameSnapshot: Current timestamp (saved for next frame)
         *  - deltaTicks: Raw tick count elapsed
         *  - deltaSeconds: Elapsed time in seconds (Fxp 16.16)
         *  - deltaMilliseconds: Elapsed time in ms (Fxp 16.16)
         *  - deltaMinutes: Elapsed time in minutes (Fxp 16.16)
         *
         *  @par Delta Time Values:
         *  | FPS  | DeltaSeconds | DeltaMilliseconds | DeltaMinutes |
         *  |------|--------------|-------------------|--------------|
         *  | 60   | ~0.0167s     | ~16.7ms           | ~0.000278m   |
         *  | 30   | ~0.0333s     | ~33.3ms           | ~0.000556m   |
         *  | 15   | ~0.0667s     | ~66.7ms           | ~0.001111m   |
         */
        static void Update()
        {
            Tickstamp now = Capture();
            deltaTicks = now - frameSnapshot;
            deltaSeconds = deltaTicks.ToSeconds();
            deltaMilliseconds = deltaTicks.ToMilliseconds();
            deltaMinutes = deltaTicks.ToMinutes();
            frameSnapshot = now;
        }

        static void __attribute__((interrupt_handler)) frtHandler()
        {
            // Increment overflow counter
            timer32 += 1;

            // Clear overflow interrupt flag
            volatile uint8_t status = statusReg;
            (void)status;
            statusReg &= ~tierOverflowIrq;
        }
        //@}

    public:
        /** @name Timing State
         * Pre-calculated timing values for frame-rate independent operations.
         */
         //@{
        /** @brief Frame delta ticks (raw elapsed ticks between frames).
         *  @details Pre-calculated each frame by Core::Synchronize(). Stores the raw 48-bit tick
         *  count elapsed since the previous frame. Access is zero-cycle (no
         *  function call overhead).
         *
         *  @par Usage:
         *  Use DeltaTicks() when you need maximum precision or when doing custom
         *  time calculations. For most cases, prefer DeltaSeconds() or DeltaMilliseconds().
         *
         *  @code
         *  const Tickstamp& elapsed = Timer::DeltaTicks();
         *  uint32_t rawTicks = elapsed.low;  // Lower 32 bits
         *  uint32_t overflows = elapsed.high;  // Upper 32 bits (overflow counter)
         *  @endcode
         *
         *  @note This is a 48-bit value split across two 32-bit fields. The total
         *  tick count is (high << 16) | (low >> 16) in 48-bit terms.
         *
         *  @see DeltaSeconds(), DeltaMilliseconds()
         */
        static const Tickstamp& DeltaTicks() noexcept { return deltaTicks; }

        /** @brief Current frame timestamp (const reference).
         *  @details Returns a const reference to the current frame's timestamp captured by Update().
         *  This avoids redundant hardware register reads when you need the current time multiple times
         *  within a frame. Use this instead of Capture() when you don't need a fresh timestamp.
         *
         *  @par When to Use:
         *  - Displaying current time in UI/debug output
         *  - Calculating time remaining in a frame
         *  - Any in-frame timing that doesn't require a fresh hardware read
         *
         *  @par When to Use Capture() Instead:
         *  - Measuring precise elapsed time between two operations
         *  - Starting a new timing operation
         *  - When you need the absolute latest hardware timestamp
         *
         *  @par Example:
         *  @code
         *  // Display current time without hardware read overhead
         *  const Tickstamp& now = Timer::CurrentTickstamp();
         *  auto elapsed = now - startTime;
         *  @endcode
         *
         *  @see Capture() for fresh hardware timestamp
         */
        static const Tickstamp& CurrentTickstamp() noexcept { return frameSnapshot; }

        /** @brief Frame delta time in seconds (fixed-point 16.16).
         *  @details Pre-calculated each frame by Core::Synchronize(). Represents the time elapsed
         *  between the current frame and the previous frame, in seconds.
         *
         *  @par Range:
         *  - Typical 60fps: ~0.0167 seconds (16.7ms)
         *  - Typical 30fps: ~0.0333 seconds (33.3ms)
         *  - **Fxp limit: 32767 seconds (~9.1 hours)**
         *
         *  @par Hardware vs Fxp Limit:
         *  The hardware timer can track up to ~673 days, but DeltaSeconds returns Fxp
         *  which has a hard limit of 32767. For game frames this is never an issue.
         *
         *  @par Usage:
         *  Use for frame-rate independent animation, physics updates, and game logic.
         *  Multiply by velocity values to get distance traveled this frame.
         *
         *  @code
         *  // Move object at constant speed regardless of frame rate
         *  Fxp speed = 100.0;  // 100 units per second
         *  position = position + speed * Timer::DeltaSeconds();
         *  @endcode
         *
         *  @see DeltaMilliseconds() for millisecond precision
         */
        static const Math::Types::Fxp& DeltaSeconds() noexcept { return deltaSeconds; }

        /** @brief Frame delta time in milliseconds (fixed-point 16.16).
         *  @details Pre-calculated each frame by Core::Synchronize(). Represents the time elapsed
         *  between frames in milliseconds with higher resolution than seconds.
         *
         *  @par Range:
         *  - Typical 60fps: ~16.7 milliseconds
         *  - Typical 30fps: ~33.3 milliseconds
         *  - **Fxp limit: 32767 milliseconds (~32.8 seconds)**
         *
         *  @par Usage:
         *  Use when you need millisecond precision for short-duration events,
         *  input debouncing, or animation keyframe timing.
         *
         *  @code
         *  // Check if enough time has passed for input repeat
         *  if (Timer::DeltaMilliseconds().As<int>() > 100) {  // 100ms elapsed
         *      // Process repeating input
         *  }
         *  @endcode
         *
         *  @warning The 32767ms limit (~32.8s) is the Fxp format limit. The hardware
         *  timer itself can track up to ~673 days (PHI_128). For measuring longer
         *  durations than 32.8s, use DeltaSeconds() instead.
         *
         *  @see DeltaSeconds() for longer range (up to 9.1 hours in Fxp)
         */
        static const Math::Types::Fxp& DeltaMilliseconds() noexcept { return deltaMilliseconds; }

        /** @brief Frame delta time in minutes (fixed-point 16.16).
         *  @details Pre-calculated each frame by Core::Synchronize(). Represents the time elapsed
         *  since the previous frame with minute precision.
         *
         *  @par Precision and Range:
         *  - **Hardware precision**: ~4.47μs per tick (PHI_128)
         *  - **Fxp precision**: ~0.9ms per unit (16.16 format)
         *  - **Fxp limit: 32767 minutes (~22.7 days)**
         *
         *  @par Usage:
         *  Use for long-term game timers, auto-save intervals, session tracking,
         *  and any timing that spans minutes to days.
         *
         *  @code
         *  // Auto-save every 5 minutes of gameplay
         *  static Fxp gameTime = 0;
         *  gameTime = gameTime + Timer::DeltaMinutes();
         *  if (gameTime > 5.0) {
         *      SaveGame();
         *      gameTime = 0;
         *  }
         *  @endcode
         *
         *  @note DeltaMinutes provides the longest range among delta variables while
         *  maintaining fixed-point precision. Ideal for persistent game state.
         *
         *  @see DeltaSeconds() for frame-level precision, DeltaMilliseconds() for short-term timing
         */
        static const Math::Types::Fxp& DeltaMinutes() noexcept { return deltaMinutes; }
        //@}

        /** @name Core Operations
         * Fundamental timer operations for frame updates and timestamp capture.
         */
         //@{
        /** @brief Captures current hardware state into a Tickstamp.
         *  @return Tickstamp containing the current 48-bit timer value.
         *
         *  @par When to Use:
         *  - Benchmarking and performance profiling
         *  - Measuring precise elapsed time between two operations
         *  - Starting a new timing operation that requires a fresh timestamp
         *  - Deadline checking where absolute latest time is critical
         *
         *  @par When to Use CurrentTickstamp() Instead:
         *  - Displaying current time in UI/debug output (no hardware read overhead)
         *  - Calculating time remaining in a frame
         *  - Any in-frame timing that doesn't require a fresh hardware read
         *
         *  @par Performance Consideration:
         *  Capture() reads hardware registers on each call. For in-frame operations
         *  where you need the current time multiple times, prefer CurrentTickstamp()
         *  which returns a const reference to the timestamp already captured by Update().
         *
         *  @par Usage:
         *  Call at the start and end of an operation to measure elapsed time:
         *  @code
         *  Tickstamp start = Timer::Capture();
         *  // ... perform operation ...
         *  Tickstamp end = Timer::Capture();
         *  Tickstamp elapsed = end - start;
         *  Fxp seconds = elapsed.ToSeconds();
         *  @endcode
         *
         *  @note This function uses a memory barrier to ensure consistent ordering
         *  of FRT and timer32 reads. The overhead is minimal (~1-2 cycles).
         *
         *  @see CurrentTickstamp() for in-frame timing without hardware read overhead
         */
        static Tickstamp Capture() noexcept
        {
            // Read FRC as two bytes: SH-2 FRC requires byte-level access (FRCH then FRCL)
            // A 16-bit read returns only the high byte on some implementations
            uint8_t frch = *reinterpret_cast<volatile uint8_t*>(frtBase + 0x02);
            uint8_t frcl = *reinterpret_cast<volatile uint8_t*>(frtBase + 0x03);
            uint16_t frtValue = (static_cast<uint16_t>(frch) << 8) | frcl;
            __asm__ volatile("" : : : "memory");
            return Tickstamp(timer32, frtValue);  // high=overflow, frt=FRT direct
        }
        //@}
    };
}
//@}

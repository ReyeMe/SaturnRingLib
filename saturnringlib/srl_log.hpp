#pragma once

#include "srl_base.hpp"
#include "srl_string.hpp"   // for snprintf
#include "srl_debug.hpp"    // for SRL_DEBUG_MAX_LOG_LENGTH

namespace SRL
{
    /** @brief Logger namespace that holds the logger functionality
     * @details This class allows for writing log messages into kronos console
     */
    namespace Logger
    {
        /** @brief LogLevels
         */
        enum class LogLevels : uint8_t
        {
            /** @brief TRACE Level, used to trace code execution while debugging
             */
            TRACE = 0,

            /** @brief DEBUG Level, debug traces, may disappear at release
             */
            TESTING = 1,

            /** @brief INFO Level, generic information messages
             */
            INFO = 2,

            /** @brief WARNING Level, warning messages
             */
            WARNING = 3,

            /** @brief FATAL Level, message display before a crash
             */
            FATAL = 4,

            /** @brief NONE Level, used to disable logging
             */
            NONE = 99
        };

        /** @brief Log class
         */
        class Log
        {
        private:
            /** @brief Log starts address
             */
            constexpr static unsigned long logStartAddress = 0x24000000UL;

            /** @brief Log character output address
             */
            constexpr static unsigned long CS1 = logStartAddress + 0x1000;

        public:

            /** @brief disable default constructor
             */
            Log() = delete;

            /** @brief disable copy constructor
             */
            Log(const Log&) = delete;

            /** @brief disable assignment operator
             */
            Log& operator = (const Log&) = delete;

#ifndef SRL_LOG_LEVEL
            /** @brief Minimum log level to be output
             */
            static constexpr SRL::Logger::LogLevels MinLevel = SRL::Logger::LogLevels::NONE;
#else

#define Stringify(U) SRL::Logger::LogLevels::U

            /** @brief Minimum log level to be output
             */
            static constexpr SRL::Logger::LogLevels MinLevel = Stringify(SRL_LOG_LEVEL);
#undef Stringify
#endif

            /** @brief Log levels helper class
             */
            class LogLevelHelper
            {
            public:

                /** @brief Disable default constructor
                 */
                LogLevelHelper() = delete;

                /** @brief Constructor
                 * @param aLevel Log level
                 */
                constexpr explicit LogLevelHelper(SRL::Logger::LogLevels aLevel) : lvl(aLevel) {}

                /** @brief Getter
                 * @returns Log level
                 */
                constexpr operator SRL::Logger::LogLevels() const { return lvl; }

                /** @brief ToString method
                 * @returns NULL terminated string representation of the current log level
                 */
                inline const char* ToString() const
                {
                    switch (this->lvl)
                    {
                    case SRL::Logger::LogLevels::TRACE:
                        return "TRACE";

                    case SRL::Logger::LogLevels::TESTING:
                        return "TESTING";

                    case SRL::Logger::LogLevels::INFO:
                        return "INFO";

                    case SRL::Logger::LogLevels::WARNING:
                        return "WARNING";

                    case SRL::Logger::LogLevels::FATAL:
                        return "FATAL";

                    default:
                        return "";
                    }
                }

            private:
                /** @brief private log level
                 */
                SRL::Logger::LogLevels lvl;
            };

            /** @brief Log message
             * @tparam lvl  Log level
             * @param message Custom message to show
             */
            template <SRL::Logger::LogLevels lvl>
            inline static void LogPrint(const char* message)
            {
                if constexpr (lvl >= MinLevel)
                {
                    static const char* separator = " : ";
                    volatile uint8_t* addr = (volatile uint8_t*)(CS1);
                    const char* s = SRL::Logger::Log::LogLevelHelper(lvl).ToString();
                    uint8_t size = 0;

                    // Write Log level
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        *addr = static_cast<uint8_t>(*s++);

                    // Write separator
                    s = separator;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        *addr = static_cast<uint8_t>(*s++);

                    // Write message
                    s = message;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        *addr = static_cast<uint8_t>(*s++);

                    // Close the string if not already done
                    if ((uint8_t) * (s - 1) != '\n')
                    {
                        *addr = '\n';
                    }
                }
            }

            /** @brief Log message
             * @param message Custom message to show
             * @param args Text arguments
             */
            template <typename ...Args>
            inline static void LogPrint(const char* message, Args...args)
            {
                LogPrint<SRL::Logger::LogLevels::INFO>(message, args ...);
            }

            /** @brief Log message
             * @tparam lvl  Log level
             * @param message Custom message to show
             * @param args Text arguments
             */
            template <SRL::Logger::LogLevels lvl, typename ...Args>
            inline static void LogPrint(const char* message, Args...args)
            {
                if constexpr (lvl >= MinLevel)
                {
                    static char buffer[SRL_DEBUG_MAX_LOG_LENGTH] = {};
                    snprintf(buffer, SRL_DEBUG_MAX_LOG_LENGTH - 1, message, args ...);
                    SRL::Logger::Log::LogPrint<lvl>(buffer);
                }
            }
        };

        /** @brief Log Trace message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogTrace(const char* message, Args...args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TRACE>(message, args ...);
        }

        /** @brief Log Info message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogInfo(const char* message, Args...args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::INFO>(message, args ...);
        }

        /** @brief Log Debug message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogDebug(const char* message, Args...args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TESTING>(message, args ...);
        }

        /** @brief Log Warning message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogWarning(const char* message, Args...args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::WARNING>(message, args ...);
        }

        /** @brief Log Fatal message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogFatal(const char* message, Args...args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::FATAL>(message, args ...);
        }

        /** @brief Log message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename ...Args>
        inline void LogPrint(const char* message, Args...args)
        {
            SRL::Logger::LogInfo(message, args ...);
        }
    };
}

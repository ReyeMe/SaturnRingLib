// saturnringlib/srl_log.hpp

#pragma once

#include "srl_base.hpp"
#include "srl_string.hpp"  // for snprintf
#include "srl_debug.hpp"   // for SRL_DEBUG_MAX_LOG_LENGTH
#include "srl_devcart.hpp" // for USB DevCart communication

namespace SRL
{

    /** @brief Logger namespace that holds the logger functionality
     * @details This class allows for writing log messages into kronos console
     */
    namespace Logger
    {
        /** @brief LogLevels
         * @details Defines the different log levels
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

        /** @brief LogOutputs
         * @details Defines the different log output targets
         */
        enum class LogOutputs : uint8_t
        {
            /** @brief DEV_CART Level, used for logging to the DevCart
             */
            DEV_CART = 0,

            /** @brief EMULATOR Level, used for logging to the Emulator
             */
            EMULATOR = 1,

            /** @brief NONE Level, used to disable logging
             */
            NONE = 99
        };

        /** @brief DummyLogger class
         * @details A dummy logger that does nothing
         */
        class DummyLogger
        {

        public:
            /** @brief disable default constructor
             */
            DummyLogger() = delete;

            /** @brief disable copy constructor
             */
            DummyLogger(const DummyLogger &) = delete;

            /** @brief disable assignment operator
             */
            DummyLogger &operator=(const DummyLogger &) = delete;

            /** @brief putc method for single character
             * @param c Character to be logged
             */
            static void putc(char c)
            {
                c;
            }

            /** @brief putc method for string
             * @param c String to be logged
             */
            static void putc(const char *c)
            {
                c;
            }
        };

        /** @brief EmulatorLogger class
         * @details A logger for the emulator
         */
        class EmulatorLogger
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
            EmulatorLogger() = delete;

            /** @brief disable copy constructor
             */
            EmulatorLogger(const EmulatorLogger &) = delete;

            /** @brief disable assignment operator
             */
            EmulatorLogger &operator=(const EmulatorLogger &) = delete;

            /** @brief putc method for single character
             * @param c Character to be logged
             */
            static void putc(char c)
            {
                putc(&c);
            }

            /** @brief putc method for string
             * @param c String to be logged
             */
            static void putc(const char *c)
            {
                static volatile uint8_t *addr = (volatile uint8_t *)(CS1);
                *addr = static_cast<uint8_t>(*c);
            }
        };

        /** @brief DevCartLogger class
         * @details A logger for the DevCart
         */
        class DevCartLogger
        {
        public:
            /** @brief disable default constructor
             */
            DevCartLogger() = delete;

            /** @brief disable copy constructor
             */
            DevCartLogger(const DevCartLogger &) = delete;

            /** @brief disable assignment operator
             */
            DevCartLogger &operator=(const DevCartLogger &) = delete;

            /** @brief Buffer size configuration
             */
            static constexpr size_t bufferSize = 128; // Example buffer size

            /** @brief Log buffer
             */

            /** @brief putc method for single character
             * @param c Character to be logged
             */
            static void putc(char c)
            {
                static uint8_t buffer[bufferSize];
                static uint8_t bufferIndex = 0;

                buffer[bufferIndex++] = c;

                if (c == '\n' || bufferIndex >= bufferSize)
                {
                    if (bufferIndex < bufferSize)
                        buffer[bufferIndex] = '\0'; // Null-terminate the string
                    else
                        buffer[bufferSize - 1] = '\0'; // Ensure null-termination if buffer is full

                    // Wait for DevCart to be available
                    while (!SRL::DevCart::CS0::isConnected())
                    {
                        SRL::Core::Synchronize(); // Prevent watchdog reset while waiting
                    }

                    if (bufferIndex > 0)
                    {
                        size_t wrote = SRL::DevCart::CS0::write(reinterpret_cast<const uint8_t *>(const_cast<uint8_t *>(buffer)), bufferIndex);
                        const uint8_t ackByte = 0x06;
                        uint8_t response = SRL::DevCart::CS0::read();
                        if (response == ackByte)
                        {
                            // Successfully sent
                        }
                        else
                        {
                            // Handle transmission error (optional)
                        }
                    }

                    bufferIndex = 0;
                }
            }

            /** @brief putc method for single character
             * @param c Character to be logged
             */
            static void putc(const char *c)
            {
                putc(*c);
            }

            /** @brief putc method for single character
             * @param c Character to be logged
             */
            static void flush()
            {
                // Flush remaining data in buffer if any
                putc('\n');
            }
        };

#ifndef SRL_LOG_OUTPUT
        /** @brief Minimum log level to be output
         */
        static constexpr SRL::Logger::LogOutputs LogOutput = SRL::Logger::LogOutputs::NONE;
#else
#define Stringify(U) SRL::Logger::LogOutputs::U

        /** @brief Minimum log level to be output
         */
        static constexpr SRL::Logger::LogOutputs LogOutput = Stringify(SRL_LOG_OUTPUT);
#undef Stringify
#endif

        // Compile-time type selection for Output
        using DefaultLogger = std::conditional_t<
            LogOutput == SRL::Logger::LogOutputs::DEV_CART, SRL::Logger::DevCartLogger,
            std::conditional_t<
                LogOutput == SRL::Logger::LogOutputs::EMULATOR, SRL::Logger::EmulatorLogger,
                SRL::Logger::DummyLogger // Default case
                >>;

        // Static assertion to catch invalid SRL_LOG_OUTPUT values
        static_assert(
            LogOutput == SRL::Logger::LogOutputs::DEV_CART ||
                LogOutput == SRL::Logger::LogOutputs::EMULATOR ||
                LogOutput == SRL::Logger::LogOutputs::NONE,
            "Invalid SRL_LOG_OUTPUT value");

        /** @brief Log class
         * @details The main log class
         */
        class Log
        {
        public:
            /** @brief disable default constructor
             */
            Log() = delete;

            /** @brief disable copy constructor
             */
            Log(const Log &) = delete;

            /** @brief disable assignment operator
             */
            Log &operator=(const Log &) = delete;

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
             * @details A helper class for log levels
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
                inline const char *ToString() const
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

            /** @brief Get log level
             * @returns Log level
             */
            inline static SRL::Logger::LogLevels GetLogLevel()
            {
                return MinLevel;
            }

            /** @brief Log message
             * @tparam lvl Log level
             * @param message Custom message to show
             */
            template <SRL::Logger::LogLevels lvl, typename Output = DefaultLogger>
            inline static void LogPrintInternal(const char *message)
            {
                if constexpr (lvl >= MinLevel)
                {
                    static const char *separator = " : ";
                    const char *s = SRL::Logger::Log::LogLevelHelper(lvl).ToString();
                    uint8_t size = 0;

                    // Write Log level
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Write separator
                    s = separator;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Write message
                    s = message;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Close the string if not already done
                    if ((uint8_t)*(s - 1) != '\n')
                    {
                        Output::putc('\n');
                    }
                }
            }

            /** @brief Log message
             * @tparam lvl Log level
             * @param message Custom message to show
             * @param args Text arguments
             */
            template <SRL::Logger::LogLevels lvl = MinLevel, typename Output = DefaultLogger, typename... Args>
            inline static void LogPrint(const char *message, Args... args)
            {
                if constexpr (lvl >= MinLevel)
                {
                    static char buffer[SRL_DEBUG_MAX_LOG_LENGTH] = {};
                    snprintf(buffer, SRL_DEBUG_MAX_LOG_LENGTH - 1, message, args...);
                    SRL::Logger::Log::LogPrintInternal<lvl, Output>(buffer);
                }
            }
        };

        /** @brief Log Trace message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogTrace(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TRACE, Output>(message, args...);
        }

        /** @brief Log Info message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogInfo(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::INFO, Output>(message, args...);
        }

        /** @brief Log Debug message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogDebug(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TESTING, Output>(message, args...);
        }

        /** @brief Log Warning message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogWarning(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::WARNING, Output>(message, args...);
        }

        /** @brief Log Fatal message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogFatal(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::FATAL, Output>(message, args...);
        }

        /** @brief Log message
         * @param message Custom message to show
         * @param args Text arguments
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogPrint(const char *message, Args... args)
        {
            SRL::Logger::LogInfo<Output>(message, args...);
        }
    };
}
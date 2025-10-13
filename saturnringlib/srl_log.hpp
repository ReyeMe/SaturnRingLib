// saturnringlib/srl_log.hpp
// 
// A flexible logging system for embedded environments (e.g., Sega Saturn homebrew via Kronos emulator or USB DevCart).
// Supports compile-time configuration of output target and minimum log level.
// Uses template metaprogramming for zero-overhead when logs are disabled.

#pragma once

#include "srl_base.hpp"      // Base definitions (e.g., uint8_t if not using std)
#include "srl_string.hpp"    // For snprintf (custom implementation)
#include "srl_debug.hpp"     // For SRL_DEBUG_MAX_LOG_LENGTH (buffer size constant)
#include "srl_devcart.hpp"   // For USB DevCart communication (CS0::write)

#include <cstdint>          // For uint8_t (ensure consistency with srl_base)
#include <type_traits>      // For std::conditional_t

namespace SRL
{

    /** @brief Logger namespace that holds the logger functionality.
     * 
     * @details This namespace provides a compile-time configurable logging system.
     *          Logs can be directed to different outputs (DevCart USB, Emulator MMIO, or none).
     *          Log levels filter messages at compile-time for efficiency.
     *          Usage: Configure via macros SRL_LOG_OUTPUT and SRL_LOG_LEVEL before include.
     *          Example: #define SRL_LOG_OUTPUT DEV_CART
     *                   #define SRL_LOG_LEVEL INFO
     */
    namespace Logger
    {
        /** @brief LogLevels enumeration.
     * 
     * @details Defines severity levels for log messages. Lower values = more verbose.
     *          Filtering is done at compile-time via MinLevel.
     */
        enum class LogLevels : uint8_t
        {
            /** @brief TRACE: Detailed code flow tracing (debug builds only). */
            TRACE = 0,

            /** @brief TESTING: Debug traces (may be compiled out in release). */
            TESTING = 1,

            /** @brief INFO: General informational messages. */
            INFO = 2,

            /** @brief WARNING: Potential issues or non-critical errors. */
            WARNING = 3,

            /** @brief FATAL: Critical errors before crash/halt. */
            FATAL = 4,

            /** @brief NONE: Disable all logging. */
            NONE = 99
        };

        /** @brief LogOutputs enumeration.
         * 
         * @details Defines possible output sinks for log messages.
         */
        enum class LogOutputs : uint8_t
        {
            /** @brief DEV_CART: Output via USB DevCart FIFO (SRL::DevCart::CS0::write). */
            DEV_CART = 0,

            /** @brief EMULATOR: Output via memory-mapped I/O for emulator console. */
            EMULATOR = 1,

            /** @brief NONE: Disable output (dummy sink). */
            NONE = 99
        };

        /** @brief DummyLogger class.
         * 
         * @details A no-op logger used when output is disabled or as fallback.
         *          All operations are optimized out by compiler.
         */
        class DummyLogger
        {
        public:
            /** @brief Deleted default constructor (static-only class). */
            DummyLogger() = delete;

            /** @brief Deleted copy constructor. */
            DummyLogger(const DummyLogger &) = delete;

            /** @brief Deleted assignment operator. */
            DummyLogger &operator=(const DummyLogger &) = delete;

            /** @brief No-op putc for single character.
             *  @param c Ignored character.
             */
            static void putc(char c)
            {
                (void)c;  // Unused parameter
            }

            /** @brief No-op putc for string pointer.
             *  @param c Ignored string.
             */
            static void putc(const char *c)
            {
                (void)c;  // Unused
            }
        };

        /** @brief EmulatorLogger class.
         * 
         * @details Logs to emulator via MMIO write to a fixed address (Kronos console emulation).
         *          Single-byte writes; inefficient but functional for debug.
         */
        class EmulatorLogger
        {
        private:
            /** @brief Base address for log region (arbitrary/emulator-specific). */
            constexpr static unsigned long logStartAddress = 0x24000000UL;

            /** @brief Specific address for character output in CS1 space. */
            constexpr static unsigned long CS1 = logStartAddress + 0x1000;

        public:
            /** @brief Deleted default constructor (static-only). */
            EmulatorLogger() = delete;

            /** @brief Deleted copy constructor. */
            EmulatorLogger(const EmulatorLogger &) = delete;

            /** @brief Deleted assignment operator. */
            EmulatorLogger &operator=(const EmulatorLogger &) = delete;

            /** @brief Write single character to emulator console.
             *  @param c Character to write.
             */
            static void putc(char c)
            {
                putc(&c);  // Delegate to pointer version
            }

            /** @brief Write single character from pointer to emulator console.
             *  @param c Pointer to character.
             *  Note: Writes only the first byte; volatile for MMIO.
             */
            static void putc(const char *c)
            {
                static volatile uint8_t *addr = (volatile uint8_t *)(CS1);
                *addr = static_cast<uint8_t>(*c);
            }
        };

        /** @brief DevCartLogger class.
         * 
         * @details Logs to USB DevCart via SRL::DevCart::CS0::write (byte-by-byte USB FIFO).
         *          No internal buffering; relies on DevCart FIFO.
         */
        class DevCartLogger
        {
        public:
            /** @brief Deleted default constructor (static-only). */
            DevCartLogger() = delete;

            /** @brief Deleted copy constructor. */
            DevCartLogger(const DevCartLogger &) = delete;

            /** @brief Deleted assignment operator. */
            DevCartLogger &operator=(const DevCartLogger &) = delete;

            /** @brief Buffer size for internal formatting (not used here; see LogPrint).
             *  Note: Currently unused in this class—consider for future buffering.
             */
            static constexpr size_t bufferSize = 32; // Example buffer size (placeholder)

            /** @brief Write single character to DevCart USB.
             *  @param c Character to send.
             */
            static void putc(const char c)
            {
                putc(&c);   
            }

            /** @brief Write single byte to DevCart USB FIFO.
             *  @param c Pointer to byte (writes only first).
             *  Note: Casts to uint8_t* for DevCart::write; may block if FIFO full.
             */
            static void putc(const char * c)
            {
                SRL::DevCart::CS0::write(reinterpret_cast<const uint8_t *>(c));
            }

            /** @brief Flush any pending data (no-op here; USB FIFO auto-flushes?).
             *  Note: Sends null terminator—may not be needed; document if for EOF.
             */
            static void flush()
            {
                // Flush remaining data in buffer if any (currently no buffer)
                putc('\0');
            }
        };

        // Compile-time configuration for output target
#ifndef SRL_LOG_OUTPUT
        /** @brief Default output if SRL_LOG_OUTPUT undefined: NONE (disabled). */
        static constexpr SRL::Logger::LogOutputs LogOutput = SRL::Logger::LogOutputs::NONE;
#else
        // Macro to stringify and select enum value
        #define Stringify(U) SRL::Logger::LogOutputs::U

        /** @brief Configured output target (e.g., DEV_CART). */
        static constexpr SRL::Logger::LogOutputs LogOutput = Stringify(SRL_LOG_OUTPUT);
        #undef Stringify
#endif

        // Select logger type at compile-time based on LogOutput
        using DefaultLogger = std::conditional_t<
            LogOutput == SRL::Logger::LogOutputs::DEV_CART, SRL::Logger::DevCartLogger,
            std::conditional_t<
                LogOutput == SRL::Logger::LogOutputs::EMULATOR, SRL::Logger::EmulatorLogger,
                SRL::Logger::DummyLogger // Fallback
                >>;

        // Ensure valid configuration
        static_assert(
            LogOutput == SRL::Logger::LogOutputs::DEV_CART ||
                LogOutput == SRL::Logger::LogOutputs::EMULATOR ||
                LogOutput == SRL::Logger::LogOutputs::NONE,
            "Invalid SRL_LOG_OUTPUT value: Must be DEV_CART, EMULATOR, or NONE");

        /** @brief Log class.
         * 
         * @details Core logging facade. Uses templates for level-based filtering and output selection.
         *          All operations are inline and constexpr where possible for zero runtime cost when filtered.
         */
        class Log
        {
        public:
            /** @brief Deleted default constructor (static-only). */
            Log() = delete;

            /** @brief Deleted copy constructor. */
            Log(const Log &) = delete;

            /** @brief Deleted assignment operator. */
            Log &operator=(const Log &) = delete;

            // Compile-time configuration for minimum log level
#ifndef SRL_LOG_LEVEL
            /** @brief Default min level if SRL_LOG_LEVEL undefined: NONE. */
            static constexpr SRL::Logger::LogLevels MinLevel = SRL::Logger::LogLevels::NONE;
#else
            #define Stringify(U) SRL::Logger::LogLevels::U
            /** @brief Configured minimum level (e.g., INFO). */
            static constexpr SRL::Logger::LogLevels MinLevel = Stringify(SRL_LOG_LEVEL);
            #undef Stringify
#endif

            // Static assert for valid level (optional: add if needed)

            /** @brief LogLevelHelper: Utility for level-to-string conversion.
             */
            class LogLevelHelper
            {
            public:
                /** @brief Deleted default constructor. */
                LogLevelHelper() = delete;

                /** @brief Constructor with level.
                 *  @param aLevel The log level to wrap.
                 */
                constexpr explicit LogLevelHelper(SRL::Logger::LogLevels aLevel) : lvl(aLevel) {}

                /** @brief Cast to enum value. */
                constexpr operator SRL::Logger::LogLevels() const { return lvl; }

                /** @brief Get string representation of level.
                 *  @return C-string (e.g., "INFO").
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
                        return "";  // Unknown/empty
                    }
                }

            private:
                SRL::Logger::LogLevels lvl;  // Stored level
            };

            /** @brief Get current minimum log level.
             *  @return MinLevel value.
             */
            inline static SRL::Logger::LogLevels GetLogLevel()
            {
                return MinLevel;
            }

            /** @brief Internal raw message printer (no formatting).
             *  @tparam lvl The level to log at.
             *  @tparam Output Logger type (default: DefaultLogger).
             *  @param message Null-terminated string to log.
             *  Note: Adds "LEVEL : message\n" prefix; truncates at SRL_DEBUG_MAX_LOG_LENGTH.
             */
            template <SRL::Logger::LogLevels lvl, typename Output = DefaultLogger>
            inline static void LogPrintInternal(const char *message)
            {
                if constexpr (lvl >= MinLevel)
                {
                    static const char *separator = " : ";
                    const char *s = SRL::Logger::Log::LogLevelHelper(lvl).ToString();
                    uint8_t size = 0;

                    // Output level string
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Output separator
                    s = separator;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Output message
                    s = message;
                    while (*s && ++size < SRL_DEBUG_MAX_LOG_LENGTH)
                        Output::putc(s++);

                    // Append newline if missing
                    if ((uint8_t)*(s - 1) != '\n')
                    {
                        Output::putc('\n');
                    }
                }
            }

            /** @brief Formatted log printer.
             *  @tparam lvl Log level (default: MinLevel).
             *  @tparam Output Logger type.
             *  @tparam Args Variadic args for snprintf.
             *  @param message Format string.
             *  @param args Format arguments.
             *  Note: Uses static buffer; thread-unsafe but fine for embedded.
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

        /** @brief Convenience wrapper for TRACE level.
         *  @tparam Output Optional output override.
         *  @tparam Args Format args.
         *  @param message Format string.
         *  @param args Args.
         */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogTrace(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TRACE, Output>(message, args...);
        }

        /** @brief Convenience wrapper for INFO level. */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogInfo(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::INFO, Output>(message, args...);
        }

        /** @brief Convenience wrapper for TESTING (debug) level. */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogDebug(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TESTING, Output>(message, args...);
        }

        /** @brief Convenience wrapper for WARNING level. */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogWarning(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::WARNING, Output>(message, args...);
        }

        /** @brief Convenience wrapper for FATAL level. */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogFatal(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::FATAL, Output>(message, args...);
        }

        /** @brief General print (defaults to INFO). */
        template <typename Output = DefaultLogger, typename... Args>
        inline void LogPrint(const char *message, Args... args)
        {
            SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::INFO, Output>(message, args...);
        }
    }  // namespace Logger
}  // namespace SRL
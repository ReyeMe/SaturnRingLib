#pragma once

namespace SRL
{
  namespace Logger
  {
    enum class LogLevels : uint8_t {
      TRACE = 0,
      DEBUG = 1,
      INFO = 2,
      WARNING = 3,
      FATAL = 4,
      NONE = 99,
    };

    class Log
    {
    private:
      /** @brief TBD
      */
      constexpr static  unsigned long logStartAddress = 0x24000000UL;

      /** @brief TBD
      */
      constexpr static  unsigned long CS1 = logStartAddress + 0x1000;

    public:

      Log() = delete;

      #ifndef SRL_LOG_LEVEL
        static constexpr SRL::Logger::LogLevels minLevel = SRL::Logger::LogLevels::INFO;
      #else
        #define Stringify(U) SRL::Logger::LogLevels::U
        static constexpr SRL::Logger::LogLevels minLevel = Stringify(SRL_LOG_LEVEL);
        #undef Stringify
       #endif

      /** @brief Log levels helper class
      */
      class LogLevelHelper {
      public:
        LogLevelHelper() = default;
        constexpr LogLevelHelper(SRL::Logger::LogLevels aLevel) : lvl(aLevel) { }
        constexpr operator SRL::Logger::LogLevels() const { return lvl; }

        /** @brief rToString method
        */
        inline const char* ToString() const
        {
          switch (lvl)
          {
            case SRL::Logger::LogLevels::TRACE:     return "TRACE";
            case SRL::Logger::LogLevels::DEBUG:     return "DEBUG";
            case SRL::Logger::LogLevels::INFO:      return "INFO";
            case SRL::Logger::LogLevels::WARNING:   return "WARNING";
            case SRL::Logger::LogLevels::FATAL:     return "FATAL";
            default:                                return "";
          }
        }

      private:
        SRL::Logger::LogLevels lvl;
      };

      /** @brief Log message
      * @param lvl  Log level
      * @param message Custom message to show
      */
      template <SRL::Logger::LogLevels lvl>
      inline static void LogPrint(const char *message) {
        if constexpr (lvl >= minLevel) {
          static const char * separator = " : ";
          volatile uint8_t *addr = (volatile uint8_t *)(CS1);
          const char *s = SRL::Logger::Log::LogLevelHelper(lvl).ToString();
          uint8_t size = 0 ;
          // Write Log level
          while (*s && ++size<SRL_DEBUG_MAX_LOG_LENGTH)
          *addr = static_cast<uint8_t>(*s++);

          // Write separator
          s = separator;
          while (*s && ++size<SRL_DEBUG_MAX_LOG_LENGTH)
          *addr = static_cast<uint8_t>(*s++);

          // Write message
          s = message;
          while (*s && ++size<SRL_DEBUG_MAX_LOG_LENGTH)
          *addr = static_cast<uint8_t>(*s++);

          // Close the string if not already done
          if((uint8_t)*(s-1) != '\n') {
            *addr = '\n';
          }
        }
      }

      /** @brief Log message
      * @param message Custom message to show
      * @param args... Text arguments
      */
      template <typename ...Args>
      inline static void LogPrint(const char *message, Args...args) {
        LogPrint<SRL::Logger::LogLevels::INFO>(message, args ...);
      }

      /** @brief Log message
      * @param message Custom message to show
      * @param args... Text arguments
      */
      template <SRL::Logger::LogLevels lvl, typename ...Args>
      inline static void LogPrint(const char *message, Args...args) {
        if constexpr (lvl >= minLevel) {
          static char buffer[SRL_DEBUG_MAX_LOG_LENGTH] = {};
          snprintf(buffer, SRL_DEBUG_MAX_LOG_LENGTH-1, message, args ...);
          LogPrint<lvl>(buffer);
        }
      }

    };

    /** @brief Log Trace message
    * @param message Custom message to show
    * @param args... Text arguments
    */
    template <typename ...Args>
    inline void LogTrace(const char *message, Args...args) {
        SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::TRACE>(message, args ...);
    }

    /** @brief Log Info message
    * @param message Custom message to show
    * @param args... Text arguments
    */
    template <typename ...Args>
    inline void LogInfo(const char *message, Args...args) {
        SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::INFO>(message, args ...);
    }

    /** @brief Log Debug message
    * @param message Custom message to show
    * @param args... Text arguments
    */
    template <typename ...Args>
    inline void LogDebug(const char *message, Args...args) {
        SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::DEBUG>(message, args ...);
    }

    /** @brief Log Warning message
    * @param message Custom message to show
    * @param args... Text arguments
    */
    template <typename ...Args>
    inline void LogWarning(const char *message, Args...args) {
        SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::WARNING>(message, args ...);
    }

    template <typename ...Args>
    inline void LogFatal(const char *message, Args...args) {
        SRL::Logger::Log::LogPrint<SRL::Logger::LogLevels::FATAL>(message, args ...);
    }

    /** @brief Log message
    * @param message Custom message to show
    * @param args... Text arguments
    */
    template <typename ...Args>
    inline void LogPrint(const char *message, Args...args) {
        LogInfo(message, args ...);
    }

  };
}

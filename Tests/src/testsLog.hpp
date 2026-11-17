// Tests/src/testsLog.hpp
// SRL::Logger: what LogTrace, LogWarning and LogFatal write, captured with a test output (the Output template
// parameter) instead of the emulator or DevCart. The campaigns build with SRL_LOG_LEVEL = TESTING.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL::Logger;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Log output that keeps what it is given in a buffer. */
class LogCapture
{
public:
    inline static char text[300];
    inline static size_t length = 0;

    static void Clear()
    {
        length = 0;
        text[0] = '\0';
    }

    static void putc(char c)
    {
        if (length < sizeof(text) - 1)
        {
            text[length++] = c;
            text[length] = '\0';
        }
    }

    static void putc(const char *c) { putc(*c); }
};

void log_test_setup(void) { LogCapture::Clear(); }

/** @brief Each level writes "LEVEL : message" and a new line; TRACE is below TESTING, so it writes nothing. */
MU_TEST(log_levels_prefix_and_filter)
{
    LogWarning<LogCapture>("disk %d", 3);
    snprintf(buffer, buffer_size, "LogWarning wrote '%s'", LogCapture::text);
    mu_assert(strcmp(LogCapture::text, "WARNING : disk 3\n") == 0, buffer);

    LogCapture::Clear();
    LogFatal<LogCapture>("halt");
    snprintf(buffer, buffer_size, "LogFatal wrote '%s'", LogCapture::text);
    mu_assert(strcmp(LogCapture::text, "FATAL : halt\n") == 0, buffer);

    LogCapture::Clear();
    LogTrace<LogCapture>("hidden");
    snprintf(buffer, buffer_size, "LogTrace wrote '%s' at level TESTING", LogCapture::text);
    mu_assert(LogCapture::length == 0, buffer);
}

/** @brief A message ending with a new line gets no second one; a long message is cut, but still ends the line. */
MU_TEST(log_newline_and_long_message)
{
    LogWarning<LogCapture>("line\n");
    snprintf(buffer, buffer_size, "LogWarning(\"line\\n\") wrote '%s'", LogCapture::text);
    mu_assert(strcmp(LogCapture::text, "WARNING : line\n") == 0, buffer);

    static char longMessage[280];
    for (size_t i = 0; i < sizeof(longMessage) - 1; i++) longMessage[i] = 'a';
    longMessage[sizeof(longMessage) - 1] = '\0';

    LogCapture::Clear();
    LogWarning<LogCapture>("%s", longMessage);
    snprintf(buffer, buffer_size, "A 279-character message gave %d characters, last one 0x%02x (max %d, then a new line)",
        static_cast<int>(LogCapture::length), LogCapture::length ? LogCapture::text[LogCapture::length - 1] : 0, SRL_DEBUG_MAX_LOG_LENGTH);
    mu_assert(LogCapture::length <= SRL_DEBUG_MAX_LOG_LENGTH && LogCapture::text[LogCapture::length - 1] == '\n', buffer);
}

MU_TEST_SUITE(log_test_suite)
{
    MU_SUITE_CONFIGURE(&log_test_setup, NULL);

    MU_RUN_TEST(log_levels_prefix_and_filter);
    MU_RUN_TEST(log_newline_and_long_message);
}

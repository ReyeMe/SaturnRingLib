#pragma once

#include <vector>
#include <stddef.h>
#include <stdint.h>
#include <srl.hpp>

/** @brief Size of the temporary formatting buffer. */
inline constexpr size_t BufferSize = 255;

/** @brief Number of character columns on the Saturn screen display. */
inline constexpr uint8_t DisplayColumns = 44;

/** @brief Number of lines on the Saturn screen display. */
inline constexpr uint8_t DisplayLines = 27;

/** @brief Struct representing a single line of test results. */
struct ResultLine
{
    char text[BufferSize];
};

/** @brief Global buffer used for building result strings. */
inline char resultsBuffer[BufferSize] = {};

/** @brief First result line shown on screen (scroll position). */
inline size_t startIndex = 0;

/** @brief Global list of test result lines. Initialized with a reserved capacity of 20 to minimize heap allocation overhead. */
inline std::vector<ResultLine> gResults = []() {
    std::vector<ResultLine> v;
    v.reserve(20);
    return v;
}();

/** @brief Appends a single character to a destination buffer.
 *  @param dst Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param pos Current write position (will be incremented).
 *  @param ch Character to append.
 */
inline void AppendChar(char *dst, const size_t size, size_t &pos, const char ch)
{
    if (!dst)
    {
        return;
    }
    if (pos + 1 < size)
    {
        dst[pos] = ch;
    }
    ++pos;
}

/** @brief Appends a null-terminated string to a destination buffer.
 *  @param dst Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param pos Current write position (will be incremented).
 *  @param src Source string to append.
 */
inline void AppendStr(char *dst, const size_t size, size_t &pos, const char *src)
{
    if (!dst || !src)
    {
        return;
    }

    for (size_t i = 0; src[i] != '\0'; ++i)
    {
        AppendChar(dst, size, pos, src[i]);
    }
}

/** @brief Formats and appends an unsigned integer to a destination buffer.
 *  @param dst Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param pos Current write position (will be incremented).
 *  @param value Unsigned integer value to append.
 */
inline void AppendUnsigned(char *dst, const size_t size, size_t &pos, unsigned int value)
{
    char tmp[10];
    size_t tmpLen = 0;

    if (value == 0)
    {
        tmp[tmpLen++] = '0';
    }
    else
    {
        while (value > 0 && tmpLen < sizeof(tmp))
        {
            tmp[tmpLen++] = static_cast<char>('0' + (value % 10));
            value /= 10;
        }
    }

    for (size_t i = 0; i < tmpLen; ++i)
    {
        AppendChar(dst, size, pos, tmp[tmpLen - 1 - i]);
    }
}

/** @brief Null-terminates the destination buffer at the specified position.
 *  @param dst Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param pos Write position to finalize.
 */
inline void FinalizeBuffer(char *dst, const size_t size, const size_t pos)
{
    if (!dst || size == 0)
    {
        return;
    }

    const size_t writePos = (pos < size) ? pos : (size - 1);
    dst[writePos] = '\0';
}

/** @brief Formats a test suite result line into the output buffer.
 *  @param out Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param suiteName Name of the test suite.
 *  @param failures Number of failures in the suite.
 */
inline void BuildSuiteLine(char *out, const size_t size, const char *suiteName, const int failures)
{
    if (!out || size == 0)
    {
        return;
    }

    size_t pos = 0;
    const char *name = suiteName ? suiteName : "";

    for (size_t i = 0; i < 20 && name[i] != '\0'; ++i)
    {
        AppendChar(out, size, pos, name[i]);
    }

    if (failures)
    {
        AppendStr(out, size, pos, " : ");
        AppendUnsigned(out, size, pos, static_cast<unsigned int>(failures));
        AppendStr(out, size, pos, " failures");
    }
    else
    {
        AppendStr(out, size, pos, " SUCCESS !");
    }

    FinalizeBuffer(out, size, pos);
}

/** @brief Formats the overall test execution statistics line.
 *  @param out Destination character buffer.
 *  @param size Maximum size of the buffer.
 *  @param tests Total number of tests executed.
 *  @param assertions Total number of assertions verified.
 *  @param failures Total number of test failures.
 */
inline void BuildStatsLine(char *out, const size_t size, const unsigned int tests,
    const unsigned int assertions, const unsigned int failures)
{
    if (!out || size == 0)
    {
        return;
    }

    size_t pos = 0;
    AppendUnsigned(out, size, pos, tests);
    AppendStr(out, size, pos, " tests, ");
    AppendUnsigned(out, size, pos, assertions);
    AppendStr(out, size, pos, " assertions, ");
    AppendUnsigned(out, size, pos, failures);
    AppendStr(out, size, pos, " failures");
    FinalizeBuffer(out, size, pos);
}

/** @brief Appends a line of text to the global test results list.
 *  @param text Null-terminated string to push.
 */
inline void PushResultLine(const char *text)
{
    ResultLine line = {};
    size_t pos = 0;
    AppendStr(line.text, BufferSize, pos, text ? text : "");
    FinalizeBuffer(line.text, BufferSize, pos);
    gResults.push_back(line);
}

/** @brief Renders the test results onto the screen display.
 *  @param startIndex Scroll index/line to start rendering from.
 */
inline void RenderResults(const size_t startIndex)
{
    char lineBuffer[DisplayColumns + 1];
    for (uint8_t i = 0; i < DisplayLines; ++i)
    {
        const size_t lineIndex = startIndex + i;
        const char *src = (lineIndex < gResults.size()) ? gResults[lineIndex].text : "";
        for (uint8_t col = 0; col < DisplayColumns; ++col)
        {
            lineBuffer[col] = ' ';
        }

        uint8_t col = 0;
        while (src[col] != '\0' && col < DisplayColumns)
        {
            lineBuffer[col] = src[col];
            ++col;
        }

        lineBuffer[DisplayColumns] = '\0';
        SRL::ASCII::Print(lineBuffer, 0, i);
    }
}

/** @brief Synchronizes the system and scrolls the screen to the latest result line.
 *  @param startIndex Reference to the scroll index/line.
 */
inline void UpdateDisplay(size_t &startIndex)
{
    if (gResults.size() > DisplayLines)
    {
        startIndex = gResults.size() - DisplayLines;
    }

    RenderResults(startIndex);
    SRL::Core::Synchronize();
}

extern "C" {
extern const uint8_t buffer_size = 255;
inline char buffer[buffer_size] = {};
}

// Define a macro to capture test suite results
#define MU_DISPLAY_SATURN(suite_name)         \
    BuildSuiteLine(resultsBuffer, BufferSize, \
        #suite_name, suite_error_counter);    \
    PushResultLine(resultsBuffer);

#define RUN_AND_DISPLAY_SUITE(suite) \
    MU_RUN_SUITE(suite);             \
    MU_DISPLAY_SATURN(suite);        \
    UpdateDisplay(startIndex);

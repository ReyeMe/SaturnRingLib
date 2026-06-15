#include <srl.hpp>
#include <srl_devcart.hpp>
#include <srl_log.hpp>
#include <cstdint>
#include <algorithm>

using namespace SRL::Types;
using namespace SRL::Logger;

namespace
{
    /** @brief Number of frames between periodic HELLO transmissions. */
    constexpr int kHelloPeriodFrames = 60;

    /** @brief Maximum number of bytes stored for TX/RX preview strings. */
    constexpr size_t kMaxLineLength = 128;

    /**
     * @brief Checks whether at least one byte is available in USB RX FIFO.
     * @return true when data is available to read.
     */
    static bool HasRxData()
    {
        return !SRL::DevCart::CS0::isRXFEmpty();
    }

    /**
     * @brief Sends a null-terminated text buffer through the DevCart FIFO.
     * @param text String to send (without implicit line ending).
     */
    static void SendText(const char *text)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(text);
        const size_t len = __builtin_strlen(text);
        SRL::DevCart::CS0::write(bytes, len);
    }

    /**
     * @brief Reads one byte from the DevCart FIFO.
     * @return Received byte value.
     */
    static uint8_t ReadByte()
    {
        return SRL::DevCart::CS0::read();
    }

    /**
     * @brief Copies a C string into a bounded destination buffer.
     * @param dst Destination buffer.
     * @param dstSize Destination capacity including null terminator.
     * @param src Source null-terminated string.
     */
    static void CopyText(char *dst, const size_t dstSize, const char *src)
    {
        if (dstSize == 0)
        {
            return;
        }

        const size_t srcLen = __builtin_strlen(src);
        const size_t copyLen = std::min(dstSize - 1, srcLen);
        std::copy_n(src, copyLen, dst);
        dst[copyLen] = '\0';
    }

    /**
     * @brief Copies at most srcLen bytes from source into a bounded destination buffer.
     * @param dst Destination buffer.
     * @param dstSize Destination capacity including null terminator.
     * @param src Source character buffer.
     * @param srcLen Maximum number of bytes to copy from src.
     */
    static void CopyTextN(char *dst, const size_t dstSize, const char *src, const size_t srcLen)
    {
        if (dstSize == 0)
        {
            return;
        }

        const size_t copyLen = std::min(dstSize - 1, srcLen);
        std::copy_n(src, copyLen, dst);
        dst[copyLen] = '\0';
    }

    /**
     * @brief Appends a C string to a bounded destination buffer.
     * @param dst Destination buffer to append into.
     * @param dstSize Destination capacity including null terminator.
     * @param src Source null-terminated string.
     */
    static void AppendText(char *dst, const size_t dstSize, const char *src)
    {
        if (dstSize == 0)
        {
            return;
        }

        const char *dstEnd = std::find(dst, dst + dstSize, '\0');
        const size_t dstLen = static_cast<size_t>(dstEnd - dst);

        if (dstLen >= dstSize)
        {
            dst[dstSize - 1] = '\0';
            return;
        }

        const size_t srcLen = __builtin_strlen(src);
        const size_t appendLen = std::min(dstSize - dstLen - 1, srcLen);
        std::copy_n(src, appendLen, dst + dstLen);
        dst[dstLen + appendLen] = '\0';
    }
}

/**
 * @brief DevCart console debug sample entry point.
 *
 * Sends periodic HELLO messages until the first received line is acknowledged.
 * Each received line terminated by CR/LF is echoed back as `OK : <line>`.
 * The on-screen debug panel shows connection status, last TX/RX, and counters.
 */
int main()
{
    SRL::Core::Initialize(HighColor::Colors::Black);

    LogInfo("Debug Console sample initialized");

    SRL::Debug::Print(1, 1, "Debug Console Sample");
    SRL::Debug::Print(1, 3, "DevCart TX: HELLO every 1 second");
    SRL::Debug::Print(1, 4, "DevCart RX: wait newline, then reply OK");
    SRL::Debug::Print(1, 5, "SRL Log: emulator console (TRACE)");

    char lineBuffer[kMaxLineLength] = {};
    size_t lineLen = 0;
    bool previousConnected = false;
    int helloFrames = 0;
    int helloCount = 0;
    int okCount = 0;
    bool helloEnabled = true;
    bool rxOverflowWarningActive = false;
    char lastTx[kMaxLineLength] = "<none>";
    char lastRx[kMaxLineLength] = "<none>";
    int frameCount = 0;

    while (true)
    {
        const bool connected = SRL::DevCart::CS0::isConnected();

        if (connected != previousConnected)
        {
            // LogInfo("DevCart connection changed: %s", connected ? "connected" : "disconnected");
            previousConnected = connected;
        }

        if (!connected)
        {
            rxOverflowWarningActive = false;
            helloEnabled = true;
            helloFrames = 0;
        }
        else
        {
            if (helloEnabled && helloFrames <= 0)
            {
                SendText("HELLO\n");
                CopyText(lastTx, kMaxLineLength, "HELLO\\n");
                ++helloCount;
                helloFrames = kHelloPeriodFrames;
                rxOverflowWarningActive = false;
            }
            else if (helloEnabled)
            {
                --helloFrames;
            }

            constexpr int kMaxRxBytesPerFrame = 64;
            int rxBytesThisFrame = 0;
            if (!HasRxData())
            {
            }
            else
            {
                while (rxBytesThisFrame < kMaxRxBytesPerFrame && HasRxData())
                {
                    const uint8_t rx = ReadByte();
                    ++rxBytesThisFrame;

                    const bool isLineEnd = (rx == '\r' || rx == '\n');

                    if (!isLineEnd && (rx < 0x20 || rx > 0x7E))
                    {
                        continue;
                    }

                    if (isLineEnd)
                    {
                        lineBuffer[lineLen] = '\0';
                        rxOverflowWarningActive = false;
                        CopyText(lastRx, kMaxLineLength, lineLen == 0 ? "<empty>" : lineBuffer);
                        LogInfo("Received line: %s", lineLen == 0 ? "<empty>" : lineBuffer);

                        SendText("OK : ");
                        if (lineLen > 0)
                        {
                            SendText(lineBuffer);
                        }
                        SendText("\n");

                        char okDisplay[kMaxLineLength] = {};
                        CopyText(okDisplay, kMaxLineLength, "OK : ");
                        if (lineLen > 0)
                        {
                            AppendText(okDisplay, kMaxLineLength, lineBuffer);
                        }
                        AppendText(okDisplay, kMaxLineLength, "\\n");
                        CopyText(lastTx, kMaxLineLength, okDisplay);

                        ++okCount;
                        helloEnabled = false;
                        lineLen = 0;
                        break;
                    }

                    if (lineLen + 1 < kMaxLineLength)
                    {
                        lineBuffer[lineLen++] = static_cast<char>(rx);
                        CopyTextN(lastRx, kMaxLineLength, lineBuffer, lineLen);
                    }
                    else
                    {
                        if (!rxOverflowWarningActive)
                        {
                            LogWarning("RX line exceeded %d bytes, clearing buffer", kMaxLineLength - 1);
                            rxOverflowWarningActive = true;
                        }
                        lineBuffer[kMaxLineLength - 1] = '\0';
                        CopyText(lastRx, kMaxLineLength, lineBuffer);
                        lineLen = 0;
                    }
                }
            }

            SRL::Debug::Print(1, 7, "Last TX: %s", lastTx);
            SRL::Debug::Print(1, 8, "Last RX: %s", lastRx);
            SRL::Debug::Print(1, 9, "HELLO sent: %d", helloCount);
            SRL::Debug::Print(1, 10, "OK sent: %d", okCount);
            SRL::Debug::Print(1, 11, "RX mode: CR/LF line end");
        }

        SRL::Debug::Print(1, 6, "DevCart: %s", connected ? "connected" : "not detected");
        SRL::Debug::Print(1, 12, "Frame: %d", frameCount);
        ++frameCount;
        SRL::Core::Synchronize();
    }

    return 0;
}

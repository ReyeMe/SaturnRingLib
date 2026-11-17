// Tests/src/testsWave.hpp
// SRL::Sound::Pcm::WaveSound: loading 8 and 16-bit, mono and stereo WAV files into PCM data (no playback).
// The 64-frame files are written by Tests/campaigns/media/make_data.py: the formulas below must match it.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_sound.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

static constexpr int WaveFrames = 64;

/** @brief Unsigned 8-bit sample of frame f, channel c, as stored in the file. */
static int wave_sample8(int f, int c) { return (f * 7 + c * 100) & 0xff; }

/** @brief 16-bit sample of frame f, channel c (0 = left). */
static int16_t wave_sample16(int f, int c) { return static_cast<int16_t>(c == 0 ? f * 300 - 5000 : 5000 - f * 200); }

/** @brief Gives the tests access to the loaded PCM data. */
class WaveProbe : public Sound::Pcm::WaveSound
{
public:
    using WaveSound::WaveSound;
    const int8_t *Data() const { return this->data; }
    uint32_t Size() const { return this->dataSize; }
    uint8_t Mode() const { return this->mode; }
    uint8_t Depth() const { return this->depth; }
    uint16_t Rate() const { return this->sampleRate; }
};

/** @brief Checks the format fields and the zero padding up to the 0x900-byte minimum slPCMOn needs. */
static bool wave_check_format(const WaveProbe &wave, const char *file, uint8_t mode, uint8_t depth, uint32_t bytes)
{
    if (wave.Data() == nullptr || wave.Mode() != mode || wave.Depth() != depth || wave.Rate() != 11025 || wave.Size() != 0x900)
    {
        snprintf(buffer, buffer_size, "%s: data %s, mode 0x%02x, depth 0x%02x, rate %d, size 0x%x (expected 0x%02x, 0x%02x, 11025, 0x900)",
            file, wave.Data() ? "set" : "null", wave.Mode(), wave.Depth(), wave.Rate(), static_cast<unsigned>(wave.Size()), mode, depth);
        return false;
    }

    for (uint32_t i = bytes; i < wave.Size(); i++)
    {
        if (wave.Data()[i] != 0)
        {
            snprintf(buffer, buffer_size, "%s: padding byte 0x%x is %d, expected 0", file, static_cast<unsigned>(i), wave.Data()[i]);
            return false;
        }
    }

    return true;
}

/** @brief 8-bit mono: samples become signed (sample - 128), the rest is zero padding. */
MU_TEST(wave_mono_8bit)
{
    WaveProbe wave("M8.WAV");
    mu_assert(wave_check_format(wave, "M8.WAV", _Mono, _PCM8Bit, WaveFrames), buffer);

    for (int f = 0; f < WaveFrames; f++)
    {
        const int expected = static_cast<int8_t>(wave_sample8(f, 0) - 128);
        snprintf(buffer, buffer_size, "Sample %d is %d, expected %d", f, wave.Data()[f], expected);
        mu_assert(wave.Data()[f] == expected, buffer);
    }
}

/** @brief 16-bit mono, with a LIST chunk before the data chunk: samples are copied as they are. */
MU_TEST(wave_mono_16bit)
{
    WaveProbe wave("M16.WAV");
    mu_assert(wave_check_format(wave, "M16.WAV", _Mono, _PCM16Bit, WaveFrames * 2), buffer);
    const int16_t *samples = reinterpret_cast<const int16_t *>(wave.Data());

    for (int f = 0; f < WaveFrames; f++)
    {
        snprintf(buffer, buffer_size, "Sample %d is %d, expected %d", f, samples[f], wave_sample16(f, 0));
        mu_assert(samples[f] == wave_sample16(f, 0), buffer);
    }
}

/** @brief 16-bit stereo: one channel per half, left first like the 8-bit stereo loader (srl_sound.hpp, LoadPcmData). */
MU_TEST(wave_stereo_16bit)
{
    WaveProbe wave("S16.WAV");
    mu_assert(wave_check_format(wave, "S16.WAV", _Stereo, _PCM16Bit, WaveFrames * 4), buffer);
    const int16_t *samples = reinterpret_cast<const int16_t *>(wave.Data());

    for (int f = 0; f < WaveFrames; f++)
    {
        snprintf(buffer, buffer_size, "Frame %d: halves hold %d and %d, expected left %d then right %d",
            f, samples[f], samples[WaveFrames + f], wave_sample16(f, 0), wave_sample16(f, 1));
        mu_assert(samples[f] == wave_sample16(f, 0) && samples[WaveFrames + f] == wave_sample16(f, 1), buffer);
    }
}

/** @brief 8-bit stereo: one channel per half, left first, samples signed. */
MU_TEST(wave_stereo_8bit)
{
    // Not run: the loader writes the right channel at data[dataSize + sample], past the end of its buffer, which corrupts the heap
    mu_fail("Not run: 8-bit stereo loading writes past the end of its buffer");
}

/** @brief Loads and destroys one sound; describes a leak in buffer. */
static bool wave_check_no_leak(const char *file)
{
    const size_t hw = Memory::HighWorkRam::GetReport().UsedBlocks;
    const size_t lw = Memory::LowWorkRam::GetReport().UsedBlocks;
    {
        WaveProbe wave(file);
    }
    const size_t hwAfter = Memory::HighWorkRam::GetReport().UsedBlocks;
    const size_t lwAfter = Memory::LowWorkRam::GetReport().UsedBlocks;
    snprintf(buffer, buffer_size, "%s: used blocks HWRam %d -> %d, LWRam %d -> %d", file, hw, hwAfter, lw, lwAfter);
    return hwAfter == hw && lwAfter == lw;
}

/** @brief Loading and destroying a sound gives all its memory back. */
MU_TEST(wave_no_leak)
{
    mu_assert(wave_check_no_leak("M8.WAV"), buffer);
    mu_assert(wave_check_no_leak("M16.WAV"), buffer);
}

MU_TEST_SUITE(wave_test_suite)
{
    MU_RUN_TEST(wave_mono_8bit);
    MU_RUN_TEST(wave_mono_16bit);
    MU_RUN_TEST(wave_stereo_16bit);
    MU_RUN_TEST(wave_stereo_8bit);
    MU_RUN_TEST(wave_no_leak);
}

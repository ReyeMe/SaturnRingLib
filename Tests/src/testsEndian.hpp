// Tests/src/testsEndian.hpp
// SRL::Endian: reading little-endian 8, 16, 24 and 32-bit numbers from byte buffers (file formats such as TGA).
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_endian.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

/** @brief Least significant byte first. */
MU_TEST(endian_little_endian_values)
{
    uint8_t bytes[4] = { 0x78, 0x56, 0x34, 0x12 };

    mu_assert(Endian::DeserializeUint8(bytes) == 0x78, "DeserializeUint8 did not return the first byte");
    snprintf(buffer, buffer_size, "DeserializeUint16 gave 0x%04x, expected 0x5678", Endian::DeserializeUint16(bytes));
    mu_assert(Endian::DeserializeUint16(bytes) == 0x5678, buffer);
    snprintf(buffer, buffer_size, "DeserializeUint24 gave 0x%06x, expected 0x345678", static_cast<unsigned>(Endian::DeserializeUint24(bytes)));
    mu_assert(Endian::DeserializeUint24(bytes) == 0x345678, buffer);
    snprintf(buffer, buffer_size, "DeserializeUint32 gave 0x%08x, expected 0x12345678", static_cast<unsigned>(Endian::DeserializeUint32(bytes)));
    mu_assert(Endian::DeserializeUint32(bytes) == 0x12345678, buffer);
}

/** @brief Bytes with the top bit set are not sign-extended. */
MU_TEST(endian_high_bytes_not_sign_extended)
{
    uint8_t bytes[4] = { 0xff, 0xfe, 0x80, 0xff };

    snprintf(buffer, buffer_size, "DeserializeUint16 gave 0x%04x, expected 0xfeff", Endian::DeserializeUint16(bytes));
    mu_assert(Endian::DeserializeUint16(bytes) == 0xfeff, buffer);
    snprintf(buffer, buffer_size, "DeserializeUint24 gave 0x%08x, expected 0x0080feff", static_cast<unsigned>(Endian::DeserializeUint24(bytes)));
    mu_assert(Endian::DeserializeUint24(bytes) == 0x80feff, buffer);
    snprintf(buffer, buffer_size, "DeserializeUint32 gave 0x%08x, expected 0xff80feff", static_cast<unsigned>(Endian::DeserializeUint32(bytes)));
    mu_assert(Endian::DeserializeUint32(bytes) == 0xff80feffu, buffer);
}

/** @brief Reading from an odd address works: the SH-2 faults on unaligned 16/32-bit reads, so bytes must be read one by one. */
MU_TEST(endian_unaligned_buffer)
{
    uint8_t bytes[8] = { 0, 0x11, 0x22, 0x33, 0x44, 0, 0, 0 };

    snprintf(buffer, buffer_size, "DeserializeUint16 at an odd address gave 0x%04x, expected 0x2211", Endian::DeserializeUint16(bytes + 1));
    mu_assert(Endian::DeserializeUint16(bytes + 1) == 0x2211, buffer);
    snprintf(buffer, buffer_size, "DeserializeUint32 at an odd address gave 0x%08x, expected 0x44332211", static_cast<unsigned>(Endian::DeserializeUint32(bytes + 1)));
    mu_assert(Endian::DeserializeUint32(bytes + 1) == 0x44332211, buffer);
}

/** @brief Zero bytes give zero. */
MU_TEST(endian_zero)
{
    uint8_t bytes[4] = { 0, 0, 0, 0 };
    mu_assert(Endian::DeserializeUint16(bytes) == 0 && Endian::DeserializeUint24(bytes) == 0 && Endian::DeserializeUint32(bytes) == 0,
        "Zero bytes did not give zero");
}

MU_TEST_SUITE(endian_test_suite)
{
    MU_RUN_TEST(endian_little_endian_values);
    MU_RUN_TEST(endian_high_bytes_not_sign_extended);
    MU_RUN_TEST(endian_unaligned_buffer);
    MU_RUN_TEST(endian_zero);
}

#pragma once

/** @brief Endian conversion utilities
 */
namespace SRL::Endian
{
    /** @brief Deserialize number
     * @param buf Value buffer
     * @return Deserialized value
     */
    constexpr inline static uint8_t DeserializeUint8(uint8_t *buf)
    {
        return *(buf);
    }

    /** @brief Deserialize number
     * @param buf Value buffer
     * @return Deserialized value
     */
    constexpr inline static uint16_t DeserializeUint16(uint8_t *buf)
    {
        return (*(buf + 1) << 8) | *(buf);
    }

    /** @brief Deserialize number
     * @param buf Value buffers
     * @return Deserialized value
     */
    constexpr inline static uint32_t DeserializeUint24(uint8_t *buf)
    {
        return (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
    }

    /** @brief Deserialize number
     * @param buf Value buffer
     * @return Deserialized value
     */
    constexpr inline static uint32_t DeserializeUint32(uint8_t *buf)
    {
        return (*(buf + 3) << 24) | (*(buf + 2) << 16) | (*(buf + 1) << 8) | *(buf);
    }
}

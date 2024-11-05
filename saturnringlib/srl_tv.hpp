#pragma once

#include "srl_base.hpp"

/** @brief Television control
 */
namespace SRL
{
    /** @brief Contains TV resolutions
     */
    class TV
    {
        /** @brief Make class purely static
         */
        TV() = delete;

        /** @brief Make class purely static
         */
        ~TV() = delete;
    public:

        /** @brief Available TV resolutions
         */
        enum class Resolutions
        {
            Normal320x224 = TV_320x224,
            Normal320x240 = TV_320x240,
            Normal352x224 = TV_352x224,
            Normal352x240 = TV_352x240,

            Interlaced640x224 = TV_640x224,
            Interlaced640x240 = TV_640x240,
            Interlaced704x224 = TV_704x224,
            Interlaced704x240 = TV_704x240,

            Normal320x448i = 16,
            Normal320x480i = 17,
            Normal352x448 = 20,
            Normal352x480 = 21,

            Interlaced640x448i = 24,
            Interlaced640x480i = 25,
            Interlaced704x448 = 28,
            Interlaced704x480 = 29
        };

#ifdef SRL_MODE_PAL
        /** @brief Screen width
         */
        inline static const uint16_t Width = 320;

        /** @brief Screen height
         */
        inline static const uint16_t Height = 256;

        /** @brief Screen resolution mode
         */
        inline static const TV::Resolutions Resolution = TV::Resolutions::Normal320x256;
#elif SRL_MODE_NTSC
    #ifdef SRL_HIGH_RES
        /** @brief Screen width
         */
        inline static const uint16_t Width = 704;

        /** @brief Screen height
         */
        inline static const uint16_t Height = 480;

        /** @brief Screen resolution mode
         */
        inline static const TV::Resolutions Resolution = TV::Resolutions::Interlaced704x480;
    #else
        /** @brief Screen width
         */
        inline static const uint16_t Width = 320;

        /** @brief Screen height
         */
        inline static const uint16_t Height = 240;

        /** @brief Screen resolution mode
         */
        inline static const TV::Resolutions Resolution = TV::Resolutions::Normal320x240;
    #endif
#elif DOXYGEN
        /** @brief Screen width
         * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
         */
        inline static const uint16_t Width;

        /** @brief Screen height
         * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
         */
        inline static const uint16_t Height;

        /** @brief Screen resolution mode
         * @note Differs based on makefile setting SRL_MODE = (PAL | NTSC) and whether SRL_HIGH_RES is set
         */
        inline static const TV::Resolutions Resolution;
#endif

    };
};

#pragma once

#include "srl_base.hpp"

/** @brief Television control
 */
namespace SRL
{
    namespace Types
    {
        /** @brief Area resolution
         */
        struct Resolution final
        {
            /** @brief Construct a new Resolution tuple
             */
            Resolution() : Width(0), Height(0) {}

            /** @brief Construct a new Resolution tuple
             * @param width Area width
             * @param height Area height
             */
            Resolution(const int16_t width, const int16_t height) : Width(width), Height(height) {}

            /** @brief Area width
             */
            int16_t Width;

            /** @brief Area height
             */
            int16_t Height;
        };
    }

    /** @brief Contains TV resolutions
     */
    class TV final
    {
    public:
    
        /** @brief Available TV resolutions
         */
        enum class Resolutions
        {
            Normal320x224 = 0,
            Normal320x240 = 1,
            Normal320x256 = 2,

            Normal352x224 = 4,
            Normal352x240 = 5,
            Normal352x256 = 6,

            Normal640x224 = 8,
            Normal640x240 = 9,
            Normal640x256 = 10,

            Normal704x224 = 12,
            Normal704x240 = 13,
            Normal704x256 = 14,

            Interlaced320x448 = 16,
            Interlaced320x480 = 17,
            Interlaced320x512 = 18,

            Interlaced352x448 = 20,
            Interlaced352x480 = 21,
            Interlaced352x512 = 22,

            Interlaced640x448 = 24,
            Interlaced640x480 = 25,
            Interlaced640x512 = 26,

            Interlaced704x448 = 28,
            Interlaced704x480 = 29,
            Interlaced704x512 = 30,
        };

    private:

        /** @brief Befriend core
         */
        friend SRL::Core;

        /** @brief Make class purely static
         */
        TV() = delete;

        /** @brief Make class purely static
         */
        ~TV() = delete;

        /** @brief Set the screen resolution state
         * @param resolution Current resolution
         */
        static void SetScreenSize(const Resolutions resolution)
        {
            TV::Resolution = resolution;

            // Set Width
            switch (resolution)
            {
            case Resolutions::Normal320x224:
            case Resolutions::Normal320x240:
            case Resolutions::Normal320x256:
            case Resolutions::Interlaced320x448:
            case Resolutions::Interlaced320x480:
            case Resolutions::Interlaced320x512:
                TV::Width = 320;
                break;
            
            case Resolutions::Normal352x224:
            case Resolutions::Normal352x240:
            case Resolutions::Normal352x256:
            case Resolutions::Interlaced352x448:
            case Resolutions::Interlaced352x480:
            case Resolutions::Interlaced352x512:
                TV::Width = 352;
                break;

            case Resolutions::Normal640x224:
            case Resolutions::Normal640x240:
            case Resolutions::Normal640x256:
            case Resolutions::Interlaced640x448:
            case Resolutions::Interlaced640x480:
            case Resolutions::Interlaced640x512:
                TV::Width = 640;
                break;

            case Resolutions::Normal704x224:
            case Resolutions::Normal704x240:
            case Resolutions::Normal704x256:
            case Resolutions::Interlaced704x448:
            case Resolutions::Interlaced704x480:
            case Resolutions::Interlaced704x512:
                TV::Width = 704;
                break;

            default:
                break;
            }
            
            // Set Height
            switch (resolution)
            {
            case Resolutions::Normal320x224:
            case Resolutions::Normal352x224:
            case Resolutions::Normal640x224:
            case Resolutions::Normal704x224:
                TV::Height = 224;
                break;

            case Resolutions::Normal320x240:
            case Resolutions::Normal352x240:
            case Resolutions::Normal640x240:
            case Resolutions::Normal704x240:
                TV::Height = 240;
                break;

            case Resolutions::Normal320x256:
            case Resolutions::Normal352x256:
            case Resolutions::Normal640x256:
            case Resolutions::Normal704x256:
                TV::Height = 256;
                break;

            case Resolutions::Interlaced320x448:
            case Resolutions::Interlaced352x448:
            case Resolutions::Interlaced640x448:
            case Resolutions::Interlaced704x448:
                TV::Height = 448;
                break;

            case Resolutions::Interlaced320x480:
            case Resolutions::Interlaced352x480:
            case Resolutions::Interlaced640x480:
            case Resolutions::Interlaced704x480:
                TV::Height = 480;
                break;

            case Resolutions::Interlaced320x512:
            case Resolutions::Interlaced352x512:
            case Resolutions::Interlaced640x512:
            case Resolutions::Interlaced704x512:
                TV::Height = 512;
                break;
            
            default:
                break;
            }
        }

    public:

        /** @brief Turn on TV display
         */
        static void TVOn()
        {
            slTVOn();
        }

        /** @brief Turn off TV display
         */
        static void TVOff()
        {
            slTVOff();
        }

        /** @brief Read only screen width
         */
        inline static int16_t Width;

        /** @brief Read only screen height
         */
        inline static int16_t Height;

        /** @brief Read only screen resolution mode
         */
        inline static TV::Resolutions Resolution;
    };
};

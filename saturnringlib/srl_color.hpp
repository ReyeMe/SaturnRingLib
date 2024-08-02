#pragma once

#include "srl_base.hpp"

namespace SRL::Types
{
    /** @brief Color in ABGR1555 format
     * @details 16-bit color format where each color channel is 5 bits (values range 0-31) and there is 1 bit for opacity
     */
    struct HighColor
    {
        /** @brief Indicates whether color is opaque or not
         */
        uint16_t Opaque : 1;

        /** @brief Blue color channel
         */
        uint16_t Blue : 5;
        
        /** @brief Green color channel
         */
        uint16_t Green : 5;
        
        /** @brief Red color channel
         */
        uint16_t Red : 5;

        /** @brief Defines new transparent color
         */
        constexpr HighColor() : Opaque(0), Blue(0), Green(0), Red(0)
        {
            // Do nothing
        }
        
        /** @brief Defines new color from saturn ABGR555 value
         * @param value VAlue in ABGR555
         */
        constexpr HighColor(const uint16_t& value) :
            Opaque((value >> 15) & 0x1),
            Blue((value >> 10) & 0x1f),
            Green((value >> 5) & 0x1f),
            Red(value & 0x1f)
        {
            // Do nothing
        }
        
        /** @brief Defines new color (components are in rage of 0-255 but are divided by 8)
         * @param r Red component
         * @param g Green component
         * @param b Blue component
         */
        constexpr HighColor(const uint8_t& r, const uint8_t& g, const uint8_t& b) :
            Opaque(1),
            Red((r >> 3) & 0x1f),
            Green((g >> 3) & 0x1f),
            Blue((b >> 3) & 0x1f)
        {
            // Do nothing
        }

        /** @brief Get color from RGB555 (components are in range o 0-31)
         * @param r Red component
         * @param g Green component
         * @param b Blue component
         * @return constexpr HighColor 
         */
        constexpr inline static HighColor FromRGB555(const uint8_t& r, const uint8_t& g, const uint8_t& b)
        {
            return HighColor(0x8000 | ((b & 0x1f) << 10) | ((g & 0x1f) << 5) | (r & 0x1f));
        }

        /** @brief Get from RGB555
         * @param value Value in ARGB555 format
         * @return HighColor color
         */
        constexpr inline static HighColor FromARGB15(const uint16_t& value)
        {
            return ((value >> 15) & 0x1) != 0 ? HighColor::FromRGB555((value >> 10) & 0x1f, (value >> 5) & 0x1f, value & 0x1f) : HighColor();
        }
        
        /** @brief Get from RGB888
         * @param value Value in ARGB888 format
         * @return HighColor color
         */
        constexpr inline static HighColor FromRGB24(const uint32_t& value)
        {
            return HighColor::FromRGB555(
                (((value >> 16) & 0xff) >> 3) & 0x1f,
                (((value >> 8) & 0xff) >> 3) & 0x1f,
                ((value & 0xff) >> 3) & 0x1f);
        }
        
        /** @brief Get from ARGB888
         * @param value Value in ARGB888 format
         * @return HighColor color
         */
        constexpr inline static HighColor FromARGB32(const uint32_t& value)
        {
            if (((value >> 24) & 0xff) > 128)
            {
                return HighColor::FromRGB555(
                    (((value >> 16) & 0xff) >> 3) & 0x1f,
                    (((value >> 8) & 0xff) >> 3) & 0x1f,
                    ((value & 0xff) >> 3) & 0x1f);
            }

            return HighColor();
        }
        
        /** @brief Get color represented as ABGR555 value
         * @return Single ABGR555 value
         */
        constexpr inline uint16_t GetABGR()
        {
            return *((uint16_t*)this);
        }
        
        // This is automatically called 
        // when '=' operator is 
        // used between C1 and C2. 
    
        constexpr inline void operator=(const uint16_t& value) 
        { 
            this->Opaque = (value >> 15) & 0x1;
            this->Red = value & 0x1f;
            this->Green = (value >> 5) & 0x1f;
            this->Blue = (value >> 10) & 0x1f; 
        } 
  
        /** @brief Cast to ABGR555
         * @return ABGR value
         */
        constexpr inline operator uint16_t&() { return *((uint16_t*)this); }
        
        /** @brief Cast to ABGR555
         * @return ABGR value
         */
        constexpr inline operator uint16_t() const { return *((uint16_t*)this); }

        /** @brief Predefined colors
         */
        class Colors;
    };

    /** @brief Predefined colors
     */
    class HighColor::Colors
    {
    public:
        /** @brief White color
         */
        static const constexpr Types::HighColor White = Types::HighColor(0xffff);
        
        /** @brief White color
         */
        static const constexpr Types::HighColor Black = Types::HighColor(0x8000);

        /** @brief Red color
         */
        static const constexpr Types::HighColor Red = Types::HighColor(255, 0, 0);
        
        /** @brief Green color
         */
        static const constexpr Types::HighColor Green = Types::HighColor(0, 255, 0);
        
        /** @brief Blue color
         */
        static const constexpr Types::HighColor Blue = Types::HighColor(0, 0, 255);
        
        /** @brief Yellow color
         */
        static const constexpr Types::HighColor Yellow = Types::HighColor(255, 255, 0);
        
        /** @brief Magenta color
         */
        static const constexpr Types::HighColor Magenta = Types::HighColor(255, 0, 255);
    };
    /** @brief Color in ABGR1888 format
     * @details 32-bit color format where each color channel is 8 bits (values range 0-31) and there is 1 bit for opacity. 7 bits are unused.
     */
    struct TrueColor
    {
        /** @brief Indicates whether color is opaque or not
         */
        uint8_t Opaque : 1;

        /** @brief Unused bits
         */
        uint8_t Unused : 7;

        /** @brief Blue color channel
         */
        uint8_t Blue;
        
        /** @brief Green color channel
         */
        uint8_t Green;
        
        /** @brief Red color channel
         */
        uint8_t Red;

        /** @brief Defines new transparent color
         */
        constexpr TrueColor() : Opaque(0), Blue(0), Green(0), Red(0), Unused(0)
        {
            // Do nothing
        }
        
        /** @brief Defines new color from saturn ABGR1888 value
         * @param value VAlue in ABGR1888
         */
        constexpr TrueColor(const uint32_t& value) :
            Opaque(value & 0x80000000 != 0),
            Blue((value >> 16) & 0xff),
            Green((value >> 8) & 0xff),
            Red(value & 0xff),
            Unused(0)
        {
            // Do nothing
        }
        
        /** @brief Defines new color
         * @param r Red component
         * @param g Green component
         * @param b Blue component
         */
        constexpr TrueColor(const uint8_t& r, const uint8_t& g, const uint8_t& b) :
            Opaque(1),
            Red(r),
            Green(g),
            Blue(b),
            Unused(0)
        {
            // Do nothing
        }

        /** @brief Get from ARGB1555
         * @param value Value in ARGB1555 format
         * @return HighColor color
         */
        constexpr inline static TrueColor FromARGB32(const uint16_t& value)
        {
            return ((value & 0x80000000) != 0) ? TrueColor((value >> 16) & 0xff, (value >> 8) & 0xff, value & 0xff) : TrueColor();
        }
        
        /** @brief Get from RGB888
         * @param value Value in ARGB888 format
         * @return HighColor color
         */
        constexpr inline static TrueColor FromRGB24(const uint32_t& value)
        {
            return TrueColor(
                (value >> 16) & 0xff,
                (value >> 8) & 0xff,
                value & 0xff);
        }
        
        /** @brief Get from ARGB888
         * @param value Value in ARGB888 format
         * @return HighColor color
         */
        constexpr inline static TrueColor FromARGB32(const uint32_t value)
        {
            if (value & 0xff000000 > 0)
            {
                return TrueColor(
                    (value >> 16) & 0xff,
                    (value >> 8) & 0xff,
                    value & 0xff);
            }

            return TrueColor();
        }
        
        /** @brief Get color represented as ABGR1888 value
         * @return Single ABGR1888 value
         */
        constexpr inline uint32_t GetABGR()
        {
            return *((uint32_t*)this);
        }
        
        /** @brief Assign color value to color type
         * @param value Color value
         */
        constexpr inline void operator=(const uint32_t& value) 
        { 
            this->Opaque = value & 0x80000000 != 0;
            this->Red = value & 0xff;
            this->Green = (value >> 8) & 0xff;
            this->Blue = (value >> 16) & 0xff; 
        } 
  
        /** @brief Cast to ABGR1888
         * @return ABGR value
         */
        constexpr inline operator uint32_t&() { return *((uint32_t*)this); }
        
        /** @brief Cast to ABGR1888
         * @return ABGR value
         */
        constexpr inline operator uint32_t() const { return *((uint32_t*)this); }
        
        /** @brief Predefined colors
         */
        class Colors;
    };
    
    /** @brief Predefined colors
     */
    class TrueColor::Colors
    {
    public:
        /** @brief White color
         */
        static const constexpr Types::TrueColor White = Types::TrueColor(0xffff);
        
        /** @brief White color
         */
        static const constexpr Types::TrueColor Black = Types::TrueColor(0x8000);

        /** @brief Red color
         */
        static const constexpr Types::TrueColor Red = Types::TrueColor(255, 0, 0);
        
        /** @brief Green color
         */
        static const constexpr Types::TrueColor Green = Types::TrueColor(0, 255, 0);
        
        /** @brief Blue color
         */
        static const constexpr Types::TrueColor Blue = Types::TrueColor(0, 0, 255);
        
        /** @brief Yellow color
         */
        static const constexpr Types::TrueColor Yellow = Types::TrueColor(255, 255, 0);
        
        /** @brief Magenta color
         */
        static const constexpr Types::TrueColor Magenta = Types::TrueColor(255, 0, 255);
    };
}

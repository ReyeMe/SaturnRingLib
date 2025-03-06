#pragma once

/** @brief Main SGL namespace
 */
extern "C" {
    #include <sgl.h>
    #include <sega_gfs.h>
    #include <sega_bup.h>
    #include <sega_int.h>
    #include "stdint-gcc.h"
    #include "stddef.h"

    extern uint8_t SynchConst;
    extern int8_t SynchCount;
    extern uint16_t VDP2_EXTEN;
    extern FIXED MsScreenDist;
    extern uint16_t VDP2_RAMCTL;
}

// Include math library

/** @brief SaturnMath++ library functions
 * @details SaturnMath++ is a C++23 library dedicated to Sega Saturn hardware, offering essential mathematical operations tailored for fixed-point arithmetic and geometric calculations.<br/>
 * Project url: <a href="https://github.com/robertoduarte/SaturnMathPP">Here</a>
 */
namespace SRL::Math { }

#define SaturnMath SRL::Math
#include <saturn_math.hpp>
#undef SaturnMath

#if !DOXYGEN
namespace std {

    /** @brief Some hacks to make STD behave
     */

    /** @brief Hack to get rid of exception https://forum.edgeimpulse.com/t/error-with-arduino-library-on-adafruit-nrf-board/422/13
     */
    void __throw_length_error(char const*) {
        while(1);
    }
}
#endif

/** @brief Saturn ring library
 */
namespace SRL { }

/** @brief Value types
 */
namespace SRL::Types { }

/** @brief SGL helpers
 */
namespace SRL::SGL
{
    /** @brief SGL type helper
     * @tparam Class C++ class
     * @tparam Type SGL type
     */
    template <class Class, typename Type>
    struct SglType
    {
        /** @brief Get pointer to SGL type
         * @return Pointer to SGL type
         */
        Type* SglPtr()
        {
            static_assert(sizeof(Class) == sizeof(Type), "Casts are impossible, size does not match");
            return (Type*)this;
        }

        /** @brief Get pointer to SGL type
         * @return Pointer to SGL type
         */
        const Type* SglConstPtr() const
        {
            static_assert(sizeof(Class) == sizeof(Type), "Casts are impossible, size does not match");
            return (const Type*)this;
        }
    };
}
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
/**
 * @brief C++ runtime support workarounds.
 * 
 * These functions provide minimal implementations that prevent compilation errors.
 */
namespace std
{
    /** @brief Minimal std::length_error handler
     * 
     * This function is a minimal implementation to prevent compilation errors
     * when the standard library expects a length error handler. It enters an
     * infinite loop to prevent undefined behavior.
     */
    inline void __throw_length_error(char const* message)
    {
        while (1);
    }
}
/**
 * @brief Minimal pure virtual function handler
 * 
 * This function is a minimal implementation to prevent compilation errors
 * when calling pure virtual functions. Uses C linkage and weak attribute
 * to allow multiple definitions across different modules.
 */
extern "C" void __attribute__((weak)) __cxa_pure_virtual()
{
    while (1);
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
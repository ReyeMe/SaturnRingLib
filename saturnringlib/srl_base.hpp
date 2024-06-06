#pragma once

/** @brief Main SGL namespace
 */
extern "C" {
    #include <SGL.H>
    #include <SEGA_GFS.H>
    #include <SEGA_BUP.H>
    #include <SEGA_INT.H>
    #include "stddef.h"
}

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
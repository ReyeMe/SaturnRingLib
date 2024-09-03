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
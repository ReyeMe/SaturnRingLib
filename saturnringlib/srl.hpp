#pragma once

#include "srl_core.hpp"
#include "bitmap/srl_tga.hpp"

template <class Class, typename Type>
struct SglType
{
    Type* SglPtr()
    {
        static_assert(sizeof(Class) == sizeof(Type), "Casts are impossible, size does not match");
        return (Type*)this;
    }

    const Type* SglConstPtr() const
    {
        static_assert(sizeof(Class) == sizeof(Type), "Casts are impossible, size does not match");
        return (const Type*)this;
    }
};

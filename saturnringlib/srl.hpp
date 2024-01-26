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

inline void* operator new(size_t size)
{
    return SRL::Memory::Malloc(size);
}

inline void operator delete(void* ptr)
{
    SRL::Memory::Free(ptr);
}

inline void operator delete(void* ptr, unsigned int)
{
    SRL::Memory::Free(ptr);
}

inline void* operator new(size_t, void* ptr)
{
    return ptr;
}

// Add overload for array form of new operator
inline void* operator new[](size_t size)
{
    return SRL::Memory::Malloc(size);
}

// Add overload for array form of delete operator
inline void operator delete[](void* ptr)
{
    SRL::Memory::Free(ptr);
}

// Add overload for array form of delete operator with size parameter
inline void operator delete[](void* ptr, size_t size)
{
    SRL::Memory::Free(ptr);
}

// Add overload for array form of placement new
inline void* operator new[](size_t, void* ptr)
{
    return ptr;
}

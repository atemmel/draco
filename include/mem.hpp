#pragma once

#include "types.hpp"

struct Allocator {
    using Alloc_Fn = void* (*)(void* context, usize size);
    using Free_Fn  = void (*)(void* context, void* pointer);

    Alloc_Fn alloc;
    Free_Fn free;
    void* context;

    template <typename T>
    auto Alloc(usize count) -> T* {
        return (T*)alloc(context, sizeof(T) * count);
    }

    template <typename T>
    auto Create() -> T* {
        return (T*)alloc(context, sizeof(T));
    }

    template <typename T>
    auto Free(const T* ptr) -> T* {
        free(context, ptr);
    }
};

const extern Allocator default_allocator;

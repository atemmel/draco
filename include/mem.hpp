#pragma once

#include "types.hpp"

struct Allocator {
    using Alloc_Fn = void* (*)(void* context, usize size);
    using Free_Fn  = void (*)(void* context, void* pointer);

    Alloc_Fn alloc;
    Free_Fn  free;
    void*    context;

    template <typename T>
    auto Alloc(usize count) -> T* {
        return (T*)alloc(context, sizeof(T) * count);
    }

    template <typename T>
    auto Create() -> T* {
        return (T*)alloc(context, sizeof(T));
    }

    template <typename T>
    auto Free(const T* ptr) -> void {
        free(context, (void*)ptr);
    }
};

struct Tracing_Allocator {
    u64       n_allocs;
    u64       n_frees;
    Allocator base_allocator;

    auto Interface() -> Allocator;
};

struct Arena_Allocator {
    struct Region {
        Region* next;
        u8*     data;
        usize   occupied;
        usize   capacity;
    };

    Region*   begin;
    Allocator base_allocator;

    auto Interface() -> Allocator;
    auto Reset() -> void;
    auto Destroy() -> void;
};

auto Create_Tracing_Allocator(Allocator base_allocator) -> Tracing_Allocator;
auto Create_Arena_Allocator(Allocator base_allocator) -> Arena_Allocator;

const extern Allocator default_allocator;

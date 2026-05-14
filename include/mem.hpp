#pragma once

#include <cstring>

#include "slice.hpp"
#include "types.hpp"

struct String;

struct Allocator {
    using Alloc_Fn = void* (*)(void* context, s64 size);
    using Free_Fn  = void (*)(void* context, void* pointer);

    Alloc_Fn alloc;
    Free_Fn  free;
    void*    context;

    template <typename T>
    auto Alloc(s64 count) -> T* {
        if (count == 0) return null;
        auto size = sizeof(T) * count;
        auto ptr  = (T*)alloc(context, size);
        memset(ptr, 0, size);
        return ptr;
    }

    template <typename T>
    auto Create() -> T* {
        return Alloc<T>(1);
    }

    template <typename T>
    auto Free(const T* ptr) -> void {
        if (ptr) free(context, (void*)ptr);
    }

    template <typename T>
    auto Dupe(Slice<T> slice) -> Slice<T> {
        auto ptr = Alloc<T>(slice.size);
        memcpy(ptr, slice.data, sizeof(T) * slice.size);
        return {
            .data = ptr,
            .size = slice.size,
        };
    }

    auto DupeString(String string) -> String;

    // Allocates according to format. Provides null terminator
    auto AllocPrint(const char* __restrict fmt, ...) -> String;
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
        s64     occupied;
        s64     capacity;
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

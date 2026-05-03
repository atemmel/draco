#include "mem.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "runtime.hpp"
#include "strings.hpp"
#include "types.hpp"

auto Allocator::DupeString(String string) -> String {
    if (string.size == 0) {
        return {
            .data = null,
            .size = 0,
        };
    }
    auto ptr = Alloc<u8>(string.size + 1);
    memcpy(ptr, string.data, sizeof(u8) * string.size);
    ptr[string.size] = 0;
    return {
        .data = ptr,
        .size = string.size,
    };
}

auto Allocator::AllocPrint(const char* fmt, ...) -> String {
    va_list args_1;
    va_start(args_1, fmt);
    va_list args_2;
    va_copy(args_2, args_1);
    usize required_len = vsnprintf(null, 0, fmt, args_1);
    va_end(args_1);
    auto ptr = this->Alloc<u8>(required_len + 1);
    auto str = Sprintf(ptr, required_len, fmt, args_2);
    va_end(args_2);
    return str;
}

auto Default_Allocator_Alloc(void*, usize size) -> void* {
    return malloc(size);
}

auto Default_Allocator_Free(void*, void* ptr) -> void {
    free(ptr);
}

const Allocator default_allocator = {
    .alloc   = Default_Allocator_Alloc,
    .free    = Default_Allocator_Free,
    .context = null,
};

auto Tracing_Allocator_Alloc(void* context, usize size) -> void* {
    auto ctx = (Tracing_Allocator*)context;
    ctx->n_allocs += 1;
    return ctx->base_allocator.Alloc<u8>(size);
}

auto Tracing_Allocator_Free(void* context, void* ptr) -> void {
    auto ctx = (Tracing_Allocator*)context;
    ctx->n_frees += 1;
    ctx->base_allocator.Free(ptr);
}

auto Tracing_Allocator::Interface() -> struct Allocator {
    return {
        .alloc   = Tracing_Allocator_Alloc,
        .free    = Tracing_Allocator_Free,
        .context = (void*)this,
    };
};

static auto Arena_Allocator_New_Region(Arena_Allocator* arena, usize requested_size) -> Arena_Allocator::Region* {
    constexpr usize default_size = 1 * 1000 * 1000;
    auto            new_size     = max(default_size, requested_size * 2);
    auto            region       = arena->base_allocator.Create<Arena_Allocator::Region>();
    Assert(region);

    *region = {
        .next     = null,
        .data     = arena->base_allocator.Alloc<u8>(new_size),
        .occupied = 0,
        .capacity = new_size,
    };

    Assert(region->data);
    return region;
}

auto Arena_Allocator_Alloc(void* context, usize size) -> void* {
    auto ctx = (Arena_Allocator*)context;

    Arena_Allocator::Region* previous = null;
    for (auto region = ctx->begin; region; previous = region, region = region->next) {
        if (region->capacity - region->occupied >= size) {
            auto ptr = region->data + region->occupied;
            region->occupied += size;
            return ptr;
        }
    }

    auto region      = Arena_Allocator_New_Region(ctx, size);
    region->occupied = size;

    if (previous) {
        previous->next = region;
    } else {
        ctx->begin = region;
    }

    return region->data;
}

auto Arena_Allocator_Free(void* context, void* ptr) -> void {
    // no-op
}

auto Arena_Allocator::Interface() -> Allocator {
    return {
        .alloc   = Arena_Allocator_Alloc,
        .free    = Arena_Allocator_Free,
        .context = (void*)this,
    };
}

auto Arena_Allocator::Reset() -> void {
    for (auto region = begin; region; region = region->next) {
        region->occupied = 0;
    }
}

auto Arena_Allocator::Destroy() -> void {
    for (auto region = begin; region;) {
        auto next = region->next;
        base_allocator.Free(region->data);
        base_allocator.Free(region);
        region = next;
    }
    begin = null;
}

auto Create_Tracing_Allocator(Allocator base_allocator) -> Tracing_Allocator {
    return {
        .n_allocs       = 0,
        .n_frees        = 0,
        .base_allocator = base_allocator,
    };
};

auto Create_Arena_Allocator(Allocator base_allocator) -> Arena_Allocator {
    return {
        .begin          = null,
        .base_allocator = base_allocator,
    };
}

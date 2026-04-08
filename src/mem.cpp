#include "mem.hpp"

#include <cstdlib>

#include "types.hpp"

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

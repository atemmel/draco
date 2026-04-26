
#include <cstdio>

#include "array.hpp"
#include "defer.hpp"
#include "mem.hpp"
#include "strings.hpp"

auto main() -> int {
    auto arena = Create_Arena_Allocator(default_allocator);
    defer(arena.Destroy());
    auto allocator = arena.Interface();

    auto bytes = Create_Array_Capacity<u8>(allocator, 16);
    Append_Slice(allocator, bytes, "abcdefgh"_s.Slice());

    Remove(bytes, 2);

    printf("\n");
}

#pragma once

#include "mem.hpp"
#include "slice.hpp"
#include "types.hpp"

struct String {
    u8*   data;
    usize size;

    operator const char*() const;

    auto Slice() -> Slice<u8>;
};

auto operator""_s(const char* ptr, unsigned long size) -> String;

auto Destroy_String(Allocator allocator, String& str) -> void;

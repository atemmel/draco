#pragma once

#include "mem.hpp"
#include "slice.hpp"
#include "types.hpp"

struct String {
    u8* data;
    s64 size;

    operator const char*() const;

    auto Slice() -> Slice<u8>;
};

auto operator""_s(const char* ptr, unsigned long size) -> String;

auto As_String(const char* ptr) -> String;

auto Destroy_String(Allocator allocator, String& str) -> void;

String Sprintf(u8* __restrict buffer, s64 buffer_size, const char* __restrict format, ...) __attribute__((format(printf, 3, 4)));

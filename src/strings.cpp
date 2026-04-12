#include "strings.hpp"

String::operator const char*() const {
    return (char*)data;
}

auto String::Slice() -> ::Slice<u8> {
    return {
        .data = data,
        .size = size,
    };
}

auto operator""_s(const char* ptr, unsigned long size) -> String {
    return {
        .data = (u8*)ptr,
        .size = size,
    };
}

auto Destroy_String(Allocator allocator, String& str) -> void {
    allocator.Free(str.data);
    str = {
        .data = null,
        .size = 0,
    };
}

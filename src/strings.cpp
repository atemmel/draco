#include "strings.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>

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
        .size = s64(size),
    };
}

auto As_String(const char* ptr) -> String {
    return {
        .data = (u8*)ptr,
        .size = s64(strlen(ptr)),
    };
};

auto Destroy_String(Allocator allocator, String& str) -> void {
    allocator.Free(str.data);
    str = {
        .data = null,
        .size = 0,
    };
}

auto Sprintf(u8* s, s64 maxlen, const char* format, ...) -> String {
    va_list args;
    va_start(args, format);
    s64 size = vsnprintf((char*)s, maxlen, format, args);
    va_end(args);
    return String{
        .data = s,
        .size = size,
    };
}

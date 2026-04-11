#include "strings.hpp"

String::operator const char*() const {
    return (char*)ptr;
}

auto operator""_s(const char* ptr, unsigned long size) -> String {
    return {
        .ptr  = (u8*)ptr,
        .size = size,
    };
}

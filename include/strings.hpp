#pragma once

#include "types.hpp"

struct String {
    u8* ptr;
    usize size;

    operator const char*() const;
};

auto operator""_s(const char* ptr, unsigned long size) -> String;

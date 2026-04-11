#pragma once

#include <cstddef>
#include <cstdint>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using usize = size_t;

constexpr auto null = nullptr;

#define size_of_array(x) (sizeof(x) / sizeof(x[0]))

template <typename T>
auto max(const T& lhs, const T& rhs) -> T {
    return lhs < rhs ? rhs : lhs;
}

template <typename T>
auto min(const T& lhs, const T& rhs) -> T {
    return lhs < rhs ? lhs : rhs;
}

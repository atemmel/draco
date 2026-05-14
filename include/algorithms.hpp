#pragma once

#include "slice.hpp"

template <typename T>
auto Equals(Slice<T> lhs, Slice<T> rhs) -> bool {
    if (lhs.size != rhs.size) return false;
    for (usize i = 0; i < lhs.size; i++) {
        if (lhs[i] != rhs[i]) return false;
    }
    return true;
}

#pragma once

#include "types.hpp"
template <typename T>
struct Array {
    T*    data     = null;
    usize size     = 0;
    usize capacity = 0;

    auto Append(const T& value) -> void;
    auto Remove(usize idx) -> void;
    auto Clear();
};

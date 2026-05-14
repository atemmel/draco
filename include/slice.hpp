#pragma once

#include "runtime.hpp"
#include "types.hpp"
template <typename T>
struct Slice {
    T*  data;
    s64 size;

    auto operator[](s64 idx) -> T& {
        Assert(idx < size);
        return data[idx];
    }

    auto operator[](s64 idx) const -> const T& {
        Assert(idx < size);
        return data[idx];
    }

    auto begin() -> T* {
        return data;
    }

    auto begin() const -> const T* {
        return data;
    }

    auto end() -> T* {
        return data + size;
    }

    auto end() const -> const T* {
        return data + size;
    }

    auto slice(s64 from = 0) -> Slice<T> {
        return slice(from, size);
    }

    auto slice(s64 from, s64 to) -> Slice<T> {
        Assert(from <= size);
        Assert(to <= size);
        Assert(from <= to);
        return {
            .data = data + from,
            .size = to - from,
        };
    }
};

#pragma once

#include <cassert>
#include <cstring>

#include "mem.hpp"
#include "types.hpp"

template <typename T>
struct Array {
    T*    data     = null;
    usize size     = 0;
    usize capacity = 0;

    auto Remove(usize idx) -> void;
    auto Clear();
};

template <typename T>
auto Append(Allocator allocator, Array<T>& array, const T& value) -> Array<T> {
    if (array.size < array.capacity) {
        array.data[array.size++] = value;
        return array;
    }

    // new array
    usize new_capacity   = max(4, array.capacity * 2);
    auto  new_data       = allocator.Alloc<T>(new_capacity);
    auto  new_size       = array.size + 1;
    new_data[array.size] = value;
    memcpy(new_data, array.data, sizeof(T) * array.size);

    // destroy old array
    allocator.Free(array.data);

    array = {
        .data     = new_data,
        .size     = new_size,
        .capacity = new_capacity,
    };

    return array;
}

template <typename T>
auto Insert(Allocator allocator, Array<T>& array, const T& value, usize idx) -> void {
    Append(allocator, array, {});

    auto insertion_point = array.data + idx;
    memmove(insertion_point + 1, insertion_point, sizeof(T) * (array.size - insertion_point));
    array.data[array.size++] = value;
}

template <typename T>
auto Pop(Array<T>& array) -> void {
    assert(array.size > 0);
    array.size--;
}

template <typename T>
auto Clear(Array<T>& array) -> void {
    array.size = 0;
}

template <typename T>
auto Free_Array(Allocator allocator, Array<T>& array) -> void {
    allocator.Free(array.data);
    array = {
        .data     = null,
        .size     = 0,
        .capacity = 0,
    };
}

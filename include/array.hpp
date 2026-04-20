#pragma once

#include <cassert>
#include <cstring>

#include "mem.hpp"
#include "slice.hpp"
#include "types.hpp"

template <typename T>
struct Array {
    T*    data     = null;
    usize size     = 0;
    usize capacity = 0;

    auto operator[](usize idx) -> T& {
        assert(idx < size);
        return data[idx];
    }

    auto operator[](usize idx) const -> const T& {
        assert(idx < size);
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

    auto slice(usize from = 0) -> Slice<T> {
        return slice(from, size);
    }

    auto slice(usize from, usize to) -> Slice<T> {
        assert(from <= size);
        assert(to <= size);
        assert(from <= to);
        return {
            .data = data + from,
            .size = to - from,
        };
    }
};

template <typename T>
auto Create_Array_Capacity(Allocator allocator, usize capacity) -> Array<T> {
    return Array<T>{
        .data     = allocator.Alloc<T>(capacity),
        .size     = 0,
        .capacity = capacity,
    };
}

template <typename T>
auto Append(Allocator allocator, Array<T>& array, const T& value) -> Array<T> {
    if (array.size < array.capacity) {
        array.data[array.size++] = value;
        return array;
    }

    // new array
    usize new_capacity   = max(usize(4), array.capacity * 2);
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
auto Append_Slice(Allocator allocator, Array<T>& array, Slice<T> slice) -> Array<T> {
    if (!slice.size) {
        return array;
    }

    if (array.size + slice.size < array.capacity) {
        memcpy(array.data + array.size, slice.data, sizeof(T) * slice.size);
        array.size += slice.size;
        return array;
    }

    // new array
    usize new_capacity = (array.capacity + slice.size) * 2;
    auto  new_data     = allocator.Alloc<T>(new_capacity);
    auto  new_size     = array.size + 1;
    memcpy(new_data, array.data, sizeof(T) * array.size);
    memcpy(new_data + array.size, slice.data, sizeof(T) * slice.size);

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
    array.data[idx] = value;
    array.size++;
}

template <typename T>
auto Insert_Slice(Allocator allocator, Array<T>& array, Slice<T> slice, usize idx) -> Array<T> {
    if (!slice.size) {
        return array;
    }

    if (array.size + slice.size < array.capacity) {
        auto insert_slice_begin = array.data + idx;
        auto insert_slice_end   = insert_slice_begin + slice.size;
        memmove(insert_slice_end, insert_slice_begin, sizeof(T) * (array.size - idx));
        memcpy(insert_slice_begin, slice.data, sizeof(T) * slice.size);
        array.size += slice.size;
        return array;
    }

    // new array
    usize new_capacity = (array.capacity + slice.size) * 2;
    auto  new_data     = allocator.Alloc<T>(new_capacity);
    auto  new_size     = array.size + slice.size;
    memcpy(new_data, array.data, sizeof(T) * idx);
    memcpy(new_data + idx, slice.data, sizeof(T) * slice.size);
    memcpy(new_data + idx + slice.size, array.data + idx, sizeof(T) * (array.size - idx));

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
auto Pop(Array<T>& array) -> void {
    assert(array.size > 0);
    array.size--;
}

template <typename T>
auto Clear(Array<T>& array) -> void {
    array.size = 0;
}

template <typename T>
auto Destroy_Array(Allocator allocator, Array<T>& array) -> void {
    allocator.Free(array.data);
    array = {
        .data     = null,
        .size     = 0,
        .capacity = 0,
    };
}

#ifdef Array_Each
#undef Array_Each
#endif
#define Array_Each(array, element, idx, body)      \
    for (usize idx = 0; idx < array.size; idx++) { \
        auto element = array[idx];                 \
        {                                          \
            body;                                  \
        }                                          \
    }

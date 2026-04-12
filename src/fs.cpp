#include "fs.hpp"

#include <cassert>
#include <cstdio>

auto Read_All_From_File_As_String(Allocator allocator, String path) -> String {
    auto handle = fopen(path, "r");
    if (!handle) return {0};
    fseek(handle, 0, SEEK_END);
    usize file_size_in_bytes = ftell(handle);
    fseek(handle, 0, SEEK_SET);

    auto destination = allocator.Alloc<u8>(file_size_in_bytes);
    {
        auto result = fread(destination, sizeof(u8), file_size_in_bytes, handle);
        assert(result == file_size_in_bytes);
    }
    fclose(handle);
    return {
        .data = destination,
        .size = file_size_in_bytes,
    };
}

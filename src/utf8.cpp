#include "utf8.hpp"

typedef struct {
    unsigned char mask;
    unsigned char lead;
    int           bits_stored;
} Utf8_Rune;

static Utf8_Rune utf_length_table[] = {
    {0x3f, 0x80, 6},
    {0x7f, 0, 7},
    {0x1f, 0xc0, 5},
    {0xf, 0xe0, 4},
    {0x7, 0xf0, 3},
};

auto Utf8_Length(u8 byte) -> u32 {
    u32 len = 0;

    auto utf_length_table_end = utf_length_table + size_of_array(utf_length_table);

    for (auto* u = utf_length_table; u < utf_length_table_end; ++u) {
        if ((byte & ~u->mask) == u->lead) {
            break;
        }
        ++len;
    }

    if (len > 4) { /* Malformed leading byte */
        return -1;
    }

    return len;
}

auto Utf8_Length(Slice<u8> slice) -> s64 {
    s64 sum = 0;

    for (s64 i = 0; i < slice.size; sum++) {
        auto c     = slice[i];
        auto c_len = Utf8_Length(c);
        if (c_len == -1) {
            return -1;
        }
        i += c_len;
    }

    return sum;
}

auto Utf8_Decode(const u8* sequence, const u8** end_ptr) -> u32 {
    int bytes = Utf8_Length(*sequence);
    int shift = utf_length_table[0].bits_stored * (bytes - 1);
    u32 codep = (*sequence++ & utf_length_table[bytes].mask) << shift;

    for (int i = 1; i < bytes; ++i, ++sequence) {
        shift -= utf_length_table[0].bits_stored;
        codep |= ((char)*sequence & utf_length_table[0].mask) << shift;
    }

    *end_ptr = sequence - 1;
    return (codep);
}

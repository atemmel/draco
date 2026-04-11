#pragma once

#include "types.hpp"

auto Utf8_Length(u8 byte) -> u32;

auto Utf8_Decode(const u8* sequence, const u8** end_ptr) -> u32;

#pragma once

#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "types.hpp"

struct Font;

struct Glyph_Metric {
    Rect rect;
    Point bearing;
    i32 advance;
};

struct Font_Metrics {
    i64 height;
    u32 ptsize;
    i32 max_glyph_width;
    i32 max_glyph_height;
    i32 max_glyph_advance;
};

extern Font* monospace_font;

auto Init_Fonts() -> void;

auto Destroy_Fonts() -> void;

auto Create_Font_From_Bytes(Allocator allocator, Renderer renderer, const u8* bytes, usize byte_count, u32 font_size) -> Font*;

auto Draw_Text(const Font* font, const char* text, u32 length, f32 x, f32 y) -> void;

auto Draw_Font_Atlas(const Font* font, Point pt) -> void;

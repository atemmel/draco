#pragma once

#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "strings.hpp"
#include "types.hpp"

struct Font;

struct Glyph_Metric {
    Rect rect;
    Vec2 bearing;
    i32  advance;
    i32  bbox_ymax;
};

struct Font_Metrics {
    i64 height;
    u32 ptsize;
    i32 max_glyph_width;
    i32 max_glyph_height;
    i32 max_glyph_advance;
};

extern Font* monospace_font;
extern Font* monospace2_font;
extern Font* monospace3_font;
extern Font* monospace4_font;

auto Init_Fonts(Allocator allocator) -> void;

auto Destroy_Fonts() -> void;

auto Create_Font_From_Bytes(Allocator allocator, Renderer renderer, const u8* bytes, usize byte_count, u32 font_size) -> Font*;

auto Destroy_Font(Font* font) -> void;

auto Render_Text(const Font* font, String text, f32 x, f32 y) -> void;

auto Render_Text(const Font* font, const u8* text, u32 length, f32 x, f32 y) -> void;

auto Render_Font_Atlas(const Font* font, Vec2 pt) -> void;

auto Calculate_Text_Dimensions_With_Font(const Font* font, Slice<u8> text) -> Vec2;
auto Calculate_Text_Dimensions_With_Font(const Font* font, String text) -> Vec2;

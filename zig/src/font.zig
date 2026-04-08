const std = @import("std");
const c = @import("c.zig").c;
const cast = @import("util.zig").cast;

var freetype_context: c.FT_Library = null;

pub const GlyphMetric = struct {
    rect: c.SDL_Rect,
    bearing: c.SDL_Point,
    advance: i32,
};

pub const FontMetrics = struct {
    height: i64,
    ptsize: u32,
    max_glyph_width: i32,
    max_glyph_height: i32,
    max_glyph_advance: i32,
};

pub const Font = struct {
    renderer: *c.SDL_Renderer,
    ft_face: c.FT_Face,
    count_glyphs_of_atlas_side: i32,
    font_metrics: FontMetrics,
    glyph_metrics: []GlyphMetric,
    atlas: *c.SDL_Texture,
    use_kerning: bool,
    allocator: std.mem.Allocator,
};

pub fn initFonts() !void {
    if (c.FT_Init_FreeType(&freetype_context) != 0) {
        return error.freetype_init;
    }
}

pub fn newFont(allocator: std.mem.Allocator, renderer: *c.SDL_Renderer, font_bytes: []const u8, font_size: u32) !Font {
    var new_font: Font = .{
        .renderer = renderer,
        .ft_face = undefined,
        .count_glyphs_of_atlas_side = 0,
        .font_metrics = undefined,
        .glyph_metrics = undefined,
        .atlas = undefined,
        .use_kerning = undefined,
        .allocator = allocator,
    };

    if (c.FT_New_Memory_Face(freetype_context, font_bytes.ptr, @intCast(font_bytes.len), 0, &new_font.ft_face) != 0) {
        return error.freetype_new;
    }
    errdefer _ = c.FT_Done_Face(new_font.ft_face);

    if (c.FT_Set_Pixel_Sizes(new_font.ft_face, font_size, font_size) != 0) {
        return error.freetype_pixel_size;
    }

    new_font.count_glyphs_of_atlas_side = @intFromFloat(@ceil(@sqrt(@as(f32, @floatFromInt(new_font.ft_face.*.num_glyphs)))));
    new_font.font_metrics.ptsize = font_size;
    new_font.font_metrics.height = new_font.ft_face.*.size.*.metrics.height >> 6;
    new_font.use_kerning = c.FT_HAS_KERNING(new_font.ft_face);

    const surface = try renderFontToSurface(&new_font);
    defer c.SDL_DestroySurface(surface);

    new_font.atlas = c.SDL_CreateTextureFromSurface(new_font.renderer, surface) orelse {
        return error.sdl_create_texture;
    };
    _ = c.SDL_SetTextureBlendMode(new_font.atlas, c.SDL_BLENDMODE_BLEND);
    return new_font;
}

pub fn renderFontToSurface(font: *Font) !*c.SDL_Surface {
    const side_of_square = font.count_glyphs_of_atlas_side * @as(i32, @intCast(font.font_metrics.ptsize));
    const surface = c.SDL_CreateSurface(side_of_square, side_of_square, c.SDL_PIXELFORMAT_RGBA32) orelse {
        return error.sdl_create_surface;
    };

    errdefer c.SDL_DestroySurface(surface);

    font.glyph_metrics = try font.allocator.alloc(GlyphMetric, @intCast(font.ft_face.*.num_glyphs));

    var index: c.FT_UInt = undefined;
    var xpos: i32 = 0;
    var ypos: i32 = 0;

    var charcode = c.FT_Get_First_Char(font.ft_face, &index);
    while (index != 0) : (charcode = c.FT_Get_Next_Char(font.ft_face, charcode, &index)) {
        if (xpos < (font.count_glyphs_of_atlas_side - 1)) {
            xpos += 1;
        } else {
            xpos = 1;
            ypos += 1;
        }

        _ = c.FT_Load_Char(font.ft_face, charcode, c.FT_LOAD_RENDER);
        const bitmap = font.ft_face.*.glyph.*.bitmap;
        if (bitmap.pixel_mode != c.ft_pixel_mode_grays) {
            break;
        }

        setGlyphMetrics(font, index, xpos, ypos);

        const xreal = xpos * cast(i32, font.font_metrics.ptsize);
        const yreal = ypos * cast(i32, font.font_metrics.ptsize);

        var y: i32 = 0;
        while (y < bitmap.rows) : (y += 1) {
            var x: i32 = 0;
            while (x < bitmap.width) : (x += 1) {
                const surface_index = cast(usize, (yreal + y) * surface.*.w + xreal + x);
                const pixel = &@as([*]c_uint, @ptrCast(@alignCast(surface.*.pixels)))[surface_index];
                const alpha = bitmap.buffer[cast(usize, y * bitmap.pitch + x)];
                std.debug.print("surface.*.format: {any}\n", .{surface.*.pixels});
                pixel.* = c.SDL_MapRGBA(c.SDL_GetPixelFormatDetails(surface.*.format), null, 255, 255, 255, alpha);
            }
        }
    }

    return surface;
}

fn setGlyphMetrics(font: *Font, index: u32, xpos: i32, ypos: i32) void {
    font.glyph_metrics[index].rect.x = xpos * cast(i32, font.font_metrics.ptsize);
    font.glyph_metrics[index].rect.y = ypos * cast(i32, font.font_metrics.ptsize);
    font.glyph_metrics[index].rect.w = cast(c_int, font.ft_face.*.glyph.*.metrics.width >> 6);
    font.glyph_metrics[index].rect.h = cast(c_int, font.ft_face.*.glyph.*.metrics.height >> 6);
    font.glyph_metrics[index].bearing.x = cast(c_int, font.ft_face.*.glyph.*.metrics.horiBearingX >> 6);
    font.glyph_metrics[index].bearing.y = cast(c_int, font.ft_face.*.glyph.*.metrics.horiBearingY >> 6);
    font.glyph_metrics[index].advance = cast(c_int, font.ft_face.*.glyph.*.metrics.horiAdvance >> 6);
    if (font.font_metrics.max_glyph_width < font.glyph_metrics[index].rect.w) {
        font.font_metrics.max_glyph_width = font.glyph_metrics[index].rect.w;
    }
    if (font.font_metrics.max_glyph_height < font.glyph_metrics[index].rect.h) {
        font.font_metrics.max_glyph_height = font.glyph_metrics[index].rect.h;
    }
    if (font.font_metrics.max_glyph_advance < font.glyph_metrics[index].advance) {
        font.font_metrics.max_glyph_advance = font.glyph_metrics[index].advance;
    }
}

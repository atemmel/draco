#include "fonts.hpp"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>

#include <cassert>

#include "defer.hpp"
#include "freetype.h"
#include "freetype/freetype.h"
#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "types.hpp"

static auto Render_Font_To_Surface(Font* font, Allocator allocator) -> SDL_Surface*;

struct Font {
    Renderer renderer;
    FT_Face ft_face;
    i32 glyphs_of_atlas_side_count;
    Font_Metrics font_metrics;
    Glyph_Metric* glyph_metrics;
    usize glyph_metrics_count;
    Texture atlas;
    bool use_kerning;
    Allocator allocator;
};

const u8 monospace_font_bytes[] = {
#embed "../embed/JetBrainsMonoNerdFont-Regular.ttf"
};

const u8 regular_bold_italic_font_bytes[] = {
#embed "../embed/TinosNerdFont-BoldItalic.ttf"
};

static FT_Library ft_library;

Font* monospace_font;

auto Init_Fonts() -> void {
    TTF_Init();
    assert(!FT_Init_FreeType(&ft_library));

    monospace_font = Create_Font_From_Bytes(default_allocator, renderer, monospace_font_bytes, sizeof(monospace_font_bytes), 20);
    assert(monospace_font);
}

auto Destroy_Fonts() -> void {
    // TODO: clean up monospace_font
    TTF_Quit();
}

auto Create_Font_From_Bytes(Allocator allocator, Renderer renderer, const u8* bytes, usize byte_count, u32 font_size) -> Font* {
    auto font = allocator.Create<Font>();
    assert(font);

    font->allocator = allocator;

    auto sdl_font = TTF_OpenFontIO(SDL_IOFromConstMem(bytes, byte_count), false, font_size);
    assert(sdl_font);
    defer(TTF_CloseFont(sdl_font));

    assert(!FT_New_Memory_Face(ft_library, bytes, byte_count, 0, &font->ft_face));

    assert(!FT_Set_Pixel_Sizes(font->ft_face, font_size, font_size));

    font->glyphs_of_atlas_side_count = ceil(sqrt(font->ft_face->num_glyphs));
    font->font_metrics.ptsize        = font_size;
    font->font_metrics.height        = font->ft_face->size->metrics.height >> 6;
    font->use_kerning                = FT_HAS_KERNING(font->ft_face);

    auto surface = Render_Font_To_Surface(font, allocator);
    defer(SDL_DestroySurface(surface));

    font->atlas = SDL_CreateTextureFromSurface(renderer, surface);
    assert(font->atlas);
    SDL_SetTextureBlendMode(font->atlas, SDL_BLENDMODE_BLEND);
    return font;
};

/*
auto Draw_Text(const Font* font, const char* text, u32 length, f32 x, f32 y) -> void {
    auto surface = TTF_RenderText_Blended(font, text, length, SDL_Color{255, 255, 255, 255});
    defer(SDL_DestroySurface(surface));
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    defer(SDL_DestroyTexture(texture));

    auto dst = SDL_FRect{
        .x = x,
        .y = y,
        .w = (f32)(texture->w),
        .h = (f32)(texture->h),
    };

    SDL_RenderTexture(renderer, texture, nullptr, &dst);
}
*/

auto Set_Glyph_Metrics_Of_Font(Font* font, usize index, i32 x, i32 y) -> void {
    font->glyph_metrics[index].rect.x    = x * font->font_metrics.ptsize;
    font->glyph_metrics[index].rect.y    = y * font->font_metrics.ptsize;
    font->glyph_metrics[index].rect.w    = font->ft_face->glyph->metrics.width >> 6;
    font->glyph_metrics[index].rect.h    = font->ft_face->glyph->metrics.height >> 6;
    font->glyph_metrics[index].bearing.x = font->ft_face->glyph->metrics.horiBearingX >> 6;
    font->glyph_metrics[index].bearing.y = font->ft_face->glyph->metrics.horiBearingY >> 6;
    font->glyph_metrics[index].advance   = font->ft_face->glyph->metrics.horiAdvance >> 6;
    if (font->font_metrics.max_glyph_width < font->glyph_metrics[index].rect.w) {
        font->font_metrics.max_glyph_width = font->glyph_metrics[index].rect.w;
    }
    if (font->font_metrics.max_glyph_height < font->glyph_metrics[index].rect.h) {
        font->font_metrics.max_glyph_height = font->glyph_metrics[index].rect.h;
    }
    if (font->font_metrics.max_glyph_advance < font->glyph_metrics[index].advance) {
        font->font_metrics.max_glyph_advance = font->glyph_metrics[index].advance;
    }
}

static auto Render_Font_To_Surface(Font* font, Allocator allocator) -> SDL_Surface* {
    auto size_of_atlas_side = font->glyphs_of_atlas_side_count * font->font_metrics.ptsize;
    auto surface            = SDL_CreateSurface(size_of_atlas_side, size_of_atlas_side, SDL_PIXELFORMAT_RGBA32);
    assert(surface);

    font->glyph_metrics_count = font->ft_face->num_glyphs;
    font->glyph_metrics       = allocator.Alloc<Glyph_Metric>(font->glyph_metrics_count);
    assert(font->glyph_metrics);

    FT_UInt index;
    i32 xpos = 0, ypos = 0;

    for (auto charcode = FT_Get_First_Char(font->ft_face, &index);
         index != 0;
         charcode = FT_Get_Next_Char(font->ft_face, charcode, &index)) {
        if (xpos < (font->glyphs_of_atlas_side_count - 1)) {
            xpos++;
        } else {
            xpos = 0;
            ypos++;
        }

        FT_Load_Char(font->ft_face, charcode, FT_LOAD_RENDER);
        FT_Bitmap* bitmap = &font->ft_face->glyph->bitmap;
        if (bitmap->pixel_mode != ft_pixel_mode_grays) {
            break;
        }

        Set_Glyph_Metrics_Of_Font(font, index, xpos, ypos);

        i32 xreal = xpos * font->font_metrics.ptsize;
        i32 yreal = ypos * font->font_metrics.ptsize;
        for (i32 y = 0; y < bitmap->rows; y++) {
            for (i32 x = 0; x < bitmap->width; x++) {
                i32 index  = (yreal + y) * surface->w + xreal + x;
                u32* pixel = &((u32*)surface->pixels)[index];
                u8 alpha   = bitmap->buffer[y * bitmap->pitch + x];
                *pixel     = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), null, 255, 255, 255, alpha);
            }
        }
    }

    return surface;
}

auto Draw_Font_Atlas(const Font* font, Point pt) -> void {
    auto dest_rect = SDL_FRect{
        f32(pt.x),
        f32(pt.y),
        f32(font->glyphs_of_atlas_side_count * font->font_metrics.ptsize),
        f32(font->glyphs_of_atlas_side_count * font->font_metrics.ptsize),
    };
    SDL_RenderTexture(font->renderer, font->atlas, null, &dest_rect);
}

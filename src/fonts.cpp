#include "fonts.hpp"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <math.h>

#include <climits>
#include <cmath>

#include "freetype.h"
#include "freetype/freetype.h"
#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "runtime.hpp"
#include "types.hpp"
#include "utf8.hpp"

const static s32 n_glyphs_of_ascii_atlas_side = s32(ceilf(sqrtf(128.f)));

static auto Render_Font_To_Ascii_Surface(Font* font) -> SDL_Surface*;
static auto Init_Font_With_New_Size(Font* font, u32 new_size) -> void;

struct Font {
    Renderer      renderer;
    FT_Face       ft_face;
    Font_Metrics  font_metrics;
    Glyph_Metric* glyph_metrics;
    s64           glyph_metrics_count;
    Texture       ascii_atlas;
    Texture       lru_atlas;
    Surface       ascii_surface;
    Surface       lru_surface;
    s32           glyphs_of_lru_atlas_side_count;
    u32           base_size;
    bool          use_kerning;
    Allocator     allocator;
};

const u8 monospace_font_bytes[] = {
#embed "../embed/IBMPlexMono-Regular.ttf"
};

const u8 monospace2_font_bytes[] = {
#embed "../embed/IosevkaNerdFontMono-Regular.ttf"
};

const u8 monospace3_font_bytes[] = {
#embed "../embed/GeistMono-Regular.ttf"
};

const u8 regular_bold_italic_font_bytes[] = {
#embed "../embed/TinosNerdFont-BoldItalic.ttf"
};

static FT_Library ft_library;

Font* monospace_font;
Font* regular_font;
Font* monospace2_font;
Font* monospace3_font;
Font* monospace4_font;

auto Init_Fonts(Allocator allocator) -> void {
    Assert(!FT_Init_FreeType(&ft_library));

    monospace_font = Create_Font_From_Bytes(allocator, renderer, monospace_font_bytes, sizeof(monospace_font_bytes), 24);
    regular_font   = Create_Font_From_Bytes(allocator, renderer, regular_bold_italic_font_bytes, sizeof(regular_bold_italic_font_bytes), 24);
    // monospace2_font = Create_Font_From_Bytes(allocator, renderer, monospace2_font_bytes, sizeof(monospace2_font_bytes), 24);
    // monospace3_font = Create_Font_From_Bytes(allocator, renderer, monospace3_font_bytes, sizeof(monospace3_font_bytes), 24);
    // monospace4_font = Create_Font_From_Bytes(allocator, renderer, regular_bold_italic_font_bytes, sizeof(regular_bold_italic_font_bytes), 24);
    Assert(monospace_font);
    Assert(regular_font);
    // Assert(monospace2_font);
    // Assert(monospace3_font);
    // Assert(monospace4_font);
}

auto Destroy_Fonts() -> void {
    Destroy_Font(monospace_font);
    Destroy_Font(regular_font);
    // Destroy_Font(monospace2_font);
    // Destroy_Font(monospace3_font);
    // Destroy_Font(monospace4_font);
}

auto Create_Font_From_Bytes(Allocator allocator, Renderer renderer, const u8* bytes, s64 byte_count, u32 font_size) -> Font* {
    auto font = allocator.Create<Font>();
    Assert(font);
    font->renderer  = renderer;
    font->allocator = allocator;
    Assert(!FT_New_Memory_Face(ft_library, bytes, byte_count, 0, &font->ft_face));
    font->base_size = font_size;

    font->glyph_metrics = nullptr;
    Init_Font_With_New_Size(font, font_size);

    return font;
};

auto Destroy_Font(Font* font) -> void {
    SDL_DestroyTexture(font->ascii_atlas);
    SDL_DestroySurface(font->ascii_surface);
    SDL_DestroyTexture(font->lru_atlas);
    SDL_DestroySurface(font->lru_surface);
    FT_Done_Face(font->ft_face);
    font->allocator.Free(font->glyph_metrics);
    font->allocator.Free(font);
};

auto Font_Size(const Font* font) -> f32 {
    f32 offset = (f32(font->ft_face->descender >> 6) + f32(font->ft_face->ascender >> 6)) * Renderer_Zoom_Current();
    return f32(font->font_metrics.ptsize) + offset;
}

auto Font_Resize(Font* font, u32 ptsize) -> void {
    if (font->font_metrics.ptsize == ptsize) return;
    SDL_DestroySurface(font->ascii_surface);
    SDL_DestroyTexture(font->ascii_atlas);
    Init_Font_With_New_Size(font, ptsize);
}

auto Font_Scale(Font* font, f32 scale) -> void {
    u32 new_size = roundf(f32(font->base_size) * scale);
    Font_Resize(font, new_size);
}

auto Query_Glyph_Metrics(const Font* font, Uint32 ch) -> const Glyph_Metric* {
    const Glyph_Metric* metrics     = null;
    FT_UInt             glyph_index = FT_Get_Char_Index(font->ft_face, ch);
    if (glyph_index != 0) {
        metrics = &font->glyph_metrics[ch];
    }

    return metrics;
}

auto Get_Kerning_Offset(const Font* font, Uint32 ch, Uint32 previous_ch) -> s32 {
    int offset = 0;

    if (font->use_kerning) {
        FT_UInt glyph_index          = FT_Get_Char_Index(font->ft_face, ch);
        FT_UInt previous_glyph_index = FT_Get_Char_Index(font->ft_face, previous_ch);
        if (glyph_index && previous_glyph_index) {
            FT_Vector delta;
            FT_Get_Kerning(font->ft_face, previous_glyph_index, glyph_index,
                           FT_KERNING_DEFAULT, &delta);
            offset = delta.x >> 6;
        }
    }

    return offset;
}

static auto Render_Char(const Font* font, u32 ch, u32 previous_ch, Vec2 position) -> u32 {
    int  advance = 0;
    auto metrics = Query_Glyph_Metrics(font, ch);
    if (metrics) {
        SDL_Color color;
        SDL_FRect dstrect, srcrect = metrics->rect;
        f32       offset = f32(font->ft_face->descender >> 6) + f32(font->ft_face->ascender >> 6);

        dstrect.x = position.x;
        dstrect.y = position.y + ((f32(-metrics->bearing.y) + f32(font->font_metrics.height) - offset));
        dstrect.w = metrics->rect.w;
        dstrect.h = metrics->rect.h;

        if (previous_ch) {
            advance += Get_Kerning_Offset(font, ch, previous_ch);
        }
        advance += metrics->advance;

        f32 off_x = f32(advance - metrics->rect.w) * 0.5f;
        dstrect.x += off_x;

        if (ch < 128) {
            SDL_GetRenderDrawColor(font->renderer, &color.r, &color.g, &color.b, &color.a);
            SDL_SetTextureColorMod(font->ascii_atlas, color.r, color.g, color.b);
            SDL_RenderTextureRotated(font->renderer, font->ascii_atlas, &srcrect, &dstrect, 0.0, null, SDL_FLIP_NONE);
        } else {
            fprintf(stderr, "I don't know how to render a %d :/ \n", ch);
        }
    }

    return advance;
}

auto Render_Text(const Font* font, String text, f32 x, f32 y) -> void {
    Render_Text(font, text.data, text.size, x, y);
}

auto Render_Text(const Font* font, const u8* text, u32 text_number_of_bytes, f32 x, f32 y) -> void {
    Vec2 cursor      = {x, y};
    u32  previous_ch = 0;
    auto end_of_text = text + text_number_of_bytes;

    for (; text < end_of_text; text++) {
        u32 ch = Utf8_Decode(text, &text);
        if (ch == '\n') {
            cursor.x = x;
            cursor.y += font->font_metrics.height;
            previous_ch = 0;
            continue;
        } else {
            cursor.x += Render_Char(font, ch, previous_ch, cursor);
            previous_ch = ch;
        }
    }
}

auto Set_Glyph_Metrics_Of_Font(Font* font, s64 index, s32 x, s32 y) -> void {
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

static auto Render_Font_To_Ascii_Surface(Font* font) -> SDL_Surface* {
    auto size_of_atlas_side = n_glyphs_of_ascii_atlas_side * font->font_metrics.ptsize;
    auto surface            = SDL_CreateSurface(size_of_atlas_side, size_of_atlas_side, SDL_PIXELFORMAT_RGBA32);
    Assert(surface);

    font->glyph_metrics_count = font->ft_face->num_glyphs;
    font->glyph_metrics       = font->allocator.Alloc<Glyph_Metric>(128);
    Assert(font->glyph_metrics);

    s32 xpos = 0, ypos = 0;

    for (FT_ULong charcode = 0; charcode < 128; ++charcode) {
        if (xpos < (n_glyphs_of_ascii_atlas_side - 1)) {
            xpos++;
        } else {
            xpos = 0;
            ypos++;
        }

        auto glyph_index = FT_Get_Char_Index(font->ft_face, charcode);
        if (glyph_index == 0) {  // charcode not present in font
            continue;
        }

        FT_Load_Glyph(font->ft_face, glyph_index, FT_LOAD_RENDER);
        FT_Bitmap* bitmap = &font->ft_face->glyph->bitmap;
        if (bitmap->pixel_mode != ft_pixel_mode_grays) {
            break;
        }

        Set_Glyph_Metrics_Of_Font(font, charcode, xpos, ypos);

        s32 xreal = xpos * font->font_metrics.ptsize;
        s32 yreal = ypos * font->font_metrics.ptsize;
        for (s32 y = 0; y < bitmap->rows; y++) {
            for (s32 x = 0; x < bitmap->width; x++) {
                s32  index = (yreal + y) * surface->w + xreal + x;
                u32* pixel = &((u32*)surface->pixels)[index];
                u8   alpha = bitmap->buffer[y * bitmap->pitch + x];
                *pixel     = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), null, 255, 255, 255, alpha);
            }
        }
    }

    /*
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
                i32  index = (yreal + y) * surface->w + xreal + x;
                u32* pixel = &((u32*)surface->pixels)[index];
                u8   alpha = bitmap->buffer[y * bitmap->pitch + x];
                *pixel     = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), null, 255, 255, 255, alpha);
            }
        }
    }
    */

    return surface;
}

static auto Init_Font_With_New_Size(Font* font, u32 new_size) -> void {
    Assert(!FT_Set_Pixel_Sizes(font->ft_face, new_size, new_size));
    font->allocator.Free(font->glyph_metrics);

    font->glyphs_of_lru_atlas_side_count = ceil(sqrt(font->ft_face->num_glyphs));
    font->font_metrics.ptsize            = new_size;
    font->font_metrics.height            = font->ft_face->size->metrics.height >> 6;
    font->font_metrics.max_glyph_height  = INT_MIN;
    font->font_metrics.max_glyph_advance = INT_MIN;
    font->font_metrics.max_glyph_width   = INT_MIN;
    font->use_kerning                    = FT_HAS_KERNING(font->ft_face);

    auto surface        = Render_Font_To_Ascii_Surface(font);
    font->ascii_surface = surface;
    font->ascii_atlas   = SDL_CreateTextureFromSurface(renderer, surface);
    Assert(font->ascii_atlas);
    SDL_SetTextureBlendMode(font->ascii_atlas, SDL_BLENDMODE_BLEND);

    // TODO: init lru atlas
}

auto Calculate_Text_Dimensions_With_Font(const Font* font, Slice<u8> text_as_string) -> Vec2 {
    u32       previous_ch = 0;
    const u8* text        = text_as_string.data;
    u8*       end_of_text = text_as_string.data + text_as_string.size;
    Vec2      area        = {0.f, f32(font->font_metrics.height + f32(font->ft_face->descender >> 6))};

    for (; text < end_of_text; text++) {
        u32  ch      = Utf8_Decode(text, &text);
        auto metrics = Query_Glyph_Metrics(font, ch);
        if (metrics) {
            area.x += metrics->advance;
            if (previous_ch) {
                area.x += Get_Kerning_Offset(font, ch, previous_ch);
            }
        }
        previous_ch = ch;
    }

    return area;
}

auto Calculate_Text_Dimensions_With_Font(const Font* font, String text_as_string) -> Vec2 {
    return Calculate_Text_Dimensions_With_Font(font, text_as_string.Slice());
}

auto Dump_Font_Information_For_Debugging(const Font* font) -> void {
    if (font->ascii_surface) {
        SDL_SavePNG(font->ascii_surface, "ascii-atlas.png");
        printf("Dumped Ascii atlas of size: %d x %d\n", font->ascii_surface->w, font->ascii_surface->h);
    }
    if (font->lru_surface && font->lru_surface) {
        SDL_SavePNG(font->lru_surface, "lru-atlas.png");
        printf("Dumped LRU atlas of size: %d x %d\n", font->lru_surface->w, font->lru_surface->h);
    }
}

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <cassert>
#include <cstdio>

#include "defer.hpp"
#include "editor.hpp"
#include "fonts.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "strings.hpp"

const auto BG = Vec4{0.137, 0.137, 0.176, 1.0};

const auto FG = Vec4{1.0, 1.0, 1.0, 1.0};

const auto FG_2 = Vec4{0.5, 0.5, 0.5, 1.0};

auto loop() -> void;
auto draw(f32 dt) -> void;
auto Font_Correctness_Test(Font* font, String string, f32 f) -> void;

Editor editor;

auto main() -> int {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init() Error: %s", SDL_GetError());
        return -1;
    }
    defer(SDL_Quit());

    auto tracing_allocator = Create_Tracing_Allocator(default_allocator);
    auto allocator         = tracing_allocator.Interface();
    defer({
        printf("Allocations/frees: %llu/%llu\n",
               (unsigned long long)tracing_allocator.n_allocs,
               (unsigned long long)tracing_allocator.n_frees);
        assert(tracing_allocator.n_allocs == tracing_allocator.n_frees);
    });

    Init_Renderer();
    defer(Destroy_Renderer());
    Init_Fonts(allocator);
    defer(Destroy_Fonts());

    editor = Create_Editor(allocator, monospace_font);
    Destroy_Editor(&editor);

    Editor_Open_Source(&editor, "Hello, World"_s);

    loop();
    return 0;
}

bool animating   = false;
i64  last_tick   = 0;
Vec2 was_pos     = {-1.f, -1.f};
f32  was_scroll  = 0.f;
f32  zoom_scalar = 0.f;
bool ctrl_down   = false;

auto loop() -> void {
    auto      running = true;
    SDL_Event event;
    do {
        auto current_tick = SDL_GetTicksNS();
        defer(last_tick = current_tick;);
        const auto dt = f32(SDL_NS_TO_SECONDS(current_tick) - SDL_NS_TO_SECONDS(last_tick));

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    switch (event.key.key) {
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                    }
                    break;
            }
        }

        if (!running) break;

        /*
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        auto str = "gaming->man + 50 * My_Func() {} [0]"_s;

        Font_Correctness_Test(monospace_font, str, 0.f);
        Font_Correctness_Test(monospace2_font, str, 1.f);
        Font_Correctness_Test(monospace3_font, str, 3.f);
        Font_Correctness_Test(monospace4_font, str, 2.f);

        SDL_RenderPresent(renderer);
        */

        draw(dt);

        Sleep_Until_Next_Frame();
    } while (true);
};

auto draw(f32 dt) -> void {
    static u8 buffer[1024];
    Font*     font             = editor.font;
    f32       offset_x         = 100.0;
    f32       line_no_offset_x = offset_x - 20.0;
    f32       offset_y         = 200.0;
    f32       line_height      = Font_Size(font) + 4.0;
    auto      window_size      = Window_Size();

    editor.lines_on_screen = ((window_size.y + offset_y) / line_height) - 3;

    auto cursor_data = Editor_Cursor_Draw_Data(&editor);
    auto dim         = Calculate_Text_Dimensions_With_Font(font, cursor_data.text_left_of_cursor);
    Vec2 is_pos      = {
        .x = offset_x + dim.x,
        .y = offset_y + line_height * (cursor_data.virtual_row - f32(editor.scroll_offset)),
    };
    auto is_scroll = f32(editor.scroll_offset) * line_height;

    if (is_pos.y < offset_y) {
        editor.scroll_offset -= 1;
    }

    if (was_pos.x == -1.0 and was_pos.y == -1.0) {
        was_pos = is_pos;
    }

    animating = false                              //
                || !was_pos.Eql(is_pos, 0.1)       // position changed
                || max(was_scroll, 0.1f) != 0.1f;  // scroll changed

    const auto dampning = 0.001f;
    const auto dt_mult  = 2.f;

    was_pos = {Damp(is_pos.x, was_pos.x, dampning, dt * dt_mult), Damp(is_pos.y, was_pos.y, dampning, dt * dt_mult)};

    was_scroll = Damp(is_scroll, was_scroll, dampning, dt * dt_mult);

    auto rect = Rect{
        .x = was_pos.x,
        .y = was_pos.y,
        .w = 2.f * zoom_scalar,
        .h = 24.f * zoom_scalar,
    };

    Renderer_Clear(BG);
    Renderer_Set_Color(FG);
    Render_Text(regular_font, "Title q8^)"_s, 100.f, 100.f);
    /*
    rend.drawText(rend.header_font, "Title  q8^)", FG, 100.0, 100.0);
    var n_virtual_line : i64 = 0;
    for (pane.editor.allRealLines(), 0..) | _, idx | {
            {
                const y           = offset_y + @as(f32, @floatFromInt(n_virtual_line)) * line_height - was_scroll;
                const line_no_str = std.fmt.bufPrint(&static.buffer, "{}", .{idx + 1}) catch "X";
                const line_no_dim = rend.strdim(rend.body_font, line_no_str);
                if (@ceil(y) >= offset_y) {
                    rend.drawText(rend.body_font, line_no_str, FG_2, line_no_offset_x - line_no_dim.w, y);
                }
            }

            const virtual_lines = pane.editor.virtualLines(idx);

            for (virtual_lines) | virtual_line | {
                    const slice = pane.editor.buffer.items[virtual_line.begin..virtual_line.end];
                    const y     = offset_y + @as(f32, @floatFromInt(n_virtual_line)) * line_height - was_scroll;
                    if (@ceil(y) >= offset_y) {
                        rend.drawText(rend.body_font, slice, FG, offset_x, y);
                    }
                    n_virtual_line += 1;
                }
        }

    _ = c.SDL_SetRenderDrawColorFloat(rend.renderer, FG.x, FG.y, FG.z, FG.w);
    _ = c.SDL_RenderFillRect(rend.renderer, &rect);
    rend.drawWindowDecoration(gui);
    */
    Renderer_Present();
}

auto Font_Correctness_Test(Font* font, String string, f32 f) -> void {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    Render_Text(font, string, 0, 64.f * f);
    auto dim  = Calculate_Text_Dimensions_With_Font(font, string);
    auto rect = SDL_FRect{0.f, 64.f * f, dim.x, dim.y};
    SDL_RenderRect(renderer, &rect);
}

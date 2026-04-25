#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <cassert>
#include <cmath>
#include <cstdio>

#include "array.hpp"
#include "defer.hpp"
#include "editor.hpp"
#include "fonts.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "strings.hpp"

const auto BG   = Vec4{0.137, 0.137, 0.176, 1.0};
const auto FG   = Vec4{1.0, 1.0, 1.0, 1.0};
const auto FG_2 = Vec4{0.5, 0.5, 0.5, 1.0};

auto loop() -> void;
auto draw(f32 dt) -> void;
auto Font_Correctness_Test(Font* font, String string, f32 f) -> void;

Editor editor;

auto main(int argc, char* argv[]) -> int {
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
    defer(Destroy_Editor(&editor));

    printf("argv: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    if (argc > 1) {
        Editor_Open_File(&editor, As_String(argv[1]));
    } else {
        Editor_Open_Source(&editor, ""_s);
    }

    SDL_StartTextInput(window);

    loop();
    return 0;
}

bool animating  = false;
i64  last_tick  = 0;
Vec2 was_pos    = {-1.f, -1.f};
f32  was_scroll = 0.f;
bool ctrl_down  = false;

auto loop() -> void {
    auto      running = true;
    SDL_Event event;
    do {
        auto current_tick = SDL_GetTicksNS();
        defer(last_tick = current_tick;);
        const auto dt = SDL_NS_TO_SECONDS(f32(current_tick)) - SDL_NS_TO_SECONDS(f32(last_tick));

        auto did_input = false;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    did_input = true;
                    ctrl_down = event.key.mod & SDL_KMOD_CTRL;
                    switch (event.key.key) {
                        case SDLK_ESCAPE:
                            running = false;
                            break;
                        case SDLK_RETURN:
                            Editor_Insert_Newline(&editor);
                            break;
                        case SDLK_BACKSPACE:
                            Editor_Remove_Left_Of_Cursor(&editor);
                            break;
                        case SDLK_LEFT:
                            Editor_Left(&editor);
                            break;
                        case SDLK_RIGHT:
                            Editor_Right(&editor);
                            break;
                        case SDLK_UP:
                            Editor_Up(&editor);
                            break;
                        case SDLK_DOWN:
                            Editor_Down(&editor);
                            break;
                        case SDLK_PLUS:
                            if (ctrl_down) {
                                Renderer_Zoom_Delta(0.1f);
                            }
                            break;
                        case SDLK_MINUS:
                            if (ctrl_down) {
                                Renderer_Zoom_Delta(-0.1f);
                            }
                            break;
                    }
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    if (ctrl_down) continue;
                    did_input = true;
                    Editor_Insert_Text(&editor, As_String(event.text.text));
                    break;
            }
        }

        if (!running) break;

        defer(Sleep_Until_Next_Frame());

        if (!animating && !did_input && last_tick > 0 /* !input.state.window_changed_somehow*/) {
            continue;
        }

        draw(dt);
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
        .w = 2.f,
        .h = f32(Font_Size(font)) * Renderer_Zoom_Current(),
    };

    Renderer_Clear(BG);
    Renderer_Set_Color(FG);
    Render_Text(regular_font, "Title q8^)"_s, 100.f, 100.f, Renderer_Zoom_Current());

    i64 n_virtual_line = 0;
    for (usize idx = 0; idx < editor.lines.size; idx++) {
        {
            f32  y           = offset_y + f32(n_virtual_line) * line_height - was_scroll;
            auto line_no_str = Sprintf(buffer, sizeof(buffer), "%lu", idx + 1);
            auto line_no_dim = Calculate_Text_Dimensions_With_Font(font, line_no_str);
            if (ceilf(y) >= offset_y) {
                Renderer_Set_Color(FG);
                Render_Text(font, line_no_str, line_no_offset_x - line_no_dim.x, y, Renderer_Zoom_Current());
            }
        }

        Renderer_Set_Color(FG);
        auto virtual_lines = Editor_Virtual_Lines(&editor, idx);
        for (auto virtual_line : virtual_lines) {
            auto slice = editor.buffer.slice(virtual_line.begin, virtual_line.end);
            f32  y     = offset_y + f32(n_virtual_line) * line_height - was_scroll;
            if (ceilf(y) >= offset_y) {
                Render_Text(font, slice.data, slice.size, offset_x, y, Renderer_Zoom_Current());
            }
            n_virtual_line += 1;
        }
    }

    Renderer_Set_Color(FG);
    Renderer_Draw_Rect(rect);

    Rect editor_outline = {offset_x, offset_y, 500.f, 500.f};
    Renderer_Draw_Outline(editor_outline);
    /*
    rend.drawWindowDecoration(gui);
    */
    Renderer_Present();
}

auto Font_Correctness_Test(Font* font, String string, f32 f) -> void {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    Render_Text(font, string, 0, 64.f * f, Renderer_Zoom_Current());
    auto dim  = Calculate_Text_Dimensions_With_Font(font, string);
    auto rect = SDL_FRect{0.f, 64.f * f, dim.x, dim.y};
    SDL_RenderRect(renderer, &rect);
}

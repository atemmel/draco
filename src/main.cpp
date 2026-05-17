#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <cmath>
#include <cstdio>

#include "array.hpp"
#include "defer.hpp"
#include "editor.hpp"
#include "fonts.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "runtime.hpp"
#include "strings.hpp"

const auto BG   = Vec4{0.137, 0.137, 0.176, 1.0};
const auto BG_2 = Vec4{0.157, 0.157, 0.196, 1.0};
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
        Assert(tracing_allocator.n_allocs == tracing_allocator.n_frees);
    });

    Init_Renderer();
    defer(Destroy_Renderer());
    Init_Fonts(allocator);
    defer(Destroy_Fonts());

    editor = Create_Editor(allocator, monospace_font, {600, 800});
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
s64  last_tick  = 0;
Vec2 was_pos    = {-1.f, -1.f};
f32  was_scroll = 0.f;
bool ctrl_down  = false;
Vec2 camera     = {};
bool dragging   = false;

auto loop() -> void {
    auto      running = true;
    SDL_Event event;
    do {
        auto current_tick = SDL_GetTicksNS();
        defer(last_tick = current_tick;);
        const auto dt = SDL_NS_TO_SECONDS(f32(current_tick - last_tick));

        auto did_input = false;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    dragging = true;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    dragging = false;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (dragging) {
                        Vec2 delta = {event.motion.xrel, event.motion.yrel};
                        camera     = camera + delta;
                        did_input  = true;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    if (ctrl_down) {
                        auto zoom = event.wheel.y < 0.f ? -0.1f : 0.1f;
                        Renderer_Zoom_Delta(zoom);
                        Font_Scale(monospace_font, Renderer_Zoom_Current());
                        editor.scale = Renderer_Zoom_Current();
                        Editor_Font_Size_Changed(&editor);
                        did_input = true;
                    } else {
                        if (event.wheel.y < 0.f) {
                            Editor_Down(&editor);
                        } else {
                            Editor_Up(&editor);
                        }
                    }
                    break;
                case SDL_EVENT_KEY_UP:
                    did_input = true;
                    ctrl_down = event.key.mod & SDL_KMOD_CTRL;
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
                        case SDLK_HOME:
                            Editor_Beginning_Of_Line(&editor);
                            break;
                        case SDLK_END:
                            Editor_End_Of_Line(&editor);
                            break;
                        case SDLK_PLUS:
                            if (ctrl_down) {
                                Renderer_Zoom_Delta(0.1f);
                                Font_Scale(monospace_font, Renderer_Zoom_Current());
                                editor.scale = Renderer_Zoom_Current();
                                Editor_Font_Size_Changed(&editor);
                            }
                            break;
                        case SDLK_MINUS:
                            if (ctrl_down) {
                                Renderer_Zoom_Delta(-0.1f);
                                Font_Scale(monospace_font, Renderer_Zoom_Current());
                                Editor_Font_Size_Changed(&editor);
                                editor.scale = Renderer_Zoom_Current();
                            }
                            break;
                        case SDLK_F2:
                            Dump_Font_Information_For_Debugging(monospace_font);
                            break;
                    }
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    if (ctrl_down) continue;
                    did_input = true;
                    auto str  = As_String(event.text.text);
                    Editor_Insert_Text(&editor, str);
                    break;
            }
        }

        if (!running) break;

        defer(Sleep_Until_Next_Frame());

        if (last_tick == 0.f) {
            draw(dt);
            continue;
        }

        if (!animating && !did_input /* !input.state.window_changed_somehow*/) {
            continue;
        }

        draw(dt);
    } while (true);
};

auto draw(f32 dt) -> void {
    static u8 buffer[1024];
    Font*     font             = editor.font;
    f32       offset_x         = 100.0 + camera.x;
    f32       line_no_offset_x = offset_x - 20.0;
    f32       offset_y         = 200.0 + camera.y;
    f32       line_height      = Font_Size(font) + 4.0;
    auto      window_size      = Window_Size();

    editor.lines_on_screen = ((window_size.y - offset_y) / line_height);

    auto cursor_data = Editor_Cursor_Draw_Data(&editor);
    auto dim         = Calculate_Text_Dimensions_With_Font(font, cursor_data.text_left_of_cursor);
    Vec2 is_pos      = {
        .x = offset_x + dim.x,
        .y = offset_y + line_height * (f32(cursor_data.virtual_row) - f32(editor.scroll_offset)),
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

    was_pos = {
        Damp(was_pos.x, is_pos.x, dampning, dt * dt_mult),
        Damp(was_pos.y, is_pos.y, dampning, dt * dt_mult),
    };

    was_scroll = Damp(was_scroll, is_scroll, dampning, dt * dt_mult);

    auto rect = Rect{
        .x = was_pos.x,
        .y = was_pos.y,
        .w = 2.f,
        .h = f32(Font_Size(font)),
    };

    Renderer_Clear(BG);

    // Create grid
    {
        Renderer_Set_Color(BG_2);
        const f32 grid_gap = 32.f;

        f32 offset_x = fmodf(camera.x, grid_gap);
        f32 offset_y = fmodf(camera.y, grid_gap);

        for (f32 x = offset_x; x < window_size.x; x += grid_gap) {
            Renderer_Draw_Rect({
                .x = x,
                .y = 0.f,
                .w = 2.f,
                .h = window_size.y,
            });
        }

        for (f32 y = offset_y; y < window_size.y; y += grid_gap) {
            Renderer_Draw_Rect({
                .x = 0.f,
                .y = y,
                .w = window_size.x,
                .h = 2.f,
            });
        }
    }

    Renderer_Set_Color(FG);
    Render_Text(regular_font, "Title q8^)"_s, 100.f, 100.f);

    auto editor_size = Editor_Size(&editor);

    s64 n_virtual_line = 0;
    for (s64 idx = 0; idx < editor.lines.size; idx++) {
        {
            f32 y = offset_y + f32(n_virtual_line) * line_height - was_scroll;
            if (y - offset_y > editor_size.y) break;
            auto line_no_str = Sprintf(buffer, sizeof(buffer), "%lu", idx + 1);
            auto line_no_dim = Calculate_Text_Dimensions_With_Font(font, line_no_str);
            if (ceilf(y + line_height - 1.f) >= offset_y) {
                Renderer_Set_Color(FG);
                Render_Text(font, line_no_str, line_no_offset_x - line_no_dim.x, y);
            }
        }

        Renderer_Set_Color(FG);
        auto virtual_lines = Editor_Virtual_Lines(&editor, idx);
        for (auto virtual_line : virtual_lines) {
            auto slice = editor.buffer.slice(virtual_line.begin, virtual_line.end);
            f32  y     = offset_y + f32(n_virtual_line) * line_height - was_scroll;
            if (y - offset_y > editor_size.y) break;
            if (ceilf(y + line_height - 1.f) >= offset_y) {
                Render_Text(font, slice.data, slice.size, offset_x, y);
            }
            n_virtual_line += 1;
        }
    }

    Renderer_Set_Color(FG);
    Renderer_Draw_Rect(rect);

    Rect editor_outline = {
        offset_x,
        offset_y,
        editor_size.x,
        editor_size.y,
    };
    Renderer_Draw_Outline(editor_outline);
    /*
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

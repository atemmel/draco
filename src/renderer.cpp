#include "renderer.hpp"

#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <unistd.h>

#include <cstdio>

#include "runtime.hpp"
#include "types.hpp"

const auto PROG_NAME = "draco";
const auto W         = 1600;
const auto H         = 900;

u64      refresh_rate_ns;
Renderer renderer;
Window   window;

f32 zoom = 1.f;

auto Set_Refresh_Rate(f32 display_fps) -> void {
    refresh_rate_ns = (1000 * 1000) / display_fps;
}

auto Hit_Test_Callback(Window window, const SDL_Point* hit, void*) -> SDL_HitTestResult {
    const int mouse_grab_padding = 10;
    int       width;
    int       height;
    SDL_GetWindowSize(window, &width, &height);

    if (hit->y < mouse_grab_padding) {
        if (hit->x < mouse_grab_padding) {
            return SDL_HITTEST_RESIZE_TOPLEFT;
        } else if (hit->x > width - mouse_grab_padding) {
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        } else {
            return SDL_HITTEST_RESIZE_TOP;
        }
    } else if (hit->y > height - mouse_grab_padding) {
        if (hit->x < mouse_grab_padding) {
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        } else if (hit->x > width - mouse_grab_padding) {
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        } else {
            return SDL_HITTEST_RESIZE_BOTTOM;
        }
    } else if (hit->y < 200) {
        // input.state.window_changed_somehow = false;
        return SDL_HITTEST_DRAGGABLE;
    } else if (hit->x < mouse_grab_padding) {
        return SDL_HITTEST_RESIZE_LEFT;
    } else if (hit->x > width - mouse_grab_padding) {
        return SDL_HITTEST_RESIZE_RIGHT;
    }

    // input.state.window_changed_somehow = false;
    return SDL_HITTEST_NORMAL;
}

auto Init_Renderer() -> void {
    auto display_mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    Assert(display_mode);

    refresh_rate_ns = display_mode->refresh_rate;

    auto window_flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                        SDL_WINDOW_RESIZABLE;  // | SDL_WINDOW_BORDERLESS;

    window = SDL_CreateWindow(PROG_NAME, W, H, window_flags);
    Assert(window);

    renderer = SDL_CreateRenderer(window, "vulkan");
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, "software");
    }
    Assert(renderer);

    // TODO
    SDL_SetWindowHitTest(window, &Hit_Test_Callback, null);
}

auto Destroy_Renderer() -> void {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    renderer = null;
    window   = null;
}

auto Dump_Available_Drivers() -> void {
    printf("Available drivers:\n");

    s32         i    = 0;
    const char* driv = SDL_GetRenderDriver(0);
    for (; driv; i++, driv = SDL_GetRenderDriver(i)) {
        printf("%s\n", driv);
    }
}

auto Window_Size() -> Vec2 {
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    return {f32(w), f32(h)};
}

auto Renderer_Set_Color(Vec4 color) -> void {
    SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
}

auto Renderer_Draw_Rect(Rect rect) -> void {
    auto r = SDL_FRect(rect);

    SDL_RenderFillRect(renderer, &r);
}

auto Renderer_Draw_Outline(Rect rect) -> void {
    auto r = SDL_FRect(rect);
    SDL_RenderRect(renderer, &r);
}

auto Renderer_Zoom_Delta(f32 f) -> void {
    const f32 min_zoom = 0.5f;
    const f32 max_zoom = 2.0f;
    zoom += f;
    zoom = max(min_zoom, zoom);
    zoom = min(max_zoom, zoom);
}

auto Renderer_Zoom_Current() -> f32 {
    return zoom;
}

auto Renderer_Clear(Vec4 color) -> void {
    Renderer_Set_Color(color);
    SDL_RenderClear(renderer);
}

auto Renderer_Present() -> void {
    SDL_RenderPresent(renderer);
};

auto Sleep_Until_Next_Frame() -> void {
    usleep(refresh_rate_ns);
}

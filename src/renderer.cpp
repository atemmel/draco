#include "renderer.hpp"

#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <unistd.h>

#include <cstdio>

#include "runtime.hpp"
#include "types.hpp"

const auto PROG_NAME = "draco";
const auto W         = 1200;
const auto H         = 800;

u64      refresh_rate_ns;
Renderer renderer;
Window   window;

f32 zoom = 1.f;

auto Set_Refresh_Rate(f32 display_fps) -> void {
    refresh_rate_ns = (1000 * 1000) / display_fps;
}

auto Init_Renderer() -> void {
    auto display_mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    Assert(display_mode);

    refresh_rate_ns = display_mode->refresh_rate;

    auto window_flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                        SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;

    window = SDL_CreateWindow(PROG_NAME, W, H, window_flags);
    Assert(window);

    renderer = SDL_CreateRenderer(window, "vulkan");
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, "software");
    }
    Assert(renderer);

    // TODO
    // SDL_SetWindowHitTest
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
    zoom += f;
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

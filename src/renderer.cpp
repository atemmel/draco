#include "renderer.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>

#include "types.hpp"

const auto PROG_NAME = "draco";
const auto W         = 1200;
const auto H         = 800;

u64 refresh_rate_ns;
Renderer renderer;
Window window;

auto Set_Refresh_Rate(f32 display_fps) -> void {
    refresh_rate_ns = (1000 * 1000) / display_fps;
}

auto Init_Renderer() -> void {
    auto display_mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    assert(display_mode);

    refresh_rate_ns = display_mode->refresh_rate;

    auto window_flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                        SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS;

    window = SDL_CreateWindow(PROG_NAME, W, H, window_flags);
    assert(window);

    renderer = SDL_CreateRenderer(window, "vulkan");
    assert(renderer);

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

    i32 i            = 0;
    const char* driv = SDL_GetRenderDriver(0);
    for (; driv; i++, driv = SDL_GetRenderDriver(i)) {
        printf("%s\n", driv);
    }
}

auto Renderer_Set_Color(Vec4 color) -> void {
    SDL_SetRenderDrawColorFloat(renderer, color.r, color.g, color.b, color.a);
}

auto Renderer_Clear(Vec4 color) -> void {
    Renderer_Set_Color(color);
    SDL_RenderClear(renderer);
}

auto Sleep_Until_Next_Frame() -> void {
    usleep(refresh_rate_ns);
}

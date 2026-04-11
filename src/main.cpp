#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>

#include <cassert>
#include <cstdio>

#include "defer.hpp"
#include "fonts.hpp"
#include "mem.hpp"
#include "renderer.hpp"
#include "strings.hpp"

auto loop() -> void;

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

    loop();
    return 0;
}

bool animating  = false;
i64 last_tick   = 0;
Vec2 was_pos    = {-1.f, -1.f};
f32 was_scroll  = 0.f;
f32 zoom_scalar = 0.f;
bool ctrl_down  = false;

auto loop() -> void {
    auto running = true;
    SDL_Event event;
    for (;;) {
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

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);

        auto str = "gaming->man + 50 * My_Func() {} [0]"_s;

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        Render_Text(monospace_font, str, 0, 0);

        SDL_RenderPresent(renderer);

        Sleep_Until_Next_Frame();
    }
};

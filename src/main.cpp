#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>

#include <cstring>

#include "defer.hpp"
#include "fonts.hpp"
#include "mem.hpp"
#include "renderer.hpp"

auto loop() -> void;

auto main() -> int {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init() Error: %s", SDL_GetError());
        return -1;
    }
    defer(SDL_Quit());

    auto allocator = default_allocator;

    Init_Renderer();
    defer(Destroy_Renderer());
    Init_Fonts();
    defer(Destroy_Fonts());

    SDL_Event event;
    bool running = true;

    SDL_FRect mouseRect;
    mouseRect.x = mouseRect.y = -1000;  // ensure it's offscreen at startup
    mouseRect.w = mouseRect.h = 50;

    loop();
    return 0;
}

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

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Draw_Text(renderer, "gaming", strlen("gaming"), 100, 100, true);
        // Draw_Text(renderer, "gaming", strlen("gaming"), 100, 200, false);

        // draw everything to screen
        SDL_RenderPresent(renderer);

        Sleep_Until_Next_Frame();
    }
};

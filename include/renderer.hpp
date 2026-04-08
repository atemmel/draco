#pragma once

#include <SDL3/SDL_render.h>

using Renderer = SDL_Renderer*;
using Window   = SDL_Window*;
using Texture  = SDL_Texture*;

extern Renderer renderer;
extern Window window;

auto Init_Renderer() -> void;
auto Destroy_Renderer() -> void;

auto Sleep_Until_Next_Frame() -> void;

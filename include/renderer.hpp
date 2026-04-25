#pragma once

#include <SDL3/SDL_render.h>

#include "math.hpp"

using Renderer = SDL_Renderer*;
using Window   = SDL_Window*;
using Texture  = SDL_Texture*;
using Surface  = SDL_Surface*;

extern Renderer renderer;
extern Window   window;

auto Init_Renderer() -> void;
auto Destroy_Renderer() -> void;

auto Dump_Available_Drivers() -> void;
auto Window_Size() -> Vec2;

auto Renderer_Set_Color(Vec4 color) -> void;
auto Renderer_Draw_Rect(Rect rect) -> void;
auto Renderer_Draw_Outline(Rect rect) -> void;
auto Renderer_Zoom_Delta(f32 f) -> void;
auto Renderer_Zoom_Current() -> f32;
auto Renderer_Clear(Vec4 color) -> void;
auto Renderer_Present() -> void;

auto Sleep_Until_Next_Frame() -> void;

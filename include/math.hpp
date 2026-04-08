#pragma once

#include <SDL3/SDL_rect.h>

#include "types.hpp"

struct Rect {
    i32 x, y, w, h;

    operator SDL_Rect() {
        return {x, y, w, h};
    }
};

struct Point {
    i32 x, y;

    operator SDL_Point() {
        return {x, y};
    }
};

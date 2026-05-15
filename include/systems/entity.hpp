#pragma once

#include "math.hpp"
#include "types.hpp"

// TODO: rework into generational indicies
using EID = s64;

using Entity_Tags = u64;

const Entity_Tags Any      = 0,
                  Drawable = 1 << 0,
                  Node     = 1 << 1;

struct Entity {
    Vec2 position;
    Vec2 size;
};

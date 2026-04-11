#pragma once

#include <SDL3/SDL_rect.h>

#include "types.hpp"

struct Rect {
    f32 x, y, w, h;

    operator SDL_Rect() const;
    operator SDL_FRect() const;

    auto Right() const -> f32;
    auto Bottom() const -> f32;
};

struct Vec2 {
    f32 x, y;

    operator SDL_Point() const;

    auto Eql(Vec2 other, f32 epsilon) const -> bool;
    auto Within(Rect other) const -> bool;
};

struct Vec4 {
    union {
        struct {
            f32 x, y, z, w;
        };
        struct {
            f32 r, g, b, a;
        };
    };

    auto Eql(Vec4 other, f32 epsilon) const -> bool;
};

auto operator+(Vec2 lhs, Vec2 rhs) -> Vec2;
auto operator-(Vec2 lhs, Vec2 rhs) -> Vec2;
auto operator*(Vec2 lhs, f32 scalar) -> Vec2;
auto operator/(Vec2 lhs, f32 scalar) -> Vec2;

auto operator+(Vec4 lhs, Vec4 rhs) -> Vec4;
auto operator-(Vec4 lhs, Vec4 rhs) -> Vec4;
auto operator*(Vec4 lhs, f32 scalar) -> Vec4;
auto operator/(Vec4 lhs, f32 scalar) -> Vec4;

auto Square(f32 x) -> f32;
auto Lerp(f32 to, f32 from, f32 t) -> f32;
auto Damp(f32 to, f32 from, f32 smoothing, f32 dt) -> f32;

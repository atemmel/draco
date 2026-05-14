#include "math.hpp"

#include <cmath>

Rect::operator SDL_Rect() const {
    return {s32(x), s32(y), s32(w), s32(h)};
}

Rect::operator SDL_FRect() const {
    return {f32(x), f32(y), f32(w), f32(h)};
}

auto Rect::Right() const -> f32 {
    return x + w;
}

auto Rect::Bottom() const -> f32 {
    return y + h;
}

Vec2::operator SDL_Point() const {
    return {s32(x), s32(y)};
}

auto Vec2::Eql(Vec2 other, f32 epsilon) const -> bool {
    return fabsf(x - other.x) <= epsilon && fabsf(y - other.y) <= epsilon;
}

auto Vec2::Within(Rect other) const -> bool {
    return x >= other.x && x <= other.Right() && y >= other.y && y <= other.Bottom();
}

auto Vec4::Eql(Vec4 other, f32 epsilon) const -> bool {
    return true                              //
           && fabsf(x - other.x) <= epsilon  //
           && fabsf(y - other.y) <= epsilon  //
           && fabsf(z - other.z) <= epsilon  //
           && fabsf(w - other.w) <= epsilon  //
        ;
}

auto operator+(Vec2 lhs, Vec2 rhs) -> Vec2 {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

auto operator-(Vec2 lhs, Vec2 rhs) -> Vec2 {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

auto operator*(Vec2 lhs, f32 scalar) -> Vec2 {
    return {lhs.x * scalar, lhs.y * scalar};
}

auto operator/(Vec2 lhs, f32 scalar) -> Vec2 {
    f32 factor = 1.f / scalar;
    return {lhs.x * factor, lhs.y * factor};
}

auto operator+(Vec4 lhs, Vec4 rhs) -> Vec4 {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

auto operator-(Vec4 lhs, Vec4 rhs) -> Vec4 {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

auto operator*(Vec4 lhs, f32 scalar) -> Vec4 {
    return {lhs.x * scalar, lhs.y * scalar, lhs.z * scalar, lhs.w * scalar};
}

auto operator/(Vec4 lhs, f32 scalar) -> Vec4 {
    f32 factor = 1.f / scalar;
    return {lhs.x * factor, lhs.y * factor, lhs.z * factor, lhs.w * factor};
}

auto Square(f32 x) -> f32 {
    return x * x;
}

auto Lerp(f32 t, f32 v0, f32 v1) -> f32 {
    return (1.f - t) * v0 + t * v1;
}

auto Damp(f32 v0, f32 v1, f32 smoothing, f32 dt) -> f32 {
    return Lerp(1.f - powf(smoothing, dt), v0, v1);
}

const std = @import("std");
const pow = std.math.pow;

pub const Vec2 = struct {
    x: f32,
    y: f32,

    pub fn eql(lhs: Vec2, rhs: Vec2, epsilon: f32) bool {
        return @abs(lhs.x - rhs.x) <= epsilon and @abs(lhs.y - rhs.y) <= epsilon;
    }

    pub fn within(self: Vec2, box: Box) bool {
        return self.x >= box.x and self.x <= box.right() and self.y >= box.y and self.y <= box.bottom();
    }
};

pub const Vec4 = struct {
    x: f32,
    y: f32,
    z: f32,
    w: f32,
};

pub const Box = struct {
    x: f32,
    y: f32,
    w: f32,
    h: f32,

    pub fn right(self: Box) f32 {
        return self.x + self.w;
    }

    pub fn bottom(self: Box) f32 {
        return self.y + self.h;
    }
};

pub fn square(x: f32) f32 {
    return x * x;
}

pub fn lerp(to: f32, from: f32, t: f32) f32 {
    return (1.0 - t) * from + t * to;
}

pub fn damp(to: f32, from: f32, smoothing: f32, dt: f32) f32 {
    return lerp(to, from, 1.0 - pow(f32, smoothing, dt));
}

const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub const Vec2 = struct {
    x: f32,
    y: f32,
};

pub const Vec4 = struct {
    x: f32,
    y: f32,
    z: f32,
    w: f32,
};

const std = @import("std");
const main = @import("main.zig");
const math = @import("math.zig");

const Vec4 = math.Vec4;

const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_ttf/SDL_ttf.h");
    @cInclude("SDL3_image/SDL_image.h");
});

pub var header_font: ?*c.TTF_Font = undefined;
pub var body_font: ?*c.TTF_Font = undefined;

pub fn drawText(font: ?*c.TTF_Font, text: []const u8, color: Vec4, x: f32, y: f32) void {
    if (text.len == 0) {
        return;
    }
    const surface = c.TTF_RenderText_Blended(font, text.ptr, text.len, asColor(color)) orelse return;
    defer c.SDL_DestroySurface(surface);
    const texture = c.SDL_CreateTextureFromSurface(main.renderer, surface) orelse return;
    defer c.SDL_DestroyTexture(texture);

    const dst = c.SDL_FRect{
        .x = x,
        .y = y,
        .h = @floatFromInt(texture.*.h),
        .w = @floatFromInt(texture.*.w),
    };

    _ = c.SDL_RenderTexture(main.renderer, texture, null, &dst);
}

pub fn str(s: []const u8) [:0]const u8 {
    const static = struct {
        var buffer: [2048]u8 = undefined;
    };
    return std.fmt.bufPrintZ(&static.buffer, "{s}", .{s}) catch {
        static.buffer[0] = 0;
        return static.buffer[0..1 :0];
    };
}

pub fn strdim(font: ?*c.TTF_Font, s: []const u8) struct { w: f32, h: f32 } {
    if (s.len == 0) {
        return .{
            .w = 0,
            .h = 0,
        };
    }
    var w: c_int = 0;
    var h: c_int = 0;
    _ = c.TTF_GetStringSize(font, s.ptr, s.len, &w, &h);
    return .{
        .w = @floatFromInt(w),
        .h = @floatFromInt(h),
    };
}

fn asColor(v: Vec4) c.SDL_Color {
    return c.SDL_Color{
        .r = @intFromFloat(v.x * 255),
        .g = @intFromFloat(v.y * 255),
        .b = @intFromFloat(v.z * 255),
        .a = @intFromFloat(v.w * 255),
    };
}

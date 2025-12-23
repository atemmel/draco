const std = @import("std");
const main = @import("main.zig");
const math = @import("math.zig");
const embed = @import("embed.zig");

const Vec4 = math.Vec4;

pub const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_ttf/SDL_ttf.h");
    @cInclude("SDL3_image/SDL_image.h");
});

pub const DEFAULT_BODY_SIZE = 20.0;

pub const Font = c.struct_TTF_Font;

pub const PROG_NAME = "draco";
pub const W = 1200;
pub const H = 800;

pub var header_font: ?*Font = undefined;
pub var body_font: ?*Font = undefined;

pub var window: ?*c.SDL_Window = undefined;
pub var renderer: ?*c.SDL_Renderer = undefined;

pub fn initFonts() !void {
    if (!c.TTF_Init()) {
        std.debug.print("TTF failed init\n", .{});
        return error.TTFInitError;
    }

    header_font = c.TTF_OpenFontIO(
        c.SDL_IOFromConstMem(
            embed.font_regular_bold_italic_bytes.ptr,
            embed.font_regular_bold_italic_bytes.len,
        ),
        false,
        62.0,
    ) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{c.SDL_GetError()});
        return error.TTFInitError;
    };

    body_font = c.TTF_OpenFontIO(
        c.SDL_IOFromConstMem(
            embed.font_monospace_bytes.ptr,
            embed.font_monospace_bytes.len,
        ),
        false,
        DEFAULT_BODY_SIZE,
    ) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{c.SDL_GetError()});
        return error.TTFInitError;
    };
}

pub fn initWindow() !void {
    const display_mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetPrimaryDisplay()) orelse {
        c.SDL_Log("Could not get display mode! SDL error: %s\n", c.SDL_GetError());
        return error.Whatever;
    };
    main.setRefreshRate(display_mode.*.refresh_rate);

    //const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_MAXIMIZED | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;
    const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;

    if (!c.SDL_CreateWindowAndRenderer(PROG_NAME, W, H, win_flags, &window, &renderer)) {
        std.debug.print("Couldn't create window/renderer:", .{});
        return error.Whatever;
    }
}

pub fn deinitFonts() void {
    c.TTF_Quit();
}

pub fn drawText(font: ?*Font, text: []const u8, color: Vec4, x: f32, y: f32) void {
    if (text.len == 0) {
        return;
    }
    const surface = c.TTF_RenderText_Blended(font, text.ptr, text.len, asColor(color)) orelse return;
    defer c.SDL_DestroySurface(surface);
    const texture = c.SDL_CreateTextureFromSurface(renderer, surface) orelse return;
    defer c.SDL_DestroyTexture(texture);

    const dst = c.SDL_FRect{
        .x = x,
        .y = y,
        .h = @floatFromInt(texture.*.h),
        .w = @floatFromInt(texture.*.w),
    };

    _ = c.SDL_RenderTexture(renderer, texture, null, &dst);
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

pub fn strdim(font: ?*Font, s: []const u8) struct { w: f32, h: f32 } {
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

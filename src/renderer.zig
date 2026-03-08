const std = @import("std");
const main = @import("main.zig");
const math = @import("math.zig");
const embed = @import("embed.zig");
const gui = @import("gui.zig");
const sdl = @import("sdl.zig").all;
const input = @import("input.zig");

const Vec2 = math.Vec2;
const Vec4 = math.Vec4;
const Box = math.Box;
const Gui = gui.Gui;

pub const DEFAULT_BODY_SIZE = 20.0;

pub const Font = sdl.struct_TTF_Font;

pub const PROG_NAME = "draco";
pub const W = 1200;
pub const H = 800;

pub var header_font: ?*Font = undefined;
pub var body_font: ?*Font = undefined;

pub var window: ?*sdl.SDL_Window = undefined;
pub var renderer: ?*sdl.SDL_Renderer = undefined;

pub fn initFonts() !void {
    if (!sdl.TTF_Init()) {
        std.debug.print("TTF failed init\n", .{});
        return error.TTFInitError;
    }

    header_font = sdl.TTF_OpenFontIO(
        sdl.SDL_IOFromConstMem(
            embed.font_regular_bold_italic_bytes.ptr,
            embed.font_regular_bold_italic_bytes.len,
        ),
        false,
        62.0,
    ) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{sdl.SDL_GetError()});
        return error.TTFInitError;
    };

    body_font = sdl.TTF_OpenFontIO(
        sdl.SDL_IOFromConstMem(
            embed.font_monospace_bytes.ptr,
            embed.font_monospace_bytes.len,
        ),
        false,
        DEFAULT_BODY_SIZE,
    ) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{sdl.SDL_GetError()});
        return error.TTFInitError;
    };
}

pub fn initWindow() !void {
    const display_mode = sdl.SDL_GetCurrentDisplayMode(sdl.SDL_GetPrimaryDisplay()) orelse {
        sdl.SDL_Log("Could not get display mode! SDL error: %s\n", sdl.SDL_GetError());
        return error.Whatever;
    };
    main.setRefreshRate(display_mode.*.refresh_rate);

    //const win_flags = sdl.SDL_WINDOW_INPUT_FOCUS | sdl.SDL_WINDOW_HIGH_PIXEL_DENSITY | sdl.SDL_WINDOW_MAXIMIZED | sdl.SDL_WINDOW_RESIZABLE | sdl.SDL_WINDOW_BORDERLESS;
    const win_flags = sdl.SDL_WINDOW_INPUT_FOCUS | sdl.SDL_WINDOW_HIGH_PIXEL_DENSITY | sdl.SDL_WINDOW_RESIZABLE | sdl.SDL_WINDOW_BORDERLESS;

    window = sdl.SDL_CreateWindow(PROG_NAME, W, H, win_flags) orelse {
        std.debug.print("Couldn't create window", .{});
        return error.Whatever;
    };
    renderer = sdl.SDL_CreateRenderer(window, null) orelse {
        std.debug.print("Couldn't create renderer", .{});
        return error.Whatever;
    };

    _ = sdl.SDL_SetWindowSize(window, W, H);
    _ = sdl.SDL_SetWindowHitTest(window, &hitTestCallback, null);
}

fn hitTestCallback(w: ?*sdl.SDL_Window, area: [*c]const sdl.SDL_Point, _: ?*anyopaque) callconv(.c) sdl.SDL_HitTestResult {
    const mouse_grab_padding = 10;
    var width: c_int = undefined;
    var height: c_int = undefined;
    _ = sdl.SDL_GetWindowSize(w, &width, &height);
    input.state.window_changed_somehow = true;

    if (area.*.y < mouse_grab_padding) {
        if (area.*.x < mouse_grab_padding) {
            return sdl.SDL_HITTEST_RESIZE_TOPLEFT;
        } else if (area.*.x > width - mouse_grab_padding) {
            if (area.*.x < mouse_grab_padding) {
                return sdl.SDL_HITTEST_RESIZE_BOTTOMLEFT;
            } else if (area.*.x > width - mouse_grab_padding) {
                return sdl.SDL_HITTEST_RESIZE_BOTTOMRIGHT;
            } else {
                return sdl.SDL_HITTEST_RESIZE_BOTTOM;
            }
        } else if (area.*.x < mouse_grab_padding) {
            return sdl.SDL_HITTEST_RESIZE_LEFT;
        } else if (area.*.x > width - mouse_grab_padding) {
            return sdl.SDL_HITTEST_RESIZE_RIGHT;
        } else if (area.*.y < 200) {
            input.state.window_changed_somehow = false;
            return sdl.SDL_HITTEST_DRAGGABLE;
        }
    }

    input.state.window_changed_somehow = false;
    return sdl.SDL_HITTEST_NORMAL;
}

pub fn deinitFonts() void {
    sdl.TTF_Quit();
}

pub fn setDrawColor(color: Vec4) void {
    _ = sdl.SDL_SetRenderDrawColorFloat(renderer, color.x, color.y, color.z, color.w);
}

pub fn drawWindowDecoration(g: Gui) void {
    for (g.window_decoration_state.operations) |op| {
        if (op.kind == g.window_decoration_state.hovered_operation) {
            setDrawColor(main.FG);
        } else {
            setDrawColor(main.FG_2);
        }

        const b = op.box;
        switch (op.kind) {
            .Minimize => {
                _ = sdl.SDL_RenderLine(
                    renderer,
                    b.x + b.w,
                    b.y + b.h * 0.15,
                    b.x + b.w * 0.5,
                    b.y + b.h * 0.85,
                );
                _ = sdl.SDL_RenderLine(
                    renderer,
                    b.x,
                    b.y + b.h * 0.15,
                    b.x + b.w * 0.5,
                    b.y + b.h * 0.85,
                );
            },
            .Maximize => {
                var rect = sdl.SDL_FRect{
                    .x = b.x,
                    .y = b.y,
                    .w = b.w,
                    .h = b.h,
                };
                _ = sdl.SDL_RenderRect(
                    renderer,
                    &rect,
                );
                rect = sdl.SDL_FRect{
                    .x = (b.x),
                    .y = (b.y),
                    .w = (b.w * 0.65),
                    .h = (b.h * 0.65),
                };
                _ = sdl.SDL_RenderRect(
                    renderer,
                    &rect,
                );
            },
            .Close => {
                _ = sdl.SDL_RenderLine(
                    renderer,
                    b.x,
                    b.y,
                    b.x + b.w,
                    b.y + b.h,
                );
                _ = sdl.SDL_RenderLine(
                    renderer,
                    b.x + b.w,
                    b.y,
                    b.x,
                    b.y + b.h,
                );
            },
        }
    }
}

pub fn windowSize() Vec2 {
    var w: c_int = 0;
    var h: c_int = 0;
    _ = sdl.SDL_GetWindowSize(window, &w, &h);
    const scale = sdl.SDL_GetWindowDisplayScale(window);
    return Vec2{
        .x = @as(f32, @floatFromInt(w)) * scale,
        .y = @as(f32, @floatFromInt(h)) * scale,
    };
}

pub fn mousePos() Vec2 {
    var pos = Vec2{
        .x = 0,
        .y = 0,
    };
    _ = sdl.SDL_GetMouseState(&pos.x, &pos.y);
    const scale = sdl.SDL_GetWindowDisplayScale(window);
    return pos.mul(scale);
}

pub fn drawText(font: ?*Font, text: []const u8, color: Vec4, x: f32, y: f32) void {
    if (text.len == 0) {
        return;
    }
    const surface = sdl.TTF_RenderText_Blended(font, text.ptr, text.len, asColor(color)) orelse return;
    defer sdl.SDL_DestroySurface(surface);
    const texture = sdl.SDL_CreateTextureFromSurface(renderer, surface) orelse return;
    defer sdl.SDL_DestroyTexture(texture);

    const dst = sdl.SDL_FRect{
        .x = x,
        .y = y,
        .h = @floatFromInt(texture.*.h),
        .w = @floatFromInt(texture.*.w),
    };

    _ = sdl.SDL_RenderTexture(renderer, texture, null, &dst);
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
    _ = sdl.TTF_GetStringSize(font, s.ptr, s.len, &w, &h);
    return .{
        .w = @floatFromInt(w),
        .h = @floatFromInt(h),
    };
}

fn asColor(v: Vec4) sdl.SDL_Color {
    return sdl.SDL_Color{
        .r = @intFromFloat(v.x * 255),
        .g = @intFromFloat(v.y * 255),
        .b = @intFromFloat(v.z * 255),
        .a = @intFromFloat(v.w * 255),
    };
}

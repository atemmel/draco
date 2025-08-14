const std = @import("std");

const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

const Vec4 = struct {
    x: f32,
    y: f32,
    z: f32,
    w: f32,
};

const PROG_NAME = "draco";
const W = 1200;
const H = 800;

const BG = Vec4{
    .x = 0.0,
    .y = 0.0,
    .z = 0.0,
    .w = 1.0,
};

var window: ?*c.SDL_Window = undefined;
var renderer: ?*c.SDL_Renderer = undefined;
var refresh_rate_ns: u64 = undefined;

fn setRefreshRate(display_fps: f32) void {
    refresh_rate_ns = @intFromFloat(1_000_000 / display_fps);
}

pub fn main() !void {
    //Initialize SDL
    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        c.SDL_Log("SDL could not initialize! SDL error: %s\n", c.SDL_GetError());
        return;
    }
    defer c.SDL_Quit();

    const display_mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetPrimaryDisplay()) orelse {
        c.SDL_Log("Could not get display mode! SDL error: %s\n", c.SDL_GetError());
        return;
    };
    setRefreshRate(display_mode.*.refresh_rate);

    _ = c.SDL_SetHint(c.SDL_HINT_WINDOW_ALLOW_TOPMOST, "1");

    //const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_MAXIMIZED | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;
    const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;

    if (!c.SDL_CreateWindowAndRenderer(PROG_NAME, W, H, win_flags, &window, &renderer)) {
        std.debug.print("Couldn't create window/renderer:", .{});
        return;
    }

    var running = true;
    var event: c.SDL_Event = undefined;
    while (running) {
        while (c.SDL_PollEvent(&event)) {
            switch (event.type) {
                c.SDL_EVENT_QUIT => {
                    running = false;
                },
                c.SDL_EVENT_KEY_DOWN => {
                    switch (event.key.key) {
                        c.SDLK_ESCAPE => {
                            running = false;
                        },
                        else => {},
                    }
                },
                else => {},
            }
        }
        if (!running) {
            break;
        }

        _ = c.SDL_SetRenderDrawColorFloat(renderer, BG.x, BG.y, BG.z, BG.w);
        _ = c.SDL_RenderClear(renderer);
        _ = c.SDL_RenderPresent(renderer);
    }
}

comptime {
    std.testing.refAllDecls(@import("window.zig"));
}

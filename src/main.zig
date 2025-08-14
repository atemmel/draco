const std = @import("std");
const math = @import("math.zig");
const rend = @import("renderer.zig");

const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_ttf/SDL_ttf.h");
    @cInclude("SDL3_image/SDL_image.h");
});

const Vec4 = math.Vec4;
const cwd = std.fs.cwd;

const PROG_NAME = "draco";
const W = 1200;
const H = 800;

const BG = Vec4{
    .x = 0.0,
    .y = 0.0,
    .z = 0.0,
    .w = 1.0,
};

const FG = Vec4{
    .x = 1.0,
    .y = 1.0,
    .z = 1.0,
    .w = 1.0,
};

var window: ?*c.SDL_Window = undefined;
pub var renderer: ?*c.SDL_Renderer = undefined;
var refresh_rate_ns: u64 = undefined;
var header_font: ?*c.TTF_Font = undefined;
var font_bytes: []const u8 = "";
var arena_impl: std.heap.ArenaAllocator = undefined;

fn setRefreshRate(display_fps: f32) void {
    refresh_rate_ns = @intFromFloat(1_000_000 / display_fps);
}

fn sleepNextFrame() void {
    std.Thread.sleep(refresh_rate_ns);
}

pub fn main() !void {
    //Initialize SDL
    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        c.SDL_Log("SDL could not initialize! SDL error: %s\n", c.SDL_GetError());
        return;
    }
    defer c.SDL_Quit();

    if (!c.TTF_Init()) {
        std.debug.print("TTF failed init\n", .{});
        return;
    }
    defer c.TTF_Quit();

    arena_impl = std.heap.ArenaAllocator.init(std.heap.c_allocator);
    defer arena_impl.deinit();
    const arena = arena_impl.allocator();

    font_bytes = cwd().readFileAlloc(arena, "/usr/share/fonts/TTF/TinosNerdFont-BoldItalic.ttf", 10_000_000) catch |e| {
        std.debug.print("Couldn't open font: {any}\n", .{e});
        return;
    };

    header_font = c.TTF_OpenFontIO(c.SDL_IOFromConstMem(font_bytes.ptr, font_bytes.len), false, 62.0) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{c.SDL_GetError()});
        return;
    };

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

    loop();
}

fn loop() void {
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

        rend.drawText(header_font, "Draco", FG, 100.0, 100.0);

        _ = c.SDL_RenderPresent(renderer);

        sleepNextFrame();
    }
}

comptime {
    std.testing.refAllDecls(@import("window.zig"));
}

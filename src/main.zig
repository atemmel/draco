const std = @import("std");
const math = @import("math.zig");
const rend = @import("renderer.zig");
const Editor = @import("editor.zig").Editor;

const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_ttf/SDL_ttf.h");
    @cInclude("SDL3_image/SDL_image.h");
});

const Vec2 = math.Vec2;
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

const FG_2 = Vec4{
    .x = 0.5,
    .y = 0.5,
    .z = 0.5,
    .w = 1.0,
};

const DEFAULT_BODY_SIZE = 20.0;

var window: ?*c.SDL_Window = undefined;
pub var renderer: ?*rend.c.SDL_Renderer = undefined;
var refresh_rate_ns: u64 = undefined;
var font_bytes: []const u8 = "";
var font_bold_italic_bytes: []const u8 = "";
var editor: Editor = undefined;
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

    font_bold_italic_bytes = cwd().readFileAlloc(arena, "/usr/share/fonts/TTF/TinosNerdFont-BoldItalic.ttf", 10_000_000) catch |e| {
        std.debug.print("Couldn't open font: {any}\n", .{e});
        return;
    };
    font_bytes = cwd().readFileAlloc(arena, "/usr/share/fonts/TTF/TinosNerdFont-Regular.ttf", 10_000_000) catch |e| {
        std.debug.print("Couldn't open font: {any}\n", .{e});
        return;
    };

    rend.header_font = rend.c.TTF_OpenFontIO(rend.c.SDL_IOFromConstMem(font_bold_italic_bytes.ptr, font_bold_italic_bytes.len), false, 62.0);
    rend.body_font = rend.c.TTF_OpenFontIO(rend.c.SDL_IOFromConstMem(font_bytes.ptr, font_bytes.len), false, DEFAULT_BODY_SIZE) orelse {
        std.debug.print("Couldn't open font: {s}\n", .{c.SDL_GetError()});
        return;
    };

    editor = try Editor.init(std.heap.c_allocator);
    defer editor.deinit();

    const args = try std.process.argsAlloc(arena);
    if (args.len > 1) {
        editor.window.openFile(args[1]);
    }

    const display_mode = c.SDL_GetCurrentDisplayMode(c.SDL_GetPrimaryDisplay()) orelse {
        c.SDL_Log("Could not get display mode! SDL error: %s\n", c.SDL_GetError());
        return;
    };
    setRefreshRate(display_mode.*.refresh_rate);

    //const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_MAXIMIZED | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;
    const win_flags = c.SDL_WINDOW_INPUT_FOCUS | c.SDL_WINDOW_HIGH_PIXEL_DENSITY | c.SDL_WINDOW_RESIZABLE | c.SDL_WINDOW_BORDERLESS;

    if (!c.SDL_CreateWindowAndRenderer(PROG_NAME, W, H, win_flags, &window, &renderer)) {
        std.debug.print("Couldn't create window/renderer:", .{});
        return;
    }

    _ = c.SDL_StartTextInput(window);

    loop();
}

var animating = false;
var last_tick: i64 = 0;
var was_pos = Vec2{
    .x = -1.0,
    .y = -1.0,
};
var zoom_scalar: f32 = 1.0;
var ctrl_down = false;
var viewport = Vec2{};

fn loop() void {
    var running = true;
    var event: c.SDL_Event = undefined;
    while (running) {
        const current_tick = std.time.microTimestamp();
        defer last_tick = current_tick;
        const dt = @as(f32, @floatFromInt(current_tick - last_tick)) / std.time.us_per_s;
        var did_input = false;
        while (c.SDL_PollEvent(&event)) {
            switch (event.type) {
                c.SDL_EVENT_QUIT => {
                    running = false;
                },
                c.SDL_EVENT_KEY_DOWN => {
                    did_input = true;
                    ctrl_down = event.key.mod & c.SDL_KMOD_CTRL != 0;
                    switch (event.key.key) {
                        c.SDLK_ESCAPE => {
                            running = false;
                        },
                        c.SDLK_RETURN => {
                            editor.window.insertNewline();
                        },
                        c.SDLK_BACKSPACE => {
                            editor.window.removeLeftCursor();
                        },
                        c.SDLK_DELETE => {
                            editor.window.removeRightCursor();
                        },
                        c.SDLK_LEFT => {
                            editor.window.left();
                        },
                        c.SDLK_RIGHT => {
                            editor.window.right();
                        },
                        c.SDLK_UP => {
                            editor.window.up();
                        },
                        c.SDLK_DOWN => {
                            editor.window.down();
                        },
                        c.SDLK_HOME => {
                            editor.window.beginningOfLine();
                        },
                        c.SDLK_END => {
                            editor.window.endOfLine();
                        },
                        c.SDLK_PLUS => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                zoom_scalar += 0.1;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_MINUS => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                zoom_scalar -= 0.1;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_0 => {
                            if (event.key.mod & (c.SDL_KMOD_CTRL | c.SDL_KMOD_SHIFT) != 0) {
                                zoom_scalar = 1.0;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_S => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                editor.window.save();
                            }
                        },
                        else => {},
                    }
                },
                c.SDL_EVENT_KEY_UP => {
                    ctrl_down = event.key.mod & c.SDL_KMOD_CTRL != 0;
                },
                c.SDL_EVENT_TEXT_INPUT => {
                    if (ctrl_down) {
                        continue;
                    }
                    did_input = true;
                    editor.window.insert(std.mem.span(event.text.text));
                },
                else => {},
            }
        }
        if (!running) {
            break;
        }

        defer sleepNextFrame();

        if (!animating and !did_input and last_tick > 0) {
            continue;
        }

        draw(dt);
    }
}

fn draw(dt: f32) void {
    const offset_x = 100.0;
    const line_no_offset_x = offset_x - 20.0;
    const offset_y = 200.0;
    const line_height = rend.c.TTF_GetFontSize(rend.body_font) + 4.0;

    editor.window.lines_on_screen = @intFromFloat((H + offset_y) / line_height);

    const static = struct {
        var buffer: [1024]u8 = undefined;
    };

    const cursor_data = editor.window.cursorDrawData();
    const dim = rend.strdim(rend.body_font, cursor_data.text_left_of_cursor);
    const is_pos = Vec2{
        .x = offset_x + dim.w,
        .y = offset_y + line_height * (cursor_data.virtual_row - @as(f32, @floatFromInt(editor.window.scroll_offset))),
    };

    if (was_pos.x == -1.0 and was_pos.y == -1.0) {
        was_pos = is_pos;
    }

    animating = !was_pos.eql(is_pos, 0.1);

    const dampning = 0.001;
    const dt_mult = 2;

    was_pos = .{
        .x = math.damp(is_pos.x, was_pos.x, dampning, dt * dt_mult),
        .y = math.damp(is_pos.y, was_pos.y, dampning, dt * dt_mult),
    };

    const rect = rend.c.SDL_FRect{
        .x = was_pos.x,
        .y = was_pos.y,
        .w = 2.0 * zoom_scalar,
        .h = 20.0 * zoom_scalar,
    };

    _ = rend.c.SDL_SetRenderDrawColorFloat(renderer, BG.x, BG.y, BG.z, BG.w);
    _ = rend.c.SDL_RenderClear(renderer);
    rend.drawText(rend.header_font, "Title  q8^)", FG, 100.0, 100.0);
    var n_virtual_line: i64 = 0;
    for (editor.window.allRealLines(), 0..) |_, idx| {
        const line_no_str = std.fmt.bufPrint(&static.buffer, "{}", .{idx + 1}) catch "X";
        const line_no_dim = rend.strdim(rend.body_font, line_no_str);
        rend.drawText(rend.body_font, line_no_str, FG_2, line_no_offset_x - line_no_dim.w, offset_y + @as(f32, @floatFromInt(n_virtual_line - editor.window.scroll_offset)) * line_height);

        const virtual_lines = editor.window.virtualLines(idx);

        for (virtual_lines) |virtual_line| {
            const slice = editor.window.buffer.items[virtual_line.begin..virtual_line.end];
            rend.drawText(rend.body_font, slice, FG, offset_x, offset_y + @as(f32, @floatFromInt(n_virtual_line - editor.window.scroll_offset)) * line_height);
            n_virtual_line += 1;
        }
    }

    _ = rend.c.SDL_SetRenderDrawColorFloat(renderer, FG.x, FG.y, FG.z, FG.w);
    _ = rend.c.SDL_RenderFillRect(renderer, &rect);
    _ = rend.c.SDL_RenderPresent(renderer);
}

comptime {
    std.testing.refAllDecls(@import("window.zig"));
}

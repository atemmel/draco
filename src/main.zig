const std = @import("std");
const math = @import("math.zig");
const rend = @import("renderer.zig");
const Pane = @import("pane.zig").Pane;

const c = @cImport({
    @cInclude("SDL3/SDL.h");
    @cInclude("SDL3_image/SDL_image.h");
});

const Vec2 = math.Vec2;
const Vec4 = math.Vec4;
const cwd = std.fs.cwd;

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

var scale: f32 = 1.0;
var refresh_rate_ns: u64 = undefined;
var pane: Pane = undefined;
var arena_impl: std.heap.ArenaAllocator = undefined;

fn sleepNextFrame() void {
    std.Thread.sleep(refresh_rate_ns);
}

pub fn setRefreshRate(display_fps: f32) void {
    refresh_rate_ns = @intFromFloat(1_000_000 / display_fps);
}

pub fn main() !void {
    //Initialize SDL
    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        c.SDL_Log("SDL could not initialize! SDL error: %s\n", c.SDL_GetError());
        return;
    }
    defer c.SDL_Quit();

    try rend.initFonts();
    defer rend.deinitFonts();

    arena_impl = std.heap.ArenaAllocator.init(std.heap.c_allocator);
    defer arena_impl.deinit();
    const arena = arena_impl.allocator();

    pane = try Pane.init(std.heap.c_allocator);
    defer pane.deinit();

    const args = try std.process.argsAlloc(arena);
    if (args.len > 1) {
        pane.editor.openFile(args[1]);
    }

    try rend.initWindow();
    _ = rend.c.SDL_StartTextInput(rend.window);

    loop();
}

var animating = false;
var last_tick: i64 = 0;
var was_pos = Vec2{
    .x = -1.0,
    .y = -1.0,
};
var was_scroll: f32 = 0.0;
var zoom_scalar: f32 = 1.0;
var ctrl_down = false;

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
                            pane.editor.insertNewline();
                        },
                        c.SDLK_BACKSPACE => {
                            pane.editor.removeLeftCursor();
                        },
                        c.SDLK_DELETE => {
                            pane.editor.removeRightCursor();
                        },
                        c.SDLK_LEFT => {
                            pane.editor.left();
                        },
                        c.SDLK_RIGHT => {
                            pane.editor.right();
                        },
                        c.SDLK_UP => {
                            pane.editor.up();
                        },
                        c.SDLK_DOWN => {
                            pane.editor.down();
                        },
                        c.SDLK_HOME => {
                            pane.editor.beginningOfLine();
                        },
                        c.SDLK_END => {
                            pane.editor.endOfLine();
                        },
                        c.SDLK_PLUS => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                zoom_scalar += 0.1;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, rend.DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_MINUS => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                zoom_scalar -= 0.1;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, rend.DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_0 => {
                            if (event.key.mod & (c.SDL_KMOD_CTRL | c.SDL_KMOD_SHIFT) != 0) {
                                zoom_scalar = 1.0;
                                _ = rend.c.TTF_SetFontSize(rend.body_font, rend.DEFAULT_BODY_SIZE * zoom_scalar);
                            }
                        },
                        c.SDLK_S => {
                            if (event.key.mod & c.SDL_KMOD_CTRL != 0) {
                                pane.editor.save();
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
                    pane.editor.insert(std.mem.span(event.text.text));
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

    pane.editor.lines_on_screen = @as(i32, @intFromFloat((rend.H + offset_y) / line_height)) - 3;

    const static = struct {
        var buffer: [1024]u8 = undefined;
    };

    const cursor_data = pane.editor.cursorDrawData();
    const dim = rend.strdim(rend.body_font, cursor_data.text_left_of_cursor);
    const is_pos = Vec2{
        .x = offset_x + dim.w,
        .y = offset_y + line_height * (cursor_data.virtual_row - @as(f32, @floatFromInt(pane.editor.scroll_offset))),
    };
    const is_scroll = @as(f32, @floatFromInt(pane.editor.scroll_offset)) * line_height;

    if (is_pos.y < offset_y) {
        pane.editor.scroll_offset -= 1;
    }

    if (was_pos.x == -1.0 and was_pos.y == -1.0) {
        was_pos = is_pos;
    }

    animating = !was_pos.eql(is_pos, 0.1) or @max(was_scroll, 0.1) != 0.1;

    const dampning = 0.001;
    const dt_mult = 2;

    was_pos = .{
        .x = math.damp(is_pos.x, was_pos.x, dampning, dt * dt_mult),
        .y = math.damp(is_pos.y, was_pos.y, dampning, dt * dt_mult),
    };

    was_scroll = math.damp(is_scroll, was_scroll, dampning, dt * dt_mult);

    const rect = rend.c.SDL_FRect{
        .x = was_pos.x,
        .y = was_pos.y,
        .w = 2.0 * zoom_scalar,
        .h = 24.0 * zoom_scalar,
    };

    _ = rend.c.SDL_SetRenderDrawColorFloat(rend.renderer, BG.x, BG.y, BG.z, BG.w);
    _ = rend.c.SDL_RenderClear(rend.renderer);
    rend.drawText(rend.header_font, "Title  q8^)", FG, 100.0, 100.0);
    var n_virtual_line: i64 = 0;
    for (pane.editor.allRealLines(), 0..) |_, idx| {
        {
            const y = offset_y + @as(f32, @floatFromInt(n_virtual_line)) * line_height - was_scroll;
            const line_no_str = std.fmt.bufPrint(&static.buffer, "{}", .{idx + 1}) catch "X";
            const line_no_dim = rend.strdim(rend.body_font, line_no_str);
            if (y >= offset_y) {
                rend.drawText(rend.body_font, line_no_str, FG_2, line_no_offset_x - line_no_dim.w, y);
            }
        }

        const virtual_lines = pane.editor.virtualLines(idx);

        for (virtual_lines) |virtual_line| {
            const slice = pane.editor.buffer.items[virtual_line.begin..virtual_line.end];
            const y = offset_y + @as(f32, @floatFromInt(n_virtual_line)) * line_height - was_scroll;
            if (y >= offset_y) {
                rend.drawText(rend.body_font, slice, FG, offset_x, y);
            }
            n_virtual_line += 1;
        }
    }

    _ = rend.c.SDL_SetRenderDrawColorFloat(rend.renderer, FG.x, FG.y, FG.z, FG.w);
    _ = rend.c.SDL_RenderFillRect(rend.renderer, &rect);
    _ = rend.c.SDL_RenderPresent(rend.renderer);
}

comptime {
    std.testing.refAllDecls(@import("pane.zig"));
}

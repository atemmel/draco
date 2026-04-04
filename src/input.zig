const std = @import("std");
const c = @import("c.zig").c;

pub const KeyInput = struct {
    key: c.SDL_Keycode,
    mod: packed struct {
        ctrl: bool,
        alt: bool,
        shift: bool,
    },
    write: []const u8,
};

pub const WriteInput = struct {
    key: c.SDL_Keycode,
    mod: packed struct {
        ctrl: bool,
        alt: bool,
        shift: bool,
    },
};

const State = struct {
    mouse_down: bool,
    mouse_just_up: bool,
    window_changed_somehow: bool,
};

pub var state: State = .{
    .mouse_down = false,
    .mouse_just_up = false,
    .window_changed_somehow = false,
};

pub fn newEventTick() void {
    state.mouse_just_up = false;
    state.window_changed_somehow = false;
}

pub fn registerEvent(event: c.SDL_Event) void {
    switch (event.type) {
        c.SDL_EVENT_WINDOW_RESIZED, c.SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED => {
            state.window_changed_somehow = true;
        },
        c.SDL_EVENT_MOUSE_BUTTON_DOWN => {
            state.mouse_down = true;
        },
        c.SDL_EVENT_MOUSE_BUTTON_UP => {
            state.mouse_down = false;
            state.mouse_just_up = true;
        },
        else => {},
    }
}

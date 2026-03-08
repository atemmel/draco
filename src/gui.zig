const std = @import("std");
const rend = @import("renderer.zig");
const math = @import("math.zig");
const input = @import("input.zig");

const Box = math.Box;

pub const WindowOperation = struct {
    pub const Kind = enum {
        Minimize,
        Maximize,
        Close,
    };

    kind: Kind,
    box: Box,
};

pub const WindowDecorationState = struct {
    hovered_operation: ?WindowOperation.Kind = null,
    operations: [3]WindowOperation = [3]WindowOperation{
        .{
            .kind = .Close,
            .box = undefined,
        },
        .{
            .kind = .Maximize,
            .box = undefined,
        },
        .{
            .kind = .Minimize,
            .box = undefined,
        },
    },
    grabbing_titlebar: bool = false,
};

pub const Gui = struct {
    window_decoration_state: WindowDecorationState = .{},
    running: bool = true,
    mouse_just_clicked: bool = false,
    mouse_down: bool = false,
    scale: f32 = 1.0,
};

pub const UpdateResult = enum {
    Nothing,
    Close,
    Minimize,
    Maximize,
};

pub fn update(gui: *Gui) UpdateResult {
    defer {
        gui.mouse_just_clicked = false;
        gui.mouse_down = false;
    }
    const window_size = rend.windowSize();
    const mouse_pos = rend.mousePos();

    const decoration_padding_right = 16.0 * gui.scale;
    const decoration_padding_top = 16.0 * gui.scale;

    const operation_height = 20.0 * gui.scale;
    const operation_width = 20.0 * gui.scale;

    const operation_gap = 32 * gui.scale;

    for (gui.window_decoration_state.operations, 0..) |_, idx| {
        const i: f32 = @floatFromInt(idx);
        gui.window_decoration_state.operations[idx].box = .{
            .x = window_size.x - operation_width - decoration_padding_right - operation_gap * i,
            .y = 0 + decoration_padding_top,
            .w = operation_width,
            .h = operation_height,
        };
    }

    gui.window_decoration_state.hovered_operation = null;
    for (gui.window_decoration_state.operations) |op| {
        if (mouse_pos.within(op.box)) {
            gui.window_decoration_state.hovered_operation = op.kind;
            if (input.state.mouse_just_up) {
                return switch (op.kind) {
                    .Close => .Close,
                    .Minimize => .Minimize,
                    .Maximize => .Maximize,
                };
            }
        }
    }
    return .Nothing;
}

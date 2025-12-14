const std = @import("std");
const editor = @import("editor.zig");

const Editor = editor.Editor;

pub const Pane = struct {
    allocator: std.mem.Allocator,
    editor: Editor,

    pub fn init(base_allocator: std.mem.Allocator) !Pane {
        return Pane{
            .allocator = base_allocator,
            .editor = try Editor.init(base_allocator),
        };
    }

    pub fn deinit(self: *Pane) void {
        self.editor.deinit();
    }
};

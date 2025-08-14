const std = @import("std");

const Line = struct {
    begin: usize,
    end: usize,
};

pub const Window = struct {
    arena: std.heap.ArenaAllocator,
    buffer: std.ArrayList(u8),
    lines: std.ArrayList(Line),
    cursor: usize = 0,

    pub fn init(base_allocator: std.mem.Allocator) !Window {
        var arena = std.heap.ArenaAllocator.init(base_allocator);
        const buffer = try std.ArrayList(u8).initCapacity(arena.allocator(), 1024);
        const lines = try std.ArrayList(Line).initCapacity(arena.allocator, 128);
        return Window{
            .arena = arena,
            .buffer = buffer,
            .lines = lines,
        };
    }

    pub fn deinit(self: *Window) void {
        self.arena.deinit();
    }

    pub fn left(self: *Window) void {
        if (self.cursor > 0) {
            self.cursor -= 1;
        }
    }

    pub fn right(self: *Window) void {
        if (self.cursor + 1 < self.buffer.items.len) {
            self.cursor += 1;
        }
    }

    pub fn up(self: *Window) void {
        _ = self;
        //const left_buffer = self.buffer.items[0..self.cursor];
        //const newline = std.mem.lastIndexOfScalar(u8, left_buffer, '\n');
        //TODO: simd search to newline backwards from cursor

    }
};

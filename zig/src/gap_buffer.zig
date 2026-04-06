const std = @import("std");

pub const GapBuffer = struct {
    allocator: std.mem.Allocator,
    buffer: []u8,
    cursor: usize,
    gap_end: usize,

    pub fn init(alloc: std.mem.Allocator, size: usize) !GapBuffer {
        const self = GapBuffer{
            .allocator = alloc,
            .buffer = try alloc.alloc(u8, size),
            .cursor = 0,
            .gap_end = size,
        };
        return self;
    }

    pub fn deinit(self: *GapBuffer) void {
        self.allocator.free(self.buffer);
        self.buffer = &.{};
        self.cursor = 0;
        self.gap_end = 0;
    }

    pub fn front(self: *GapBuffer) []u8 {
        return self.buffer[0..self.cursor];
    }

    pub fn back(self: *GapBuffer) []u8 {
        return self.buffer[self.gap_end..];
    }

    fn grow(self: *GapBuffer, size: usize) !void {
        const new_size = @max(size, self.buffer.len);
        if (new_size <= self.buffer.len) {
            return;
        }
        self.allocator.realloc(self.buffer, new_size);
    }
};

const std = @import("std");

const Vec2 = @import("math.zig").Vec2;

const Line = struct {
    begin: usize,
    end: usize,
};

pub const Window = struct {
    arena: std.heap.ArenaAllocator,
    buffer: std.ArrayList(u8),
    lines: std.ArrayList(Line),
    cursor: usize = 0,
    row: usize = 0,

    pub fn init(base_allocator: std.mem.Allocator) !Window {
        var arena = std.heap.ArenaAllocator.init(base_allocator);
        const buffer = try std.ArrayList(u8).initCapacity(arena.allocator(), 1024);
        var lines = try std.ArrayList(Line).initCapacity(arena.allocator(), 128);
        lines.appendAssumeCapacity(Line{
            .begin = 0,
            .end = 0,
        });
        return Window{
            .arena = arena,
            .buffer = buffer,
            .lines = lines,
        };
    }

    pub fn deinit(self: *Window) void {
        self.arena.deinit();
    }

    fn reindex(self: *Window) void {
        self.reindexLines() catch @panic("OOM");
    }

    fn reindexLines(self: *Window) !void {
        self.lines.clearRetainingCapacity();
        var begin: usize = 0;
        for (self.buffer.items, 0..) |c, idx| {
            if (c == '\n') {
                const end = idx;
                try self.lines.append(.{
                    .begin = begin,
                    .end = end,
                });
                begin = end + 1;
            }
        }
        try self.lines.append(.{
            .begin = begin,
            .end = self.buffer.items.len,
        });
    }

    pub fn insert(self: *Window, what: []const u8) void {
        self.buffer.insertSlice(self.cursor, what) catch @panic("OOM");
        self.cursor += what.len;
        self.reindex();
    }

    pub fn insertNewline(self: *Window) void {
        self.buffer.insert(self.cursor, '\n') catch @panic("OOM");
        self.reindex();
        self.cursor += 1;
        self.down();
    }

    pub fn left(self: *Window) void {
        if (self.cursor > 0) {
            self.cursor -= 1;
        }
    }

    pub fn right(self: *Window) void {
        if (self.cursor < self.buffer.items.len) {
            self.cursor += 1;
        }
    }

    pub fn up(self: *Window) void {
        if (self.row > 0) {
            self.row -= 1;
        }
    }

    pub fn down(self: *Window) void {
        if (self.row < self.lines.items.len) {
            self.row += 1;
        }
    }

    pub fn removeBehindCursor(self: *Window) void {
        if (self.cursor < self.buffer.items.len) {
            _ = self.buffer.orderedRemove(self.cursor);
        }
        self.reindex();
    }

    pub fn removeFrontCursor(self: *Window) void {
        if (self.cursor > 0) {
            _ = self.buffer.orderedRemove(self.cursor - 1);
            self.cursor -= 1;
        }
        self.reindex();
    }

    pub fn allLines(self: *Window) []const Line {
        return self.lines.items;
    }

    pub fn lineSlice(self: *Window, idx: usize) []const u8 {
        const line = self.lines.items[idx];
        return self.buffer.items[line.begin..line.end];
    }

    pub fn cursorPos(self: *Window) struct {
        row: f32,
        text_left_of_cursor: []const u8,
    } {
        const cursor = self.cursor;
        for (self.allLines(), 0..) |line, idx| {
            if (cursor >= line.begin and cursor <= line.end) {
                return .{
                    .row = @floatFromInt(idx),
                    .text_left_of_cursor = self.buffer.items[line.begin..cursor],
                };
            }
        }
        unreachable;
    }
};

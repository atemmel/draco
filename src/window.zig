const std = @import("std");
const rend = @import("renderer.zig");

const Vec2 = @import("math.zig").Vec2;

const assert = std.debug.assert;

const RealLine = struct {
    begin: usize,
    end: usize,
    virtual_lines: VirtualLine = .{
        .begin = 0,
        .end = 0,
    },
};

const VirtualLine = struct {
    begin: usize,
    end: usize,
};

pub const Window = struct {
    arena: std.heap.ArenaAllocator,
    buffer: std.ArrayList(u8),
    lines: std.ArrayList(RealLine),
    virtual_lines: std.ArrayList(VirtualLine),
    cursor: usize = 0,
    active_file: []const u8 = "",
    rightmost_cursor_codepoint: usize = 0,
    scroll_offset: i64 = 0,
    lines_on_screen: i64 = 1,

    pub fn init(base_allocator: std.mem.Allocator) !Window {
        const arena = std.heap.ArenaAllocator.init(base_allocator);
        const buffer = try std.ArrayList(u8).initCapacity(base_allocator, 1024);
        var lines = try std.ArrayList(RealLine).initCapacity(base_allocator, 128);
        lines.appendAssumeCapacity(RealLine{
            .begin = 0,
            .end = 0,
        });
        var virtual_lines = try std.ArrayList(VirtualLine).initCapacity(base_allocator, 128);
        virtual_lines.appendAssumeCapacity(VirtualLine{
            .begin = 0,
            .end = 0,
        });
        return Window{
            .arena = arena,
            .buffer = buffer,
            .lines = lines,
            .virtual_lines = virtual_lines,
        };
    }

    pub fn deinit(self: *Window) void {
        self.buffer.deinit();
        self.lines.deinit();
        self.virtual_lines.deinit();
        self.arena.deinit();
    }

    pub fn openFile(self: *Window, filename: []const u8) void {
        self.active_file = self.arena.allocator().dupe(u8, filename) catch @panic("OOM");
        const file = std.fs.cwd().openFile(filename, .{}) catch |e| {
            switch (e) {
                error.FileNotFound => {
                    std.debug.print("TODO: File does not exist, show that file is unwritten\n", .{});
                },
                error.NoSpaceLeft,
                error.InvalidUtf8,
                error.FileTooBig,
                error.DeviceBusy,
                error.AccessDenied,
                error.SystemResources,
                error.WouldBlock,
                error.NoDevice,
                error.Unexpected,
                error.SharingViolation,
                error.PathAlreadyExists,
                error.PipeBusy,
                error.NameTooLong,
                error.InvalidWtf8,
                error.BadPathName,
                error.NetworkNotFound,
                error.AntivirusInterference,
                error.SymLinkLoop,
                error.ProcessFdQuotaExceeded,
                error.SystemFdQuotaExceeded,
                error.IsDir,
                error.NotDir,
                error.FileLocksNotSupported,
                error.FileBusy,
                => {
                    std.debug.print("TODO: Show this error better: {}\n", .{e});
                },
            }
            return;
        };
        const writer = self.buffer.writer();
        const reader = file.reader();
        var fifo = std.fifo.LinearFifo(u8, .{ .Static = 8192 }).init();
        fifo.pump(reader, writer) catch |e| {
            std.debug.print("{any}\n", .{e});
            unreachable;
        };
        self.reindex();
    }

    pub fn save(self: *Window) void {
        const file = std.fs.cwd().createFile(self.active_file, .{}) catch |e| {
            std.debug.print("error: {}\n", .{e});
            return;
        };
        file.writeAll(self.buffer.items) catch |e| {
            std.debug.print("error: {}\n", .{e});
            return;
        };
    }

    fn reindex(self: *Window) void {
        self.reindexRealLines() catch @panic("OOM");
        self.reindexVirtualLines() catch @panic("OOM");
    }

    fn reindexRealLines(self: *Window) !void {
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

    fn reindexVirtualLines(self: *Window) !void {
        self.virtual_lines.clearRetainingCapacity();

        const max_width = 800; //TODO: no hardcoded

        for (self.allRealLines(), 0..) |line, idx| {
            const virtual_line_slice_begin = self.virtual_lines.items.len;
            const line_slice = self.buffer.items[0..line.end];
            var virtual_line_begin: usize = line.begin;
            var word_begin: usize = line.begin;
            var i: usize = line.begin;
            var x: f32 = 0;
            while (i < line_slice.len) : (i += 1) {
                if (line_slice[i] != ' ') {
                    continue;
                }
                i = @min(i + 1, line_slice.len);
                const word = line_slice[word_begin..i];
                const word_dim = rend.strdim(rend.body_font, word);
                if (word_dim.w + x > max_width) {
                    try self.virtual_lines.append(.{
                        .begin = virtual_line_begin,
                        .end = i - 1,
                    });
                    x = 0;
                    word_begin = i;
                    virtual_line_begin = i;
                }

                word_begin = i;
                x += word_dim.w;
            }
            const word = line_slice[word_begin..];
            const word_dim = rend.strdim(rend.body_font, word);
            if (word_dim.w + x > max_width) {
                try self.virtual_lines.append(.{
                    .begin = virtual_line_begin,
                    .end = i,
                });
                x = 0;
                word_begin = i;
                virtual_line_begin = i;
            }
            try self.virtual_lines.append(.{
                .begin = virtual_line_begin,
                .end = i,
            });

            const virtual_line_slice_end = self.virtual_lines.items.len;
            self.lines.items[idx].virtual_lines = .{
                .begin = virtual_line_slice_begin,
                .end = virtual_line_slice_end,
            };
        }
    }

    pub fn insert(self: *Window, what: []const u8) void {
        self.buffer.insertSlice(self.cursor, what) catch @panic("OOM");
        self.cursor += what.len;
        self.reindex();
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn insertNewline(self: *Window) void {
        self.buffer.insert(self.cursor, '\n') catch @panic("OOM");
        self.reindex();
        self.cursor += 1;
        self.down();
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    fn byte(self: *Window, idx: usize) u8 {
        return self.buffer.items[idx];
    }

    const terminating_codepoints: []const u8 = &.{
        0b1100_0000,
        0b1110_0000,
        0b1111_0000,
    };

    const utf_1_inv_mask = 0b1000_0000;
    const utf_2_mask = 0b1100_0000;
    const utf_3_mask = 0b1110_0000;
    const utf_4_mask = 0b1111_0000;

    fn bytesUntilNearestCodepointLeft(self: *Window) usize {
        assert(self.cursor > 0);
        const offset: usize = if (self.cursor + 1 == self.buffer.items.len) 1 else 0;
        if (utf_1_inv_mask & self.byte(self.cursor - 1 - offset) == 0 or self.cursor < 2) {
            return 1;
        }
        if (utf_2_mask & self.byte(self.cursor - 2 - offset) != 0 or self.cursor < 3) {
            return 2;
        }
        if (utf_3_mask & self.byte(self.cursor - 3 - offset) != 0 or self.cursor < 4) {
            return 3;
        }
        return 4;
    }

    fn bytesUntilNearestCodepointRight(self: *Window) usize {
        const last_byte_index = @max(self.buffer.items.len, 1) - 1;
        if (self.cursor >= last_byte_index) {
            return 0;
        }
        const b = self.byte(self.cursor);
        if (b < 0x7F or self.cursor + 1 == last_byte_index) {
            return 1;
        }
        if (utf_2_mask & b != 0 or self.cursor + 2 == last_byte_index) {
            return 2;
        }
        if (utf_3_mask & b != 0 or self.cursor + 3 == last_byte_index) {
            return 3;
        }
        return 4;
    }

    fn codepointsLeftOfCursor(self: *Window) usize {
        const pos = self.virtualCursorPos();
        const line = self.allVirtualLines()[pos.virtual_row];
        const slice = self.buffer.items[line.begin..@min(self.buffer.items.len, self.cursor)];
        return std.unicode.utf8CountCodepoints(slice) catch self.cursor - line.begin;
    }

    fn byteOfNthCodepointOfVirtualLine(self: *Window, line: VirtualLine, n: usize) usize {
        const slice = self.buffer.items[line.begin..line.end];
        const view = std.unicode.Utf8View.init(slice) catch @panic("OOPS");
        var result: usize = 0;
        var i: usize = 0;
        var it = view.iterator();
        while (it.nextCodepointSlice()) |u| {
            if (i >= n) {
                break;
            }
            result += u.len;
            i += 1;
        }
        return result;
    }

    pub fn left(self: *Window) void {
        if (self.cursor <= 0) {
            return;
        }
        const n = self.bytesUntilNearestCodepointLeft();
        self.cursor -= n;
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn right(self: *Window) void {
        if (self.cursor >= self.buffer.items.len) {
            return;
        }
        const n = self.bytesUntilNearestCodepointRight();
        self.cursor += if (n > 0) n else 1;
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn up(self: *Window) void {
        const pos = self.virtualCursorPos();
        if (pos.virtual_row <= 0) {
            return;
        }
        const line = self.allVirtualLines()[pos.virtual_row - 1];
        const offset = self.byteOfNthCodepointOfVirtualLine(line, self.rightmost_cursor_codepoint);
        self.cursor = @min(line.begin + offset, line.end);

        if (pos.virtual_row < self.scroll_offset) {
            self.scroll_offset -= 1;
        }
    }

    pub fn down(self: *Window) void {
        const pos = self.virtualCursorPos();
        if (pos.virtual_row + 1 >= self.scroll_offset + self.lines_on_screen) {
            self.scroll_offset += 1;
        }
        if (pos.virtual_row + 1 >= self.allVirtualLines().len) {
            return;
        }
        const line = self.allVirtualLines()[pos.virtual_row + 1];
        const offset = self.byteOfNthCodepointOfVirtualLine(line, self.rightmost_cursor_codepoint);
        self.cursor = @min(line.begin + offset, line.end);
    }

    pub fn beginningOfLine(self: *Window) void {
        const pos = self.virtualCursorPos();
        const line = self.allRealLines()[pos.virtual_row];
        self.cursor = line.begin;
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn endOfLine(self: *Window) void {
        const pos = self.virtualCursorPos();
        const line = self.allRealLines()[pos.virtual_row];
        self.cursor = line.end;
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn removeRightCursor(self: *Window) void {
        if (self.cursor < self.buffer.items.len) {
            const n = self.bytesUntilNearestCodepointRight();
            for (0..n) |_| {
                _ = self.buffer.orderedRemove(self.cursor);
            }
        }
        self.reindex();
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn removeLeftCursor(self: *Window) void {
        if (self.cursor > 0) {
            const n = self.bytesUntilNearestCodepointLeft();
            for (0..n) |_| {
                _ = self.buffer.orderedRemove(self.cursor - 1);
                self.cursor -= 1;
            }
        }
        self.reindex();
        self.rightmost_cursor_codepoint = self.codepointsLeftOfCursor();
    }

    pub fn allRealLines(self: *Window) []const RealLine {
        return self.lines.items;
    }

    pub fn allVirtualLines(self: *Window) []const VirtualLine {
        return self.virtual_lines.items;
    }

    pub fn lineSlice(self: *Window, idx: usize) []const u8 {
        const line = self.lines.items[idx];
        return self.buffer.items[line.begin..line.end];
    }

    pub fn virtualLines(self: *Window, real_line: usize) []VirtualLine {
        const virtual_line_idx = self.lines.items[real_line].virtual_lines;
        return self.virtual_lines.items[virtual_line_idx.begin..virtual_line_idx.end];
    }

    pub fn virtualCursorPos(self: *Window) struct {
        virtual_row: usize,
        column: usize,
    } {
        const cursor = self.cursor;
        for (self.allRealLines()) |line| {
            if (cursor >= line.begin and cursor <= line.end) {
                const slice = self.virtual_lines.items[line.virtual_lines.begin..line.virtual_lines.end];
                for (slice, 0..) |virt_line, idx| {
                    if (cursor >= virt_line.begin and cursor <= virt_line.end) {
                        return .{
                            .virtual_row = line.virtual_lines.begin + idx,
                            .column = cursor - virt_line.begin,
                        };
                    }
                }
            }
        }
        return .{
            .virtual_row = 0,
            .column = 0,
        };
    }

    pub fn cursorDrawData(self: *Window) struct {
        virtual_row: f32,
        text_left_of_cursor: []const u8,
    } {
        const pos = self.virtualCursorPos();
        const line = self.virtual_lines.items[pos.virtual_row];
        return .{
            .virtual_row = @floatFromInt(pos.virtual_row),
            .text_left_of_cursor = self.buffer.items[line.begin .. line.begin + pos.column],
        };
    }
};

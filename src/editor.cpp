#include "editor.hpp"

#include <SDL3/SDL_render.h>

#include "array.hpp"
#include "fs.hpp"
#include "mem.hpp"
#include "strings.hpp"
#include "types.hpp"

// Internal
static auto Editor_Reindex(Editor* editor) -> void;
static auto Editor_Reindex_Real_Lines(Editor* editor) -> void;
static auto Editor_Reindex_Virtual_Lines(Editor* editor) -> void;

auto Create_Editor(Allocator base_allocator, Font* font) -> Editor {
    auto arena         = Create_Arena_Allocator(base_allocator);
    auto buffer        = Create_Array_Capacity<u8>(base_allocator, 1024);
    auto lines         = Create_Array_Capacity<Real_Line>(base_allocator, 128);
    auto virtual_lines = Create_Array_Capacity<Virtual_Line>(base_allocator, 128);
    Append(base_allocator, lines, {
                                      .begin         = 0,
                                      .end           = 0,
                                      .virtual_lines = {
                                          .begin = 0,
                                          .end   = 0,
                                      },
                                  });
    Append(base_allocator, virtual_lines, {
                                              .begin = 0,
                                              .end   = 0,
                                          });
    return {
        .arena                      = arena,
        .base_allocator             = base_allocator,
        .buffer                     = buffer,
        .lines                      = lines,
        .virtual_lines              = virtual_lines,
        .font                       = font,
        .cursor                     = 0,
        .active_file                = {0},
        .rightmost_cursor_codepoint = 0,
        .scroll_offset              = 0,
        .lines_on_screen            = 1,
    };
}

auto Destroy_Editor(Editor* editor) -> void {
    Destroy_Array(editor->base_allocator, editor->buffer);
    Destroy_Array(editor->base_allocator, editor->lines);
    Destroy_Array(editor->base_allocator, editor->virtual_lines);
    editor->arena.Destroy();
}

auto Editor_Open_File(Editor* editor, String filename) -> void {
    editor->active_file = editor->arena.Interface().DupeString(filename);
    auto file_contents  = Read_All_From_File_As_String(editor->base_allocator, editor->active_file);
    Append_Slice(editor->base_allocator, editor->buffer, file_contents.Slice());
    Editor_Reindex(editor);
}

auto Editor_Open_Source(Editor* editor, String source) -> void {
}

auto Editor_Save(Editor* editor) -> void {
}

auto Editor_Insert_Text(Editor* editor, String content) -> void {
}

auto Editor_Insert_Newline(Editor* editor) -> void {
}

auto Editor_Left(Editor* editor) -> void {
}

auto Editor_Right(Editor* editor) -> void {
}

auto Editor_Up(Editor* editor) -> void {
}

auto Editor_Down(Editor* editor) -> void {
}

auto Editor_Beginning_Of_Line(Editor* editor) -> void {
}

auto Editor_End_Of_Line(Editor* editor) -> void {
}

auto Editor_Remove_Left_Of_Cursor(Editor* editor) -> void {
}

auto Editor_Remove_Right_Of_Cursor(Editor* editor) -> void {
}

auto Editor_Virtual_Lines(Editor* editor, usize real_line) -> Slice<Virtual_Line> {
}

auto Editor_Virtual_Cursor_Position(Editor* editor, usize real_line) -> Virtual_Cursor {
}

auto Editor_Cursor_Draw_Data(Editor* editor) -> Cursor_Draw_Data {
}

// Internal
static auto Editor_Reindex(Editor* editor) -> void {
    Editor_Reindex_Real_Lines(editor);
    Editor_Reindex_Virtual_Lines(editor);
}

static auto Editor_Reindex_Real_Lines(Editor* editor) -> void {
    Clear(editor->lines);
    usize begin = 0;

    Array_Each(editor->buffer, c, idx, {
        if (c == '\n') {
            auto end = idx;
            Append(editor->base_allocator, editor->lines, {
                                                              .begin = begin,
                                                              .end   = end,
                                                          });
            begin = end + 1;
        }
    });

    Append(editor->base_allocator, editor->lines, {
                                                      .begin = begin,
                                                      .end   = editor->buffer.size,
                                                  });
}

static auto Editor_Reindex_Virtual_Lines(Editor* editor) -> void {
    Clear(editor->virtual_lines);

    const auto max_width = 800;  // TODO: no hardcoded

    Array_Each(editor->lines, line, idx, {
        auto  virtual_line_slice_begin = editor->virtual_lines.size;
        auto  line_slice               = editor->buffer.slice(0, line.end);
        auto  virtual_line_begin       = line.begin;
        auto  word_begin               = line.begin;
        usize i                        = 0;
        f32   x                        = 0;

        for (; i < line_slice.size; i += 1) {
            if (line_slice[i] != ' ') {
                continue;
            }
            i             = min(i + 1, line_slice.size);
            auto word     = line_slice.slice(word_begin, i);
            auto word_dim = Calculate_Text_Dimensions_With_Font(editor->font, word);
        }
    });

    /*
    for (self.allRealLines(), 0..) | line, idx | {
            const virtual_line_slice_begin = self.virtual_lines.items.len;
            const line_slice               = self.buffer.items[0..line.end];
            var virtual_line_begin : usize = line.begin;
            var word_begin : usize         = line.begin;
            var i : usize                  = line.begin;
            var x : f32                    = 0;
            while (i < line_slice.len) : (i += 1) {
                    if (line_slice[i] != ' ') {
                        continue;
                    }
                    i              = @min(i + 1, line_slice.len);
                    const word     = line_slice[word_begin..i];
                    const word_dim = rend.strdim(rend.body_font, word);
                    if (word_dim.w + x > max_width) {
                        try self.virtual_lines.append(self.base_allocator, .{
                                                                               .begin = virtual_line_begin,
                                                                               .end   = i - 1,
                                                                           });
                        x                  = 0;
                        word_begin         = i;
                        virtual_line_begin = i;
                    }

                    word_begin = i;
                    x += word_dim.w;
                }
            const word     = line_slice[word_begin..];
            const word_dim = rend.strdim(rend.body_font, word);
            if (word_dim.w + x > max_width) {
                try self.virtual_lines.append(self.base_allocator, .{
                                                                       .begin = virtual_line_begin,
                                                                       .end   = i,
                                                                   });
                x                  = 0;
                word_begin         = i;
                virtual_line_begin = i;
            }

            // only allow trailing empty virtual rows if the original row is empty
            if (line.begin == virtual_line_begin or virtual_line_begin != i) {
                try self.virtual_lines.append(self.base_allocator, .{ .begin = virtual_line_begin,
                                                                      .end   = @min(i, line_slice.len), });
            }

            const virtual_line_slice_end        = self.virtual_lines.items.len;
            self.lines.items[idx].virtual_lines =.{
                .begin = virtual_line_slice_begin,
                .end   = virtual_line_slice_end,
            };
        }
        */
}

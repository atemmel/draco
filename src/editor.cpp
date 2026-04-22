#include "editor.hpp"

#include <SDL3/SDL_render.h>

#include "array.hpp"
#include "fs.hpp"
#include "mem.hpp"
#include "strings.hpp"
#include "types.hpp"
#include "utf8.hpp"

// Internal
static auto Editor_Reindex(Editor* editor) -> void;
static auto Editor_Reindex_Real_Lines(Editor* editor) -> void;
static auto Editor_Reindex_Virtual_Lines(Editor* editor) -> void;
static auto Editor_Codepoints_Left_Of_Cursor(Editor* editor) -> usize;
static auto Editor_Bytes_Until_Nearest_Codepoint_Left(Editor* editor) -> usize;
static auto Editor_Bytes_Until_Nearest_Codepoint_Right(Editor* editor) -> usize;
static auto Editor_Byte_Of_Nth_Codepoint_Of_Virtual_Line(Editor* editor, Virtual_Line line, usize n) -> usize;

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
    auto file_contents  = Read_All_From_File_As_String(editor->arena.Interface(), editor->active_file);
    Append_Slice(editor->base_allocator, editor->buffer, file_contents.Slice());
    Editor_Reindex(editor);
}

auto Editor_Open_Source(Editor* editor, String source) -> void {
    Clear(editor->buffer);
    Append_Slice(editor->base_allocator, editor->buffer, source.Slice());
    Editor_Reindex(editor);
}

auto Editor_Save(Editor* editor) -> void {
}

auto Editor_Insert_Text(Editor* editor, String content) -> void {
    Insert_Slice(editor->base_allocator, editor->buffer, content.Slice(), editor->cursor);
    editor->cursor += content.size;
    Editor_Reindex(editor);
    editor->rightmost_cursor_codepoint = Editor_Codepoints_Left_Of_Cursor(editor);
}

auto Editor_Insert_Newline(Editor* editor) -> void {
    Insert(editor->base_allocator, editor->buffer, editor->cursor, u8('\n'));
    Editor_Reindex(editor);
    editor->cursor++;
    auto pos = Editor_Virtual_Cursor_Position(editor);
    if (pos.row >= editor->scroll_offset + editor->lines_on_screen) {
        editor->scroll_offset += 1;
    }
    editor->rightmost_cursor_codepoint = Editor_Codepoints_Left_Of_Cursor(editor);
}

auto Editor_Left(Editor* editor) -> void {
    if (editor->cursor <= 0) {
        return;
    }

    auto n = Editor_Bytes_Until_Nearest_Codepoint_Left(editor);
    editor->cursor -= n;
    editor->rightmost_cursor_codepoint = Editor_Codepoints_Left_Of_Cursor(editor);
}

auto Editor_Right(Editor* editor) -> void {
    if (editor->cursor >= editor->buffer.size) {
        return;
    }
    auto n = Editor_Bytes_Until_Nearest_Codepoint_Right(editor);
    editor->cursor += n;
    editor->rightmost_cursor_codepoint = Editor_Codepoints_Left_Of_Cursor(editor);
}

auto Editor_Up(Editor* editor) -> void {
    auto pos = Editor_Virtual_Cursor_Position(editor);
    if (pos.row <= 0) {
        return;
    }
    auto line      = editor->virtual_lines[pos.row - 1];
    auto offset    = Editor_Byte_Of_Nth_Codepoint_Of_Virtual_Line(editor, line, editor->rightmost_cursor_codepoint);
    editor->cursor = min(line.begin + offset, line.end);

    if (pos.row < editor->scroll_offset) {
        editor->scroll_offset--;
    }
}

auto Editor_Down(Editor* editor) -> void {
    auto pos = Editor_Virtual_Cursor_Position(editor);
    if (pos.row + 2 >= editor->scroll_offset + editor->lines_on_screen) {
        editor->scroll_offset += 1;
    }
    if (pos.row + 1 >= editor->virtual_lines.size) {
        return;
    }
    auto line      = editor->virtual_lines[pos.row + 1];
    auto offset    = Editor_Byte_Of_Nth_Codepoint_Of_Virtual_Line(editor, line, editor->rightmost_cursor_codepoint);
    editor->cursor = min(line.begin + offset, line.end);

    if (pos.row < editor->scroll_offset) {
        editor->scroll_offset--;
    }
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
    auto virtual_line_idx = editor->lines[real_line].virtual_lines;
    return editor->virtual_lines.slice(virtual_line_idx.begin, virtual_line_idx.end);
}

auto Editor_Virtual_Cursor_Position(Editor* editor) -> Virtual_Cursor {
    auto cursor = editor->cursor;
    for (auto line : editor->lines) {
        if (cursor >= line.begin && cursor <= line.end) {
            auto slice = editor->virtual_lines.slice(line.virtual_lines.begin, line.virtual_lines.end);
            for (usize idx = 0; idx < slice.size; idx++) {
                const auto virt_line = slice[idx];
                if (cursor >= virt_line.begin && cursor <= virt_line.end) {
                    return {
                        .row    = line.virtual_lines.begin + idx,
                        .column = cursor - virt_line.begin,
                    };
                }
            }
        }
    }
    return {
        .row    = 0,
        .column = 0,
    };
}

auto Editor_Cursor_Draw_Data(Editor* editor) -> Cursor_Draw_Data {
    auto pos  = Editor_Virtual_Cursor_Position(editor);
    auto line = editor->virtual_lines[pos.row];
    return {
        .virtual_row         = f32(pos.row),
        .text_left_of_cursor = editor->buffer.slice(line.begin, line.begin + pos.column),
    };
}

// Internal
static auto Editor_Reindex(Editor* editor) -> void {
    Editor_Reindex_Real_Lines(editor);
    Editor_Reindex_Virtual_Lines(editor);
}

static auto Editor_Reindex_Real_Lines(Editor* editor) -> void {
    Clear(editor->lines);
    usize begin = 0;

    for (usize idx = 0; idx < editor->buffer.size; idx++) {
        u8 c = editor->buffer[idx];
        if (c == '\n') {
            auto end = idx;
            Append(editor->base_allocator, editor->lines, {
                                                              .begin = begin,
                                                              .end   = end,
                                                          });
            begin = end + 1;
        }
    };

    Append(editor->base_allocator, editor->lines, {
                                                      .begin = begin,
                                                      .end   = editor->buffer.size,
                                                  });
}

static auto Editor_Reindex_Virtual_Lines(Editor* editor) -> void {
    Clear(editor->virtual_lines);

    const auto max_width = 800;  // TODO: no hardcoded

    for (usize idx = 0; idx < editor->lines.size; idx++) {
        const auto line                     = editor->lines[idx];
        auto       virtual_line_slice_begin = editor->virtual_lines.size;
        auto       line_slice               = editor->buffer.slice(0, line.end);
        auto       virtual_line_begin       = line.begin;
        auto       word_begin               = line.begin;
        usize      i                        = 0;
        f32        x                        = 0;

        for (; i < line_slice.size; i += 1) {
            if (line_slice[i] != ' ') {
                continue;
            }
            i             = min(i + 1, line_slice.size);
            auto word     = line_slice.slice(min(word_begin, i), max(word_begin, i));
            auto word_dim = Calculate_Text_Dimensions_With_Font(editor->font, word);
            if (word_dim.x + x > max_width) {
                Append(editor->base_allocator, editor->virtual_lines, {virtual_line_begin, i - 1});
                x                  = 0;
                word_begin         = i;
                virtual_line_begin = i;
            }

            word_begin = i;
            x += word_dim.x;
        }

        auto word     = line_slice.slice(word_begin);
        auto word_dim = Calculate_Text_Dimensions_With_Font(editor->font, word);
        if (word_dim.x + x > max_width) {
            Append(editor->base_allocator, editor->virtual_lines, {virtual_line_begin, i});
            x                  = 0;
            word_begin         = i;
            virtual_line_begin = i;
        }

        // only allow trailing empty virtual rows if the original row is empty
        if (line.begin == virtual_line_begin || virtual_line_begin != i) {
            Append(editor->base_allocator, editor->virtual_lines, {virtual_line_begin, min(i, line_slice.size)});
        }

        auto virtual_line_slice_end      = editor->virtual_lines.size;
        editor->lines[idx].virtual_lines = {virtual_line_slice_begin, virtual_line_slice_end};
    }
}

static auto Editor_Codepoints_Left_Of_Cursor(Editor* editor) -> usize {
    auto pos    = Editor_Virtual_Cursor_Position(editor);
    auto line   = editor->virtual_lines[pos.row];
    auto slice  = editor->buffer.slice(line.begin, min(editor->buffer.size, editor->cursor));
    auto length = Utf8_Length(slice);
    return length != -1 ? length : editor->cursor - line.begin;
}

const static u8 utf_1_mask = 0b10000000;
const static u8 utf_2_mask = 0b11000000;
const static u8 utf_3_mask = 0b11100000;
const static u8 utf_4_mask = 0b11110000;

static auto Editor_Bytes_Until_Nearest_Codepoint_Left(Editor* editor) -> usize {
    if (editor->cursor < 1) {
        return 0;
    }
    auto offset = editor->cursor + 1 >= editor->buffer.size ? 1 : 0;
    if (editor->cursor < 2 || (editor->buffer[editor->cursor - 1 - offset] & utf_1_mask) == 0) {
        return 1;
    }
    if (editor->cursor < 3 || editor->buffer[editor->cursor - 2 - offset] & utf_2_mask) {
        return 2;
    }
    if (editor->cursor < 4 || editor->buffer[editor->cursor - 3 - offset] & utf_3_mask) {
        return 3;
    }
    return 4;
}

static auto Editor_Bytes_Until_Nearest_Codepoint_Right(Editor* editor) -> usize {
    auto last_byte_index = max(editor->buffer.size, usize(1)) - 1;
    // cursor can be at buffer.size, that's fine
    if (editor->cursor >= last_byte_index + 1) {
        return 0;
    }

    auto b = editor->buffer[editor->cursor];
    if (b <= 0x7F || editor->cursor + 1 == last_byte_index) {
        return 1;
    }
    if ((utf_2_mask & b) != 0 || editor->cursor + 2 == last_byte_index) {
        return 2;
    }
    if ((utf_3_mask & b) != 0 || editor->cursor + 3 == last_byte_index) {
        return 3;
    }
    return 4;
}

static auto Editor_Byte_Of_Nth_Codepoint_Of_Virtual_Line(Editor* editor, Virtual_Line line, usize n) -> usize {
    auto slice = editor->buffer.slice(line.begin, line.end);

    usize i = 0;

    for (usize codepoints = 0; i < slice.size && codepoints < n; ++codepoints) {
        if (auto c = Utf8_Length(slice[i]); i != -1) {
            i += c;
        } else {
            break;
        }
    }

    return i;
}

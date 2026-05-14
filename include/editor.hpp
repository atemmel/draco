#pragma once

#include "array.hpp"
#include "fonts.hpp"
#include "math.hpp"
#include "mem.hpp"
#include "slice.hpp"
#include "strings.hpp"

struct Virtual_Line {
    s64 begin, end;
};

struct Virtual_Line_Idx {
    s64 begin, end;
};

struct Real_Line {
    s64              begin;
    s64              end;
    Virtual_Line_Idx virtual_lines;
};

struct Virtual_Cursor {
    s64 row, column;
};

struct Cursor_Draw_Data {
    f32       virtual_row;
    Slice<u8> text_left_of_cursor;
};

struct Editor {
    Arena_Allocator     arena;
    Allocator           base_allocator;
    Array<u8>           buffer;
    Array<Real_Line>    lines;
    Array<Virtual_Line> virtual_lines;
    Font*               font;
    s64                 cursor;
    String              active_file;
    s64                 rightmost_cursor_codepoint;
    s64                 scroll_offset;
    s64                 lines_on_screen;
    f32                 width;
    f32                 height;
};

auto Create_Editor(Allocator base_allocator, Font* font, Vec2 size) -> Editor;
auto Destroy_Editor(Editor* editor) -> void;

auto Editor_Open_File(Editor* editor, String filename) -> void;
auto Editor_Open_Source(Editor* editor, String source) -> void;
auto Editor_Save(Editor* editor) -> void;

auto Editor_Insert_Text(Editor* editor, String content) -> void;
auto Editor_Insert_Newline(Editor* editor) -> void;
auto Editor_Left(Editor* editor) -> void;
auto Editor_Right(Editor* editor) -> void;
auto Editor_Up(Editor* editor) -> void;
auto Editor_Down(Editor* editor) -> void;
auto Editor_Beginning_Of_Line(Editor* editor) -> void;
auto Editor_End_Of_Line(Editor* editor) -> void;
auto Editor_Remove_Left_Of_Cursor(Editor* editor) -> void;
auto Editor_Remove_Right_Of_Cursor(Editor* editor) -> void;
auto Editor_Resize(Editor* editor, Vec2 new_size) -> void;
auto Editor_Font_Size_Changed(Editor* editor) -> void;

auto Editor_Virtual_Lines(Editor* editor, s64 real_line) -> Slice<Virtual_Line>;
auto Editor_Virtual_Cursor_Position(Editor* editor) -> Virtual_Cursor;
auto Editor_Cursor_Draw_Data(Editor* editor) -> Cursor_Draw_Data;

#pragma once

#include "strings.hpp"

// TODO: figure out a way to represent errors
auto Read_All_From_File_As_String(Allocator allocator, String path) -> String;

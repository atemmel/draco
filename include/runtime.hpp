#pragma once

auto Trigger_Breakpoint() -> void;

#ifndef NDEBUG
#define Assert(x) _Assert_Impl(x)
#else
#define Assert(x)
#endif

auto _Assert_Impl(bool condition) -> void;

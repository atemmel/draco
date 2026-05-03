#include "runtime.hpp"

#include <cstdio>
#include <cstdlib>

#ifndef NDEBUG
#include <stacktrace>
#include <string>
#endif

#ifdef WIN32
#else
#include <signal.h>
#endif

auto Trigger_Breakpoint() -> void {
#ifdef WIN32
    __debugbreak();
#else
    raise(SIGABRT);
#endif
}

auto _Assert_Impl(bool condition) -> void {
#ifndef NDEBUG
    if (!condition) {
        auto stacktrace = std::to_string(std::stacktrace::current());
        fprintf(stderr, "Assertion failed, current stacktrace:\n%s", stacktrace.data());
        Trigger_Breakpoint();
    }
#endif
}

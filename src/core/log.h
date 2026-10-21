// Logging shared by the game and the tools. The core library has no SDL dependency; the game
// installs a fatal-error hook so errors can be shown in a message box.
#pragma once
#include <cstdarg>
#include <cstdio>

// fopen for a UTF-8 path (plain fopen on Windows reads the system code page).
FILE* OpenFileUtf8(const char* path, const char* mode);

void LogInit(const char* path);      // nullptr: log to stderr only
void LogSetEcho(bool echoToStderr);  // also print each line to stderr (tools)
void Log(const char* fmt, ...)
#if defined(__GNUC__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

using FatalHook = void (*)(const char* title, const char* message);
void SetFatalHook(FatalHook hook);
[[noreturn]] void FatalError(const char* title, const char* fmt, ...);

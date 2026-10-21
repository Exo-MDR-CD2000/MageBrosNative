#include "log.h"
#include <cstdlib>
#include <string>
#ifdef _WIN32
#include <windows.h>
#endif

namespace {
FILE* g_log = nullptr;
bool g_echo = false;
FatalHook g_fatalHook = nullptr;
}  // namespace

FILE* OpenFileUtf8(const char* path, const char* mode) {
#ifdef _WIN32
    auto widen = [](const char* s) {
        int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
        std::wstring w(n > 0 ? n : 1, L'\0');
        if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], n);
        return w;
    };
    return _wfopen(widen(path).c_str(), widen(mode).c_str());
#else
    return std::fopen(path, mode);
#endif
}

void LogInit(const char* path) {
    if (g_log) std::fclose(g_log);
    g_log = path ? OpenFileUtf8(path, "w") : nullptr;
}

void LogSetEcho(bool echo) { g_echo = echo; }

static void LogV(const char* fmt, va_list ap) {
    char buf[2048];
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    if (g_log) {
        std::fprintf(g_log, "%s\n", buf);
        std::fflush(g_log);
    }
    if (g_echo || !g_log) std::fprintf(stderr, "%s\n", buf);
}

void Log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    LogV(fmt, ap);
    va_end(ap);
}

void SetFatalHook(FatalHook hook) { g_fatalHook = hook; }

void FatalError(const char* title, const char* fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    Log("FATAL: %s: %s", title, buf);
    if (g_fatalHook) g_fatalHook(title, buf);
    std::exit(1);
}

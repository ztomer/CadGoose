#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

LogLevel g_logLevel = LogLevel::Info;

void Log_InitLevel(bool debugMode) {
    if (const char* v = std::getenv("CADGOOSE_VERBOSE")) {
        if (*v && std::strcmp(v, "0") != 0) {
            g_logLevel = LogLevel::Debug;
            return;
        }
    }
    g_logLevel = debugMode ? LogLevel::Debug : LogLevel::Info;
}

void CG_Logv(LogLevel level, const char* tag, const char* fmt, ...) {
    if (static_cast<int>(level) > static_cast<int>(g_logLevel)) return;
    fprintf(stderr, "[%s] ", tag);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

#include "log.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <atomic>
#include <chrono>

#include "config.h"
#include "cursor_backend.h"
#include "world.h"
#include "actor.h"
#include "goose.h"

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

// --- Async logging implementation ---

struct LogEntry {
    LogLevel level;
    std::string tag;
    std::string message;
};

static std::queue<LogEntry> g_logQueue;
static std::mutex g_logQueueMutex;
static std::condition_variable g_logQueueCV;
static std::thread g_logThread;
static std::atomic<bool> g_logRunning{false};
static FILE* g_logFile = nullptr;
static LogLevel g_asyncMinLevel = LogLevel::Debug;

static void LogThreadMain() {
    while (g_logRunning) {
        LogEntry entry;
        {
            std::unique_lock<std::mutex> lock(g_logQueueMutex);
            g_logQueueCV.wait_for(lock, std::chrono::milliseconds(100), [] {
                return !g_logQueue.empty() || !g_logRunning;
            });
            if (!g_logQueue.empty()) {
                entry = std::move(g_logQueue.front());
                g_logQueue.pop();
            } else {
                continue;
            }
        }
        if (g_logFile && static_cast<int>(entry.level) <= static_cast<int>(g_asyncMinLevel)) {
            fprintf(g_logFile, "[%s] %s\n", entry.tag.c_str(), entry.message.c_str());
            fflush(g_logFile);
        }
    }
    // Flush remaining
    while (!g_logQueue.empty()) {
        auto entry = std::move(g_logQueue.front());
        g_logQueue.pop();
        if (g_logFile && static_cast<int>(entry.level) <= static_cast<int>(g_asyncMinLevel)) {
            fprintf(g_logFile, "[%s] %s\n", entry.tag.c_str(), entry.message.c_str());
        }
    }
    if (g_logFile) {
        fflush(g_logFile);
    }
}

void Log_EnableFile(const char* path, LogLevel minLevel) {
    if (g_logRunning) return;
    g_logFile = fopen(path, "w");
    if (!g_logFile) return;
    g_asyncMinLevel = minLevel;
    g_logRunning = true;
    g_logThread = std::thread(LogThreadMain);
}

void Log_Shutdown() {
    if (!g_logRunning) return;
    g_logRunning = false;
    g_logQueueCV.notify_all();
    if (g_logThread.joinable()) {
        g_logThread.join();
    }
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = nullptr;
    }
}

void CG_LogvAsync(LogLevel level, const char* tag, const char* fmt, ...) {
    if (static_cast<int>(level) > static_cast<int>(g_asyncMinLevel)) return;
    if (!g_logRunning) return;

    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    LogEntry entry;
    entry.level = level;
    entry.tag = tag;
    entry.message = buffer;

    {
        std::lock_guard<std::mutex> lock(g_logQueueMutex);
        g_logQueue.push(std::move(entry));
    }
    g_logQueueCV.notify_one();
}

// --- Centralized debug logging API (DRY) ---
// Compiled out entirely in release builds — call sites become ((void)0) via the
// macros in log.h, so these definitions are unreferenced and need not exist.
#ifndef CG_DISABLE_DEBUG_LOG
extern bool g_debugMode;  // from main.mm

void DebugLog(const char* fmt, ...) {
    if (!g_config.debug.toTerminal) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    CG_LogvAsync(LogLevel::Debug, "DEBUG", "%s", buffer);
}

void DebugLogv(LogLevel level, const char* tag, const char* fmt, ...) {
    if (!g_config.debug.toTerminal) return;
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    CG_LogvAsync(level, tag, "%s", buffer);
}

void LogTick(double time, const struct CursorState& cursor) {
    if (!g_config.debug.toTerminal) return;
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer), "[T%.1f] cur=%d", time, g_world.cursorGrabberId);
    if (cursor.hasPos())
        len += snprintf(buffer + len, sizeof(buffer) - len, " c(%.0f,%.0f)", cursor.position.x, cursor.position.y);
    else
        len += snprintf(buffer + len, sizeof(buffer) - len, " c(-,-)");
    len += snprintf(buffer + len, sizeof(buffer) - len, " geese:");
    for (auto* g : ActorManager::Instance().getGeese()) {
        static constexpr const char* stateNames[] = {"W", "F", "R", "C", "S"};
        int si = static_cast<int>(g->state);
        const char* sn = (si >= 0 && si < (int)(sizeof(stateNames)/sizeof(stateNames[0]))) ? stateNames[si] : "?";
        len += snprintf(buffer + len, sizeof(buffer) - len, " %d%s@(%d,%d)d%dv(%d,%d)s%d",
                        g->id, sn, (int)g->pos.x, (int)g->pos.y, (int)g->dir,
                        (int)g->vel.x, (int)g->vel.y, (int)g->currentSpeed);
        if (g->state == GooseState::SNATCH_CURSOR)
            len += snprintf(buffer + len, sizeof(buffer) - len, " a=%.1f r=%.0f", g->snatchAngle, g->snatchRadius);
    }
    CG_LogvAsync(LogLevel::Debug, "GOOSE", "%s", buffer);
}
#endif  // CG_DISABLE_DEBUG_LOG
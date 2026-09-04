#include "crash_logger.h"
#include "config.h"

#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <fcntl.h>
#include <execinfo.h>
#include <filesystem>

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

namespace fs = std::filesystem;

namespace {

// Full path to the crash log for this run. Computed once at init so the signal
// handler does no path formatting (which is not async-signal-safe).
char g_crashFilePath[1024] = {0};
char g_logDir[1024] = {0};
bool g_installed = false;

// Signals we treat as crashes and capture a backtrace for.
const int kCrashSignals[] = { SIGSEGV, SIGABRT, SIGBUS, SIGILL, SIGFPE, SIGTRAP };

const char* SignalName(int sig) {
    switch (sig) {
        case SIGSEGV: return "SIGSEGV (segmentation fault)";
        case SIGABRT: return "SIGABRT (abort)";
        case SIGBUS:  return "SIGBUS (bus error)";
        case SIGILL:  return "SIGILL (illegal instruction)";
        case SIGFPE:  return "SIGFPE (floating point exception)";
        case SIGTRAP: return "SIGTRAP (trap)";
        default:      return "unknown signal";
    }
}

// Async-signal-safe-ish crash handler: capture a backtrace and write it to the
// pre-computed crash file, then restore the default handler and re-raise so the
// OS still produces its own crash report.
void HandleCrashSignal(int sig) {
    void* frames[64];
    int n = backtrace(frames, 64);

    int fd = open(g_crashFilePath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd >= 0) {
        const char* hdr = "=== CadGoose crash ===\nsignal: ";
        write(fd, hdr, strlen(hdr));
        const char* name = SignalName(sig);
        write(fd, name, strlen(name));
        const char* btHdr = "\nbacktrace:\n";
        write(fd, btHdr, strlen(btHdr));
        backtrace_symbols_fd(frames, n, fd);
        close(fd);
    }

    // Also dump to stderr (captured in the session log when redirected).
    const char* msg = "\n[CRASH] CadGoose crashed — backtrace written to log dir\n";
    write(STDERR_FILENO, msg, strlen(msg));
    backtrace_symbols_fd(frames, n, STDERR_FILENO);

    signal(sig, SIG_DFL);
    raise(sig);
}

#if defined(__APPLE__)
void HandleUncaughtException(NSException* exception) {
    FILE* f = fopen(g_crashFilePath, "w");
    if (f) {
        fprintf(f, "=== CadGoose uncaught exception ===\n");
        fprintf(f, "name: %s\n", exception.name.UTF8String ?: "?");
        fprintf(f, "reason: %s\n", exception.reason.UTF8String ?: "?");
        fprintf(f, "backtrace:\n");
        for (NSString* frame in exception.callStackSymbols) {
            fprintf(f, "%s\n", frame.UTF8String);
        }
        fclose(f);
    }
    fprintf(stderr, "[CRASH] Uncaught exception: %s — %s\n",
            exception.name.UTF8String ?: "?", exception.reason.UTF8String ?: "?");
}
#endif

// If stderr isn't a terminal (launched from the bundle / Finder), redirect it
// to a per-run session log so the app's fprintf(stderr, ...) diagnostics are
// preserved. When run from a terminal, leave stderr alone for live dev output.
void RedirectStderrIfHeadless(const fs::path& logDir, const char* stamp) {
    if (isatty(STDERR_FILENO)) return;
    if (getenv("CI") || getenv("GITHUB_ACTIONS")) return; // Do not redirect in CI
    fs::path sessionLog = logDir / (std::string("session-") + stamp + ".log");
    FILE* f = freopen(sessionLog.string().c_str(), "w", stderr);
    if (f) {
        setvbuf(stderr, nullptr, _IOLBF, 0); // line-buffered so logs survive a crash
    }
}

} // namespace

void CrashLogger_Init() {
    if (g_installed) return;
    g_installed = true;

    std::error_code ec;
    fs::path logDir = ConfigDirPath() / "logs";
    fs::create_directories(logDir, ec);
    strncpy(g_logDir, logDir.string().c_str(), sizeof(g_logDir) - 1);

    time_t now = time(nullptr);
    struct tm tmv;
    localtime_r(&now, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmv);

    fs::path crashFile = logDir / (std::string("crash-") + stamp + ".log");
    strncpy(g_crashFilePath, crashFile.string().c_str(), sizeof(g_crashFilePath) - 1);

    RedirectStderrIfHeadless(logDir, stamp);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = HandleCrashSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    for (int sig : kCrashSignals) {
        sigaction(sig, &sa, nullptr);
    }

#if defined(__APPLE__)
    NSSetUncaughtExceptionHandler(&HandleUncaughtException);
#endif

    fprintf(stderr, "[CRASH] Crash logger active — logs in %s\n", g_logDir);
}

const char* CrashLogger_LogDir() {
    return g_logDir;
}

#pragma once

// Unified leveled logging. Replaces scattered fprintf(stderr, "[TAG] …") calls
// and the duplicated LOG/DEBUG_LOG macros. Output goes to stderr (captured into
// the session log by crash_logger when launched headless).
//
//   CG_ERROR("AI", "no answer: %d", code);   // always shown
//   CG_DEBUG("LOCAL_LLM", "scanned %d", n);  // only when g_logLevel >= Debug
//
// g_logLevel defaults to Info (quiet release); set to Debug via --debug or the
// CADGOOSE_VERBOSE env var (see Log_InitLevel).

enum class LogLevel { Error = 0, Warn = 1, Info = 2, Debug = 3 };

extern LogLevel g_logLevel;

// Set g_logLevel from debugMode / the CADGOOSE_VERBOSE env var. Call once at start.
void Log_InitLevel(bool debugMode);

// Implementation; prefer the macros below. Adds the "[tag] " prefix and newline.
void CG_Logv(LogLevel level, const char* tag, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

#define CG_ERROR(tag, ...) CG_Logv(LogLevel::Error, tag, __VA_ARGS__)
#define CG_WARN(tag, ...)  CG_Logv(LogLevel::Warn,  tag, __VA_ARGS__)
#define CG_INFO(tag, ...)  CG_Logv(LogLevel::Info,  tag, __VA_ARGS__)

// --- Async logging engine for hot paths (e.g., per-frame debug) ---
// Call once at startup to enable file logging with background thread.
void Log_EnableFile(const char* path, LogLevel minLevel = LogLevel::Debug);
// Shutdown and flush (call at exit).
void Log_Shutdown();
// Async version - never blocks calling thread. Use for per-frame logging.
void CG_LogvAsync(LogLevel level, const char* tag, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));

// --- Debug printouts (compiled out in release builds) ---
// Defining CG_DISABLE_DEBUG_LOG (build_release.sh / CI) expands every debug-level
// log call to ((void)0): no function call, no argument evaluation, no string
// formatting — zero overhead in the shipped binary. The error/warn/info macros
// above are unaffected. Debug builds (build_debug.sh) leave these fully enabled.
#if defined(CG_DISABLE_DEBUG_LOG)
  #define CG_DEBUG(tag, ...)       ((void)0)
  #define CG_DEBUG_ASYNC(tag, ...) ((void)0)
  #define DebugLog(...)            ((void)0)
  #define DebugLogv(...)           ((void)0)
  #define LogTick(...)             ((void)0)
#else
  #define CG_DEBUG(tag, ...)       CG_Logv(LogLevel::Debug, tag, __VA_ARGS__)
  #define CG_DEBUG_ASYNC(tag, ...) CG_LogvAsync(LogLevel::Debug, tag, __VA_ARGS__)
  // Centralized debug logging API (DRY). Replaces per-file GetDebugLog() / FILE*
  // statics. Guards on g_config.debug.toTerminal internally.
  void DebugLog(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
  void DebugLogv(LogLevel level, const char* tag, const char* fmt, ...) __attribute__((format(printf, 3, 4)));
  void LogTick(double time, const struct CursorState& cursor);
#endif

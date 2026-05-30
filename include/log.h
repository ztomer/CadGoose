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
#define CG_DEBUG(tag, ...) CG_Logv(LogLevel::Debug, tag, __VA_ARGS__)

#pragma once

// Best-effort crash + log capture for end-user machines.
//
// CrashLogger_Init() installs signal and uncaught-exception handlers that
// write a backtrace to <ConfigDir>/logs/crash-<timestamp>.log, and — when the
// process is NOT attached to a terminal (i.e. launched from the .app bundle /
// Finder, where stderr would otherwise be discarded) — redirects stderr to
// <ConfigDir>/logs/session-<timestamp>.log so the normal diagnostic output is
// preserved for bug reports.
//
// Safe to call once, early in main(). No-op on platforms without support.
void CrashLogger_Init();

// Absolute path to the directory where crash/session logs are written.
// Valid after CrashLogger_Init(); returns "" before.
const char* CrashLogger_LogDir();

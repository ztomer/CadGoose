#pragma once

// Single-instance guard for the GUI goose process.
//
// Acquires a process-lifetime exclusive advisory lock (flock on a per-user
// lock file). Returns true if this process now owns the lock — or if locking
// is unavailable, in which case we fail open rather than block startup.
// Returns false only when another live instance already holds the lock.
//
// The lock is released automatically when the process exits, including on a
// crash, because the kernel drops the flock when the file descriptor closes.
// There is intentionally no release function: the lock is held for the whole
// lifetime of the process.
bool SingleInstance_Acquire();

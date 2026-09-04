#pragma once
#include <string>

// Lock-free cache of resolved hotkey key codes.
//
// The user-rebindable hotkey *strings* live in g_config (so they serialize to
// TOML). But they are read every frame on the main thread by the behavior ticks
// (honcker/jail/portal/breadcrumbs), and they can be rewritten off the main
// thread by the MCP server. Reading a std::string while another thread
// reassigns it is a data race, so instead of locking the per-frame path we
// cache each hotkey's resolved key code in a std::atomic<int>:
//
//   * hot path  -> Hotkey_KeyCode(id)   lock-free atomic read, no string parse
//   * cold path -> Hotkey_Name/SetName  std::string access under a mutex
//
// Call Hotkey_SyncFromConfig() once after config load and from OnConfigChange()
// so the cache tracks the canonical g_config strings.

enum class HotkeyId {
    HonckerHonk,
    JailSet,
    JailToggle,
    Portal1,
    Portal2,
    Portal0,
    Breadcrumbs,
    Count
};

// Hot path: cached macOS kVK key code for this hotkey (-1 if unknown/unset).
// Lock-free; safe to call every frame.
int Hotkey_KeyCode(HotkeyId id);

// Cold path: current key-name string (e.g. "p"). Thread-safe.
std::string Hotkey_Name(HotkeyId id);

// Cold path: set the key-name string (in g_config) and refresh the cached key
// code atomically. Thread-safe; use from MCP set_hotkey and GUI edits.
void Hotkey_SetName(HotkeyId id, const std::string& name);

// Recompute every cached key code from the current g_config strings. Call after
// config load/reload and from OnConfigChange().
void Hotkey_SyncFromConfig();

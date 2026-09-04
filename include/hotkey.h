#pragma once
#include <string>
#include <optional>

struct ParsedHotkey {
    int keyCode = 0;
    int modifierMask = 0;
};

// Parse "cmd+shift+p" -> ParsedHotkey
// Returns nullopt on failure (logs to stderr)
std::optional<ParsedHotkey> ParseHotkeyString(const std::string& hotkey);

// Reverse: {keyCode, modifierMask} -> "cmd+shift+p"
std::string FormatHotkeyString(int keyCode, int modifierMask);

// Key name -> macOS kVK key code ("f" -> 0x03). Returns -1 if unknown.
int KeyNameToKeyCode(const std::string& name);

// macOS kVK key code -> display name (0x03 -> "f"). Returns "0x%X" for unknown.
std::string KeyCodeToDisplayString(int keyCode);

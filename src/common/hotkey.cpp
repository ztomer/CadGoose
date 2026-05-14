#include "hotkey.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cctype>
#include <CoreGraphics/CGEventSource.h>

struct KeyMap { const char* name; int code; };
struct ModMap { const char* name; int flag; };

static const KeyMap s_keyNames[] = {
    {"a", 0x00}, {"b", 0x0B}, {"c", 0x08}, {"d", 0x02}, {"e", 0x0E},
    {"f", 0x03}, {"g", 0x05}, {"h", 0x04}, {"i", 0x22}, {"j", 0x26},
    {"k", 0x28}, {"l", 0x25}, {"m", 0x2E}, {"n", 0x2D}, {"o", 0x1F},
    {"p", 0x23}, {"q", 0x0C}, {"r", 0x0F}, {"s", 0x01}, {"t", 0x11},
    {"u", 0x20}, {"v", 0x09}, {"w", 0x0D}, {"x", 0x07}, {"y", 0x10},
    {"z", 0x06},
    {"0", 0x1D}, {"1", 0x12}, {"2", 0x13}, {"3", 0x14}, {"4", 0x15},
    {"5", 0x17}, {"6", 0x16}, {"7", 0x1A}, {"8", 0x1C}, {"9", 0x19},
    {"escape", 0x35},
    {"tab", 0x30},
    {"space", 0x31},
    {"return", 0x24},
    {"delete", 0x33},
    {"left shift", 56},
    {"right shift", 60},
    {"up arrow", 0x7E},
    {"down arrow", 0x7D},
    {"left arrow", 0x7B},
    {"right arrow", 0x7C},
};

static const ModMap s_modifiers[] = {
    {"cmd", kCGEventFlagMaskCommand},
    {"command", kCGEventFlagMaskCommand},
    {"alt", kCGEventFlagMaskAlternate},
    {"option", kCGEventFlagMaskAlternate},
    {"opt", kCGEventFlagMaskAlternate},
    {"ctrl", kCGEventFlagMaskControl},
    {"control", kCGEventFlagMaskControl},
    {"shift", kCGEventFlagMaskShift},
    {"fn", kCGEventFlagMaskSecondaryFn},
};

static std::string Lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static std::string Trim(std::string s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

int KeyNameToKeyCode(const std::string& name) {
    std::string lower = Lower(Trim(name));
    for (const auto& m : s_keyNames) {
        if (lower == m.name) return m.code;
    }
    return -1;
}

static int ModifierNameToFlag(const std::string& name) {
    std::string lower = Lower(Trim(name));
    for (const auto& m : s_modifiers) {
        if (lower == m.name) return m.flag;
    }
    return 0;
}

std::optional<ParsedHotkey> ParseHotkeyString(const std::string& hotkey) {
    std::string input = Trim(hotkey);
    if (input.empty()) {
        fprintf(stderr, "[Hotkey] Empty hotkey string\n");
        return std::nullopt;
    }

    ParsedHotkey result;
    size_t start = 0;
    int keyTokens = 0;

    while (start < input.length()) {
        size_t plus = input.find('+', start);
        std::string token = Lower(Trim(input.substr(start, plus - start)));
        if (token.empty()) {
            fprintf(stderr, "[Hotkey] Empty token in \"%s\"\n", hotkey.c_str());
            return std::nullopt;
        }

        int modFlag = ModifierNameToFlag(token);
        if (modFlag) {
            result.modifierMask |= modFlag;
        } else {
            int code = KeyNameToKeyCode(token);
            if (code < 0) {
                // Try single ASCII letter/digit
                if (token.length() == 1) {
                    char c = token[0];
                    if (c >= 'a' && c <= 'z') {
                        // Map a-z to ANSI kVK: a=0x00, b=0x0B, etc.
                        // Use a simple linear search since it's small
                        for (const auto& m : s_keyNames) {
                            if (token == m.name && strlen(m.name) == 1 && m.name[0] >= 'a' && m.name[0] <= 'z') {
                                code = m.code;
                                break;
                            }
                        }
                        // If still not found, try direct: 'a' -> 0x00 via offset
                        // kVK mapping is not alphabetical, so we need the table
                        // Let's just use the table above
                    } else if (c >= '0' && c <= '9') {
                        for (const auto& m : s_keyNames) {
                            if (token == m.name) {
                                code = m.code;
                                break;
                            }
                        }
                    }
                }
                if (code < 0) {
                    fprintf(stderr, "[Hotkey] Unknown key \"%s\" in \"%s\"\n", token.c_str(), hotkey.c_str());
                    return std::nullopt;
                }
            }
            result.keyCode = code;
            keyTokens++;
        }

        if (plus == std::string::npos) break;
        start = plus + 1;
    }

    if (keyTokens != 1) {
        fprintf(stderr, "[Hotkey] Expected 1 key, got %d in \"%s\"\n", keyTokens, hotkey.c_str());
        return std::nullopt;
    }

    return result;
}

std::string FormatHotkeyString(int keyCode, int modifierMask) {
    std::string result;

    struct { int flag; const char* name; } modOrder[] = {
        {kCGEventFlagMaskControl, "ctrl"},
        {kCGEventFlagMaskAlternate, "alt"},
        {kCGEventFlagMaskShift, "shift"},
        {kCGEventFlagMaskCommand, "cmd"},
        {kCGEventFlagMaskSecondaryFn, "fn"},
    };

    for (const auto& m : modOrder) {
        if (modifierMask & m.flag) {
            if (!result.empty()) result += "+";
            result += m.name;
        }
    }

    std::string keyName = KeyCodeToDisplayString(keyCode);
    if (!result.empty()) result += "+";
    result += keyName;

    return result;
}

std::string KeyCodeToDisplayString(int keyCode) {
    for (const auto& m : s_keyNames) {
        if (m.code == keyCode) return m.name;
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%X", keyCode);
    return buf;
}

#include <gtest/gtest.h>
#include <CoreGraphics/CoreGraphics.h>
#include "hotkey.h"

TEST(Hotkey, KeyNameToKeyCode) {
    EXPECT_EQ(KeyNameToKeyCode("a"), 0x00);
    EXPECT_EQ(KeyNameToKeyCode("b"), 0x0B);
    EXPECT_EQ(KeyNameToKeyCode("z"), 0x06);
    EXPECT_EQ(KeyNameToKeyCode("0"), 0x1D);
    EXPECT_EQ(KeyNameToKeyCode("1"), 0x12);
    EXPECT_EQ(KeyNameToKeyCode("9"), 0x19);
    EXPECT_EQ(KeyNameToKeyCode("escape"), 0x35);
    EXPECT_EQ(KeyNameToKeyCode("tab"), 0x30);
    EXPECT_EQ(KeyNameToKeyCode("space"), 0x31);
    EXPECT_EQ(KeyNameToKeyCode("return"), 0x24);
    EXPECT_EQ(KeyNameToKeyCode("delete"), 0x33);
    EXPECT_EQ(KeyNameToKeyCode("left shift"), 56);
    EXPECT_EQ(KeyNameToKeyCode("right shift"), 60);
    EXPECT_EQ(KeyNameToKeyCode("up arrow"), 0x7E);
    EXPECT_EQ(KeyNameToKeyCode("down arrow"), 0x7D);
    EXPECT_EQ(KeyNameToKeyCode("left arrow"), 0x7B);
    EXPECT_EQ(KeyNameToKeyCode("right arrow"), 0x7C);
}

TEST(Hotkey, KeyNameToKeyCodeCaseInsensitive) {
    EXPECT_EQ(KeyNameToKeyCode("A"), 0x00);
    EXPECT_EQ(KeyNameToKeyCode("Escape"), 0x35);
    EXPECT_EQ(KeyNameToKeyCode("RIGHT SHIFT"), 60);
    EXPECT_EQ(KeyNameToKeyCode("Left Arrow"), 0x7B);
}

TEST(Hotkey, KeyNameToKeyCodeTrimmed) {
    EXPECT_EQ(KeyNameToKeyCode("  f  "), 0x03);
    EXPECT_EQ(KeyNameToKeyCode("\tspace\n"), 0x31);
}

TEST(Hotkey, KeyNameToKeyCodeUnknown) {
    EXPECT_EQ(KeyNameToKeyCode(""), -1);
    EXPECT_EQ(KeyNameToKeyCode("nonexistent"), -1);
    EXPECT_EQ(KeyNameToKeyCode("f1"), -1);
}

TEST(Hotkey, KeyCodeToDisplayString) {
    EXPECT_EQ(KeyCodeToDisplayString(0x00), "a");
    EXPECT_EQ(KeyCodeToDisplayString(0x0B), "b");
    EXPECT_EQ(KeyCodeToDisplayString(0x35), "escape");
    EXPECT_EQ(KeyCodeToDisplayString(0x31), "space");
    EXPECT_EQ(KeyCodeToDisplayString(60), "right shift");
    EXPECT_EQ(KeyCodeToDisplayString(56), "left shift");
    EXPECT_EQ(KeyCodeToDisplayString(0x12), "1");
    EXPECT_EQ(KeyCodeToDisplayString(0x7E), "up arrow");
}

TEST(Hotkey, KeyCodeToDisplayStringUnknown) {
    std::string result = KeyCodeToDisplayString(0xFF);
    EXPECT_EQ(result, "0xFF");
}

TEST(Hotkey, ParseHotkeyStringSimple) {
    auto parsed = ParseHotkeyString("f");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keyCode, 0x03);
    EXPECT_EQ(parsed->modifierMask, 0);
}

TEST(Hotkey, ParseHotkeyStringWithModifier) {
    auto parsed = ParseHotkeyString("cmd+p");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keyCode, 0x23);
    EXPECT_EQ(parsed->modifierMask, kCGEventFlagMaskCommand);
}

TEST(Hotkey, ParseHotkeyStringMultipleModifiers) {
    auto parsed = ParseHotkeyString("cmd+shift+escape");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keyCode, 0x35);
    EXPECT_EQ(parsed->modifierMask, kCGEventFlagMaskCommand | kCGEventFlagMaskShift);
}

TEST(Hotkey, ParseHotkeyStringAllModifiers) {
    auto parsed = ParseHotkeyString("ctrl+alt+cmd+shift+fn+space");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keyCode, 0x31);
    int expected = kCGEventFlagMaskControl | kCGEventFlagMaskAlternate |
                   kCGEventFlagMaskCommand | kCGEventFlagMaskShift |
                   kCGEventFlagMaskSecondaryFn;
    EXPECT_EQ(parsed->modifierMask, expected);
}

TEST(Hotkey, ParseHotkeyStringModifierAliases) {
    auto parsed1 = ParseHotkeyString("command+opt+control+a");
    ASSERT_TRUE(parsed1.has_value());
    EXPECT_EQ(parsed1->keyCode, 0x00);
    EXPECT_EQ(parsed1->modifierMask, kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate | kCGEventFlagMaskControl);

    auto parsed2 = ParseHotkeyString("cmd+alt+ctrl+b");
    ASSERT_TRUE(parsed2.has_value());
    EXPECT_EQ(parsed2->keyCode, 0x0B);
    EXPECT_EQ(parsed2->modifierMask, kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate | kCGEventFlagMaskControl);
}

TEST(Hotkey, ParseHotkeyStringEmpty) {
    auto parsed = ParseHotkeyString("");
    EXPECT_FALSE(parsed.has_value());
}

TEST(Hotkey, ParseHotkeyStringNoKey) {
    auto parsed = ParseHotkeyString("cmd+");
    EXPECT_FALSE(parsed.has_value());
}

TEST(Hotkey, ParseHotkeyStringUnknownKey) {
    auto parsed = ParseHotkeyString("cmd+unknown");
    EXPECT_FALSE(parsed.has_value());
}

TEST(Hotkey, ParseHotkeyStringNoModifierKeyOnly) {
    auto parsed = ParseHotkeyString("cmd+shift");
    EXPECT_FALSE(parsed.has_value());
}

TEST(Hotkey, ParseHotkeyStringSpaceTrimmed) {
    auto parsed = ParseHotkeyString("  shift+return  ");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->keyCode, 0x24);
    EXPECT_EQ(parsed->modifierMask, kCGEventFlagMaskShift);
}

TEST(Hotkey, FormatHotkeyString) {
    EXPECT_EQ(FormatHotkeyString(0x03, 0), "f");
    EXPECT_EQ(FormatHotkeyString(0x23, kCGEventFlagMaskCommand), "cmd+p");
    EXPECT_EQ(FormatHotkeyString(0x35, kCGEventFlagMaskCommand | kCGEventFlagMaskShift), "shift+cmd+escape");
}

TEST(Hotkey, FormatHotkeyStringOrder) {
    std::string result = FormatHotkeyString(0x31, kCGEventFlagMaskCommand | kCGEventFlagMaskControl | kCGEventFlagMaskAlternate | kCGEventFlagMaskShift | kCGEventFlagMaskSecondaryFn);
    EXPECT_EQ(result, "ctrl+alt+shift+cmd+fn+space");
}

TEST(Hotkey, FormatHotkeyStringUnknownCode) {
    std::string result = FormatHotkeyString(0xFF, kCGEventFlagMaskCommand);
    EXPECT_EQ(result, "cmd+0xFF");
}

TEST(Hotkey, RoundTrip) {
    auto parsed = ParseHotkeyString("cmd+shift+p");
    ASSERT_TRUE(parsed.has_value());
    std::string formatted = FormatHotkeyString(parsed->keyCode, parsed->modifierMask);
    EXPECT_EQ(formatted, "shift+cmd+p");

    auto reparsed = ParseHotkeyString(formatted);
    ASSERT_TRUE(reparsed.has_value());
    EXPECT_EQ(reparsed->keyCode, parsed->keyCode);
    EXPECT_EQ(reparsed->modifierMask, parsed->modifierMask);
}

TEST(Hotkey, DefaultConfigHotkeysAreValid) {
    EXPECT_TRUE(ParseHotkeyString("f").has_value());
    EXPECT_TRUE(ParseHotkeyString("o").has_value());
    EXPECT_TRUE(ParseHotkeyString("p").has_value());
    EXPECT_TRUE(ParseHotkeyString("b").has_value());
    EXPECT_TRUE(ParseHotkeyString("1").has_value());
    EXPECT_TRUE(ParseHotkeyString("2").has_value());
    EXPECT_TRUE(ParseHotkeyString("0").has_value());
    EXPECT_TRUE(ParseHotkeyString("right shift").has_value());

    auto failsafe = ParseHotkeyString("cmd+shift+escape");
    ASSERT_TRUE(failsafe.has_value());
    EXPECT_EQ(failsafe->keyCode, 0x35);
    EXPECT_EQ(failsafe->modifierMask, kCGEventFlagMaskCommand | kCGEventFlagMaskShift);
}

TEST(Hotkey, DisplayAllKeyNames) {
    const char* names[] = {
        "a","b","c","d","e","f","g","h","i","j","k","l","m",
        "n","o","p","q","r","s","t","u","v","w","x","y","z",
        "0","1","2","3","4","5","6","7","8","9",
        "escape","tab","space","return","delete",
        "left shift","right shift",
        "up arrow","down arrow","left arrow","right arrow"
    };
    for (const char* name : names) {
        int code = KeyNameToKeyCode(name);
        EXPECT_GE(code, 0) << "Key name '" << name << "' should map to a valid key code";
        std::string display = KeyCodeToDisplayString(code);
        EXPECT_EQ(display, name) << "Round-trip failed for '" << name << "': got '" << display << "'";
    }
}

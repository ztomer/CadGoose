#import "gui_accessibility_fixture.h"

// =========================================================
// STATUS BAR & AI CHAT WINDOW TESTS
// =========================================================

// =========================================================
TEST_F(AccessibilityGUITest, StatusBarMenuExists) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    ASSERT_EQ(AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras), kAXErrorSuccess);
    NSArray* items = AXKids((AXUIElementRef)extras);
    CFRelease(extras);
    EXPECT_GT(items.count, 0) << "Expected at least one status bar item";
}

TEST_F(AccessibilityGUITest, StatusBarMenuHasPreferencesItem) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess) return;

    bool foundPrefs = false;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                if ([AXStr(mi, kAXRoleAttribute) isEqualToString:@"AXMenuItem"] &&
                    [AXStr(mi, kAXTitleAttribute) isEqualToString:@"Preferences..."]) {
                    foundPrefs = true;
                    break;
                }
            }
            if (foundPrefs) break;
        }
        if (foundPrefs) break;
    }
    CFRelease(extras);
    EXPECT_TRUE(foundPrefs) << "Expected 'Preferences...' in status bar menu";
}

TEST_F(AccessibilityGUITest, StatusBarMenuHasHonkItem) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess) return;

    bool foundHonk = false;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                NSString* title = AXStr(mi, kAXTitleAttribute);
                if ([title isEqualToString:@"Honk!"] || [title containsString:@"Honk"]) {
                    foundHonk = true;
                    break;
                }
            }
            if (foundHonk) break;
        }
        if (foundHonk) break;
    }
    CFRelease(extras);
    EXPECT_TRUE(foundHonk) << "Expected 'Honk!' in status bar menu";
}

TEST_F(AccessibilityGUITest, StatusBarMenuHasSpawnItem) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess) return;

    bool foundSpawn = false;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                NSString* title = AXStr(mi, kAXTitleAttribute);
                if ([title isEqualToString:@"Spawn"] || [title containsString:@"Spawn"]) {
                    foundSpawn = true;
                    break;
                }
            }
            if (foundSpawn) break;
        }
        if (foundSpawn) break;
    }
    CFRelease(extras);
    EXPECT_TRUE(foundSpawn) << "Expected 'Spawn' in status bar menu";
}

TEST_F(AccessibilityGUITest, StatusBarMenuHasClearItem) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess) return;

    bool foundClear = false;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                NSString* title = AXStr(mi, kAXTitleAttribute);
                if ([title isEqualToString:@"Clear"] || [title containsString:@"Clear"]) {
                    foundClear = true;
                    break;
                }
            }
            if (foundClear) break;
        }
        if (foundClear) break;
    }
    CFRelease(extras);
    EXPECT_TRUE(foundClear) << "Expected 'Clear' in status bar menu";
}

TEST_F(AccessibilityGUITest, StatusBarMenuHasQuitItem) {
    ASSERT_NE(s_appElem, nullptr);
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(s_appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess) return;

    bool foundQuit = false;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                NSString* title = AXStr(mi, kAXTitleAttribute);
                if ([title isEqualToString:@"Quit"] || [title containsString:@"Quit"]) {
                    foundQuit = true;
                    break;
                }
            }
            if (foundQuit) break;
        }
        if (foundQuit) break;
    }
    CFRelease(extras);
    EXPECT_TRUE(foundQuit) << "Expected 'Quit' in status bar menu";
}

// =========================================================
// AI CHAT WINDOW (if opened)
// =========================================================
TEST_F(AccessibilityGUITest, AIChatWindowAccessible) {
    ASSERT_NE(s_appElem, nullptr);
    AXUIElementRef chatWindow = nil;
    for (id w in AXKids(s_appElem)) {
        AXUIElementRef win = (__bridge AXUIElementRef)w;
        NSString* title = AXStr(win, kAXTitleAttribute);
        if ([title containsString:@"Chat"] || [title containsString:@"chat"]) {
            chatWindow = win;
            CFRetain(chatWindow);
            break;
        }
    }
    if (!chatWindow) {
        GTEST_SKIP() << "AI Chat window not open (open it via Preferences > AI > Chat to test)";
        return;
    }

    EXPECT_TRUE([AXStr(chatWindow, kAXRoleAttribute) isEqualToString:@"AXWindow"]);

    // Check for input field
    NSArray* textFields = FindElementsByRole(chatWindow, @"AXTextField");
    bool hasInputField = false;
    for (id obj in textFields) {
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        NSString* placeholder = AXStr(el, kAXPlaceholderValueAttribute);
        NSString* value = AXStr(el, kAXValueAttribute);
        if (placeholder.length > 0 || value.length > 0) {
            hasInputField = true;
            break;
        }
    }
    EXPECT_TRUE(hasInputField) << "Expected input field in AI Chat window";

    // Check for send functionality (Return-to-send, no button needed)
    NSArray* buttons = FindElementsByRole(chatWindow, @"AXButton");
    bool hasSendButton = false;
    for (id obj in buttons) {
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        NSString* title = AXStr(el, kAXTitleAttribute);
        if ([title isEqualToString:@"^"] || [title isEqualToString:@"v"]) {
            // Pin button present — send is via Return key
            hasSendButton = true;
            break;
        }
    }
    // Send is via Return key in the text field; pin button confirms the window is alive
    EXPECT_TRUE(hasSendButton) << "Expected pin button in AI Chat window";

    CFRelease(chatWindow);
}

// =========================================================
// REGRESSION: Config key consistency (GUI ↔ Registry ↔ Behavior)
// =========================================================
TEST_F(AccessibilityGUITest, AllToggleNamesMatchRegistry) {
    ASSERT_NE(s_prefsWindow, nullptr);
    // Ensure we're on the Behaviors tab
    AXUIElementRef behaviorsTab = FindTabButton(s_prefsWindow, @"Behaviors");
    if (behaviorsTab) {
        AXPress(behaviorsTab);
        usleep(150000);
        CFRelease(behaviorsTab);
    }

    NSArray* rows = CollectToggleRows(s_prefsWindow);
    ASSERT_GE(rows.count, 1) << "No toggle rows found";

    // Every toggle name should be in BehaviorDisplayNames
    for (NSArray* pair in rows) {
        NSString* name = pair[1];
        EXPECT_TRUE([BehaviorDisplayNames() containsObject:name])
            << "Toggle name \"" << name.UTF8String << "\" not in expected behavior list";
    }
}

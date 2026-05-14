// macOS AXUIElement GUI integration tests for CadGoose
// Tests the running app's GUI via the Accessibility API.
// SKIPPED if the app is not running or accessibility permission is unavailable.

#import <gtest/gtest.h>
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

// -- App detection --
static pid_t FindCadGoosePID() {
    for (NSRunningApplication* app in NSWorkspace.sharedWorkspace.runningApplications) {
        if ([app.executableURL.lastPathComponent isEqualToString:@"CadGoose"])
            return app.processIdentifier;
    }
    return -1;
}

// -- AX helpers --
static NSString* AXStr(AXUIElementRef el, CFStringRef attr) {
    CFTypeRef val;
    if (AXUIElementCopyAttributeValue(el, attr, &val) == kAXErrorSuccess && val) {
        if (CFGetTypeID(val) == CFStringGetTypeID())
            return (__bridge_transfer NSString*)val;
        CFRelease(val);
    }
    return nil;
}

static int AXCheckNum(AXUIElementRef el, CFStringRef attr) {
    CFTypeRef val;
    if (AXUIElementCopyAttributeValue(el, attr, &val) == kAXErrorSuccess && val) {
        if (CFGetTypeID(val) == CFNumberGetTypeID()) {
            int v = -1;
            CFNumberGetValue((CFNumberRef)val, kCFNumberIntType, &v);
            CFRelease(val);
            return v;
        }
        CFRelease(val);
    }
    return -1;
}

static NSArray* AXKids(AXUIElementRef el) {
    CFIndex count;
    if (AXUIElementGetAttributeValueCount(el, kAXChildrenAttribute, &count) != kAXErrorSuccess)
        return @[];
    CFArrayRef arr;
    if (AXUIElementCopyAttributeValues(el, kAXChildrenAttribute, 0, count, (CFArrayRef*)&arr) == kAXErrorSuccess)
        return (__bridge_transfer NSArray*)arr;
    return @[];
}

static bool AXPress(AXUIElementRef el) {
    return AXUIElementPerformAction(el, kAXPressAction) == kAXErrorSuccess;
}

// Open Preferences via the status bar menu (🍁 → Preferences...)
static bool OpenPreferencesViaMenu(AXUIElementRef appElem) {
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess)
        return false;

    // Find 🍁 status item
    AXUIElementRef prefsItem = nil;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
        // This status bar item has a menu; look for Preferences... inside
        for (id sub in AXKids(barItem)) {
            AXUIElementRef menu = (__bridge AXUIElementRef)sub;
            if (![AXStr(menu, kAXRoleAttribute) isEqualToString:@"AXMenu"]) continue;
            for (id menuItem in AXKids(menu)) {
                AXUIElementRef mi = (__bridge AXUIElementRef)menuItem;
                if ([AXStr(mi, kAXRoleAttribute) isEqualToString:@"AXMenuItem"] &&
                    [AXStr(mi, kAXTitleAttribute) isEqualToString:@"Preferences..."]) {
                    prefsItem = mi;
                    CFRetain(prefsItem);
                    break;
                }
            }
            if (prefsItem) break;
        }
        if (prefsItem) break;
    }
    CFRelease(extras);

    if (!prefsItem) return false;
    bool result = AXPress(prefsItem);
    CFRelease(prefsItem);
    return result;
}

// Behavior display names in the preferences table (in order)
static NSArray* BehaviorDisplayNames() {
    return @[
        @"Ball", @"Breadcrumbs", @"Hats", @"Rainbow", @"Acid", @"Anger", @"Autumn Leaves",
        @"Honcker", @"Jail", @"Portals", @"Drag", @"Banish",
        @"Nametag",
        @"Health", @"Pomodoro"
    ];
}

// Collect all (checkbox, name) pairs from the preferences table
static NSArray* CollectToggleRows(AXUIElementRef prefsWindow) {
    NSMutableArray* rows = [NSMutableArray array]; // each entry: @[ cb, name ]

    // Find the AXTable inside the window (may be nested in AXScrollArea)
    NSMutableArray* searchStack = [NSMutableArray arrayWithObject:(__bridge id)prefsWindow];
    __block AXUIElementRef table = nil;
    while (searchStack.count > 0 && !table) {
        id obj = [searchStack lastObject];  [searchStack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        NSString* role = AXStr(el, kAXRoleAttribute);
        if ([role isEqualToString:@"AXTable"]) {
            table = el;
            CFRetain(table);
            break;
        }
        for (id child in AXKids(el))
            [searchStack addObject:child];
    }
    if (!table) return rows;

    for (id rowObj in AXKids(table)) {
        AXUIElementRef row = (__bridge AXUIElementRef)rowObj;
        if (![AXStr(row, kAXRoleAttribute) isEqualToString:@"AXRow"]) continue;

        // Rows may have one or more AXCell children
        for (id cellObj in AXKids(row)) {
            AXUIElementRef cell = (__bridge AXUIElementRef)cellObj;
            if (![AXStr(cell, kAXRoleAttribute) isEqualToString:@"AXCell"]) continue;

            AXUIElementRef cb = nil;
            NSString* name = nil;
            for (id childObj in AXKids(cell)) {
                AXUIElementRef child = (__bridge AXUIElementRef)childObj;
                NSString* role = AXStr(child, kAXRoleAttribute);
                NSString* val = AXStr(child, kAXValueAttribute);
                if (!val) val = AXStr(child, kAXTitleAttribute);
                val = [val stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceCharacterSet];

                if ([role isEqualToString:@"AXCheckBox"]) {
                    cb = child;
                    CFRetain(cb);
                } else if ([role isEqualToString:@"AXStaticText"] && val.length > 0) {
                    if ([BehaviorDisplayNames() containsObject:val])
                        name = val;
                }
            }
            if (cb && name) {
                [rows addObject:@[(__bridge id)cb, name]];
                CFRelease(cb);
            }
        }
    }
    CFRelease(table);
    return rows;
}

// =========================================================
// Test Suite — opens Preferences once, runs all tests, cleans up
// =========================================================
class AccessibilityGUITest : public ::testing::Test {
protected:
    static pid_t s_appPID;
    static AXUIElementRef s_appElem;
    static AXUIElementRef s_prefsWindow;

    AXUIElementRef prefsWindow() { return s_prefsWindow; }

    static void SetUpTestSuite() {
        s_appPID = FindCadGoosePID();
        if (s_appPID == -1) return; // skip handled in SetUp
        s_appElem = AXUIElementCreateApplication(s_appPID);

        s_prefsWindow = findPrefsWindow();
        if (!s_prefsWindow) {
            NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:s_appPID];
            [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
            usleep(300000);
            if (OpenPreferencesViaMenu(s_appElem))
                usleep(800000);
            s_prefsWindow = findPrefsWindow();
        }
    }

    static void TearDownTestSuite() {
        if (s_prefsWindow) {
            AXUIElementRef btn = nil;
            if (AXUIElementCopyAttributeValue(s_prefsWindow, kAXCloseButtonAttribute, (CFTypeRef*)&btn) == kAXErrorSuccess) {
                AXPress(btn);
                CFRelease(btn);
            }
            CFRelease(s_prefsWindow);
            s_prefsWindow = nullptr;
        }
        if (s_appElem) {
            CFRelease(s_appElem);
            s_appElem = nullptr;
        }
    }

    void SetUp() override {
        if (s_appPID == -1)
            GTEST_SKIP() << "CadGoose not running. Launch it first.";
        if (!s_prefsWindow)
            GTEST_SKIP() << "Could not open Preferences. Check Accessibility prefs.";
    }

    static AXUIElementRef findPrefsWindow() {
        for (id w in AXKids(s_appElem)) {
            AXUIElementRef win = (__bridge AXUIElementRef)w;
            if ([AXStr(win, kAXRoleAttribute) isEqualToString:@"AXWindow"] &&
                [AXStr(win, kAXTitleAttribute) isEqualToString:@"Preferences"]) {
                CFRetain(win);
                return win;
            }
        }
        return nullptr;
    }
};

pid_t AccessibilityGUITest::s_appPID = -1;
AXUIElementRef AccessibilityGUITest::s_appElem = nullptr;
AXUIElementRef AccessibilityGUITest::s_prefsWindow = nullptr;

TEST_F(AccessibilityGUITest, AppRunningAndAccessible) {
    ASSERT_GT(s_appPID, 0);
    ASSERT_NE(s_appElem, nullptr);
    NSString* title = AXStr(s_appElem, kAXTitleAttribute);
    EXPECT_TRUE(title != nil && title.length > 0);
}

TEST_F(AccessibilityGUITest, PreferencesWindowAccessible) {
    ASSERT_NE(s_prefsWindow, nullptr);
    EXPECT_TRUE([AXStr(s_prefsWindow, kAXRoleAttribute) isEqualToString:@"AXWindow"]);
    EXPECT_TRUE([AXStr(s_prefsWindow, kAXTitleAttribute) isEqualToString:@"Preferences"]);
}

// Collect all AXCheckBox elements inside the preferences window
static NSArray* CollectCheckboxes(AXUIElementRef root) {
    NSMutableArray* boxes = [NSMutableArray array];
    NSMutableArray* stack = [NSMutableArray arrayWithObject:(__bridge id)root];

    while (stack.count > 0) {
        id elObj = [stack lastObject];
        [stack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)elObj;

        NSString* role = AXStr(el, kAXRoleAttribute);
        if ([role isEqualToString:@"AXCheckBox"]) {
            [boxes addObject:elObj];
        }

        NSArray* kids = AXKids(el);
        for (id child in kids) {
            [stack addObject:child];
        }
    }

    return boxes;
}

TEST_F(AccessibilityGUITest, AllBehaviorTogglesExist) {
    ASSERT_NE(s_prefsWindow, nullptr);
    NSArray* boxes = CollectCheckboxes(s_prefsWindow);
    // 15 behavior toggles (headers don't have checkboxes)
    EXPECT_EQ(boxes.count, 15) << "Expected 15 toggle switches in Preferences window";
}

TEST_F(AccessibilityGUITest, ToggleBallAndVerifyStateChange) {
    ASSERT_NE(s_prefsWindow, nullptr);

    NSArray* rows = CollectToggleRows(s_prefsWindow);
    ASSERT_GE(rows.count, 1) << "No toggle rows found in Preferences window";

    // Find the Ball row
    AXUIElementRef ballCheckbox = nil;
    for (NSArray* pair in rows) {
        NSString* name = pair[1];
        if ([name isEqualToString:@"Ball"]) {
            ballCheckbox = (__bridge AXUIElementRef)pair[0];
            CFRetain(ballCheckbox);
            break;
        }
    }
    ASSERT_NE(ballCheckbox, nullptr) << "Could not find Ball toggle in Preferences window";

    // Read and toggle
    int val0 = AXCheckNum(ballCheckbox, kAXValueAttribute);
    EXPECT_TRUE(val0 == 0 || val0 == 1) << "Checkbox AXValue should be 0 or 1, got " << val0;

    ASSERT_TRUE(AXPress(ballCheckbox)) << "AXPress on Ball checkbox failed";
    usleep(150000);

    int val1 = AXCheckNum(ballCheckbox, kAXValueAttribute);
    EXPECT_NE(val1, val0) << "Ball checkbox state should have changed after press";

    // Toggle back
    ASSERT_TRUE(AXPress(ballCheckbox)) << "Second AXPress on Ball checkbox failed";
    usleep(150000);

    int val2 = AXCheckNum(ballCheckbox, kAXValueAttribute);
    EXPECT_EQ(val2, val0) << "Ball checkbox should return to original state after second press";

    CFRelease(ballCheckbox);
}

TEST_F(AccessibilityGUITest, AllTogglesRespondToPress) {
    ASSERT_NE(s_prefsWindow, nullptr);

    NSArray* rows = CollectToggleRows(s_prefsWindow);
    int pressed = 0;
    for (NSArray* pair in rows) {
        AXUIElementRef cb = (__bridge AXUIElementRef)pair[0];
        NSString* name = pair[1];
        CFRetain(cb);

        int v0 = AXCheckNum(cb, kAXValueAttribute);
        EXPECT_TRUE(AXPress(cb)) << "AXPress failed for " << name.UTF8String;
        usleep(80000);
        int v1 = AXCheckNum(cb, kAXValueAttribute);
        EXPECT_NE(v1, v0) << "Toggle for \"" << name.UTF8String << "\" did not change state";
        // Restore
        EXPECT_TRUE(AXPress(cb)) << "Restore AXPress failed for " << name.UTF8String;
        usleep(80000);

        CFRelease(cb);
        pressed++;
    }

    EXPECT_EQ(pressed, 15) << "Should have pressed all 15 behavior toggles";
}

// macOS AXUIElement GUI integration tests for CadGoose
// Tests the running app's GUI via the Accessibility API.
// SKIPPED if the app is not running or accessibility permission is unavailable.

#import <gtest/gtest.h>
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

// -- App detection --
inline pid_t FindCadGoosePID() {
    for (NSRunningApplication* app in NSWorkspace.sharedWorkspace.runningApplications) {
        if ([app.executableURL.lastPathComponent isEqualToString:@"CadGoose"])
            return app.processIdentifier;
    }
    return -1;
}

// -- AX helpers --
inline NSString* AXStr(AXUIElementRef el, CFStringRef attr) {
    CFTypeRef val;
    if (AXUIElementCopyAttributeValue(el, attr, &val) == kAXErrorSuccess && val) {
        if (CFGetTypeID(val) == CFStringGetTypeID())
            return (__bridge_transfer NSString*)val;
        CFRelease(val);
    }
    return nil;
}

inline int AXCheckNum(AXUIElementRef el, CFStringRef attr) {
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

inline double AXCheckDouble(AXUIElementRef el, CFStringRef attr) {
    CFTypeRef val;
    if (AXUIElementCopyAttributeValue(el, attr, &val) == kAXErrorSuccess && val) {
        if (CFGetTypeID(val) == CFNumberGetTypeID()) {
            double v = -1;
            CFNumberGetValue((CFNumberRef)val, kCFNumberDoubleType, &v);
            CFRelease(val);
            return v;
        }
        CFRelease(val);
    }
    return -1;
}

inline NSArray* AXKids(AXUIElementRef el) {
    CFIndex count;
    if (AXUIElementGetAttributeValueCount(el, kAXChildrenAttribute, &count) != kAXErrorSuccess)
        return @[];
    CFArrayRef arr;
    if (AXUIElementCopyAttributeValues(el, kAXChildrenAttribute, 0, count, (CFArrayRef*)&arr) == kAXErrorSuccess)
        return (__bridge_transfer NSArray*)arr;
    return @[];
}

inline bool AXPress(AXUIElementRef el) {
    return AXUIElementPerformAction(el, kAXPressAction) == kAXErrorSuccess;
}

inline AXUIElementRef FindElementByRole(AXUIElementRef root, NSString* role) {
    for (id child in AXKids(root)) {
        AXUIElementRef el = (__bridge AXUIElementRef)child;
        if ([AXStr(el, kAXRoleAttribute) isEqualToString:role]) {
            CFRetain(el);
            return el;
        }
        AXUIElementRef found = FindElementByRole(el, role);
        if (found) return found;
    }
    return nullptr;
}

inline AXUIElementRef FindElementByTitle(AXUIElementRef root, NSString* title) {
    for (id child in AXKids(root)) {
        AXUIElementRef el = (__bridge AXUIElementRef)child;
        if ([AXStr(el, kAXTitleAttribute) isEqualToString:title]) {
            CFRetain(el);
            return el;
        }
        AXUIElementRef found = FindElementByTitle(el, title);
        if (found) return found;
    }
    return nullptr;
}

inline AXUIElementRef FindElementBySubrole(AXUIElementRef root, NSString* subrole) {
    for (id child in AXKids(root)) {
        AXUIElementRef el = (__bridge AXUIElementRef)child;
        if ([AXStr(el, kAXSubroleAttribute) isEqualToString:subrole]) {
            CFRetain(el);
            return el;
        }
        AXUIElementRef found = FindElementBySubrole(el, subrole);
        if (found) return found;
    }
    return nullptr;
}

inline NSArray* FindElementsByRole(AXUIElementRef root, NSString* role) {
    NSMutableArray* results = [NSMutableArray array];
    NSMutableArray* stack = [NSMutableArray arrayWithObject:(__bridge id)root];
    while (stack.count > 0) {
        id obj = [stack lastObject]; [stack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        if ([AXStr(el, kAXRoleAttribute) isEqualToString:role]) {
            [results addObject:(__bridge id)el];
        }
        for (id child in AXKids(el)) [stack addObject:child];
    }
    return results;
}

inline NSArray* FindElementsBySubrole(AXUIElementRef root, NSString* subrole) {
    NSMutableArray* results = [NSMutableArray array];
    NSMutableArray* stack = [NSMutableArray arrayWithObject:(__bridge id)root];
    while (stack.count > 0) {
        id obj = [stack lastObject]; [stack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        if ([AXStr(el, kAXSubroleAttribute) isEqualToString:subrole]) {
            [results addObject:(__bridge id)el];
        }
        for (id child in AXKids(el)) [stack addObject:child];
    }
    return results;
}

inline NSArray* FindElementsByRoleAndTitle(AXUIElementRef root, NSString* role, NSString* title) {
    NSMutableArray* results = [NSMutableArray array];
    NSMutableArray* stack = [NSMutableArray arrayWithObject:(__bridge id)root];
    while (stack.count > 0) {
        id obj = [stack lastObject]; [stack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        NSString* r = AXStr(el, kAXRoleAttribute);
        NSString* t = AXStr(el, kAXTitleAttribute);
        if ([r isEqualToString:role] && [t isEqualToString:title]) {
            [results addObject:(__bridge id)el];
        }
        for (id child in AXKids(el)) [stack addObject:child];
    }
    return results;
}

// Open Preferences via the status bar menu (maple leaf icon -> Preferences...)
inline bool OpenPreferencesViaMenu(AXUIElementRef appElem) {
    CFTypeRef extras;
    if (AXUIElementCopyAttributeValue(appElem, CFSTR("AXExtrasMenuBar"), &extras) != kAXErrorSuccess)
        return false;

    AXUIElementRef prefsItem = nil;
    for (id item in AXKids((AXUIElementRef)extras)) {
        AXUIElementRef barItem = (__bridge AXUIElementRef)item;
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

// Behavior display names in the Behaviors tab (in order)
inline NSArray* BehaviorDisplayNames() {
    return @[
        @"Ball", @"Hats", @"Rainbow", @"Acid", @"Anger", @"Autumn Leaves",
        @"Avoidance", @"Boredom Sigh", @"Window Peeking", @"Interactive Drops", @"Toys"
    ];
}

// Behavior display names in the Play tab (in order)
inline NSArray* PlayDisplayNames() {
    return @[
        @"Breadcrumbs", @"Honcker", @"Jail", @"Portals", @"Drag",
        @"Nametag", @"Health", @"Pomodoro"
    ];
}

// Collect all (checkbox, name) pairs from the preferences table
inline NSArray* CollectToggleRows(AXUIElementRef prefsWindow) {
    NSMutableArray* rows = [NSMutableArray array];

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

                if ([role isEqualToString:@"AXCheckBox"] || [role isEqualToString:@"AXSwitch"]) {
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

// Collect all toggle/switch elements inside the preferences window
inline NSArray* CollectToggles(AXUIElementRef root) {
    NSMutableArray* toggles = [NSMutableArray array];
    NSMutableArray* stack = [NSMutableArray arrayWithObject:(__bridge id)root];

    while (stack.count > 0) {
        id elObj = [stack lastObject];
        [stack removeLastObject];
        AXUIElementRef el = (__bridge AXUIElementRef)elObj;

        NSString* role = AXStr(el, kAXRoleAttribute);
        if ([role isEqualToString:@"AXCheckBox"] || [role isEqualToString:@"AXSwitch"]) {
            [toggles addObject:elObj];
        }

        NSArray* kids = AXKids(el);
        for (id child in kids) {
            [stack addObject:child];
        }
    }

    return toggles;
}

// Collect all sliders inside the preferences window
inline NSArray* CollectSliders(AXUIElementRef root) {
    return FindElementsBySubrole(root, @"AXSlider");
}

// Collect all text fields inside the preferences window
inline NSArray* CollectTextFields(AXUIElementRef root) {
    NSMutableArray* fields = [NSMutableArray array];
    for (id el in FindElementsByRole(root, @"AXTextField")) {
        [fields addObject:el];
    }
    return fields;
}

// Collect all pop-up buttons inside the preferences window
inline NSArray* CollectPopUpButtons(AXUIElementRef root) {
    NSMutableArray* buttons = [NSMutableArray array];
    for (id el in FindElementsByRole(root, @"AXPopUpButton")) {
        [buttons addObject:el];
    }
    return buttons;
}

// Find a tab button by name (checks AXButton, AXRadioButton, AXStaticText)
inline AXUIElementRef FindTabButton(AXUIElementRef prefsWindow, NSString* tabName) {
    NSArray* allButtons = FindElementsByRole(prefsWindow, @"AXButton");
    NSArray* allRadioButtons = FindElementsByRole(prefsWindow, @"AXRadioButton");
    NSArray* allStaticTexts = FindElementsByRole(prefsWindow, @"AXStaticText");
    NSMutableArray* allTabElements = [NSMutableArray arrayWithArray:allButtons];
    [allTabElements addObjectsFromArray:allRadioButtons];
    [allTabElements addObjectsFromArray:allStaticTexts];
    for (id obj in allTabElements) {
        AXUIElementRef el = (__bridge AXUIElementRef)obj;
        NSString* title = AXStr(el, kAXTitleAttribute);
        NSString* value = AXStr(el, kAXValueAttribute);
        NSString* desc = AXStr(el, kAXDescriptionAttribute);
        if ([title isEqualToString:tabName] || [value isEqualToString:tabName] || [desc isEqualToString:tabName]) {
            CFRetain(el);
            return el;
        }
    }
    return nullptr;
}

// =========================================================
// Test Suite — opens Preferences once, runs all tests, cleans up
// =========================================================
class AccessibilityGUITest : public ::testing::Test {
protected:
    inline static pid_t s_appPID = -1;
    inline static AXUIElementRef s_appElem = nullptr;
    inline static AXUIElementRef s_prefsWindow = nullptr;

    AXUIElementRef prefsWindow() { return s_prefsWindow; }

    static void SetUpTestSuite() {
        s_appPID = FindCadGoosePID();
        if (s_appPID == -1) return;
        s_appElem = AXUIElementCreateApplication(s_appPID);

        s_prefsWindow = findPrefsWindow();
        if (!s_prefsWindow) {
            NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:s_appPID];
            [app activateWithOptions:NSApplicationActivateAllWindows];
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


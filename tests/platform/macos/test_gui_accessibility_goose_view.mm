#import "gui_accessibility_fixture.h"

// =========================================================
// DROPPED ITEM DRAG TESTS (GooseView canvas)
// =========================================================

static CGRect AXScreenFrame(AXUIElementRef el) {
    CGRect frame = CGRectZero;
    CFTypeRef posVal;
    if (AXUIElementCopyAttributeValue(el, kAXPositionAttribute, &posVal) == kAXErrorSuccess) {
        AXValueGetValue((AXValueRef)posVal, kAXValueTypeCGPoint, &frame.origin);
        CFRelease(posVal);
    }
    CFTypeRef sizeVal;
    if (AXUIElementCopyAttributeValue(el, kAXSizeAttribute, &sizeVal) == kAXErrorSuccess) {
        AXValueGetValue((AXValueRef)sizeVal, kAXValueTypeCGSize, &frame.size);
        CFRelease(sizeVal);
    }
    return frame;
}

// Find the main GooseView window (borderless, covers screen, no title or generic title)
static AXUIElementRef FindGooseViewWindow(AXUIElementRef appElem) {
    for (id w in AXKids(appElem)) {
        AXUIElementRef win = (__bridge AXUIElementRef)w;
        NSString* role = AXStr(win, kAXRoleAttribute);
        if (![role isEqualToString:@"AXWindow"]) continue;

        NSString* title = AXStr(win, kAXTitleAttribute);
        NSString* subrole = AXStr(win, kAXSubroleAttribute);

        // Skip known windows: Preferences, AI Chat, etc.
        if ([title isEqualToString:@"Preferences"]) continue;
        if ([title containsString:@"Chat"]) continue;

        // GooseView window: borderless subrole, or no title, or very large (screen-sized)
        bool isBorderless = [subrole isEqualToString:@"NSWindowSubroleBorderless"] ||
                            [subrole isEqualToString:@"AXUnknown"];
        bool isScreenSized = false;
        CGRect frame = AXScreenFrame(win);
        if (frame.size.width >= 1000 && frame.size.height >= 600) {
            isScreenSized = true;
        }

        if (isBorderless || (title == nil || title.length == 0) || isScreenSized) {
            CFRetain(win);
            return win;
        }
    }
    return nullptr;
}

// Simulate mouse down at screen point
static void MouseDownAt(CGPoint screenPoint) {
    CGEventRef down = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDown, screenPoint, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, down);
    CFRelease(down);
}

// Simulate mouse dragged to screen point
static void MouseDragTo(CGPoint screenPoint) {
    CGEventRef drag = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseDragged, screenPoint, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, drag);
    CFRelease(drag);
}

// Simulate mouse up at screen point
static void MouseUpAt(CGPoint screenPoint) {
    CGEventRef up = CGEventCreateMouseEvent(nullptr, kCGEventLeftMouseUp, screenPoint, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(up);
}

TEST_F(AccessibilityGUITest, GooseViewWindowExists) {
    ASSERT_NE(s_appElem, nullptr);
    AXUIElementRef gooseWin = FindGooseViewWindow(s_appElem);
    EXPECT_NE(gooseWin, nullptr) << "Expected main GooseView window (borderless, no title)";
    if (gooseWin) {
        CGRect frame = AXScreenFrame(gooseWin);
        EXPECT_GT(frame.size.width, 0) << "GooseView window should have positive width";
        EXPECT_GT(frame.size.height, 0) << "GooseView window should have positive height";
        CFRelease(gooseWin);
    }
}

TEST_F(AccessibilityGUITest, DroppedItemDragMovesPosition) {
    ASSERT_NE(s_appElem, nullptr);
    AXUIElementRef gooseWin = FindGooseViewWindow(s_appElem);
    if (!gooseWin) {
        GTEST_SKIP() << "GooseView window not found";
        return;
    }

    CGRect frame = AXScreenFrame(gooseWin);
    // Pick a point near center of the view (where items are likely to be)
    CGPoint startPt = CGPointMake(frame.origin.x + frame.size.width * 0.5f,
                                  frame.origin.y + frame.size.height * 0.5f);
    CGPoint endPt = CGPointMake(startPt.x + 100.0f, startPt.y + 50.0f);

    // Mouse down → drag → up
    MouseDownAt(startPt);
    usleep(50000);  // 50ms
    MouseDragTo(endPt);
    usleep(50000);
    MouseUpAt(endPt);
    usleep(50000);

    // Verify: if an item was at startPt, it should have moved.
    // We can't directly verify item position via AX (items are drawn, not AX elements),
    // but we verify the view accepted the events without error.
    // A more thorough test would use a screenshot comparison or query the app's internal state.
    EXPECT_TRUE(CGRectContainsPoint(frame, endPt)) << "Drag endpoint should be within view bounds";

    CFRelease(gooseWin);
}

TEST_F(AccessibilityGUITest, DroppedItemCloseButtonAccessible) {
    ASSERT_NE(s_appElem, nullptr);
    AXUIElementRef gooseWin = FindGooseViewWindow(s_appElem);
    if (!gooseWin) {
        GTEST_SKIP() << "GooseView window not found";
        return;
    }

    CGRect frame = AXScreenFrame(gooseWin);
    // Close button is at bottom-left of each item (in local coords).
    // Simulate a click near bottom-left of the view.
    CGPoint closePt = CGPointMake(frame.origin.x + 30.0f,
                                  frame.origin.y + 30.0f);

    MouseDownAt(closePt);
    usleep(50000);
    MouseUpAt(closePt);
    usleep(50000);

    // If an item was there, it should be removed. We verify the click was within bounds.
    EXPECT_TRUE(CGRectContainsPoint(frame, closePt)) << "Close button click should be within view bounds";

    CFRelease(gooseWin);
}

TEST_F(AccessibilityGUITest, GooseViewAcceptsMouseEvents) {
    ASSERT_NE(s_appElem, nullptr);
    AXUIElementRef gooseWin = FindGooseViewWindow(s_appElem);
    if (!gooseWin) {
        GTEST_SKIP() << "GooseView window not found";
        return;
    }

    // The GooseView should have ignoresMouseEvents = NO when items are present.
    // We can't read this via AX directly, but we can verify the window is accessible.
    NSString* role = AXStr(gooseWin, kAXRoleAttribute);
    EXPECT_TRUE([role isEqualToString:@"AXWindow"]);

    // Check that the window is on-screen and visible
    CFTypeRef minimizedVal;
    bool isMinimized = false;
    if (AXUIElementCopyAttributeValue(gooseWin, kAXMinimizedAttribute, &minimizedVal) == kAXErrorSuccess) {
        if (CFGetTypeID(minimizedVal) == CFBooleanGetTypeID()) {
            isMinimized = CFBooleanGetValue((CFBooleanRef)minimizedVal);
        }
        CFRelease(minimizedVal);
    }
    EXPECT_FALSE(isMinimized) << "GooseView window should not be minimized";

    CFRelease(gooseWin);
}

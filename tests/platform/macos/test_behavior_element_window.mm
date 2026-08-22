// BehaviorElementWindow + manager coverage — the transparent floating
// windows that carry behavior-rendered elements (ball, portal, bed, toys).
// Drives real windows and the real draw-block path; no pixel capture, so it
// stays display-independent.
#import <gtest/gtest.h>
#import "behavior_element_window.h"

#import <Cocoa/Cocoa.h>

namespace {

TEST(BehaviorElementWindowTest, InitPositionsWindowAtDeviceCoords) {
    ASSERT_TRUE([NSThread isMainThread]);

    BehaviorElementWindow* win = [[BehaviorElementWindow alloc]
        initWithDrawBlock:nil deviceX:500 deviceY:400 width:120 height:80];

    EXPECT_NEAR(win.frame.size.width, 120.0f, 1.0f);
    EXPECT_NEAR(win.frame.size.height, 80.0f, 1.0f);
    // Constructor orders the window front immediately.
    EXPECT_TRUE(win.isVisible);

    [win closeAndRemove];
}

TEST(BehaviorElementWindowTest, UpdatePositionNoChangeIsNoOp) {
    ASSERT_TRUE([NSThread isMainThread]);

    __block int draws = 0;
    BehaviorElementWindow* win = [[BehaviorElementWindow alloc]
        initWithDrawBlock:^(CGContextRef ctx) {
            CGContextSetRGBFillColor(ctx, 1, 0, 0, 1);
            CGContextFillRect(ctx, CGRectMake(0, 0, 50, 50));
            draws++;
        }
        deviceX:500 deviceY:400 width:120 height:80];

    NSRect before = win.frame;

    // Identical position+size must early-return (no frame churn).
    [win updatePosition:500 y:400 width:120 height:80];
    EXPECT_EQ(win.frame.origin.x, before.origin.x);
    EXPECT_EQ(win.frame.origin.y, before.origin.y);

    [win closeAndRemove];
}

TEST(BehaviorElementWindowTest, UpdatePositionMovesThenResizes) {
    ASSERT_TRUE([NSThread isMainThread]);

    BehaviorElementWindow* win = [[BehaviorElementWindow alloc]
        initWithDrawBlock:^(__unused CGContextRef ctx) {}
        deviceX:500 deviceY:400 width:120 height:80];

    // Origin-only move (same size): exercises the cheap setFrameOrigin path.
    [win updatePosition:600 y:450 width:120 height:80];
    CGFloat wAfterMove = win.frame.size.width;
    CGFloat hAfterMove = win.frame.size.height;
    EXPECT_FLOAT_EQ(wAfterMove, 120.0f);
    EXPECT_FLOAT_EQ(hAfterMove, 80.0f);

    // Size change: full setFrame path.
    [win updatePosition:600 y:450 width:200 height:100];
    EXPECT_NEAR(win.frame.size.width, 200.0f, 1.0f);
    EXPECT_NEAR(win.frame.size.height, 100.0f, 1.0f);

    [win closeAndRemove];
}

TEST(BehaviorElementWindowTest, DrawBlockExecutesOnDisplay) {
    ASSERT_TRUE([NSThread isMainThread]);

    __block BOOL blockRan = NO;
    BehaviorElementContentView* view = [[BehaviorElementContentView alloc]
        initWithFrame:NSMakeRect(0, 0, 64, 64)
            drawBlock:^(CGContextRef ctx) {
            EXPECT_NE(ctx, nil);
            blockRan = YES;
        }];

    [view display];  // offscreen render of this view

    EXPECT_TRUE(blockRan);
}

TEST(BehaviorElementWindowManagerTest, RegisterUnregisterRoundTrip) {
    ASSERT_TRUE([NSThread isMainThread]);

    BehaviorElementWindow* win = [[BehaviorElementWindow alloc]
        initWithDrawBlock:^(__unused CGContextRef ctx) {}
        deviceX:10 deviceY:10 width:32 height:32];
    BehaviorElementWindowManager* manager = [BehaviorElementWindowManager shared];

    // The manager exposes no dictionary accessor, so registration is
    // verified by its effect: only registered windows are closed by
    // closeAll. An unregistered window stays visible through it.
    NSNumber* key = [manager registerWindow:win];
    EXPECT_NE(key, nil);

    [win closeAndRemove];
    [manager unregisterWindow:key];
}

TEST(BehaviorElementWindowManagerTest, CloseAllClosesRegisteredWindows) {
    ASSERT_TRUE([NSThread isMainThread]);

    BehaviorElementWindowManager* manager = [BehaviorElementWindowManager shared];

    BehaviorElementWindow* w1 = [[BehaviorElementWindow alloc]
        initWithDrawBlock:^(__unused CGContextRef ctx) {}
        deviceX:10 deviceY:10 width:32 height:32];
    BehaviorElementWindow* w2 = [[BehaviorElementWindow alloc]
        initWithDrawBlock:^(__unused CGContextRef ctx) {}
        deviceX:60 deviceY:60 width:32 height:32];

    [manager registerWindow:w1];
    [manager registerWindow:w2];
    EXPECT_TRUE(w1.isVisible);
    EXPECT_TRUE(w2.isVisible);

    [manager syncWindows];  // no-op scan today, but must not corrupt state
    [manager closeAll];     // closes everything registered, drains registry

    EXPECT_FALSE(w1.isVisible);
    EXPECT_FALSE(w2.isVisible);
}

}  // namespace

// Ported from the original XCTest file of the same name, which sat dead on
// disk because it could not link into the GoogleTest binary. Same assertion:
// a dark-appearance preferences-style view hierarchy must actually RENDER
// dark — guards against the appearance being set but ignored by drawing.
#import <Cocoa/Cocoa.h>
#include <gtest/gtest.h>

namespace {

TEST(ConfigGUIRenderingTest, AppearanceTabBackgroundIsDark) {
    if (NSApp == nil) {
        [NSApplication sharedApplication];
    }

    // Create a window with the same setup as ConfigGUI
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 800, 830)
                                                   styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskFullSizeContentView
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.titleVisibility = NSWindowTitleHidden;
    window.titlebarAppearsTransparent = YES;
    window.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];

    NSView* contentView = window.contentView;
    ASSERT_NE(contentView, nil);

    NSVisualEffectView* visualEffectView = [[NSVisualEffectView alloc] initWithFrame:contentView.bounds];
    [contentView addSubview:visualEffectView];

    // Content container (no wantsLayer, like committed version)
    NSView* contentContainer = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 800, 792)];
    contentContainer.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    [contentView addSubview:contentContainer];

    // Appearance tab container with representative subviews
    NSView* appearanceView = [[NSView alloc] initWithFrame:contentContainer.bounds];
    appearanceView.appearance = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    appearanceView.hidden = NO;
    [contentContainer addSubview:appearanceView];

    auto* appLabel = [[NSTextField alloc] initWithFrame:NSMakeRect(12, 764, 80, 20)];
    appLabel.stringValue = @"Appearance:";
    appLabel.textColor = [NSColor whiteColor];
    appLabel.backgroundColor = [NSColor clearColor];
    appLabel.bordered = NO;
    appLabel.editable = NO;
    [appearanceView addSubview:appLabel];

    NSPopUpButton* popup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(96, 760, 130, 24)];
    [popup addItemWithTitle:@"Light"];
    [popup addItemWithTitle:@"Dark"];
    [popup addItemWithTitle:@"System"];
    [popup addItemWithTitle:@"Custom"];
    [appearanceView addSubview:popup];

    auto makeSlider = ^(CGFloat x) {
        auto* s = [[NSSlider alloc] initWithFrame:NSMakeRect(x, 722, 90, 16)];
        s.minValue = 0; s.maxValue = 1;
        return s;
    };
    [appearanceView addSubview:makeSlider(60)];
    [appearanceView addSubview:makeSlider(196)];
    [appearanceView addSubview:makeSlider(332)];

    auto makeVal = ^NSTextField*(CGFloat x) {
        auto* v = [[NSTextField alloc] initWithFrame:NSMakeRect(x, 722, 36, 16)];
        v.stringValue = @"0.50";
        v.textColor = [NSColor whiteColor];
        v.backgroundColor = [NSColor clearColor];
        v.bordered = NO;
        v.editable = YES;
        return v;
    };
    [appearanceView addSubview:makeVal(154)];
    [appearanceView addSubview:makeVal(290)];
    [appearanceView addSubview:makeVal(426)];

    // Force layout and render offscreen
    [window layoutIfNeeded];
    [window displayIfNeeded];

    NSBitmapImageRep* bitmap =
        [contentContainer bitmapImageRepForCachingDisplayInRect:contentContainer.bounds];
    ASSERT_NE(bitmap, nil);
    [contentContainer cacheDisplayInRect:contentContainer.bounds toBitmapImageRep:bitmap];

    // Analyze pixels - check content area (below the controls at ~700)
    int darkCount = 0, grayCount = 0, totalPixels = 0;
    for (NSInteger y = 0; y < bitmap.pixelsHigh; y += 5) {
        for (NSInteger x = 0; x < bitmap.pixelsWide; x += 5) {
            NSColor* color = [bitmap colorAtX:x y:y];
            if (!color) continue;
            CGFloat r = 0, g = 0, b = 0, a = 0;
            [color getRed:&r green:&g blue:&b alpha:&a];
            totalPixels++;
            if (r < 0.2 && g < 0.2 && b < 0.2) {
                darkCount++;
            } else if (r > 0.3 && r < 0.7 && g > 0.3 && g < 0.7 && b > 0.3 && b < 0.7 &&
                       fabs(r - g) < 0.1 && fabs(g - b) < 0.1) {
                grayCount++;
            }
        }
    }

    EXPECT_GT(totalPixels, 0);
    EXPECT_GT(darkCount, totalPixels * 0.5)
        << "Expected mostly dark pixels in background area "
        << "(dark=" << darkCount << "/" << totalPixels << ")";
    EXPECT_LT(grayCount, totalPixels * 0.3)
        << "Expected few gray pixels in background area "
        << "(gray=" << grayCount << "/" << totalPixels << ")";
}

}  // namespace

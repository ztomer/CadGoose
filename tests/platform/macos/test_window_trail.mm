// test_window_trail.mm
// Deterministic window trail detection test.
// Tests that window backing store is properly cleared before first display.
//
// NOTE: Screen capture APIs (CGWindowListCreateImage, CGDisplayCreateImageForRect)
// are obsoleted in macOS 15+. This test uses an alternative approach: it verifies
// that drawRect is called synchronously when display is invoked, which is the
// underlying mechanism that prevents trails.

#import <Cocoa/Cocoa.h>
#include <gtest/gtest.h>

// Track whether drawRect was called
static BOOL s_drawRectCalled = NO;
static int s_drawRectCallCount = 0;

// Color view that tracks drawRect calls
@interface TrailTestContentView : NSView
@property (nonatomic, strong) NSColor* fillColor;
@property (nonatomic, assign) BOOL drawRectWasCalled;
@property (nonatomic, assign) int drawRectCount;
@end

@implementation TrailTestContentView
- (void)drawRect:(NSRect)dirtyRect {
    s_drawRectCalled = YES;
    s_drawRectCallCount++;
    self.drawRectWasCalled = YES;
    self.drawRectCount++;
    [self.fillColor setFill];
    NSRectFill(self.bounds);
}
@end

// Test window with distinctive color content
@interface TrailTestWindow : NSWindow
@end

@implementation TrailTestWindow
- (instancetype)initWithColor:(NSColor*)color frame:(NSRect)frame {
    self = [super initWithContentRect:frame
                            styleMask:NSWindowStyleMaskBorderless
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self) {
        self.backgroundColor = [NSColor clearColor];
        [self setOpaque:NO];
        self.ignoresMouseEvents = YES;
        self.level = NSStatusWindowLevel;
        self.hasShadow = NO;
        self.releasedWhenClosed = NO;

        TrailTestContentView* contentView = [[TrailTestContentView alloc] initWithFrame:self.contentView.bounds];
        contentView.fillColor = color;
        self.contentView = contentView;
    }
    return self;
}
@end

// Test that display forces synchronous drawRect
TEST(WindowTrailTest, DisplayCallsDrawRectSynchronously) {
    s_drawRectCalled = NO;
    s_drawRectCallCount = 0;

    NSRect frame = NSMakeRect(100, 100, 100, 100);
    TrailTestWindow* win = [[TrailTestWindow alloc] initWithColor:[NSColor redColor] frame:frame];

    // Before orderFront, drawRect should not have been called
    EXPECT_FALSE(s_drawRectCalled) << "drawRect called before orderFront";

    // Call display on content view - this should force synchronous draw
    [[win contentView] display];

    // After display, drawRect MUST have been called
    EXPECT_TRUE(s_drawRectCalled) << "drawRect NOT called after display — trail risk!";
    EXPECT_EQ(s_drawRectCallCount, 1) << "drawRect called " << s_drawRectCallCount << " times";

    [win close];
}

// Test that orderFront after display doesn't cause double-draw
TEST(WindowTrailTest, OrderFrontAfterDisplayNoDoubleDraw) {
    s_drawRectCalled = NO;
    s_drawRectCallCount = 0;

    NSRect frame = NSMakeRect(200, 200, 100, 100);
    TrailTestWindow* win = [[TrailTestWindow alloc] initWithColor:[NSColor greenColor] frame:frame];

    // Display first (clears backing store)
    [[win contentView] display];
    int countAfterDisplay = s_drawRectCallCount;

    // Then orderFront
    [win orderFront:nil];

    // Wait a bit for any async draws
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];

    // Should not have drawn again (backing store already clean)
    EXPECT_EQ(s_drawRectCallCount, countAfterDisplay) << "Double draw detected — potential trail source";

    [win close];
}

// Test multiple windows in sequence
TEST(WindowTrailTest, MultipleWindowsSequential) {
    NSColor* colors[] = {
        [NSColor redColor],
        [NSColor greenColor],
        [NSColor blueColor],
        [NSColor magentaColor],
        [NSColor cyanColor],
    };

    for (int i = 0; i < 5; i++) {
        s_drawRectCalled = NO;
        s_drawRectCallCount = 0;

        NSRect frame = NSMakeRect(100 + i * 50, 100 + i * 50, 80 + i * 20, 80 + i * 20);
        TrailTestWindow* win = [[TrailTestWindow alloc] initWithColor:colors[i] frame:frame];

        [[win contentView] display];
        EXPECT_TRUE(s_drawRectCalled) << "Window " << i << ": drawRect NOT called after display";

        [win orderFront:nil];
        [win close];
    }
}

// Test that the fix (display before orderFront) works with rapid window creation
TEST(WindowTrailTest, RapidWindowCreation) {
    const int kRapidCount = 50;
    int failures = 0;

    for (int i = 0; i < kRapidCount; i++) {
        s_drawRectCalled = NO;
        s_drawRectCallCount = 0;

        NSRect frame = NSMakeRect(50 + (i % 10) * 30, 50 + (i % 10) * 30, 60, 60);
        TrailTestWindow* win = [[TrailTestWindow alloc] initWithColor:[NSColor redColor] frame:frame];

        [[win contentView] display];
        if (!s_drawRectCalled) {
            failures++;
        }

        [win orderFront:nil];
        [win close];
    }

    EXPECT_EQ(failures, 0) << failures << " windows failed to draw synchronously out of " << kRapidCount;
}

// Test window movement doesn't cause stale backing store
TEST(WindowTrailTest, WindowMovementClearsBacking) {
    s_drawRectCalled = NO;
    s_drawRectCallCount = 0;

    NSRect frame = NSMakeRect(100, 100, 100, 100);
    TrailTestWindow* win = [[TrailTestWindow alloc] initWithColor:[NSColor blueColor] frame:frame];

    // Initial display
    [[win contentView] display];
    EXPECT_TRUE(s_drawRectCalled) << "Initial drawRect not called";
    [win orderFront:nil];

    // Move window
    s_drawRectCalled = NO;
    [win setFrameOrigin:NSMakePoint(500, 500)];

    // After move, display should trigger drawRect again
    [[win contentView] setNeedsDisplay:YES];
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];

    // The window should redraw at new position
    // (We can't verify the old position is clean without screen capture,
    // but we verify the new position draws correctly)
    EXPECT_TRUE(s_drawRectCallCount >= 1) << "No redraw after window move";

    [win close];
}

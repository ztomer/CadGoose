// test_window_lifecycle.mm
// Systematic investigation of window trail issue.
// Tracks buffer state through window creation → orderFront → drawRect sequence.

#import "item_window.h"
#import "world.h"
#import "config.h"
#import "coordinate_system.h"
#import "actor.h"
#import "actor_dropped_item.h"
#import "goose.h"
#import "window.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreGraphics/CoreGraphics.h>
#import <gtest/gtest.h>

#include <cstdio>
#include <chrono>
#include <vector>

static std::chrono::high_resolution_clock::time_point Now() {
    return std::chrono::high_resolution_clock::now();
}

static double ElapsedMs(std::chrono::high_resolution_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Now() - start).count();
}

// Capture screen region as pixel data for comparison
static std::vector<uint8_t> CaptureScreenRegion(NSRect frame) {
    CGImageRef screenshot = CGWindowListCreateImage(
        frame,
        kCGWindowListOptionOnScreenOnly,
        kCGNullWindowID,
        kCGWindowImageDefault
    );
    if (!screenshot) return {};

    size_t width = CGImageGetWidth(screenshot);
    size_t height = CGImageGetHeight(screenshot);
    size_t bytesPerRow = CGImageGetBytesPerRow(screenshot);
    size_t bytesPerPixel = 4;
    size_t dataSize = bytesPerRow * height;

    std::vector<uint8_t> pixels(dataSize);

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        pixels.data(), width, height, 8, bytesPerRow,
        colorSpace, kCGImageAlphaPremultipliedFirst
    );
    CGColorSpaceRelease(colorSpace);

    CGContextDrawImage(context, CGRectMake(0, 0, width, height), screenshot);
    CGContextRelease(context);
    CGImageRelease(screenshot);

    return pixels;
}

// Check if a pixel buffer is all zeros (fully transparent/black)
static bool IsBufferClear(const std::vector<uint8_t>& pixels) {
    for (size_t i = 0; i < pixels.size(); i += 4) {
        // Check RGBA - clear means all zeros or alpha=0
        if (pixels[i] != 0 || pixels[i+1] != 0 || pixels[i+2] != 0 || pixels[i+3] != 0) {
            return false;
        }
    }
    return true;
}

// Count non-zero pixels (to measure "dirtiness" of buffer)
static size_t CountDirtyPixels(const std::vector<uint8_t>& pixels) {
    size_t count = 0;
    for (size_t i = 0; i < pixels.size(); i += 4) {
        if (pixels[i] != 0 || pixels[i+1] != 0 || pixels[i+2] != 0 || pixels[i+3] != 0) {
            count++;
        }
    }
    return count;
}

// ============================================================
// Instrumented ItemWindow subclass
// ============================================================

@interface InstrumentedItemWindow : ItemWindow
@property (nonatomic, assign) double orderFrontTime;
@property (nonatomic, assign) double firstDrawRectTime;
@property (nonatomic, assign) BOOL hasDrawn;
@property (nonatomic, assign) size_t dirtyPixelsAfterOrderFront;
@property (nonatomic, assign) size_t dirtyPixelsAfterDrawRect;
@end

@implementation InstrumentedItemWindow {
    std::chrono::high_resolution_clock::time_point _creationTime;
}

- (instancetype)initWithItem:(DroppedItem*)item {
    _creationTime = Now();
    self = [super initWithItem:item];
    if (self) {
        _orderFrontTime = 0;
        _firstDrawRectTime = 0;
        _hasDrawn = NO;
        _dirtyPixelsAfterOrderFront = 0;
        _dirtyPixelsAfterDrawRect = 0;
    }
    return self;
}

- (void)orderFront:(id)sender {
    _orderFrontTime = ElapsedMs(_creationTime);
    fprintf(stderr, "[LIFECYCLE] orderFront at %.2fms after creation\n", _orderFrontTime);

    // Capture screen state immediately after orderFront
    // Give the window server a moment to composite
    usleep(1000);  // 1ms

    NSRect frame = self.frame;
    auto pixels = CaptureScreenRegion(frame);
    if (!pixels.empty()) {
        _dirtyPixelsAfterOrderFront = CountDirtyPixels(pixels);
        fprintf(stderr, "[LIFECYCLE] buffer after orderFront: %zu dirty pixels (total=%zu)\n",
                _dirtyPixelsAfterOrderFront, pixels.size() / 4);
    }

    [super orderFront:sender];
}

- (void)drawRect:(NSRect)dirtyRect {
    if (!_hasDrawn) {
        _firstDrawRectTime = ElapsedMs(_creationTime);
        _hasDrawn = YES;
        fprintf(stderr, "[LIFECYCLE] FIRST drawRect at %.2fms after creation (%.2fms after orderFront)\n",
                _firstDrawRectTime, _firstDrawRectTime - _orderFrontTime);
    }

    [super drawRect:dirtyRect];

    // Capture buffer state after drawRect
    NSRect frame = self.frame;
    auto pixels = CaptureScreenRegion(frame);
    if (!pixels.empty()) {
        _dirtyPixelsAfterDrawRect = CountDirtyPixels(pixels);
        fprintf(stderr, "[LIFECYCLE] buffer after drawRect: %zu dirty pixels (total=%zu)\n",
                _dirtyPixelsAfterDrawRect, pixels.size() / 4);
    }
}

@end

// ============================================================
// Tests
// ============================================================

TEST(WindowLifecycle, OrderFrontBeforeDrawRect) {
    // This test verifies the timing gap between orderFront and drawRect
    // which is the root cause of the window trail issue.

    __block InstrumentedItemWindow* win = nil;
    __block DroppedItem testItem;
    ItemData itemData;
    itemData.type = ItemData::MEME;
    itemData.w = 100;
    itemData.h = 100;
    itemData.image = nil;  // No image needed for this test

    testItem.data = &itemData;
    testItem.pos = {500, 500};
    testItem.rotation = 0;
    testItem.timeDropped = 0;
    testItem.pinned = false;

    dispatch_sync(dispatch_get_main_queue(), ^{
        win = [[InstrumentedItemWindow alloc] initWithItem:&testItem];
        [win orderFront:nil];

        // Run the runloop briefly to allow drawRect to fire
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    });

    fprintf(stderr, "[TEST] orderFront time: %.2fms\n", win.orderFrontTime);
    fprintf(stderr, "[TEST] first drawRect time: %.2fms\n", win.firstDrawRectTime);
    fprintf(stderr, "[TEST] gap: %.2fms\n", win.firstDrawRectTime - win.orderFrontTime);
    fprintf(stderr, "[TEST] dirty pixels after orderFront: %zu\n", win.dirtyPixelsAfterOrderFront);
    fprintf(stderr, "[TEST] dirty pixels after drawRect: %zu\n", win.dirtyPixelsAfterDrawRect);

    // The gap between orderFront and drawRect is the problem window
    // During this time, the window server composites uninitialized backing store
    EXPECT_GT(win.firstDrawRectTime, win.orderFrontTime) << "drawRect should fire after orderFront";
    EXPECT_GT(win.firstDrawRectTime - win.orderFrontTime, 0) << "There should be a measurable gap";
}

TEST(WindowLifecycle, GooseWindowSizeVsHeldItem) {
    // This test verifies whether the goose window is large enough to contain
    // the held item at all rotation angles.

    Goose* goose = new Goose(0, "Test", 1920, 1080);
    goose->pos = {960, 540};
    goose->target = {960, 540};

    // Create a held item
    ItemData* item = g_assets.GetRandomMeme(1920, 1080, 0.1f);
    ASSERT_NE(item, nullptr);

    goose->heldItem = item;
    goose->state = GooseState::RETURNING;

    // Calculate item extent at various rotation angles
    float scale = g_config.general.globalScale;
    float itemW = item->w * scale;
    float itemH = item->h * scale;

    fprintf(stderr, "[TEST] item dimensions: %.0fx%.0f (scaled: %.0fx%.0f)\n",
            item->w, item->h, itemW, itemH);

    float maxExtent = 0;
    float worstAngle = 0;

    for (int deg = 0; deg < 360; deg += 5) {
        float rad = deg * M_PI / 180.0f;
        float cosA = std::abs(std::cos(rad));
        float sinA = std::abs(std::sin(rad));
        float rotatedW = itemW * cosA + itemH * sinA;
        float rotatedH = itemW * sinA + itemH * cosA;
        float extent = std::max(rotatedW, rotatedH) * 0.5f;

        if (extent > maxExtent) {
            maxExtent = extent;
            worstAngle = deg;
        }
    }

    fprintf(stderr, "[TEST] max item extent: %.0f at angle %.0f°\n", maxExtent, worstAngle);

    // Distance from goose center to beak tip
    Vector2 neckHeadDev = WorldCoord::RigNeckHead(*goose).toVector2();
    float distToBeak = Vector2::Distance({goose->pos.x, goose->pos.y}, neckHeadDev);
    fprintf(stderr, "[TEST] dist to beak: %.0f\n", distToBeak);

    // Total extent from goose center
    float kHeldItemBeakOffset = 5.0f;
    float kHeldItemPadding = 40.0f;
    float itemBehindBeak = itemW + kHeldItemBeakOffset;
    float totalExtent = distToBeak + itemBehindBeak + maxExtent + kHeldItemPadding;
    float requiredWindowSize = totalExtent * 2.0f;

    fprintf(stderr, "[TEST] total extent from center: %.0f\n", totalExtent);
    fprintf(stderr, "[TEST] required window size: %.0f\n", requiredWindowSize);

    // Goose window is currently full-screen, so this should always pass
    // But we log it for future reference if we ever reduce window size
    NSScreen* mainScreen = [NSScreen mainScreen];
    float screenSize = std::max(mainScreen.frame.size.width, mainScreen.frame.size.height);
    fprintf(stderr, "[TEST] screen size: %.0f\n", screenSize);
    fprintf(stderr, "[TEST] window fits: %s\n", requiredWindowSize <= screenSize ? "YES" : "NO");

    delete goose;
}

TEST(WindowLifecycle, DropSequenceTiming) {
    // This test tracks the timing of the drop sequence:
    // 1. handleReturning creates DroppedItemActor
    // 2. ItemWindow is created
    // 3. orderFront makes it visible
    // 4. drawRect clears the buffer

    __block double actorCreateTime = 0;
    __block double windowCreateTime = 0;
    __block double orderFrontTime = 0;
    __block double firstDrawRectTime = 0;

    dispatch_sync(dispatch_get_main_queue(), ^{
        auto startTime = Now();

        // Create a DroppedItemActor (simulating what handleReturning does)
        ItemData* item = g_assets.GetRandomMeme(1920, 1080, 0.1f);
        DroppedItem drop;
        drop.data = item;
        drop.pos = {500, 500};
        drop.rotation = 0;
        drop.timeDropped = 0;
        drop.pinned = false;

        actorCreateTime = ElapsedMs(startTime);
        fprintf(stderr, "[TEST] DroppedItemActor created at %.2fms\n", actorCreateTime);

        // This triggers ItemWindow creation
        auto* actor = new DroppedItemActor(drop);
        windowCreateTime = ElapsedMs(startTime);
        fprintf(stderr, "[TEST] ItemWindow created at %.2fms\n", windowCreateTime);

        // Run runloop to allow drawRect
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
        firstDrawRectTime = ElapsedMs(startTime);
        fprintf(stderr, "[TEST] drawRect fired by %.2fms\n", firstDrawRectTime);

        actor->setActive(false);
    });

    fprintf(stderr, "[TEST] actor creation → window creation: %.2fms\n", windowCreateTime - actorCreateTime);
    fprintf(stderr, "[TEST] window creation → drawRect: %.2fms\n", firstDrawRectTime - windowCreateTime);
}

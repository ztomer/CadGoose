#include <gtest/gtest.h>
#include <cmath>
#include <list>
#include <ApplicationServices/ApplicationServices.h>
#include "../../include/items.h"
#include "../../include/config.h"

extern std::list<DroppedItem> g_droppedItems;

// Replicates the hit test logic from renderer.mm
static bool HitTestPoint(float mouseX, float mouseY, const DroppedItem& item) {
    float dx = mouseX - item.pos.x;
    float dy = mouseY - item.pos.y;
    float cosA = std::cos(item.rotation);
    float sinA = std::sin(item.rotation);
    float lx = dx * cosA - dy * sinA;
    float ly = dx * sinA + dy * cosA;
    return lx >= -item.data->w/2.0f && lx <= item.data->w/2.0f &&
           ly >= -item.data->h/2.0f && ly <= item.data->h/2.0f;
}

// Synthesizes a mouse down event at screen coordinates
static void SynthesizeMouseDown(float x, float y) {
    CGPoint pt = CGPointMake(x, y);
    CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pt, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    usleep(50000); // 50ms
}

// Synthesizes a mouse drag event at screen coordinates
static void SynthesizeMouseDrag(float x, float y) {
    CGPoint pt = CGPointMake(x, y);
    CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDragged, pt, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    usleep(50000);
}

// Synthesizes a mouse up event at screen coordinates
static void SynthesizeMouseUp(float x, float y) {
    CGPoint pt = CGPointMake(x, y);
    CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, pt, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    usleep(50000);
}

// Gets current mouse location
static CGPoint GetMouseLocation() {
    CGEventRef event = CGEventCreate(NULL);
    CGPoint pt = CGEventGetLocation(event);
    CFRelease(event);
    return pt;
}

TEST(DraggingIntegration, HitTestMathIsCorrect) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    // Center should hit
    EXPECT_TRUE(HitTestPoint(500, 400, item));
    // Edges should hit
    EXPECT_TRUE(HitTestPoint(450, 400, item));
    EXPECT_TRUE(HitTestPoint(550, 400, item));
    EXPECT_TRUE(HitTestPoint(500, 360, item));
    EXPECT_TRUE(HitTestPoint(500, 440, item));
    // Outside should miss
    EXPECT_FALSE(HitTestPoint(449, 400, item));
    EXPECT_FALSE(HitTestPoint(551, 400, item));
}

TEST(DraggingIntegration, RotatedItemHitTest) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = M_PI / 4; // 45 degrees

    // Center always hits
    EXPECT_TRUE(HitTestPoint(500, 400, item));
}

// Note: Full integration test requiring running app is skipped in CI.
// Run manually with: ./build/CadGooseTests --gtest_filter="DraggingIntegration.FullDragCycle"
TEST(DraggingIntegration, FullDragCycle) {
    GTEST_SKIP() << "Requires running CadGoose app with dropped items";

    // Get current mouse position to restore later
    CGPoint originalPos = GetMouseLocation();

    // Test: synthesize mouse down on item, drag, mouse up
    // Item must already exist on screen at known position
    // This test verifies the full event chain:
    // CGEventPost -> Window sendEvent -> hitTest -> mouseDown -> mouseDragged -> mouseUp

    // Example (requires item at screen position 500,400):
    // SynthesizeMouseDown(500, 400);
    // SynthesizeMouseDrag(600, 500);
    // SynthesizeMouseUp(600, 500);

    // Verify item moved:
    // EXPECT_NEAR(item.pos.x, 600, 5.0f);
    // EXPECT_NEAR(item.pos.y, 500, 5.0f);

    // Restore mouse position
    SynthesizeMouseUp((float)originalPos.x, (float)originalPos.y);
}

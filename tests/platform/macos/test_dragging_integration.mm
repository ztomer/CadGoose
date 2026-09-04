#include <gtest/gtest.h>
#include <cmath>
#include <list>
#include <ApplicationServices/ApplicationServices.h>
#include "../../include/items.h"
#include "../../include/config.h"



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

// Replicates drag offset calculation from mouseDownAtPoint:viewY:
static NSPoint CalcDragOffset(NSPoint clickPoint, float viewY, const DroppedItem& item) {
    return NSMakePoint(item.pos.x - clickPoint.x, item.pos.y - viewY);
}

// Replicates position update from mouseDraggedAtPoint:viewY:
static void UpdateDragPosition(DroppedItem& item, NSPoint currentPoint, float viewY, NSPoint dragOffset) {
    item.pos.x = currentPoint.x + dragOffset.x;
    item.pos.y = viewY + dragOffset.y;
}

// Synthesizes a mouse down event at screen coordinates
static void SynthesizeMouseDown(float x, float y) {
    CGPoint pt = CGPointMake(x, y);
    CGEventRef event = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, pt, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
    usleep(50000);
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

// ============================================================
// Hit Test Math
// ============================================================

TEST(DraggingIntegration, HitTestMathIsCorrect) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    EXPECT_TRUE(HitTestPoint(500, 400, item));
    EXPECT_TRUE(HitTestPoint(450, 400, item));
    EXPECT_TRUE(HitTestPoint(550, 400, item));
    EXPECT_TRUE(HitTestPoint(500, 360, item));
    EXPECT_TRUE(HitTestPoint(500, 440, item));
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
    item.rotation = M_PI / 4;

    EXPECT_TRUE(HitTestPoint(500, 400, item));
}

// ============================================================
// Drag Offset Calculation
// ============================================================

TEST(DraggingIntegration, DragOffsetCalculation) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    // Click at center
    NSPoint click = NSMakePoint(500, 300); // viewY = 300 (flipped)
    NSPoint offset = CalcDragOffset(click, 300, item);
    EXPECT_FLOAT_EQ(offset.x, 0);
    EXPECT_FLOAT_EQ(offset.y, 100);

    // Click at top-left corner
    click = NSMakePoint(450, 260);
    offset = CalcDragOffset(click, 260, item);
    EXPECT_FLOAT_EQ(offset.x, 50);
    EXPECT_FLOAT_EQ(offset.y, 140);
}

// ============================================================
// Drag Position Update
// ============================================================

TEST(DraggingIntegration, DragPositionUpdate) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    // Simulate click at center
    NSPoint click = NSMakePoint(500, 300);
    NSPoint offset = CalcDragOffset(click, 300, item);

    // Drag to new position
    NSPoint newPoint = NSMakePoint(600, 350);
    UpdateDragPosition(item, newPoint, 350, offset);

    EXPECT_FLOAT_EQ(item.pos.x, 600);
    EXPECT_FLOAT_EQ(item.pos.y, 450);

    // Drag back
    newPoint = NSMakePoint(400, 200);
    UpdateDragPosition(item, newPoint, 200, offset);

    EXPECT_FLOAT_EQ(item.pos.x, 400);
    EXPECT_FLOAT_EQ(item.pos.y, 300);
}

// ============================================================
// Full Drag Cycle (simulated)
// ============================================================

TEST(DraggingIntegration, SimulatedFullDragCycle) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    // Phase 1: Mouse down on item
    NSPoint downPoint = NSMakePoint(520, 380);
    float downViewY = 380;
    EXPECT_TRUE(HitTestPoint(downPoint.x, downViewY, item));

    NSPoint dragOffset = CalcDragOffset(downPoint, downViewY, item);

    // Phase 2: Drag to new position
    NSPoint dragPoint = NSMakePoint(700, 500);
    float dragViewY = 500;
    UpdateDragPosition(item, dragPoint, dragViewY, dragOffset);

    EXPECT_FLOAT_EQ(item.pos.x, 680);
    EXPECT_FLOAT_EQ(item.pos.y, 520);

    // Phase 3: Mouse up (item stays at new position)
    // draggedItem = nil (simulated)
    EXPECT_FLOAT_EQ(item.pos.x, 680);
    EXPECT_FLOAT_EQ(item.pos.y, 520);
}

// ============================================================
// Edge Cases
// ============================================================

TEST(DraggingIntegration, DragFromCorner) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = 0;

    // Click at top-left corner
    NSPoint click = NSMakePoint(450, 360);
    EXPECT_TRUE(HitTestPoint(click.x, click.y, item));

    NSPoint offset = CalcDragOffset(click, 360, item);
    EXPECT_FLOAT_EQ(offset.x, 50);
    EXPECT_FLOAT_EQ(offset.y, 40);

    // Drag should maintain corner under cursor
    NSPoint newPoint = NSMakePoint(600, 500);
    UpdateDragPosition(item, newPoint, 500, offset);

    EXPECT_FLOAT_EQ(item.pos.x, 650);
    EXPECT_FLOAT_EQ(item.pos.y, 540);
}

TEST(DraggingIntegration, DragWithRotation) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {500, 400};
    item.rotation = M_PI / 6; // 30 degrees

    // Center always hits regardless of rotation
    EXPECT_TRUE(HitTestPoint(500, 400, item));

    // Drag should work the same regardless of rotation
    NSPoint click = NSMakePoint(500, 400);
    NSPoint offset = CalcDragOffset(click, 400, item);

    NSPoint newPoint = NSMakePoint(600, 450);
    UpdateDragPosition(item, newPoint, 450, offset);

    EXPECT_FLOAT_EQ(item.pos.x, 600);
    EXPECT_FLOAT_EQ(item.pos.y, 450);
}

// ============================================================
// CGEvent Synthesis (requires GUI session, skipped in CI)
// ============================================================

TEST(DraggingIntegration, CGEventSynthesis) {
    GTEST_SKIP() << "Requires active GUI session with Accessibility permissions";

    CGPoint originalPos = GetMouseLocation();

    SynthesizeMouseDown(500, 400);
    SynthesizeMouseDrag(600, 500);
    SynthesizeMouseUp(600, 500);

    SynthesizeMouseUp((float)originalPos.x, (float)originalPos.y);
}

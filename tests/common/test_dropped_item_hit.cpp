#include <gtest/gtest.h>
#include <cmath>
#include <list>
#include "../../include/items.h"
#include "../../include/config.h"



// Replicates the hit test logic from renderer.mm
// item.pos is in view/device coordinates (top-left origin, Y-down, matching isFlipped=YES view)
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

TEST(DroppedItemHitTest, CenterHits) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {200, 150};
    item.rotation = 0;

    // Center of item in view coords (same space as pos)
    EXPECT_TRUE(HitTestPoint(item.pos.x, item.pos.y, item));

    // Slightly off center
    EXPECT_TRUE(HitTestPoint(item.pos.x + 10, item.pos.y + 10, item));
    EXPECT_TRUE(HitTestPoint(item.pos.x - 10, item.pos.y - 10, item));
}

TEST(DroppedItemHitTest, EdgesHit) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {200, 150};
    item.rotation = 0;

    // Left edge (should hit)
    EXPECT_TRUE(HitTestPoint(item.pos.x - 50, item.pos.y, item));
    // Right edge (should hit)
    EXPECT_TRUE(HitTestPoint(item.pos.x + 50, item.pos.y, item));
    // Top edge (should hit)
    EXPECT_TRUE(HitTestPoint(item.pos.x, item.pos.y - 40, item));
    // Bottom edge (should hit)
    EXPECT_TRUE(HitTestPoint(item.pos.x, item.pos.y + 40, item));
}

TEST(DroppedItemHitTest, CornersHit) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {200, 150};
    item.rotation = 0;

    // Four corners (should hit)
    EXPECT_TRUE(HitTestPoint(item.pos.x - 50, item.pos.y - 40, item));
    EXPECT_TRUE(HitTestPoint(item.pos.x + 50, item.pos.y - 40, item));
    EXPECT_TRUE(HitTestPoint(item.pos.x - 50, item.pos.y + 40, item));
    EXPECT_TRUE(HitTestPoint(item.pos.x + 50, item.pos.y + 40, item));
}

TEST(DroppedItemHitTest, OutsideMisses) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {200, 150};
    item.rotation = 0;

    // Just outside each edge (should miss)
    EXPECT_FALSE(HitTestPoint(item.pos.x - 51, item.pos.y, item));
    EXPECT_FALSE(HitTestPoint(item.pos.x + 51, item.pos.y, item));
    EXPECT_FALSE(HitTestPoint(item.pos.x, item.pos.y - 41, item));
    EXPECT_FALSE(HitTestPoint(item.pos.x, item.pos.y + 41, item));

    // Far away (should miss)
    EXPECT_FALSE(HitTestPoint(0, 0, item));
    EXPECT_FALSE(HitTestPoint(1000, 1000, item));
}

TEST(DroppedItemHitTest, RotatedItem) {
    ItemData data;
    data.w = 100;
    data.h = 80;
    data.type = ItemData::MEME;

    DroppedItem item;
    item.data = &data;
    item.pos = {200, 150};
    item.rotation = M_PI / 4;  // 45 degrees

    // Center should always hit regardless of rotation
    EXPECT_TRUE(HitTestPoint(item.pos.x, item.pos.y, item));

    // Point along rotated axis should hit
    float rotatedX = item.pos.x + 30 * std::cos(item.rotation);
    float rotatedY = item.pos.y + 30 * std::sin(item.rotation);
    EXPECT_TRUE(HitTestPoint(rotatedX, rotatedY, item));
}

TEST(DroppedItemHitTest, PositionConsistency) {
    // Verify that item.pos and mouse coordinates are in the same space (no Y-flip needed)
    ItemData data;
    data.w = 50;
    data.h = 50;
    data.type = ItemData::MEME;

    DroppedItem itemTop;
    itemTop.data = &data;
    itemTop.pos = {100, 50};  // Near top of view
    itemTop.rotation = 0;

    DroppedItem itemBottom;
    itemBottom.data = &data;
    itemBottom.pos = {100, 550};  // Near bottom of view
    itemBottom.rotation = 0;

    // Item at view Y=50 should be hit at mouse Y=50
    EXPECT_TRUE(HitTestPoint(itemTop.pos.x, itemTop.pos.y, itemTop));
    EXPECT_FALSE(HitTestPoint(itemTop.pos.x, 550, itemTop));

    // Item at view Y=550 should be hit at mouse Y=550
    EXPECT_TRUE(HitTestPoint(itemBottom.pos.x, itemBottom.pos.y, itemBottom));
    EXPECT_FALSE(HitTestPoint(itemBottom.pos.x, 50, itemBottom));
}

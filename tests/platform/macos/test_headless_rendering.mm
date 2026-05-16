#include <gtest/gtest.h>
#include <cmath>
#include <list>
#include "../../include/coordinate_system.h"
#include "../../include/item_drag_controller.h"
#include "../../include/config.h"
#include "../../include/items.h"

extern std::list<DroppedItem> g_droppedItems;

// ============================================================
// HitTest at Different Scales
// ============================================================

TEST(HeadlessRendering, HitTestAtScale_0_5x) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 0.5f;

    // Scaled half-size: 25x20
    EXPECT_TRUE(HitTest::PointInItem({500, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({520, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({500, 415}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({530, 400}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({500, 425}, itemPos, width, height, 0, scale));
}

TEST(HeadlessRendering, HitTestAtScale_1_0x) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 1.0f;

    // Scaled half-size: 50x40
    EXPECT_TRUE(HitTest::PointInItem({500, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({540, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({500, 430}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({555, 400}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({500, 445}, itemPos, width, height, 0, scale));
}

TEST(HeadlessRendering, HitTestAtScale_2_0x) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 2.0f;

    // Scaled half-size: 100x80
    EXPECT_TRUE(HitTest::PointInItem({500, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({580, 400}, itemPos, width, height, 0, scale));
    EXPECT_TRUE(HitTest::PointInItem({500, 460}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({605, 400}, itemPos, width, height, 0, scale));
    EXPECT_FALSE(HitTest::PointInItem({500, 485}, itemPos, width, height, 0, scale));
}

TEST(HeadlessRendering, HitTestWithRotation) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 1.0f;

    // At 45 degrees, corners move but center stays
    EXPECT_TRUE(HitTest::PointInItem({500, 400}, itemPos, width, height, M_PI_4, scale));

    // Point that was inside at 0 degrees may be outside at 45 degrees
    // For a 100x80 rect rotated 45°, the bounding box expands
    // But the point (540, 400) is on the edge at 0° — should still hit at 45°
    // due to the rotation bringing the longer axis toward it
    float dx = 40, dy = 0;
    float cosA = std::cos(M_PI_4), sinA = std::sin(M_PI_4);
    float lx = dx * cosA - dy * sinA; // ~28.3
    float ly = dx * sinA + dy * cosA; // ~28.3
    // halfW=50, halfH=40, so lx=28.3 < 50, ly=28.3 < 40 → hits
    EXPECT_TRUE(HitTest::PointInItem({540, 400}, itemPos, width, height, M_PI_4, scale));
}

// ============================================================
// Close Button Hit-Test
// ============================================================

TEST(HeadlessRendering, CloseButtonHitTest_NonToy) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 1.0f;
    float buttonSize = 16.0f;

    // Item spans [450, 550] x [360, 440]
    // Close button at top-left: [450, 466] x [360, 376]
    EXPECT_TRUE(HitTest::PointInCloseButton({455, 365}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_TRUE(HitTest::PointInCloseButton({450, 360}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_TRUE(HitTest::PointInCloseButton({466, 376}, itemPos, width, height, 0, buttonSize, scale));

    // Outside button area
    EXPECT_FALSE(HitTest::PointInCloseButton({470, 365}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_FALSE(HitTest::PointInCloseButton({455, 380}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_FALSE(HitTest::PointInCloseButton({500, 400}, itemPos, width, height, 0, buttonSize, scale));
}

TEST(HeadlessRendering, CloseButtonHitTest_AtScale_2x) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 2.0f;
    float buttonSize = 16.0f;

    // Scaled item: halfW=100, halfH=80
    // Item spans [400, 600] x [320, 480]
    // Close button at top-left: [400, 416] x [320, 336]
    EXPECT_TRUE(HitTest::PointInCloseButton({405, 325}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_FALSE(HitTest::PointInCloseButton({420, 325}, itemPos, width, height, 0, buttonSize, scale));
    EXPECT_FALSE(HitTest::PointInCloseButton({405, 340}, itemPos, width, height, 0, buttonSize, scale));
}

TEST(HeadlessRendering, CloseButtonHitTest_Rotated) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;
    float scale = 1.0f;
    float buttonSize = 16.0f;
    float rotation = M_PI_4;

    // Button in local space: [-50, -34] x [-40, -24]
    // Center of button in local space: (-42, -32)
    // To find screen point, apply inverse rotation:
    //   dx = -42*cos45 + (-32)*sin45 ≈ -52.3
    //   dy = 42*sin45 + (-32)*cos45 ≈ 7.1
    // Screen point: {447.7, 407.1}
    EXPECT_TRUE(HitTest::PointInCloseButton({448, 407}, itemPos, width, height, rotation, buttonSize, scale));

    // Point outside button area
    EXPECT_FALSE(HitTest::PointInCloseButton({500, 360}, itemPos, width, height, rotation, buttonSize, scale));
}

// ============================================================
// Coordinate Transforms
// ============================================================

TEST(HeadlessRendering, WorldToDevice_AtDifferentScales) {
    DevicePoint goosePos{1000, 800};
    WorldPoint worldPos{1050, 830}; // 50px right, 30px down from goose in world units

    DevicePoint result_1x = CoordTransform::WorldToDevice(worldPos, goosePos, 1.0f);
    EXPECT_FLOAT_EQ(result_1x.x, 1050);
    EXPECT_FLOAT_EQ(result_1x.y, 830);

    DevicePoint result_2x = CoordTransform::WorldToDevice(worldPos, goosePos, 2.0f);
    EXPECT_FLOAT_EQ(result_2x.x, 1100); // 1000 + (50 * 2)
    EXPECT_FLOAT_EQ(result_2x.y, 860);  // 800 + (30 * 2)

    DevicePoint result_0_5x = CoordTransform::WorldToDevice(worldPos, goosePos, 0.5f);
    EXPECT_FLOAT_EQ(result_0_5x.x, 1025); // 1000 + (50 * 0.5)
    EXPECT_FLOAT_EQ(result_0_5x.y, 815);  // 800 + (30 * 0.5)
}

TEST(HeadlessRendering, DeviceToWorld_Inverse) {
    DevicePoint goosePos{1000, 800};
    DevicePoint devicePos{1100, 860}; // 2x scale result

    WorldPoint result = CoordTransform::DeviceToWorld(devicePos, goosePos, 2.0f);
    EXPECT_FLOAT_EQ(result.x, 1050);
    EXPECT_FLOAT_EQ(result.y, 830);
}

TEST(HeadlessRendering, DeviceToWorld_RoundTrip) {
    DevicePoint goosePos{500, 400};
    WorldPoint original{550, 430};
    float scale = 1.5f;

    DevicePoint device = CoordTransform::WorldToDevice(original, goosePos, scale);
    WorldPoint roundTrip = CoordTransform::DeviceToWorld(device, goosePos, scale);

    EXPECT_FLOAT_EQ(roundTrip.x, original.x);
    EXPECT_FLOAT_EQ(roundTrip.y, original.y);
}

TEST(HeadlessRendering, ViewToDevice) {
    ViewPoint viewPt{100, 200};
    float viewY = 300;

    DevicePoint device = CoordTransform::ViewToDevice(viewPt, viewY);
    EXPECT_FLOAT_EQ(device.x, 100);
    EXPECT_FLOAT_EQ(device.y, 300); // Uses viewY, not viewPt.y
}

// ============================================================
// ItemCoords
// ============================================================

TEST(HeadlessRendering, ItemCoords_Center) {
    DevicePoint itemPos{500, 400};
    float width = 100, height = 80;

    DevicePoint center_1x = ItemCoords::Center(itemPos, width, height, 1.0f);
    EXPECT_FLOAT_EQ(center_1x.x, 550); // 500 + 100/2
    EXPECT_FLOAT_EQ(center_1x.y, 440); // 400 + 80/2

    DevicePoint center_2x = ItemCoords::Center(itemPos, width, height, 2.0f);
    EXPECT_FLOAT_EQ(center_2x.x, 600); // 500 + 200/2
    EXPECT_FLOAT_EQ(center_2x.y, 480); // 400 + 160/2
}

TEST(HeadlessRendering, ItemCoords_HalfSize) {
    DevicePoint half_1x = ItemCoords::HalfSize(100, 80, 1.0f);
    EXPECT_FLOAT_EQ(half_1x.x, 50);
    EXPECT_FLOAT_EQ(half_1x.y, 40);

    DevicePoint half_2x = ItemCoords::HalfSize(100, 80, 2.0f);
    EXPECT_FLOAT_EQ(half_2x.x, 100);
    EXPECT_FLOAT_EQ(half_2x.y, 80);
}

TEST(HeadlessRendering, ItemCoords_Size) {
    DevicePoint size_1x = ItemCoords::Size(100, 80, 1.0f);
    EXPECT_FLOAT_EQ(size_1x.x, 100);
    EXPECT_FLOAT_EQ(size_1x.y, 80);

    DevicePoint size_2x = ItemCoords::Size(100, 80, 2.0f);
    EXPECT_FLOAT_EQ(size_2x.x, 200);
    EXPECT_FLOAT_EQ(size_2x.y, 160);
}

// ============================================================
// ScreenBounds
// ============================================================

TEST(HeadlessRendering, ScreenBounds_Clamp) {
    ScreenBounds bounds = ScreenBounds::FromDimensions(1920, 1080);
    float snap = 10.0f;

    DevicePoint inside = bounds.Clamp({960, 540}, snap);
    EXPECT_FLOAT_EQ(inside.x, 960);
    EXPECT_FLOAT_EQ(inside.y, 540);

    // Points outside bounds get clamped
    DevicePoint left = bounds.Clamp({-5, 540}, snap);
    EXPECT_FLOAT_EQ(left.x, 10); // minX + snap

    DevicePoint right = bounds.Clamp({1925, 540}, snap);
    EXPECT_FLOAT_EQ(right.x, 1910); // maxX - snap

    DevicePoint top = bounds.Clamp({960, -5}, snap);
    EXPECT_FLOAT_EQ(top.y, 10); // minY + snap

    DevicePoint bottom = bounds.Clamp({960, 1085}, snap);
    EXPECT_FLOAT_EQ(bottom.y, 1070); // maxY - snap
}

TEST(HeadlessRendering, ScreenBounds_Contains) {
    ScreenBounds bounds = ScreenBounds::FromDimensions(1920, 1080);
    float margin = 50.0f;

    EXPECT_TRUE(bounds.Contains({960, 540}, margin));
    EXPECT_FALSE(bounds.Contains({30, 540}, margin));   // Too close to left
    EXPECT_FALSE(bounds.Contains({1890, 540}, margin)); // Too close to right
    EXPECT_FALSE(bounds.Contains({960, 30}, margin));   // Too close to top
    EXPECT_FALSE(bounds.Contains({960, 1050}, margin)); // Too close to bottom
}

// ============================================================
// ItemDragController
// ============================================================

static ItemData* CreateTestItem(int w, int h, ItemData::Type type) {
    ItemData* data = new ItemData();
    data->w = w;
    data->h = h;
    data->type = type;
    return data;
}

TEST(HeadlessRendering, DragController_HitAndDrag) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left corner (center will be at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    g_droppedItems.push_back(item);

    ItemDragController controller;

    // Hit center (500, 400)
    DevicePoint mouseDown{500, 400};
    EXPECT_TRUE(controller.OnMouseDown(mouseDown));
    EXPECT_NE(controller.GetDraggedItem(), nullptr);
    EXPECT_TRUE(controller.GetDraggedItem()->pinned);

    // Drag to new position
    DevicePoint mouseDrag{600, 500};
    controller.OnMouseDragged(mouseDrag);
    EXPECT_FLOAT_EQ(controller.GetDraggedItem()->pos.x, 550);  // 600 + (450-500)
    EXPECT_FLOAT_EQ(controller.GetDraggedItem()->pos.y, 460);  // 500 + (360-400)

    // Release
    controller.OnMouseUp();
    EXPECT_EQ(controller.GetDraggedItem(), nullptr);

    delete data;
    g_droppedItems.clear();
}

TEST(HeadlessRendering, DragController_Miss) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    g_droppedItems.push_back(item);

    ItemDragController controller;

    // Miss the item
    DevicePoint mouseDown{700, 700};
    EXPECT_FALSE(controller.OnMouseDown(mouseDown));
    EXPECT_EQ(controller.GetDraggedItem(), nullptr);

    delete data;
    g_droppedItems.clear();
}

TEST(HeadlessRendering, DragController_CloseButtonDeletes) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    g_droppedItems.push_back(item);

    ItemDragController controller;

    // Click on close button (top-left corner of item, near 450, 360)
    DevicePoint mouseDown{455, 365};
    EXPECT_TRUE(controller.OnMouseDown(mouseDown));
    EXPECT_EQ(controller.GetDraggedItem(), nullptr); // Item deleted, no drag
    EXPECT_EQ(g_droppedItems.size(), 0);

    g_droppedItems.clear();
}

TEST(HeadlessRendering, DragController_ToyNoCloseButton) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::TOY);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    g_droppedItems.push_back(item);

    ItemDragController controller;

    // Click on where close button would be — should start drag, not delete
    DevicePoint mouseDown{455, 365};
    EXPECT_TRUE(controller.OnMouseDown(mouseDown));
    EXPECT_NE(controller.GetDraggedItem(), nullptr); // Drag started, not deleted
    EXPECT_EQ(g_droppedItems.size(), 1);

    controller.OnMouseUp();
    delete data;
    g_droppedItems.clear();
}

TEST(HeadlessRendering, DragController_DragOffsetPreserved) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    g_droppedItems.push_back(item);

    ItemDragController controller;

    // Click off-center (bottom-right area, away from close button)
    DevicePoint mouseDown{540, 430};
    bool hit = controller.OnMouseDown(mouseDown);
    EXPECT_TRUE(hit);
    ASSERT_NE(controller.GetDraggedItem(), nullptr);

    // Drag — offset should be preserved
    DevicePoint mouseDrag{640, 530};
    controller.OnMouseDragged(mouseDrag);

    // Item moved by the same delta as the cursor
    EXPECT_FLOAT_EQ(controller.GetDraggedItem()->pos.x, 550);  // 640 + (450-540)
    EXPECT_FLOAT_EQ(controller.GetDraggedItem()->pos.y, 460);  // 530 + (360-430)

    controller.OnMouseUp();
    delete data;
    g_droppedItems.clear();
}

TEST(HeadlessRendering, DragController_MultipleItems_TopmostWins) {
    g_droppedItems.clear();
    g_config.general.globalScale = 1.0f;

    // Bottom item (larger)
    ItemData* data1 = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item1;
    item1.data = data1;
    item1.pos = {450, 360};  // top-left (center at 500, 400)
    item1.rotation = 0;
    item1.pinned = false;
    g_droppedItems.push_back(item1);

    // Top item (smaller, added later, renders on top)
    ItemData* data2 = CreateTestItem(60, 60, ItemData::TEXT);
    DroppedItem item2;
    item2.data = data2;
    item2.pos = {470, 370};  // top-left (center at 500, 400)
    item2.rotation = 0;
    item2.pinned = false;
    g_droppedItems.push_back(item2);

    ItemDragController controller;

    // Click center (500, 400) — should hit top item (smaller one)
    DevicePoint mouseDown{500, 400};
    controller.OnMouseDown(mouseDown);

    // Verify we got the top item by checking its dimensions
    DroppedItem* dragged = controller.GetDraggedItem();
    ASSERT_NE(dragged, nullptr);
    EXPECT_EQ(dragged->data->w, 60);  // Top item width
    EXPECT_EQ(dragged->data->h, 60);  // Top item height
    EXPECT_EQ(dragged->data->type, ItemData::TEXT);

    controller.OnMouseUp();
    delete data1;
    delete data2;
    g_droppedItems.clear();
}

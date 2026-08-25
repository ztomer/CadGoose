#include <gtest/gtest.h>
#include <cmath>
#include "../../include/coordinate_system.h"
#include "../../include/item_drag_controller.h"
#include "../../include/config.h"
#include "../../include/items.h"
#include "../../include/world.h"
#include "../../include/world_utils.h"
#include "../../include/actor.h"
#include "../../include/actor_dropped_item.h"


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
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left corner (center will be at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    new DroppedItemActor(item);

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

    ActorManager::Instance().removeAllDroppedItems();
}

TEST(HeadlessRendering, DragController_Miss) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    new DroppedItemActor(item);

    ItemDragController controller;

    // Miss the item
    DevicePoint mouseDown{700, 700};
    EXPECT_FALSE(controller.OnMouseDown(mouseDown));
    EXPECT_EQ(controller.GetDraggedItem(), nullptr);

    ActorManager::Instance().removeAllDroppedItems();
}

TEST(HeadlessRendering, DragController_CloseButtonDeletes) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    new DroppedItemActor(item);

    ItemDragController controller;

    // Click on close button (top-left corner of item, near 450, 360)
    DevicePoint mouseDown{455, 365};
    EXPECT_TRUE(controller.OnMouseDown(mouseDown));
    EXPECT_EQ(controller.GetDraggedItem(), nullptr); // Item deleted, no drag
    
    ActorManager::Instance().cleanup();
    EXPECT_EQ(ActorManager::Instance().getDroppedItems().size(), 0u);

    ActorManager::Instance().removeAllDroppedItems();
}

TEST(HeadlessRendering, DragController_ToyNoCloseButton) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::TOY);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    new DroppedItemActor(item);

    ItemDragController controller;

    // Click on where close button would be — should start drag, not delete
    DevicePoint mouseDown{455, 365};
    EXPECT_TRUE(controller.OnMouseDown(mouseDown));
    EXPECT_NE(controller.GetDraggedItem(), nullptr); // Drag started, not deleted
    EXPECT_EQ(ActorManager::Instance().getDroppedItems().size(), 1u);

    controller.OnMouseUp();
    ActorManager::Instance().removeAllDroppedItems();
}

TEST(HeadlessRendering, DragController_DragOffsetPreserved) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};  // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    new DroppedItemActor(item);

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
    ActorManager::Instance().removeAllDroppedItems();
}

TEST(HeadlessRendering, DragController_MultipleItems_TopmostWins) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;

    // Bottom item (larger)
    ItemData* data1 = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item1;
    item1.data = data1;
    item1.pos = {450, 360};  // top-left (center at 500, 400)
    item1.rotation = 0;
    item1.pinned = false;
    new DroppedItemActor(item1);

    // Top item (smaller, added later, renders on top)
    ItemData* data2 = CreateTestItem(60, 60, ItemData::TEXT);
    DroppedItem item2;
    item2.data = data2;
    item2.pos = {470, 370};  // top-left (center at 500, 400)
    item2.rotation = 0;
    item2.pinned = false;
    new DroppedItemActor(item2);

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
    // DroppedItemActor OWNS its ItemData and deletes it in its destructor, so
    // removeAllDroppedItems() has already freed data1 and data2. Deleting them
    // again here was a double free: it corrupted the heap, and the fallout
    // surfaced far away — usually as a crash at process exit that looked like a
    // GoogleTest teardown bug. The sibling tests below delete an ItemData only
    // while ALSO nulling actor->item().data, which is what makes that safe.
    ActorManager::Instance().removeAllDroppedItems();
}

// ============================================================
// DroppedItemActor Lifecycle and World_CleanupExpired Safety
// ============================================================

TEST(HeadlessRendering, DroppedItemActor_LifecycleAndCleanupSafety) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;

    // 1. Construct a DroppedItemActor with valid data
    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360};
    item.rotation = 0;
    item.pinned = false;
    item.timeDropped = g_time;

    DroppedItemActor* actor = new DroppedItemActor(item);
    ASSERT_EQ(ActorManager::Instance().getDroppedItems().size(), 1u);
    EXPECT_TRUE(actor->isAlive());

    // 2. Simulate closing the item by deleting its data and setting it to nullptr
    // (This mimics close button action in ItemContentView mouseDown)
    delete actor->item().data;
    actor->item().data = nullptr;
    actor->item().pinned = true;

    // 3. Verify actor is no longer alive
    EXPECT_FALSE(actor->isAlive());

    // 4. Verify World_CleanupExpired handles null-data actor gracefully without crashing
    // and correctly reaps it during ActorManager::cleanup()
    World_CleanupExpired(g_time);

    // Verify actor was reaped and deleted
    EXPECT_EQ(ActorManager::Instance().getDroppedItems().size(), 0u);
}

TEST(HeadlessRendering, ItemHitTest_GracefulNullDataHandling) {
    ActorManager::Instance().removeAllDroppedItems();
    g_config.general.globalScale = 1.0f;

    ItemData* data = CreateTestItem(100, 80, ItemData::MEME);
    DroppedItem item;
    item.data = data;
    item.pos = {450, 360}; // top-left (center at 500, 400)
    item.rotation = 0;
    item.pinned = false;
    
    DroppedItemActor* actor = new DroppedItemActor(item);

    // Simulate item close
    delete actor->item().data;
    actor->item().data = nullptr;

    // Attempt hit test on defunct item - should handle gracefully and return false
    DroppedItem* hitItem = nullptr;
    EXPECT_FALSE(ItemHitTest(NSMakePoint(500, 400), 1080, &hitItem, 16.0f));
    EXPECT_EQ(hitItem, nullptr);

    // Clean up
    ActorManager::Instance().removeAllDroppedItems();
}

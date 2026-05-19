#include "../test_framework.h"
#include "../../include/world.h"
#include "../../include/actor.h"

TEST(World_GlobalState) {
    // Verify global state exists and is initially empty
    ASSERT_EQ(ActorManager::Instance().getGeese().size(), 0u);
    ASSERT_EQ(g_world.droppedItems.size(), 0u);
    ASSERT_EQ(g_world.footprints.size(), 0u);
    ASSERT_EQ(g_world.nextId, 0);
}

TEST(World_GetGooseById_NotFound) {
    Goose* g = GetGooseById(999);
    ASSERT_TRUE(g == nullptr);
}

TEST(World_UiLog) {
    UiLogPush("Test message");
    ASSERT_EQ(g_world.uiLog.size(), 1u);
    ASSERT_EQ(g_world.uiLog.front(), "Test message");
    
    UiLogPush("Message 2");
    ASSERT_EQ(g_world.uiLog.size(), 2u);
}

TEST(World_UiLog_MaxSize) {
    for (int i = 0; i < 15; i++) {
        UiLogPush("Message " + std::to_string(i));
    }
    // Should not exceed UI_LOG_MAX (12)
    ASSERT_TRUE(g_world.uiLog.size() <= 12);
}

TEST(World_ScreenDimensions) {
    ASSERT_EQ(g_world.screenWidth, 1920);
    ASSERT_EQ(g_world.screenHeight, 1080);
}
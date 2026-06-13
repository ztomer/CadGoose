#include <gtest/gtest.h>
#include "world.h"

TEST(WorldTest, GetGooseByIdNotFound) {
    Goose* g = GetGooseById(999);
    ASSERT_EQ(g, nullptr);
}

TEST(WorldTest, UiLogPushAndRead) {
    g_world.uiLog.clear();
    UiLogPush("Test message");
    ASSERT_EQ(g_world.uiLog.size(), 1u);
    ASSERT_EQ(g_world.uiLog.front(), "Test message");

    UiLogPush("Message 2");
    ASSERT_EQ(g_world.uiLog.size(), 2u);
    ASSERT_EQ(g_world.uiLog.back(), "Message 2");
}

TEST(WorldTest, UiLogMaxSize) {
    g_world.uiLog.clear();
    for (int i = 0; i < 15; i++) {
        UiLogPush("Message " + std::to_string(i));
    }
    ASSERT_LE(g_world.uiLog.size(), 12u);
    ASSERT_EQ(g_world.uiLog.front(), "Message 3");
    ASSERT_EQ(g_world.uiLog.back(), "Message 14");
}

TEST(WorldTest, UiLogEmpty) {
    g_world.uiLog.clear();
    ASSERT_EQ(g_world.uiLog.size(), 0u);
}

TEST(WorldTest, UiLogFifoOrder) {
    g_world.uiLog.clear();
    UiLogPush("first");
    UiLogPush("second");
    UiLogPush("third");
    ASSERT_EQ(g_world.uiLog.size(), 3u);
    ASSERT_EQ(g_world.uiLog[0], "first");
    ASSERT_EQ(g_world.uiLog[1], "second");
    ASSERT_EQ(g_world.uiLog[2], "third");
}



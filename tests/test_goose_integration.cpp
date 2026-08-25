#include <gtest/gtest.h>
#include "goose.h"
#include "world.h"
#include "config.h"
#include "cursor_io.h"

TEST(Integration, Goose_WanderToChase) {
    Goose g(1, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.cursorChaseChance = 100;
    g.attackMouseBias = 100;

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    g_world.cursorGrabberId = -1;

    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::CHASE_CURSOR);
    EXPECT_EQ(g.target.x, 500.0f);
    EXPECT_EQ(g.target.y, 500.0f);
}

TEST(Integration, Goose_SnatchCursor) {
    Goose g(2, "Test", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.chaseStartTime = 0.0;
    g.pos = {500, 500};
    g.target = {500, 500};

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {0, 0};

    g_world.cursorGrabberId = -1;
    g.Update(0.1, 0.0, 1920, 1080, c);

    c.position = g.GetBeakTipDevice();

    g.Update(0.1, 0.1, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_world.cursorGrabberId, 2);
}

TEST(Integration, Goose_SnatchRelease) {
    Goose g(3, "Test", 1920, 1080);
    g.state = GooseState::SNATCH_CURSOR;
    g.snatchStartTime = 0.0;
    g.snatchDuration = 3.0f;
    g_world.cursorGrabberId = 3;

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {500, 500};

    g.Update(0.1, 1.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_world.cursorGrabberId, 3);

    g.Update(0.1, 3.5, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g_world.cursorGrabberId, -1);
}

TEST(Integration, Goose_FetchItem) {
    Goose g(4, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100};

    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.memeFetchBias = 100;
    g.noteFetchBias = 100;

    CursorState c;

    g.Update(0.1, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::FETCHING);
}

TEST(Integration, Goose_ReturningItem) {
    Goose g(5, "Test", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.forceItemFetch = FetchType::Meme;

    CursorState c;

    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::RETURNING);
    EXPECT_NE(g.heldItem, nullptr);
}

TEST(Integration, Goose_DropItem) {
    Goose g(6, "Test", 1920, 1080);
    g.state = GooseState::RETURNING;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.heldItem = g_assets.GetRandomMeme(1920, 1080, 0.1f);

    int initialDrops = ActorManager::Instance().getDroppedItems().size();

    CursorState c;
    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g.heldItem, nullptr);
    EXPECT_EQ(ActorManager::Instance().getDroppedItems().size(), initialDrops + 1);
}

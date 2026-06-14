#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "cursor_io.h"
#include "behaviors/states/drag_state.h"
#include "platform_input_mock.h"

struct MockCursorProvider : public ICursorProvider {
    CursorState cursor;
    CursorState Read() override { return cursor; }
    void Execute(const CursorAction&) override {}
    void set(float x, float y, bool hasPos = true) {
        cursor = {.position = {x, y}, .caps = hasPos ? CAP_GET_POS : CAP_NONE};
    }
};

class BehaviorDragTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("drag")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedProvider = g_cursorProvider;
        g_cursorProvider = &mockCursor;
        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        g_cursorProvider = savedProvider;
        delete goose;
        PlatformInputMock_Reset();
    }

    Goose* goose;
    BehaviorContext ctx{};
    MockCursorProvider mockCursor;
    ICursorProvider* savedProvider = nullptr;
};

TEST_F(BehaviorDragTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
}

TEST_F(BehaviorDragTest, RenderNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorDragTest, TickWithNoCursor) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);
    g_cursorProvider = nullptr;

    goose->pos.x = 100;
    goose->pos.y = 100;

    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_EQ(goose->pos.x, 100);
    EXPECT_EQ(goose->pos.y, 100);
}

TEST_F(BehaviorDragTest, TickCursorFarNoDrag) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);

    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(500, 500);

    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_EQ(goose->pos.x, 100);
}

TEST_F(BehaviorDragTest, TickCursorOnGooseNoMouseDown) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);

    goose->pos.x = 100;
    goose->pos.y = 100;
    goose->state = GooseState::WANDER;
    mockCursor.set(100, 100);

    float prevX = goose->pos.x;
    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_EQ(goose->pos.x, prevX);
}

TEST_F(BehaviorDragTest, TickCursorNotOnGoose) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);

    goose->pos.x = 0;
    goose->pos.y = 0;
    mockCursor.set(5000, 5000);

    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_EQ(goose->pos.x, 0);
}

TEST_F(BehaviorDragTest, TickMouseDownDragsGoose) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);

    goose->pos.x = 100;
    goose->pos.y = 100;
    goose->vel.x = 10;
    goose->vel.y = 10;
    goose->state = GooseState::WANDER;
    mockCursor.set(102, 100);

    PlatformInputMock_SetMouseDown(true);

    b->tick(goose, ctx, 0.016, 0.0);

    EXPECT_FLOAT_EQ(goose->pos.x, 102 - 5.0f);
    EXPECT_FLOAT_EQ(goose->pos.y, 100);
    EXPECT_FLOAT_EQ(goose->vel.x, 0);
    EXPECT_FLOAT_EQ(goose->vel.y, 0);
}

TEST_F(BehaviorDragTest, TickMouseDownNoDragDuringSnatch) {
    auto* b = BehaviorRegistry::Instance().Get("drag");
    ASSERT_NE(b, nullptr);

    goose->pos.x = 100;
    goose->pos.y = 100;
    goose->state = GooseState::SNATCH_CURSOR;
    mockCursor.set(102, 100);

    PlatformInputMock_SetMouseDown(true);

    b->tick(goose, ctx, 0.016, 0.0);

    EXPECT_EQ(goose->pos.x, 100);
    EXPECT_EQ(goose->pos.y, 100);
}

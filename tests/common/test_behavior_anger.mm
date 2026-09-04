#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "random_util.h"
#include "event_bus.h"
#include "cursor_io.h"
#include "renderer_interface.h"
#include "behaviors/states/anger_state.h"
#include "goose_drawing.h"

struct MockCursorProvider : public ICursorProvider {
    CursorState cursor;

    CursorState Read() override { return cursor; }
    void Execute(const CursorAction&) override {}

    void set(float x, float y, bool hasPos = true) {
        cursor = {.position = {x, y}, .caps = hasPos ? CAP_GET_POS : CAP_NONE};
    }
};

struct MockAngerRenderer : public IRenderer {
    int ellipseCount = 0;

    void DrawEllipse(RenderPoint, float, float, RenderColor) override { ++ellipseCount; }
    void SaveState() override {}
    void RestoreState() override {}
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawImage(void*, RenderRect) override {}
    bool GetImageSize(void*, float*, float*) override { return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
};

class BehaviorAngerTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("anger")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedProvider = g_cursorProvider;
        g_cursorProvider = &mockCursor;

        goose = new Goose(1, "angry", 1920, 1080);
        goose->behaviorsEnabled = true;
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        g_cursorProvider = savedProvider;
        delete goose;
    }

    Goose* goose;
    BehaviorContext ctx{};
    MockCursorProvider mockCursor;
    ICursorProvider* savedProvider = nullptr;
};

TEST_F(BehaviorAngerTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 0.0f);
}

TEST_F(BehaviorAngerTest, InitResetsState) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    auto* st = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    st->angerLevel = 50.0f;
    st->isPunching = true;

    b->init(ctx);
    EXPECT_EQ(st->angerLevel, 0.0f);
    EXPECT_FALSE(st->isPunching);
}

TEST_F(BehaviorAngerTest, TickCursorNearIncreasesAnger) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(105, 103); // close

    ctx.time = 0;
    b->tick(goose, ctx, 1.0, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_GT(state->angerLevel, 0.0f);
}

TEST_F(BehaviorAngerTest, TickCursorFarDecreasesAnger) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->angerLevel = 50.0f;

    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(5000, 5000); // far

    ctx.time = 0;
    b->tick(goose, ctx, 1.0, ctx.time);

    ASSERT_NE(state, nullptr);
    EXPECT_LT(state->angerLevel, 50.0f);
}

TEST_F(BehaviorAngerTest, AngerAt80StartsPunch) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->angerLevel = 80.0f;

    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(105, 103);

    ctx.time = 5.0;
    b->tick(goose, ctx, 1.0, ctx.time);

    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isPunching);
    EXPECT_EQ(state->lastPunchTime, 5.0);
}

TEST_F(BehaviorAngerTest, PunchCooldown) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->angerLevel = 80.0f;
    state->lastPunchTime = 100.0; // recently punched

    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(105, 103);

    ctx.time = 100.5; // only 0.5s since last punch, cooldown is 2.0
    b->tick(goose, ctx, 1.0, ctx.time);

    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isPunching);
}

TEST_F(BehaviorAngerTest, PunchEndsAfterDuration) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->isPunching = true;
    state->lastPunchTime = 0;

    ctx.time = 1.0; // > punchDuration (0.3)
    b->tick(goose, ctx, 1.0, ctx.time);

    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isPunching);
}

TEST_F(BehaviorAngerTest, PunchIncreasesSpeed) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->isPunching = true;
    state->lastPunchTime = 0;

    ctx.time = 0.1;
    b->tick(goose, ctx, 1.0, ctx.time);

    EXPECT_FLOAT_EQ(goose->currentSpeed, g_config.movement.baseRunSpeed * 1.3f);
}

TEST_F(BehaviorAngerTest, GooseHonkedEventIncreasesAnger) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    b->init(ctx);
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 15.0f);
}

TEST_F(BehaviorAngerTest, GooseHonkedEventWrongIdIgnored) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    b->init(ctx);
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(GooseHonkedEvent{999, 0, 0, 0});

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 0.0f);
}

TEST_F(BehaviorAngerTest, GooseHonkedCapsAt100) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    b->init(ctx);
    goose->pos.x = 100;
    goose->pos.y = 100;
    mockCursor.set(105, 103);
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(1, "anger");
    state->angerLevel = 90.0f;
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});

    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->angerLevel, 100.0f);
}

TEST_F(BehaviorAngerTest, CursorFastMoveEventNearIncreasesAnger) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    b->init(ctx);
    goose->pos.x = 100;
    goose->pos.y = 100;
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(CursorFastMoveEvent{0, 0, 105, 103});

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 5.0f);
}

TEST_F(BehaviorAngerTest, CursorFastMoveEventFarIgnored) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);

    b->init(ctx);
    goose->pos.x = 100;
    goose->pos.y = 100;
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(CursorFastMoveEvent{0, 0, 5000, 5000});

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 0.0f);
}

TEST_F(BehaviorAngerTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorAngerTest, RenderLowAngerEarlyReturn) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    MockAngerRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 0);
}

TEST_F(BehaviorAngerTest, RenderAura) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    state->angerLevel = 50.0f;

    MockAngerRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 1);
}

TEST_F(BehaviorAngerTest, RenderPunching) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    state->angerLevel = 80.0f;
    state->isPunching = true;
    ctx.time = 1.0;

    MockAngerRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 2);
}

TEST_F(BehaviorAngerTest, AngerGetLevelExported) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    state->angerLevel = 42.0f;

    EXPECT_EQ(Anger_GetLevel(1), 42.0f);
    EXPECT_EQ(Anger_GetLevel(999), 0.0f);
}

TEST_F(BehaviorAngerTest, Cleanup) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
    b->tick(goose, ctx, 1.0, 0.0);

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->angerLevel, 15.0f);

    ASSERT_NE(b->cleanup, nullptr);
    b->cleanup(ctx);

    EventBus::Instance().Publish(GooseHonkedEvent{1, 0, 0, 0});
    EXPECT_EQ(state->angerLevel, 15.0f);
}

TEST_F(BehaviorAngerTest, CursorNoPositionFallback) {
    auto* b = BehaviorRegistry::Instance().Get("anger");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    mockCursor.set(0, 0, false);
    goose->pos.x = 100;
    goose->pos.y = 100;

    auto* state = BehaviorStateManager::Instance().Get<AngerState>(1, "anger");
    ASSERT_NE(state, nullptr);
    float prev = state->angerLevel;

    b->tick(goose, ctx, 1.0, 0.0);

    EXPECT_EQ(state->angerLevel, prev);
}

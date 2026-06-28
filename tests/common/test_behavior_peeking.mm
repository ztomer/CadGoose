#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "random_util.h"
#include "renderer_interface.h"
#include "behaviors/states/peeking_state.h"

struct MockPeekingRenderer : public IRenderer {
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

class BehaviorPeekingTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("peeking")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(1, "spy", 1920, 1080);
        goose->behaviorsEnabled = true;
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorPeekingTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isPeeking);
    EXPECT_EQ(state->peekSide, 0);
}

TEST_F(BehaviorPeekingTest, InitResetsExistingState) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);

    auto* st = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(1, "peeking");
    st->isPeeking = true;
    st->peekSide = 1;

    b->init(ctx);
    EXPECT_FALSE(st->isPeeking);
    EXPECT_EQ(st->peekSide, 0);
}

TEST_F(BehaviorPeekingTest, NotAtEdgeSetsNextPeek) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 1000;
    g_world.screenWidth = 1920;

    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isPeeking);
    EXPECT_EQ(state->peekSide, 0);
    EXPECT_EQ(state->nextPeekTime, 15.0);
}

TEST_F(BehaviorPeekingTest, AtLeftEdgeSetsPeekSide) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 10;
    g_world.screenWidth = 1920;

    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->peekSide, -1);
}

TEST_F(BehaviorPeekingTest, AtRightEdgeSetsPeekSide) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 1900;
    g_world.screenWidth = 1920;

    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->peekSide, 1);
}

TEST_F(BehaviorPeekingTest, PeekingStopsAfterDuration) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    state->isPeeking = true;
    state->peekStartTime = 0;

    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(state->isPeeking);

    ctx.time = 2.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_FALSE(state->isPeeking);
    EXPECT_GE(state->nextPeekTime, 2.0);
}

TEST_F(BehaviorPeekingTest, PeekingAtLeftEdgeRngHit) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 10;
    g_world.screenWidth = 1920;

    // Seed 230 produces: RandRange(10)=X (nextPeekTime), RandRange(120)=0 (hit)
    // with the optimized fast-path RandRange. Original seed 48 was specific
    // to the old std::uniform_int_distribution path.
    rng_util::Seed(230);
    ctx.time = 0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isPeeking);
    EXPECT_EQ(state->peekStartTime, 0);
}

TEST_F(BehaviorPeekingTest, AtEdgeRngMiss) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos.x = 10;
    g_world.screenWidth = 1920;

    rng_util::Seed(1);
    ctx.time = 0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isPeeking);
}

TEST_F(BehaviorPeekingTest, NotYetTimeToPeek) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    state->nextPeekTime = 100;

    ctx.time = 50;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_FALSE(state->isPeeking);
    EXPECT_EQ(state->nextPeekTime, 100);
}

TEST_F(BehaviorPeekingTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorPeekingTest, RenderNullWhilePeeking) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    state->isPeeking = true;

    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorPeekingTest, RenderNotPeeking) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    MockPeekingRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 0);
}

TEST_F(BehaviorPeekingTest, RenderPeeking) {
    auto* b = BehaviorRegistry::Instance().Get("peeking");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<PeekingState>(1, "peeking");
    ASSERT_NE(state, nullptr);
    state->isPeeking = true;
    state->peekSide = -1;
    goose->rig.neckHead = {100, 100};

    MockPeekingRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.ellipseCount, 2);
}

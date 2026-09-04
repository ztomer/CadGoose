#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "random_util.h"
#include "behaviors/states/boredom_state.h"

struct RenderCall {
    enum Type { SaveState, RestoreState, DrawEllipse, DrawLine };
    Type type;
    int count;
};

struct MockBoredomRenderer : public IRenderer {
    int saveCount = 0;
    int restoreCount = 0;
    int ellipseCount = 0;
    int lineCount = 0;

    void SaveState() override { ++saveCount; }
    void RestoreState() override { ++restoreCount; }
    void DrawEllipse(RenderPoint, float, float, RenderColor) override { ++ellipseCount; }
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override { ++lineCount; }
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
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

struct HonkSpyGoose : public Goose {
    int honkCount = 0;
    HonkSpyGoose(int id) : Goose(id, "spy", 1920, 1080) { behaviorsEnabled = true; }
    void onHonk() override { ++honkCount; }
};

class BehaviorBoredomTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("boredom")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new HonkSpyGoose(1);
        ctx.goose = goose;
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        delete goose;
    }

    HonkSpyGoose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorBoredomTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isSighing);
    EXPECT_FALSE(state->isLyingDown);
}

TEST_F(BehaviorBoredomTest, InitResetsExistingState) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);

    auto* st = BehaviorStateManager::Instance().GetOrCreate<BoredomState>(1, "boredom");
    st->isSighing = true;
    st->isLyingDown = true;

    b->init(ctx);
    EXPECT_FALSE(st->isSighing);
    EXPECT_FALSE(st->isLyingDown);
    EXPECT_EQ(st->idleStartTime, 0);
}

TEST_F(BehaviorBoredomTest, MovingCancelsSighNotLieDown) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isSighing = true;

    goose->currentSpeed = 100.0f;
    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_FALSE(state->isSighing);
    EXPECT_EQ(state->idleStartTime, 0);
}

TEST_F(BehaviorBoredomTest, LyingDownGetUpAfter8Seconds) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isLyingDown = true;
    state->lieDownStartTime = 0;
    goose->currentSpeed = 5.0f;

    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(state->isLyingDown);

    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_FALSE(state->isLyingDown);
    EXPECT_FALSE(state->isSighing);
}

TEST_F(BehaviorBoredomTest, SighToLieDown) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isSighing = true;
    state->sighStartTime = 0;
    goose->currentSpeed = 5.0f;

    ctx.time = 1.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(state->isSighing);
    EXPECT_FALSE(state->isLyingDown);

    ctx.time = 3.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(state->isLyingDown);
}

TEST_F(BehaviorBoredomTest, MaxSighDurationResets) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isSighing = true;
    state->sighStartTime = 0;
    goose->currentSpeed = 5.0f;

    ctx.time = 15.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_FALSE(state->isSighing);
    EXPECT_FALSE(state->isLyingDown);
}

TEST_F(BehaviorBoredomTest, IdleStartsTimer) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->idleStartTime, 0);

    goose->currentSpeed = 5.0f;
    ctx.time = 10.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(state->idleStartTime, 10.0);
}

TEST_F(BehaviorBoredomTest, LyingDownDoesNotCheckSpeed) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isLyingDown = true;
    state->lieDownStartTime = 0;

    goose->currentSpeed = 100.0f;
    b->tick(goose, ctx, 0.016, 0.0);
    EXPECT_TRUE(state->isLyingDown);
}

TEST_F(BehaviorBoredomTest, IdleLongEnoughTriggersSighWithRng) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->currentSpeed = 5.0f;

    ctx.time = 0.001;
    b->tick(goose, ctx, 0.016, ctx.time);  // sets idleStartTime = 0.001

    rng_util::Seed(474);

    ctx.time = 601.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isSighing);
    EXPECT_EQ(state->sighStartTime, 601.0);
    EXPECT_EQ(goose->honkCount, 1);
}

TEST_F(BehaviorBoredomTest, RenderNull) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorBoredomTest, RenderLyingDown) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isLyingDown = true;
    goose->rig.body = {100, 200};
    goose->rig.neckHead = {105, 195};

    MockBoredomRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.ellipseCount, 2);
    EXPECT_EQ(renderer.lineCount, 1);
}

TEST_F(BehaviorBoredomTest, RenderSighing) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isSighing = true;
    state->sighStartTime = 0;
    goose->rig.neckHead = {100, 100};
    goose->lastUpdateTime = 1.0;

    MockBoredomRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.ellipseCount, 1);
}

TEST_F(BehaviorBoredomTest, RenderSighingPastPuff) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    state->isSighing = true;
    state->sighStartTime = 0;
    goose->rig.neckHead = {100, 100};
    goose->lastUpdateTime = 5.0;

    MockBoredomRenderer renderer;
    b->render(goose, ctx, &renderer);
    EXPECT_EQ(renderer.saveCount, 1);
    EXPECT_EQ(renderer.restoreCount, 1);
    EXPECT_EQ(renderer.ellipseCount, 0);
}

TEST_F(BehaviorBoredomTest, TickSetsLastUpdateTimeOnMovingGoose) {
    auto* b = BehaviorRegistry::Instance().Get("boredom");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->currentSpeed = 100.0f;
    ctx.time = 5.0;
    b->tick(goose, ctx, 0.016, ctx.time);

    auto* state = BehaviorStateManager::Instance().Get<BoredomState>(1, "boredom");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isSighing);
    EXPECT_EQ(state->idleStartTime, 0);
}

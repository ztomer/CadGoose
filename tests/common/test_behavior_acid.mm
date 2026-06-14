#include <gtest/gtest.h>
#include <cmath>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "random_util.h"
#include "behaviors/states/acid_state.h"

struct HonkSpyGoose : public Goose {
    int honkCount = 0;
    HonkSpyGoose(int id, int w = 1920, int h = 1080) : Goose(id, "spy", w, h) {
        behaviorsEnabled = true;
    }
    void onHonk() override { ++honkCount; }
};

class BehaviorAcidTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("acid")) {
            BehaviorRegistry::Instance().Restore();
        }
        savedSpinSpeed = g_config.behaviors.acid.spinSpeed;
        savedHonkInterval = g_config.behaviors.acid.honkInterval;
        savedRotationTotal = g_config.behaviors.acid.rotationTotal;
        savedTriggerChance = g_config.behaviors.acid.triggerChance;
        g_config.behaviors.acid.spinSpeed = 360.0f;
        g_config.behaviors.acid.honkInterval = 0.1;
        g_config.behaviors.acid.rotationTotal = 720.0f;
        g_config.behaviors.acid.triggerChance = 0; // always spin
    }

    void TearDown() override {
        g_config.behaviors.acid.spinSpeed = savedSpinSpeed;
        g_config.behaviors.acid.honkInterval = savedHonkInterval;
        g_config.behaviors.acid.rotationTotal = savedRotationTotal;
        g_config.behaviors.acid.triggerChance = savedTriggerChance;
    }

    float savedSpinSpeed, savedHonkInterval, savedRotationTotal;
    int savedTriggerChance;
};

TEST_F(BehaviorAcidTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(1);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<AcidState>(1, "acid");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isSpinning);
    EXPECT_EQ(state->rotationAccumulator, 0.0f);
}

TEST_F(BehaviorAcidTest, InitResetsExistingState) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(2);
    BehaviorContext ctx{&g, 0};

    auto* st = BehaviorStateManager::Instance().GetOrCreate<AcidState>(2, "acid");
    st->isSpinning = true;
    st->rotationAccumulator = 500.0f;

    b->init(ctx);
    EXPECT_FALSE(st->isSpinning);
    EXPECT_EQ(st->rotationAccumulator, 0.0f);
}

TEST_F(BehaviorAcidTest, StartsSpinningWhenTriggered) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(3);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    auto* state = BehaviorStateManager::Instance().Get<AcidState>(3, "acid");
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isSpinning);
}

TEST_F(BehaviorAcidTest, RotatesDuringSpin) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(4);
    g.dir = 45.0f;
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    b->tick(&g, ctx, 1.0, 0.0);
    float expectedDir = fmodf(45.0f + 360.0f, 360.0f);
    EXPECT_FLOAT_EQ(g.dir, expectedDir);
}

TEST_F(BehaviorAcidTest, HonksAtInterval) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(5);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    b->tick(&g, ctx, 0.016, 0.2);
    EXPECT_GT(g.honkCount, 0);
}

TEST_F(BehaviorAcidTest, HonkRateLimited) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(6);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    int count = g.honkCount;

    b->tick(&g, ctx, 0.016, 0.05);
    EXPECT_EQ(g.honkCount, count);
}

TEST_F(BehaviorAcidTest, StopsSpinningAfterFullRotation) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(7);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    b->tick(&g, ctx, 2.0, 0.0);
    auto* state = BehaviorStateManager::Instance().Get<AcidState>(7, "acid");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isSpinning);
    EXPECT_EQ(state->rotationAccumulator, 0.0f);
}

TEST_F(BehaviorAcidTest, RenderNoop) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);
    HonkSpyGoose g(0);
    BehaviorContext ctx{&g, 0};
    b->render(&g, ctx, nullptr);
}

TEST_F(BehaviorAcidTest, PerGooseStateIsolation) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g1(8), g2(9);
    BehaviorContext ctx1{&g1, 0}, ctx2{&g2, 0};

    b->init(ctx1);
    b->init(ctx2);

    b->tick(&g1, ctx1, 0.016, 0.0);
    b->tick(&g2, ctx2, 0.016, 0.0);

    auto* s1 = BehaviorStateManager::Instance().Get<AcidState>(8, "acid");
    auto* s2 = BehaviorStateManager::Instance().Get<AcidState>(9, "acid");
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    EXPECT_TRUE(s1->isSpinning);
    EXPECT_TRUE(s2->isSpinning);

    b->tick(&g1, ctx1, 1.0, 0.2);
    EXPECT_GT(g1.honkCount, 0);
    EXPECT_EQ(g2.honkCount, 0);
}

TEST_F(BehaviorAcidTest, DirWrapsAt360) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(10);
    g.dir = 359.0f;
    BehaviorContext ctx{&g, 0};
    g_config.behaviors.acid.spinSpeed = 720.0f;
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    b->tick(&g, ctx, 0.005, 0.0);
    EXPECT_GE(g.dir, 0.0f);
    EXPECT_LT(g.dir, 360.0f);
}

TEST_F(BehaviorAcidTest, NoSpinWhenNotTriggered) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(11);
    BehaviorContext ctx{&g, 0};
    g_config.behaviors.acid.triggerChance = 10000;
    g_config.behaviors.acid.spinSpeed = 1.0f;
    b->init(ctx);

    rng_util::Seed(42);
    b->tick(&g, ctx, 1.0, 0.0);

    auto* state = BehaviorStateManager::Instance().Get<AcidState>(11, "acid");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isSpinning);
}

TEST_F(BehaviorAcidTest, MultipleHonksDuringLongSpin) {
    auto* b = BehaviorRegistry::Instance().Get("acid");
    ASSERT_NE(b, nullptr);

    HonkSpyGoose g(12);
    BehaviorContext ctx{&g, 0};
    b->init(ctx);

    b->tick(&g, ctx, 0.016, 0.0);
    b->tick(&g, ctx, 0.016, 0.2);
    b->tick(&g, ctx, 0.016, 0.4);
    EXPECT_EQ(g.honkCount, 2);
}

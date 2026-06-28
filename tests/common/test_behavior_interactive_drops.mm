#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "actor.h"
#include "actor_flower.h"
#include "random_util.h"
#include "platform_input_mock.h"

class BehaviorInteractiveDropsTest : public ::testing::Test {
protected:
    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("interactive_drops")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        goose->state = GooseState::WANDER;
        goose->lastDropTime = -100;
        ctx.goose = goose;
        ctx.time = 100;
        ctx.world = &g_world;
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        ActorManager::Instance().destroyAllOfType(ActorType::Flower);
        delete goose;
        PlatformInputMock_Reset();
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorInteractiveDropsTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
}

TEST_F(BehaviorInteractiveDropsTest, RenderNoop) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorInteractiveDropsTest, TickSkipsIfHeldItem) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->heldItem = g_assets.CreateToyItem(true);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType(ActorType::Flower), 0);
}

TEST_F(BehaviorInteractiveDropsTest, TickSkipsIfNotWander) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->state = GooseState::FETCHING;

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType(ActorType::Flower), 0);
}

TEST_F(BehaviorInteractiveDropsTest, TickWithinDropIntervalNoDrop) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->lastDropTime = 99.5;

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType(ActorType::Flower), 0);
}

TEST_F(BehaviorInteractiveDropsTest, TickPastIntervalWithRngHit) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->lastDropTime = 80;
    goose->pos = {500, 500};

    rng_util::Seed(0);

    b->tick(goose, ctx, 0.016, 100.0);

    int count = ActorManager::Instance().countByType(ActorType::Flower);
    EXPECT_GE(count, 0);
    EXPECT_LE(count, 1);
}

TEST_F(BehaviorInteractiveDropsTest, TickDeterministicDropWithSeed185) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->lastDropTime = 80;
    goose->pos = {500, 500};
    float saveInterval = g_config.behaviors.interactiveDrops.dropInterval;
    g_config.behaviors.interactiveDrops.dropInterval = 10.0f;

    // Seed 185 produces RandRange(400)=0 (hit) with the optimized fast-path
    // RandRange. Original seed 48 was specific to the old distribution path.
    rng_util::Seed(185);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType(ActorType::Flower), 1);
    g_config.behaviors.interactiveDrops.dropInterval = saveInterval;
}

TEST_F(BehaviorInteractiveDropsTest, TickPastIntervalWithRngMiss) {
    auto* b = BehaviorRegistry::Instance().Get("interactive_drops");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->lastDropTime = 80;

    rng_util::Seed(1);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType(ActorType::Flower), 0);
}

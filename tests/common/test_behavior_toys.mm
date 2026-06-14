#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "event_bus.h"
#include "actor.h"
#include "actor_toy.h"
#include "platform_input_mock.h"

class BehaviorToysTest : public ::testing::Test {
protected:
    void SetUp() override {
        EventBus::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("toys")) {
            BehaviorRegistry::Instance().Restore();
        }
        ActorManager::Instance().destroyAllOfType("toy");
        goose = new Goose(1, "goose", 1920, 1080);
        goose->behaviorsEnabled = true;
        goose->pos = {500, 500};
        goose->heldItem = nullptr;
        goose->state = GooseState::WANDER;
        ctx.goose = goose;
        ctx.time = 100;
        ctx.world = &g_world;
        PlatformInputMock_Reset();
    }

    void TearDown() override {
        ActorManager::Instance().destroyAllOfType("toy");
        delete goose;
        PlatformInputMock_Reset();
    }

    void ensureNoAutoSpawn() {
        ctx.time = 0;
    }

    Goose* goose;
    BehaviorContext ctx{};
};

TEST_F(BehaviorToysTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
}

TEST_F(BehaviorToysTest, RenderNoop) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

TEST_F(BehaviorToysTest, TickSpawnsToyAfterInterval) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    EXPECT_EQ(ActorManager::Instance().countByType("toy"), 0);

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType("toy"), 1);
}

TEST_F(BehaviorToysTest, TickNoToyIfMaxToys) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    for (int i = 0; i < 5; i++) {
        ToyActor* toy = new ToyActor(ToyActor::Stick, {100.0f + i * 50.0f, 200.0f}, i);
        ActorManager::Instance().add(toy);
    }

    b->tick(goose, ctx, 0.016, 100.0);

    EXPECT_EQ(ActorManager::Instance().countByType("toy"), 5);
}

TEST_F(BehaviorToysTest, ToySpawnPublishesEvent) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    int eventCount = 0;
    auto sub = EventBus::Instance().Subscribe<ToySpawnedEvent>([&](const ToySpawnedEvent&) { ++eventCount; });

    ctx.time = 1000;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_EQ(eventCount, 1);
    EventBus::Instance().Unsubscribe(sub);
}

TEST_F(BehaviorToysTest, TickFetchesNearbyToy) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {500, 500};
    ToyActor* toy = new ToyActor(ToyActor::Stick, {505.0f, 500.0f}, 1);
    ActorManager::Instance().add(toy);

    EXPECT_EQ(goose->heldItem, nullptr);
    EXPECT_EQ(goose->state, GooseState::WANDER);

    ctx.time = 0;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_NE(goose->state, GooseState::WANDER);
}

TEST_F(BehaviorToysTest, TickWandersTowardDistantToy) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {400, 400};
    ToyActor* toy = new ToyActor(ToyActor::Ball, {450.0f, 450.0f}, 2);
    ActorManager::Instance().add(toy);

    goose->state = GooseState::WANDER;

    ctx.time = 0;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_EQ(goose->state, GooseState::WANDER);
    EXPECT_FLOAT_EQ(goose->target.x, 450.0f);
    EXPECT_FLOAT_EQ(goose->target.y, 450.0f);
}

TEST_F(BehaviorToysTest, TickIgnoresInactiveToy) {
    auto* b = BehaviorRegistry::Instance().Get("toys");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->pos = {400, 400};
    ToyActor* toy = new ToyActor(ToyActor::Stick, {500.0f, 500.0f}, 3);
    toy->setActive(false);
    ActorManager::Instance().add(toy);

    goose->state = GooseState::WANDER;

    ctx.time = 0;
    b->tick(goose, ctx, 0.016, ctx.time);

    EXPECT_EQ(goose->state, GooseState::WANDER);
}

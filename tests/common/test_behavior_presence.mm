#include <gtest/gtest.h>
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

// Tracking globals set by stub implementations below
static int s_updateCount = 0;
static std::string s_lastStatus;
static int s_visibleCallCount = 0;
static bool s_lastVisibleArg = false;

extern "C" void Presence_UpdateStatusFromBehavior(const char* status) {
    ++s_updateCount;
    s_lastStatus = status ? status : "";
}

extern "C" void Presence_SetGooseWindowVisible(bool visible) {
    ++s_visibleCallCount;
    s_lastVisibleArg = visible;
}

class BehaviorPresenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        s_updateCount = 0;
        s_lastStatus = "";
        s_visibleCallCount = 0;
        s_lastVisibleArg = false;

        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("presence")) {
            BehaviorRegistry::Instance().Restore();
        }
        goose = new Goose(1, "goose", 1920, 1080);
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

TEST_F(BehaviorPresenceTest, Init) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->init(ctx);
}

TEST_F(BehaviorPresenceTest, TickCallsUpdateStatus) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->state = GooseState::WANDER;

    ctx.time = 100.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(s_updateCount, 1);
    EXPECT_TRUE(s_lastStatus.find("Wandering") != std::string::npos);
}

TEST_F(BehaviorPresenceTest, TickEarlyReturn) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->state = GooseState::WANDER;

    ctx.time = 200.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(s_updateCount, 1);

    ctx.time = 200.3;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(s_updateCount, 1);

    ctx.time = 201.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(s_updateCount, 2);
}

TEST_F(BehaviorPresenceTest, TickStates) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    goose->state = GooseState::FETCHING;
    ctx.time = 300.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(s_lastStatus.find("Fetching") != std::string::npos);

    goose->state = GooseState::RETURNING;
    ctx.time = 301.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(s_lastStatus.find("Returning") != std::string::npos);

    goose->state = GooseState::CHASE_CURSOR;
    ctx.time = 302.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(s_lastStatus.find("Chasing") != std::string::npos);

    goose->state = GooseState::SNATCH_CURSOR;
    ctx.time = 303.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_TRUE(s_lastStatus.find("Snatching") != std::string::npos);
}

TEST_F(BehaviorPresenceTest, TickVisibilityChange) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    g_config.behaviors.info.visible = false;
    ctx.time = 400.0;
    b->tick(goose, ctx, 0.016, ctx.time);
    EXPECT_EQ(s_visibleCallCount, 1);
    EXPECT_FALSE(s_lastVisibleArg);

    b->tick(goose, ctx, 0.016, 401.0);
    EXPECT_EQ(s_visibleCallCount, 1);
}

TEST_F(BehaviorPresenceTest, RenderNoOp) {
    auto* b = BehaviorRegistry::Instance().Get("presence");
    ASSERT_NE(b, nullptr);
    b->render(goose, ctx, nullptr);
}

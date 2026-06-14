#include <gtest/gtest.h>
#include <vector>
#include "behavior.h"
#include "goose.h"
#include "behaviors/states/rainbow_state.h"
#include "config.h"

class BehaviorRainbowTest : public ::testing::Test {
protected:
    Goose* MakeGoose(int id) {
        Goose* g = new Goose(id, "test", 1920, 1080);
        g->behaviorsEnabled = true;
        geese.push_back(g);
        return g;
    }

    void SetUp() override {
        BehaviorStateManager::Instance().ClearAll();
        if (!BehaviorRegistry::Instance().Get("rainbow")) {
            BehaviorRegistry::Instance().Restore();
        }
        ctx.goose = MakeGoose(0);
        ctx.time = 0;
        ctx.world = &g_world;
    }

    void TearDown() override {
        for (auto* g : geese) delete g;
        geese.clear();
    }

    BehaviorContext ctx{};
    std::vector<Goose*> geese;
};

TEST_F(BehaviorRainbowTest, InitCreatesState) {
    auto* b = BehaviorRegistry::Instance().Get("rainbow");
    ASSERT_NE(b, nullptr);
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<RainbowState>(0, "rainbow");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->hue, 0.0f);
}

TEST_F(BehaviorRainbowTest, TickIncrementsHue) {
    auto* b = BehaviorRegistry::Instance().Get("rainbow");
    ASSERT_NE(b, nullptr);
    Goose* g = MakeGoose(1);
    ctx.goose = g;
    b->init(ctx);
    ctx.time = 1.0;

    b->tick(ctx.goose, ctx, 1.0, ctx.time);
    auto* state = BehaviorStateManager::Instance().Get<RainbowState>(1, "rainbow");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->hue, g_config.behaviors.rainbow.hueSpeed);
}

TEST_F(BehaviorRainbowTest, TickWrapsAt360) {
    auto* b = BehaviorRegistry::Instance().Get("rainbow");
    ASSERT_NE(b, nullptr);
    Goose* g = MakeGoose(2);
    ctx.goose = g;
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<RainbowState>(2, "rainbow");
    ASSERT_NE(state, nullptr);
    state->hue = 359.0f;

    ctx.time = 2.0;
    b->tick(ctx.goose, ctx, 0.01, ctx.time);
    EXPECT_GE(state->hue, 0.0f);
}

TEST_F(BehaviorRainbowTest, RenderNoop) {
    auto* b = BehaviorRegistry::Instance().Get("rainbow");
    ASSERT_NE(b, nullptr);
    b->render(ctx.goose, ctx, nullptr);
}

TEST_F(BehaviorRainbowTest, GetHueWhenEnabled) {
    bool prev = g_config.behaviors.fun.rainbow;
    g_config.behaviors.fun.rainbow = true;

    Rainbow_SetHue(3, 180.0f);
    EXPECT_FLOAT_EQ(Rainbow_GetHue(3), 180.0f);

    g_config.behaviors.fun.rainbow = prev;
}

TEST_F(BehaviorRainbowTest, GetHueWhenDisabled) {
    bool prev = g_config.behaviors.fun.rainbow;
    g_config.behaviors.fun.rainbow = false;

    Rainbow_SetHue(4, 90.0f);
    EXPECT_FLOAT_EQ(Rainbow_GetHue(4), 0.0f);

    g_config.behaviors.fun.rainbow = prev;
}

TEST_F(BehaviorRainbowTest, SetHue) {
    Rainbow_SetHue(5, 45.0f);
    auto* state = BehaviorStateManager::Instance().Get<RainbowState>(5, "rainbow");
    ASSERT_NE(state, nullptr);
    EXPECT_FLOAT_EQ(state->hue, 45.0f);
}

TEST_F(BehaviorRainbowTest, MultipleTicks) {
    auto* b = BehaviorRegistry::Instance().Get("rainbow");
    ASSERT_NE(b, nullptr);
    Goose* g = MakeGoose(6);
    ctx.goose = g;
    b->init(ctx);

    auto* state = BehaviorStateManager::Instance().Get<RainbowState>(6, "rainbow");
    ASSERT_NE(state, nullptr);
    EXPECT_EQ(state->hue, 0.0f);

    float speed = g_config.behaviors.rainbow.hueSpeed;
    float perTick = speed * 0.5f;
    ctx.time = 0.5;
    b->tick(ctx.goose, ctx, 0.5, ctx.time);
    EXPECT_FLOAT_EQ(state->hue, perTick);

    ctx.time = 1.0;
    b->tick(ctx.goose, ctx, 0.5, ctx.time);
    EXPECT_FLOAT_EQ(state->hue, perTick * 2);

    ctx.time = 1.5;
    b->tick(ctx.goose, ctx, 0.5, ctx.time);
    EXPECT_FLOAT_EQ(state->hue, perTick * 3);
}

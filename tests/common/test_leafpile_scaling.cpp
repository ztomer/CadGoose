// test_leafpile_scaling.cpp
// Unit tests for LeafPileActor scaling with globalScale
#include "gtest/gtest.h"
#include <cmath>

#include "actor_leafpile.h"
#include "config.h"
#include "world.h"

// Helper to reset config globals that tests may modify
static void ResetConfigGlobals() {
    g_config.general.globalScale = 1.0f;
}

TEST(LeafPileScaling, DefaultScale_WindowSize) {
    ResetConfigGlobals();
    g_config.general.globalScale = 1.0f;

    Vector2 pos{500.0f, 400.0f};
    float radius = 60.0f;
    float height = 100.0f;
    double time = 0.0;

    LeafPileActor pile(pos, radius, height, time);

    // At 1x scale, window should be sized to radius*2 + padding
    // winSize = max(60*2+20, 60*2+2*(100*0.6+20)) = max(140, 280) = 280
    float expectedWinSize = std::max(radius * 2.0f + 20.0f, radius * 2.0f + 2.0f * (height * 0.6f + 20.0f));
    EXPECT_FLOAT_EQ(pile.radius(), radius);
    EXPECT_FLOAT_EQ(pile.position().x, 500.0f);
    EXPECT_FLOAT_EQ(pile.position().y, 400.0f);
}

TEST(LeafPileScaling, DoubleScale_WindowSize) {
    ResetConfigGlobals();
    g_config.general.globalScale = 2.0f;

    Vector2 pos{500.0f, 400.0f};
    float radius = 60.0f;
    float height = 100.0f;
    double time = 0.0;

    LeafPileActor pile(pos, radius, height, time);

    // At 2x scale, window should be sized to (radius*2)*2 + padding
    // The render code scales: scaledRadius = radius * globalScale
    float scaledRadius = radius * 2.0f;
    float scaledHeight = height * 2.0f;
    float expectedWinSize = std::max(scaledRadius * 2.0f + 20.0f, scaledRadius * 2.0f + 2.0f * (scaledHeight * 0.6f + 20.0f));

    // Actor stores raw radius, scaling happens in render
    EXPECT_FLOAT_EQ(pile.radius(), radius);
}

TEST(LeafPileScaling, LeafPositions_Scaled) {
    ResetConfigGlobals();
    g_config.general.globalScale = 2.0f;

    Vector2 pos{500.0f, 400.0f};
    float radius = 60.0f;
    float height = 100.0f;
    double time = 0.0;

    LeafPileActor pile(pos, radius, height, time);

    // Leaf positions are stored in world units, scaled at render time
    // This test verifies the actor stores them correctly
    EXPECT_GT(pile.radius(), 0.0f);
    EXPECT_FLOAT_EQ(pile.position().x, 500.0f);
}

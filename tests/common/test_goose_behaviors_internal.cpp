#include <gtest/gtest.h>
#include "goose.h"
#include "goose_math.h"
#include "config.h"
#include "world.h"
#include "actor.h"

extern void triggerHonk(Goose& g, double time, double cd, double& lastBucket);
extern void initHonkState(Goose& g, double time);
extern void updateIdleHonk(Goose& g, double time, double cd, double& lastGeneric);
extern Vector2 GetSnatchForward(float dir, const Vector2& isoScale);

TEST(GooseInternal, GetSnatchForward_Zero) {
    Vector2 result = GetSnatchForward(0.0f, {1.0f, 1.0f});
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(GooseInternal, GetSnatchForward_Ninety) {
    Vector2 result = GetSnatchForward(90.0f, {1.0f, 1.0f});
    EXPECT_NEAR(result.x, 0.0f, 0.001f);
    EXPECT_NEAR(result.y, 1.0f, 0.001f);
}

TEST(GooseInternal, GetSnatchForward_WithScale) {
    Vector2 result = GetSnatchForward(0.0f, {2.0f, 1.5f});
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 0.0f);
}

TEST(GooseInternal, GetSnatchForward_OneEighty) {
    Vector2 result = GetSnatchForward(180.0f, {1.0f, 1.0f});
    EXPECT_NEAR(result.x, -1.0f, 0.001f);
    EXPECT_NEAR(result.y, 0.0f, 0.001f);
}

TEST(GooseInternal, InitHonkState_Fresh) {
    Goose g(1, "Test", 1920, 1080);
    g.honkState.init = false;

    initHonkState(g, 100.0);

    EXPECT_TRUE(g.honkState.init);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, -1e9);
    EXPECT_DOUBLE_EQ(g.honkState.lastChase, -1e9);
    EXPECT_DOUBLE_EQ(g.honkState.lastFetch, -1e9);
    EXPECT_DOUBLE_EQ(g.honkState.lastGeneric, -1e9);
    EXPECT_GT(g.honkState.nextIdleHonk, 100.0);
}

TEST(GooseInternal, InitHonkState_AlreadyInitialized) {
    Goose g(2, "Skip", 1920, 1080);
    g.honkState.init = true;
    g.honkState.lastAny = 50.0;

    initHonkState(g, 100.0);

    EXPECT_TRUE(g.honkState.init);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, 50.0);
}

TEST(GooseInternal, UpdateIdleHonk_BeforeCheckAhead) {
    Goose g(3, "Early", 1920, 1080);
    g.honkState.init = true;
    g.honkState.lastGeneric = -1e9;
    g.honkState.nextIdleHonk = 200.0;
    g.isResting = false;
    g.m_canHonk = true;

    double lastGeneric = -1e9;
    updateIdleHonk(g, 100.0, 0.9, lastGeneric);

    EXPECT_DOUBLE_EQ(lastGeneric, -1e9);
}

TEST(GooseInternal, UpdateIdleHonk_PastCheckAhead) {
    Goose g(4, "Late", 1920, 1080);
    g.honkState.init = true;
    g.honkState.lastAny = -1e9;
    g.honkState.lastGeneric = 0.0;
    g.honkState.nextIdleHonk = 50.0;
    g.isResting = false;
    g.m_canHonk = true;

    g_config.honk.idleCheckAhead = 2.0;
    g_config.honk.idleMin = 6.0;
    g_config.honk.idleMax = 14.0;

    double lastGeneric = 0.0;
    updateIdleHonk(g, 100.0, 0.0, lastGeneric);

    EXPECT_GT(lastGeneric, 50.0);
    EXPECT_GT(g.honkState.nextIdleHonk, 100.0);
}

TEST(GooseInternal, TriggerHonk_Basic) {
    Goose g(5, "Honker", 1920, 1080);
    g.isResting = false;
    g.m_canHonk = true;
    g.honkState.init = true;
    g.honkState.lastAny = -1e9;

    g_config.honk.minGap = 0.6;

    double lastBucket = -1e9;
    triggerHonk(g, 100.0, 0.0, lastBucket);

    EXPECT_DOUBLE_EQ(lastBucket, 100.0);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, 100.0);
}

TEST(GooseInternal, TriggerHonk_Resting) {
    Goose g(6, "Silent", 1920, 1080);
    g.isResting = true;
    g.m_canHonk = true;
    g.honkState.init = true;
    g.honkState.lastAny = -1e9;

    double lastBucket = -1e9;
    triggerHonk(g, 100.0, 0.0, lastBucket);

    EXPECT_DOUBLE_EQ(lastBucket, -1e9);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, -1e9);
}

TEST(GooseInternal, TriggerHonk_CannotHonk) {
    Goose g(7, "Muted", 1920, 1080);
    g.isResting = false;
    g.m_canHonk = false;
    g.honkState.init = true;
    g.honkState.lastAny = -1e9;

    double lastBucket = -1e9;
    triggerHonk(g, 100.0, 0.0, lastBucket);

    EXPECT_DOUBLE_EQ(lastBucket, -1e9);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, -1e9);
}

TEST(GooseInternal, TriggerHonk_MinGapTooSoon) {
    Goose g(8, "Quick", 1920, 1080);
    g.isResting = false;
    g.m_canHonk = true;
    g.honkState.init = true;
    g.honkState.lastAny = 99.5;

    g_config.honk.minGap = 0.6;

    double lastBucket = -1e9;
    triggerHonk(g, 100.0, 0.0, lastBucket);

    EXPECT_DOUBLE_EQ(lastBucket, -1e9);
}

TEST(GooseInternal, TriggerHonk_BucketCooldownActive) {
    Goose g(9, "Bucketed", 1920, 1080);
    g.isResting = false;
    g.m_canHonk = true;
    g.honkState.init = true;
    g.honkState.lastAny = -1e9;

    double lastBucket = 99.5;
    triggerHonk(g, 100.0, 0.6, lastBucket);

    EXPECT_DOUBLE_EQ(lastBucket, 99.5);
    EXPECT_DOUBLE_EQ(g.honkState.lastAny, -1e9);
}

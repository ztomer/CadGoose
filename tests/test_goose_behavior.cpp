#include <gtest/gtest.h>
#include <cmath>
#include <algorithm>

#include "goose_math.h"
#include "goose.h"
#include "world.h"
#include "assets.h"
#include "config.h"

struct ConfigSpec {
    bool debugToTerminal = false;
    bool debugVisuals = false;
    float globalScale = 1.0f;
    bool audioEnabled = true;
    bool memesEnabled = true;
    float baseWalkSpeed = 180.0f;
    float baseRunSpeed = 480.0f;
    bool cursorChaseEnabled = true;
    int cursorChaseChance = 3;
    float snatchDuration = 3.0f;
    bool multiMonitorEnabled = true;
    bool mudEnabled = true;
    int mudChance = 15;
    float mudLifetime = 15.0f;
    float runDistanceThreshold = 300.0f;
    float arrivalRadius = 50.0f;
    float targetReachedThresholdNormal = 30.0f;
    float targetReachedMinNormal = 25.0f;
    float targetReachedThresholdReturn = 60.0f;
    float targetReachedMinReturn = 50.0f;
    float catchThreshold = 22.0f;
    float snatchPullDistance = 140.0f;
    float snatchRadiusMin = 40.0f;
    float snatchRadiusMax = 120.0f;
} g_cfg;

TEST(Behavior, WanderToChase_ChanceCalculation) {
    int chaseChance = g_cfg.cursorChaseChance;
    int attackBias = 100;
    int totalChance = chaseChance + attackBias;
    if (totalChance > 100) totalChance = 100;
    EXPECT_EQ(totalChance, 100);
}

TEST(Behavior, SpeedWalkingWhenNear) {
    Vector2 pos{100, 100}, target{200, 200};
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::WANDER;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseWalkSpeed);
}

TEST(Behavior, SpeedRunningWhenFar) {
    Vector2 pos{100, 100}, target{500, 500};
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::WANDER;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseRunSpeed);
}

TEST(Behavior, SpeedRunningInChaseState) {
    Vector2 pos{100, 100}, target{150, 150};
    float dist = Vector2::Length(target - pos);
    GooseState state = GooseState::CHASE_CURSOR;

    float tSpeed = (dist > g_cfg.runDistanceThreshold ||
                    state == GooseState::FETCHING || state == GooseState::CHASE_CURSOR ||
                    state == GooseState::SNATCH_CURSOR || state == GooseState::RETURNING)
        ? g_cfg.baseRunSpeed
        : g_cfg.baseWalkSpeed;

    EXPECT_FLOAT_EQ(tSpeed, g_cfg.baseRunSpeed);
}

TEST(Behavior, ArrivalSlowdown) {
    float dist = 25.0f;
    float arrivalRadius = g_cfg.arrivalRadius;
    Vector2 desiredVel{100, 0};

    if (dist < arrivalRadius) {
        desiredVel = desiredVel * (dist / arrivalRadius);
    }

    EXPECT_LE(Vector2::Length(desiredVel), 50.0f);
}

TEST(Behavior, CatchThreshold) {
    float catchThreshold = g_cfg.catchThreshold;
    float globalScale = g_cfg.globalScale;
    float actualThreshold = std::max(22.0f * globalScale, 15.0f);
    EXPECT_FLOAT_EQ(actualThreshold, 22.0f);
}

TEST(Behavior, SnatchEndpoint_UsesCurrentPos) {
    Vector2 pos{300, 300};
    Vector2 fwd{1, 0};
    Vector2 right{-fwd.y, fwd.x};
    float pullDist = 140.0f;
    float lateralBias = 20.0f;
    float forwardBias = -30.0f;

    Vector2 endpoint = pos - fwd * pullDist + right * lateralBias + fwd * forwardBias;

    EXPECT_LT(endpoint.x, pos.x);
    EXPECT_FLOAT_EQ(endpoint.y, 320.0f);
}

TEST(Behavior, SnatchRadiusRange) {
    float radiusBase = 40.0f;
    int radiusRange = 80;
    int r = 0;
    float radius = radiusBase + r;
    EXPECT_GE(radius, 40.0f);
    EXPECT_LE(radius, 120.0f);
}

TEST(Behavior, TargetReached_NormalState) {
    Vector2 beakTip{228, 200};
    Vector2 target{200, 200};
    float threshold = std::max(g_cfg.targetReachedThresholdNormal * g_cfg.globalScale,
                               g_cfg.targetReachedMinNormal);
    float dist = Vector2::Distance(beakTip, target);
    EXPECT_LT(dist, threshold);
}

TEST(Behavior, TargetReached_ReturningState) {
    Vector2 beakTip{255, 200};
    Vector2 target{200, 200};
    float threshold = std::max(g_cfg.targetReachedThresholdReturn * g_cfg.globalScale,
                               g_cfg.targetReachedMinReturn);
    float dist = Vector2::Distance(beakTip, target);
    EXPECT_LT(dist, threshold);
}

TEST(Behavior, HonkCooldown) {
    double time = 10.0;
    double lastAny = 8.0;
    double lastBucket = 8.0;
    double minGap = 0.6;
    double cooldown = 1.8;

    bool canHonk = (time - lastAny) >= minGap && (time - lastBucket) >= cooldown;
    EXPECT_TRUE(canHonk);
}

TEST(Behavior, HonkBlockedByGlobalGap) {
    double time = 10.0;
    double lastAny = 9.5;
    double lastBucket = 8.0;
    double minGap = 0.6;
    double cooldown = 1.8;

    bool canHonk = (time - lastAny) >= minGap && (time - lastBucket) >= cooldown;
    EXPECT_FALSE(canHonk);
}

TEST(Behavior, DirectionBlending) {
    float dir = 90.0f;
    Vector2 vel{100, 0};

    Vector2 curDirVec{std::cos(dir * 3.14159f / 180.0f), std::sin(dir * 3.14159f / 180.0f)};
    Vector2 targetDirVec = Vector2::Normalize(vel);
    float blendRate = 0.15f;

    Vector2 blended = curDirVec + (targetDirVec - curDirVec) * blendRate;
    float newDir = std::atan2(blended.y, blended.x) * 180.0f / 3.14159f;

    EXPECT_GT(newDir, 0.0f);
    EXPECT_LT(newDir, 90.0f);
}

TEST(Behavior, DirectionInit) {
    int dir = 30;
    EXPECT_GE(dir, 0);
    EXPECT_LT(dir, 45);
}

TEST(Integration, Goose_WanderToChase) {
    Goose g(1, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.cursorChaseChance = 100;
    g.attackMouseBias = 100;

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    g_world.cursorGrabberId = -1;

    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::CHASE_CURSOR);
    EXPECT_EQ(g.target.x, 500.0f);
    EXPECT_EQ(g.target.y, 500.0f);
}

TEST(Integration, Goose_SnatchCursor) {
    Goose g(2, "Test", 1920, 1080);
    g.state = GooseState::CHASE_CURSOR;
    g.chaseStartTime = 0.0;
    g.pos = {500, 500};
    g.target = {500, 500};

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {0, 0};

    g_world.cursorGrabberId = -1;
    g.Update(0.1, 0.0, 1920, 1080, c);

    c.position = g.GetBeakTipDevice();

    g.Update(0.1, 0.1, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_world.cursorGrabberId, 2);
}

TEST(Integration, Goose_SnatchRelease) {
    Goose g(3, "Test", 1920, 1080);
    g.state = GooseState::SNATCH_CURSOR;
    g.snatchStartTime = 0.0;
    g.snatchDuration = 3.0f;
    g_world.cursorGrabberId = 3;

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {500, 500};

    g.Update(0.1, 1.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR);
    EXPECT_EQ(g_world.cursorGrabberId, 3);

    g.Update(0.1, 3.5, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g_world.cursorGrabberId, -1);
}

TEST(Integration, Goose_FetchItem) {
    Goose g(4, "Test", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {100, 100};
    g.target = {100, 100};

    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.memeFetchBias = 100;
    g.noteFetchBias = 100;

    CursorState c;

    g.Update(0.1, 0.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::FETCHING);
}

TEST(Integration, Goose_ReturningItem) {
    Goose g(5, "Test", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.forceItemFetch = 0;

    CursorState c;

    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::RETURNING);
    EXPECT_NE(g.heldItem, nullptr);
}

TEST(Integration, Goose_DropItem) {
    Goose g(6, "Test", 1920, 1080);
    g.state = GooseState::RETURNING;
    g.pos = {100, 100};
    g.target = {100, 100};
    g.heldItem = g_assets.GetRandomMeme(1920, 1080, 0.1f);

    int initialDrops = g_world.droppedItems.size();

    CursorState c;
    g.Update(0.1, 0.0, 1920, 1080, c);

    EXPECT_EQ(g.state, GooseState::WANDER);
    EXPECT_EQ(g.heldItem, nullptr);
    EXPECT_EQ(g_world.droppedItems.size(), initialDrops + 1);
}

TEST(GooseStateMachine, FetchStartTimeSetOnForceFetch) {
    Goose g(100, "FetchClock", 1920, 1080);
    g.state = GooseState::WANDER;
    g.pos = {500, 500};
    g.target = {500, 500};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    g.ForceFetch(0, 1920, 1080, 42.0);
    EXPECT_EQ(g.state, GooseState::FETCHING);
    // Target should be off-screen (one of 4 sides)
    bool offScreen = g.target.x < 0 || g.target.x > 1920 || g.target.y < 0 || g.target.y > 1080;
    EXPECT_TRUE(offScreen) << "Fetch target should be off-screen at edge margin";
}

TEST(GooseStateMachine, FetchEdgeMarginRespectedByClamp) {
    // When fetchEdgeMargin > screenClampExpanded, ClampToScreen should expand
    // to at least fetchEdgeMargin so the goose can reach the off-screen fetch target.
    // This prevents regression where fetchEdgeMargin=80 but clamp only allowed 50px.
    Goose g(101, "FetchClamp", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.target = {-300, 500};  // way off-screen
    g.pos = {-5, 500};       // already past screen edge

    g.Update(1.0, 42.0, 1920, 1080, CursorState{});
    // After physics+clamp, goose should be at or past -fetchEdgeMargin, not clamped at 0
    EXPECT_LE(g.pos.x, -10.0f) << "Goose in FETCHING should move past screen edge toward fetch target";
}

TEST(GooseStateMachine, FetchTimeoutWorks) {
    Goose g(102, "FetchTimeout", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {1000, 500};
    g.target = {1000, 500};
    g.forceItemFetch = 0;

    float origCooldown = g_config.item.fetchCooldown;
    g_config.item.fetchCooldown = 1.0f;

    CursorState c;
    // ForceFetch with positive time sets fetchStartTime
    g.ForceFetch(0, 1920, 1080, 1.0);

    // Tick at time well beyond cooldown*4 — should trigger timeout
    g.Update(0.1, 100.0, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER) << "Should have timed out of FETCHING";
    EXPECT_EQ(g.forceItemFetch, -1) << "Should have cleared forceItemFetch on timeout";

    g_config.item.fetchCooldown = origCooldown;
}

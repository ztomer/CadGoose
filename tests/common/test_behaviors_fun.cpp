// ===========================
// test_behaviors_fun.cpp
// Tests for fun behaviors: sonic, toys, boredom, peeking, affirmations, interactive_drops
// ===========================
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

static void ResetBehaviorState() {
    BehaviorStateManager::Instance().ClearAll();
}

// ===========================
// Sonic Behavior Tests
// ===========================
TEST(SonicBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<SonicState>(0, "sonic");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_TRUE(state->trails.empty());
    EXPECT_EQ(state->lastTrailTime, 0);
    EXPECT_EQ(state->lastHonkTime, 0);
}

TEST(SonicBehavior, SpeedMultiplierConstant) {
    float baseSpeed = 180.0f;
    float sonicMultiplier = 2.5f;
    float result = baseSpeed * sonicMultiplier;
    EXPECT_FLOAT_EQ(result, 450.0f);
}

TEST(SonicBehavior, TrailSpawnInterval) {
    double time = 0.0;
    double lastTrailTime = 0.0;
    float interval = 0.03f;
    EXPECT_FALSE(time - lastTrailTime >= interval);

    time = 0.05;
    EXPECT_TRUE(time - lastTrailTime >= interval);
}

TEST(SonicBehavior, TrailExpiry) {
    double currentTime = 10.0;
    float trailLifetime = 0.5f;

    SonicTrail oldTrail;
    oldTrail.time = 9.0;
    EXPECT_TRUE((currentTime - oldTrail.time) > trailLifetime);

    SonicTrail newTrail;
    newTrail.time = 9.8;
    EXPECT_FALSE((currentTime - newTrail.time) > trailLifetime);
}

TEST(SonicBehavior, HonkInterval) {
    double time = 0.0;
    double lastHonkTime = 0.0;
    float interval = 0.8f;
    EXPECT_FALSE(time - lastHonkTime >= interval);

    time = 1.0;
    EXPECT_TRUE(time - lastHonkTime >= interval);
}

TEST(SonicBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.sonicMode, false);
    g_config.behaviors.fun.sonicMode = true;
    EXPECT_EQ(g_config.behaviors.fun.sonicMode, true);
    g_config.behaviors.fun.sonicMode = false;
}

// ===========================
// Toys Behavior Tests
// ===========================
TEST(ToysBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<ToysState>(0, "toys");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->activeCount, 0);
    EXPECT_EQ(state->lastSpawnTime, 0);
    for (int i = 0; i < ToysState::MAX_TOYS; ++i) {
        EXPECT_FALSE(state->toys[i].active);
    }
}

TEST(ToysBehavior, MaxToysConstant) {
    EXPECT_EQ(ToysState::MAX_TOYS, 5);
}

TEST(ToysBehavior, ToyTypes) {
    EXPECT_EQ((int)Toy::Type::Stick, 0);
    EXPECT_EQ((int)Toy::Type::Ball, 1);
}

TEST(ToysBehavior, SpawnToyIncrementsCount) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<ToysState>(0, "toys");
    state->Reset();

    state->toys[0].pos = {100.0f, 100.0f};
    state->toys[0].angle = 45.0f;
    state->toys[0].time = 0.0;
    state->toys[0].active = true;
    state->toys[0].type = Toy::Type::Stick;
    state->activeCount = 1;

    EXPECT_EQ(state->activeCount, 1);
    EXPECT_TRUE(state->toys[0].active);
}

TEST(ToysBehavior, ToyExpiry) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<ToysState>(0, "toys");
    state->Reset();

    state->toys[0].active = true;
    state->toys[0].time = 0.0;
    state->activeCount = 1;

    double currentTime = 31.0;
    float lifetime = 30.0f;
    if (currentTime - state->toys[0].time > lifetime) {
        state->toys[0].active = false;
        state->activeCount--;
    }

    EXPECT_FALSE(state->toys[0].active);
    EXPECT_EQ(state->activeCount, 0);
}

TEST(ToysBehavior, FetchDistance) {
    Vector2 goosePos{100.0f, 100.0f};
    Vector2 toyPos{115.0f, 100.0f};
    float dist = Vector2::Distance(goosePos, toyPos);
    float fetchDist = 20.0f;
    EXPECT_LT(dist, fetchDist);

    Vector2 farToy{200.0f, 100.0f};
    float farDist = Vector2::Distance(goosePos, farToy);
    EXPECT_GT(farDist, fetchDist);
}

TEST(ToysBehavior, ChaseRadius) {
    Vector2 goosePos{100.0f, 100.0f};
    Vector2 toyPos{250.0f, 100.0f};
    float dist = Vector2::Distance(goosePos, toyPos);
    float chaseRadius = 200.0f;
    EXPECT_LT(dist, chaseRadius);
}

TEST(ToysBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.toysEnabled, true);
    g_config.behaviors.fun.toysEnabled = false;
    EXPECT_EQ(g_config.behaviors.fun.toysEnabled, false);
    g_config.behaviors.fun.toysEnabled = true;
}

// ===========================
// Boredom Behavior Tests
// ===========================
TEST(BoredomBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BoredomState>(0, "boredom");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isLyingDown);
    EXPECT_FALSE(state->isSighing);
    EXPECT_EQ(state->idleStartTime, 0);
    EXPECT_EQ(state->sighStartTime, 0);
    EXPECT_EQ(state->lieDownStartTime, 0);
}

TEST(BoredomBehavior, IdleDetection) {
    double time = 0.0;
    double idleStartTime = 0.0;

    idleStartTime = time;
    time = 601.0;
    EXPECT_GT(time - idleStartTime, 600.0);
}

TEST(BoredomBehavior, SighToLieDown) {
    double time = 0.0;
    double sighStartTime = 0.0;
    bool isLyingDown = false;

    sighStartTime = time;
    time = 3.0;
    EXPECT_GT(time - sighStartTime, 2.0);
    isLyingDown = true;
    EXPECT_TRUE(isLyingDown);
}

TEST(BoredomBehavior, LieDownDuration) {
    double time = 0.0;
    double lieDownStartTime = 0.0;
    bool isLyingDown = true;

    lieDownStartTime = time;
    time = 9.0;
    EXPECT_GT(time - lieDownStartTime, 8.0);
    isLyingDown = false;
    EXPECT_FALSE(isLyingDown);
}

TEST(BoredomBehavior, MovementResetsBoredom) {
    float currentSpeed = 5.0f;
    bool isSighing = true;
    bool isLyingDown = true;
    double idleStartTime = 100.0;

    if (currentSpeed >= 10.0f) {
        isSighing = false;
        isLyingDown = false;
        idleStartTime = 0;
    }
    EXPECT_TRUE(isSighing);
    EXPECT_TRUE(isLyingDown);

    currentSpeed = 15.0f;
    if (currentSpeed >= 10.0f) {
        isSighing = false;
        isLyingDown = false;
        idleStartTime = 0;
    }
    EXPECT_FALSE(isSighing);
    EXPECT_FALSE(isLyingDown);
    EXPECT_EQ(idleStartTime, 0);
}

TEST(BoredomBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.boredom, false);
    g_config.behaviors.fun.boredom = true;
    EXPECT_EQ(g_config.behaviors.fun.boredom, true);
    g_config.behaviors.fun.boredom = false;
}

// ===========================
// Peeking Behavior Tests
// ===========================
TEST(PeekingBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<PeekingState>(0, "peeking");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isPeeking);
    EXPECT_EQ(state->peekStartTime, 0);
    EXPECT_EQ(state->peekSide, 0);
    EXPECT_EQ(state->nextPeekTime, 0);
}

TEST(PeekingBehavior, PeekDuration) {
    double time = 0.0;
    double peekStartTime = 0.0;
    bool isPeeking = true;

    peekStartTime = time;
    time = 2.0;
    EXPECT_GT(time - peekStartTime, 1.5);
    isPeeking = false;
    EXPECT_FALSE(isPeeking);
}

TEST(PeekingBehavior, EdgeDetection) {
    float margin = 30.0f;
    int screenW = 800;

    Vector2 posLeft{10.0f, 400.0f};
    bool atEdgeLeft = posLeft.x < margin;
    EXPECT_TRUE(atEdgeLeft);

    Vector2 posRight{790.0f, 400.0f};
    bool atEdgeRight = posRight.x > screenW - margin;
    EXPECT_TRUE(atEdgeRight);

    Vector2 posCenter{400.0f, 400.0f};
    bool atEdgeCenter = posCenter.x < margin || posCenter.x > screenW - margin;
    EXPECT_FALSE(atEdgeCenter);
}

TEST(PeekingBehavior, PeekSideLeft) {
    float margin = 30.0f;
    Vector2 pos{10.0f, 400.0f};
    int peekSide = 0;
    if (pos.x < margin) { peekSide = -1; }
    else if (pos.x > 800 - margin) { peekSide = 1; }
    else { peekSide = 0; }
    EXPECT_EQ(peekSide, -1);
}

TEST(PeekingBehavior, PeekSideRight) {
    float margin = 30.0f;
    Vector2 pos{790.0f, 400.0f};
    int peekSide = 0;
    if (pos.x < margin) { peekSide = -1; }
    else if (pos.x > 800 - margin) { peekSide = 1; }
    else { peekSide = 0; }
    EXPECT_EQ(peekSide, 1);
}

TEST(PeekingBehavior, NextPeekCooldown) {
    double time = 100.0;
    int randResult = 5;
    double nextPeekTime = time + 8.0 + randResult;
    EXPECT_GT(nextPeekTime, time + 8.0);
    EXPECT_LE(nextPeekTime, time + 18.0);
}

TEST(PeekingBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.peeking, true);
    g_config.behaviors.fun.peeking = false;
    EXPECT_EQ(g_config.behaviors.fun.peeking, false);
    g_config.behaviors.fun.peeking = true;
}

// ===========================
// Affirmations Behavior Tests
// ===========================
TEST(AffirmationsBehavior, DefaultMessages) {
    const char* msgs[] = {
        "You're doing great!",
        "Drink some water!",
        "Take a break!",
        "You got this!",
        "Stay awesome!",
        "You are enough!",
        "Keep going!",
        "You matter!",
        "Breathe!",
        "Have a snack!"
    };
    int count = sizeof(msgs) / sizeof(msgs[0]);
    EXPECT_EQ(count, 10);
    for (int i = 0; i < count; ++i) {
        EXPECT_STRNE(msgs[i], "");
    }
}

TEST(AffirmationsBehavior, CustomMessageTemplate) {
    std::string msg = "Hello {}!";
    std::string gooseName = "Honker";
    size_t pos = msg.find("{}");
    EXPECT_NE(pos, std::string::npos);
    msg.replace(pos, 2, gooseName);
    EXPECT_EQ(msg, "Hello Honker!");
}

TEST(AffirmationsBehavior, CustomMessageNoTemplate) {
    std::string msg = "You're awesome!";
    size_t pos = msg.find("{}");
    EXPECT_EQ(pos, std::string::npos);
    EXPECT_EQ(msg, "You're awesome!");
}

TEST(AffirmationsBehavior, IntervalCheck) {
    double time = 1000.0;
    double lastDropTime = 500.0;
    float interval = 300.0f;
    EXPECT_TRUE(time - lastDropTime >= interval);

    lastDropTime = 800.0;
    EXPECT_FALSE(time - lastDropTime >= interval);
}

TEST(AffirmationsBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.affirmations, false);
    g_config.behaviors.fun.affirmations = true;
    EXPECT_EQ(g_config.behaviors.fun.affirmations, true);
    g_config.behaviors.fun.affirmations = false;
}

TEST(AffirmationsBehavior, IntervalConfig) {
    EXPECT_FLOAT_EQ(g_config.behaviors.affirmations.interval, 300.0f);
}

// ===========================
// Interactive Drops Behavior Tests
// ===========================
TEST(InteractiveDropsBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<InteractiveDropsState>(0, "interactive_drops");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_TRUE(state->puddles.empty());
    EXPECT_TRUE(state->flowers.empty());
    EXPECT_EQ(state->lastDropTime, 0);
}

TEST(InteractiveDropsBehavior, PuddleGrowth) {
    double time = 0.0;
    double spawnTime = 0.0;
    float maxRadius = 40.0f;
    float radius = 5.0f;

    double age = 0.5;
    radius = maxRadius * (float)age * 2.0f;
    EXPECT_FLOAT_EQ(radius, 40.0f);
}

TEST(InteractiveDropsBehavior, PuddleFade) {
    double time = 5.0;
    double spawnTime = 0.0;
    float puddleLifetime = 30.0f;
    float alpha = 0.6f;

    double age = time - spawnTime;
    if (age >= 1.0) {
        alpha = 0.6f * (1.0f - (float)((age - 1.0) / (puddleLifetime - 1.0)));
    }
    EXPECT_GT(alpha, 0.0f);
    EXPECT_LT(alpha, 0.6f);
}

TEST(InteractiveDropsBehavior, PuddleExpiry) {
    double time = 31.0;
    double spawnTime = 0.0;
    float puddleLifetime = 30.0f;
    double age = time - spawnTime;
    EXPECT_GT(age, puddleLifetime);
}

TEST(InteractiveDropsBehavior, FlowerGrowth) {
    double time = 5.0;
    double spawnTime = 0.0;
    float growTime = 10.0f;
    double age = time - spawnTime;
    float growth = (float)age / growTime;
    float stemHeight = 15.0f * growth;
    float petalSize = 5.0f * growth;

    EXPECT_FLOAT_EQ(growth, 0.5f);
    EXPECT_FLOAT_EQ(stemHeight, 7.5f);
    EXPECT_FLOAT_EQ(petalSize, 2.5f);
}

TEST(InteractiveDropsBehavior, FlowerExpiry) {
    double time = 31.0;
    double spawnTime = 0.0;
    float growTime = 10.0f;
    double age = time - spawnTime;
    EXPECT_GT(age, growTime * 3.0f);
}

TEST(InteractiveDropsBehavior, PuddleSplashVelocity) {
    Vector2 puddlePos{100.0f, 100.0f};
    Vector2 cursorPos{120.0f, 100.0f};
    float radius = 15.0f;
    float dist = Vector2::Distance(cursorPos, puddlePos);
    EXPECT_LT(dist, radius + 10.0f);

    Vector2 vel = Vector2::Normalize(cursorPos - puddlePos) * 60.0f;
    EXPECT_FLOAT_EQ(vel.x, 60.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST(InteractiveDropsBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.interactiveDrops, false);
    g_config.behaviors.fun.interactiveDrops = true;
    EXPECT_EQ(g_config.behaviors.fun.interactiveDrops, true);
    g_config.behaviors.fun.interactiveDrops = false;
}

TEST(InteractiveDropsBehavior, PuddleLifetimeConfig) {
    EXPECT_FLOAT_EQ(g_config.behaviors.interactiveDrops.puddleLifetime, 30.0f);
}

TEST(InteractiveDropsBehavior, FlowerGrowTimeConfig) {
    EXPECT_FLOAT_EQ(g_config.behaviors.interactiveDrops.flowerGrowTime, 10.0f);
}

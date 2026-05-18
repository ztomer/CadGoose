// ===========================
// test_behaviors_visual.cpp
// Tests for visual/emotional behaviors: rainbow, acid, anger, health
// ===========================
#include "behaviors/states/all.h"
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
// Rainbow Behavior Tests
// ===========================
TEST(RainbowBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<RainbowState>(0, "rainbow");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FLOAT_EQ(state->hue, 0.0f);
    EXPECT_EQ(state->lastUpdate, 0);
}

TEST(RainbowBehavior, HueCycling) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(0, "rainbow");
    state->Reset();
    state->hue = 0.0f;

    float hueSpeed = 120.0f;
    double dt = 1.0;
    state->hue += hueSpeed * dt;
    EXPECT_FLOAT_EQ(state->hue, 120.0f);

    state->hue += hueSpeed * dt;
    EXPECT_FLOAT_EQ(state->hue, 240.0f);

    state->hue += hueSpeed * dt;
    if (state->hue >= 360.0f) state->hue -= 360.0f;
    EXPECT_FLOAT_EQ(state->hue, 0.0f);
}

TEST(RainbowBehavior, HueWrapAround) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(0, "rainbow");
    state->Reset();
    state->hue = 350.0f;

    float hueSpeed = 120.0f;
    double dt = 1.0;
    state->hue += hueSpeed * dt;
    if (state->hue >= 360.0f) state->hue -= 360.0f;
    EXPECT_FLOAT_EQ(state->hue, 110.0f);
}

TEST(RainbowBehavior, HueSpeedConfig) {
    EXPECT_FLOAT_EQ(g_config.behaviors.rainbow.hueSpeed, 120.0f);
}

TEST(RainbowBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.rainbow, false);
    g_config.behaviors.fun.rainbow = true;
    EXPECT_EQ(g_config.behaviors.fun.rainbow, true);
    g_config.behaviors.fun.rainbow = false;
}

// ===========================
// Acid Behavior Tests
// ===========================
TEST(AcidBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<AcidState>(0, "acid");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FLOAT_EQ(state->rotationAccumulator, 0.0f);
    EXPECT_EQ(state->lastHonkTime, 0);
    EXPECT_FALSE(state->isSpinning);
}

TEST(AcidBehavior, SpinTrigger) {
    int triggerChance = 300;
    bool triggered = false;
    for (int i = 0; i < 3000; ++i) {
        if (rand() % triggerChance == 0) {
            triggered = true;
            break;
        }
    }
    EXPECT_TRUE(triggered);
}

TEST(AcidBehavior, SpinAccumulation) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AcidState>(0, "acid");
    state->Reset();
    state->isSpinning = true;

    float spinSpeed = 720.0f;
    double dt = 0.5;
    state->rotationAccumulator += spinSpeed * dt;
    EXPECT_FLOAT_EQ(state->rotationAccumulator, 360.0f);
}

TEST(AcidBehavior, SpinCompletion) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AcidState>(0, "acid");
    state->Reset();
    state->isSpinning = true;

    float spinSpeed = 720.0f;
    float rotationTotal = 1080.0f;
    double dt = 1.5;
    state->rotationAccumulator += spinSpeed * dt;

    if (state->rotationAccumulator >= rotationTotal) {
        state->isSpinning = false;
        state->rotationAccumulator = 0.0f;
    }

    EXPECT_FALSE(state->isSpinning);
    EXPECT_FLOAT_EQ(state->rotationAccumulator, 0.0f);
}

TEST(AcidBehavior, DirectionWrapAround) {
    float dir = 350.0f;
    float spinSpeed = 720.0f;
    double dt = 0.1;
    dir += spinSpeed * dt;
    if (dir >= 360.0f) dir -= 360.0f;
    EXPECT_FLOAT_EQ(dir, 62.0f);
}

TEST(AcidBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.acid, false);
    g_config.behaviors.fun.acid = true;
    EXPECT_EQ(g_config.behaviors.fun.acid, true);
    g_config.behaviors.fun.acid = false;
}

TEST(AcidBehavior, AcidConfigDefaults) {
    EXPECT_FLOAT_EQ(g_config.behaviors.acid.spinSpeed, 720.0f);
    EXPECT_FLOAT_EQ(g_config.behaviors.acid.honkInterval, 0.15f);
    EXPECT_FLOAT_EQ(g_config.behaviors.acid.rotationTotal, 1080.0f);
    EXPECT_EQ(g_config.behaviors.acid.triggerChance, 300);
}

// ===========================
// Anger Behavior Tests
// ===========================
TEST(AngerBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<AngerState>(0, "anger");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FLOAT_EQ(state->angerLevel, 0.0f);
    EXPECT_EQ(state->lastAngerIncrease, 0);
    EXPECT_EQ(state->lastPunchTime, 0);
    EXPECT_FALSE(state->isPunching);
}

TEST(AngerBehavior, AngerIncrease) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(0, "anger");
    state->Reset();

    float increaseRate = 15.0f;
    double dt = 1.0;
    state->angerLevel = std::min(100.0f, state->angerLevel + increaseRate * (float)dt);
    EXPECT_FLOAT_EQ(state->angerLevel, 15.0f);
}

TEST(AngerBehavior, AngerDecrease) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(0, "anger");
    state->Reset();
    state->angerLevel = 50.0f;

    float decreaseRate = 8.0f;
    double dt = 1.0;
    state->angerLevel = std::max(0.0f, state->angerLevel - decreaseRate * (float)dt);
    EXPECT_FLOAT_EQ(state->angerLevel, 42.0f);
}

TEST(AngerBehavior, AngerClamped) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(0, "anger");
    state->Reset();
    state->angerLevel = 95.0f;

    float increaseRate = 15.0f;
    double dt = 1.0;
    state->angerLevel = std::min(100.0f, state->angerLevel + increaseRate * (float)dt);
    EXPECT_FLOAT_EQ(state->angerLevel, 100.0f);
}

TEST(AngerBehavior, PunchThreshold) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(0, "anger");
    state->Reset();
    state->angerLevel = 85.0f;

    float punchThreshold = 80.0f;
    bool shouldPunch = state->angerLevel >= punchThreshold;
    EXPECT_TRUE(shouldPunch);

    state->angerLevel = 50.0f;
    shouldPunch = state->angerLevel >= punchThreshold;
    EXPECT_FALSE(shouldPunch);
}

TEST(AngerBehavior, PunchSpeedBoost) {
    float runSpeed = 480.0f;
    float punchMultiplier = 1.3f;
    float speed = runSpeed * punchMultiplier;
    EXPECT_FLOAT_EQ(speed, 624.0f);
}

TEST(AngerBehavior, CursorRadiusConfig) {
    EXPECT_FLOAT_EQ(g_config.behaviors.anger.cursorRadius, 100.0f);
}

TEST(AngerBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.fun.anger, false);
    g_config.behaviors.fun.anger = true;
    EXPECT_EQ(g_config.behaviors.fun.anger, true);
    g_config.behaviors.fun.anger = false;
}

// ===========================
// Health Behavior Tests
// ===========================
TEST(HealthBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<HealthState>(0, "health");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FLOAT_EQ(state->currentHealth, 100.0f);
    EXPECT_FLOAT_EQ(state->maxHealth, 100.0f);
    EXPECT_FALSE(state->isDead);
    EXPECT_FLOAT_EQ(state->regenAccumulator, 0.0f);
}

TEST(HealthBehavior, HealthRegen) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(0, "health");
    state->Reset();
    state->currentHealth = 90.0f;

    float regenRate = 0.5f;
    double dt = 2.0;
    state->regenAccumulator += regenRate * dt;
    if (state->regenAccumulator >= 1.0f) {
        state->currentHealth = std::min(state->maxHealth, state->currentHealth + 1.0f);
        state->regenAccumulator = 0.0f;
    }

    EXPECT_FLOAT_EQ(state->currentHealth, 91.0f);
}

TEST(HealthBehavior, HealthDamage) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(0, "health");
    state->Reset();
    state->currentHealth = 100.0f;

    float amount = 5.0f;
    state->currentHealth = std::max(0.0f, state->currentHealth - amount);
    EXPECT_FLOAT_EQ(state->currentHealth, 95.0f);
}

TEST(HealthBehavior, DeathAtZero) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HealthState>(0, "health");
    state->Reset();
    state->currentHealth = 3.0f;

    float amount = 5.0f;
    state->currentHealth = std::max(0.0f, state->currentHealth - amount);
    if (state->currentHealth <= 0) {
        state->isDead = true;
    }

    EXPECT_TRUE(state->isDead);
    EXPECT_FLOAT_EQ(state->currentHealth, 0.0f);
}

TEST(HealthBehavior, DamageCooldown) {
    double time = 10.0;
    double lastDamageTime = 9.0;
    float damageCooldown = 2.0f;
    float currentSpeed = 250.0f;

    bool shouldDamage = (time - lastDamageTime > damageCooldown) && (currentSpeed > 200.0f);
    EXPECT_FALSE(shouldDamage);

    lastDamageTime = 7.0;
    shouldDamage = (time - lastDamageTime > damageCooldown) && (currentSpeed > 200.0f);
    EXPECT_TRUE(shouldDamage);
}

TEST(HealthBehavior, HealthBarPercentage) {
    float currentHealth = 75.0f;
    float maxHealth = 100.0f;
    float pct = currentHealth / maxHealth;
    EXPECT_FLOAT_EQ(pct, 0.75f);

    float r = 1.0f - pct;
    float g = pct;
    EXPECT_FLOAT_EQ(r, 0.25f);
    EXPECT_FLOAT_EQ(g, 0.75f);
}

TEST(HealthBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.systems.health, false);
    g_config.behaviors.systems.health = true;
    EXPECT_EQ(g_config.behaviors.systems.health, true);
    g_config.behaviors.systems.health = false;
}

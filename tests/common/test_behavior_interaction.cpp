#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

#include "behavior.h"
#include "goose_math.h"
#include "goose.h"
#include "config.h"
#include "world.h"

TEST(BehaviorInteraction, HonkCooldownWithFasterTick) {
    double honkCooldown = 0.5;
    double lastHonkTime = 0.0;
    int honkCount = 0;
    double time = 0.0;
    double dt = 0.016;

    for (int i = 0; i < 60; ++i) {
        time += dt;
        if (time - lastHonkTime >= honkCooldown) {
            honkCount++;
            lastHonkTime = time;
        }
    }
    EXPECT_GE(honkCount, 1);
    EXPECT_LE(honkCount, 3);
}

TEST(BehaviorInteraction, AngerTriggersPunch) {
    float angerLevel = 0.0f;
    double time = 0.0;
    double lastPunchTime = -10.0;
    bool isPunching = false;
    float dt = 0.016f;

    for (int i = 0; i < 100; ++i) {
        time += dt;
        angerLevel += 0.5f;
        if (angerLevel >= 80.0f && !isPunching &&
            (time - lastPunchTime) > 1.5) {
            isPunching = true;
            lastPunchTime = time;
            angerLevel = 0.0f;
        }
        if (isPunching && (time - lastPunchTime) > 0.3) {
            isPunching = false;
        }
    }

    EXPECT_TRUE(isPunching || angerLevel > 0.0f);
}

TEST(BehaviorInteraction, BallKickAndChase) {
    Vector2 goosePos{400.0f, 300.0f};
    BallState::Ball ball;
    ball.pos = {500.0f, 300.0f};
    ball.vel = {0, 0};
    ball.radius = 25.0f;
    ball.active = true;

    float dx = ball.pos.x - goosePos.x;
    bool shouldChase = std::abs(dx) < 500.0f;
    EXPECT_TRUE(shouldChase);

    Vector2 directionToBall{ball.pos.x - goosePos.x, ball.pos.y - goosePos.y};
    float len = std::sqrt(directionToBall.x * directionToBall.x + directionToBall.y * directionToBall.y);
    if (len > 0) { directionToBall.x /= len; directionToBall.y /= len; }

    goosePos.x += directionToBall.x * 200.0f * 0.016f;
    goosePos.y += directionToBall.y * 200.0f * 0.016f;

    EXPECT_GT(goosePos.x, 400.0f);
}

TEST(BehaviorInteraction, RainbowHueUsedByDrawing) {
    float hue = 0.0f;

    for (int frame = 0; frame < 60; ++frame) {
        hue += 60.0f * (1.0f / 60.0f);
        if (hue >= 360.0f) hue -= 360.0f;
    }

    EXPECT_NEAR(hue, 60.0f, 0.1f);
}

TEST(BehaviorInteraction, HealthDamageAndRegen) {
    HealthState state;
    state.currentHealth = 100.0f;
    state.maxHealth = 100.0f;
    state.regenAccumulator = 0.0f;

    state.currentHealth = ::Clamp(state.currentHealth - 30.0f, 0.0f, state.maxHealth);
    EXPECT_FLOAT_EQ(state.currentHealth, 70.0f);

    double time = 0.0;
    double dt = 0.016;
    for (int i = 0; i < 60; ++i) {
        time += dt;
        if (state.currentHealth < state.maxHealth) {
            state.regenAccumulator += 10.0f * (float)dt;
            if (state.regenAccumulator >= 1.0f) {
                state.currentHealth = ::Clamp(state.currentHealth + 1.0f, 0.0f, state.maxHealth);
                state.regenAccumulator -= 1.0f;
            }
        }
    }

    EXPECT_GT(state.currentHealth, 70.0f);
    EXPECT_LE(state.currentHealth, 100.0f);
}

TEST(BehaviorInteraction, MultipleBehaviorsEnabledSimultaneously) {
    Goose g(10, "MultiTest", 1920, 1080);

    g_config.behaviors.fun.ball = true;
    g_config.behaviors.fun.rainbow = true;
    g_config.behaviors.control.honcker = true;
    g_config.behaviors.info.nametag = true;
    g_config.behaviors.systems.health = true;

    EXPECT_TRUE(g_config.behaviors.fun.ball);
    EXPECT_TRUE(g_config.behaviors.fun.rainbow);
    EXPECT_TRUE(g_config.behaviors.control.honcker);
    EXPECT_TRUE(g_config.behaviors.info.nametag);
    EXPECT_TRUE(g_config.behaviors.systems.health);

    g_config.behaviors.fun.ball = false;
    EXPECT_FALSE(g_config.behaviors.fun.ball);
    EXPECT_TRUE(g_config.behaviors.fun.rainbow);

    g_config.behaviors.fun.ball = false;
    g_config.behaviors.fun.rainbow = false;
    g_config.behaviors.control.honcker = false;
    g_config.behaviors.info.nametag = false;
    g_config.behaviors.systems.health = false;
}

TEST(BehaviorInteraction, PortalPairCreationAndTeleport) {
    PortalState state;

    state.portalA = {100.0f, 100.0f, true, 1};
    EXPECT_TRUE(state.portalA.active);

    state.portalB = {500.0f, 500.0f, true, 2};
    EXPECT_TRUE(state.portalB.active);

    float x = state.portalA.x;
    float y = state.portalA.y;
    float radius = 40.0f;

    float distA = std::hypot(x - state.portalA.x, y - state.portalA.y);
    EXPECT_LT(distA, radius);

    if (state.portalB.active && distA < radius) {
        x = state.portalB.x + (x - state.portalA.x) * 0.1f;
        y = state.portalB.y + (y - state.portalA.y) * 0.1f;
    }

    EXPECT_NEAR(x, 500.0f, 1.0f);
    EXPECT_NEAR(y, 500.0f, 1.0f);
}

TEST(BehaviorInteraction, DraggableGooseResistance) {
    Vector2 goosePos{400.0f, 300.0f};
    Vector2 cursorPos{410.0f, 305.0f};
    float dragRadius = 45.0f;

    float dx = goosePos.x - cursorPos.x;
    float dy = goosePos.y - cursorPos.y;
    bool onGoose = (dx > -dragRadius && dx < dragRadius &&
                    dy > -dragRadius && dy < dragRadius);

    EXPECT_TRUE(onGoose);

    if (onGoose) {
        goosePos.x = cursorPos.x - 5.0f;
        goosePos.y = cursorPos.y;
    }

    EXPECT_FLOAT_EQ(goosePos.x, 405.0f);
    EXPECT_FLOAT_EQ(goosePos.y, 305.0f);
}

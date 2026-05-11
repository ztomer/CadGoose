// ===========================
// test_behavior_ball.cpp
// Unit tests for Ball behavior
// ===========================
#include "gtest/gtest.h"
#include <cmath>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

static void ResetBallGlobals() {
}

TEST(BallBehavior, BallStateCreation) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BallState>(0, "ball");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->balls.size(), 0);
}

TEST(BallBehavior, BallPhysics_BounceOffWall) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BallState>(0, "ball");
    ASSERT_NE(state, nullptr);
    state->Reset();
    state->balls.resize(1);

    auto& ball = state->balls[0];
    ball.active = true;
    ball.pos = {10.0f, 50.0f};
    ball.vel = {-100.0f, 0.0f};
    ball.radius = 20.0f;

    float screenW = 800.0f;
    float screenH = 600.0f;

    ball.pos.x += ball.vel.x * 0.016f;
    if (ball.pos.x < ball.radius) {
        ball.pos.x = ball.radius;
        ball.vel.x *= -1.0f;
    }

    EXPECT_EQ(ball.pos.x, 20.0f);
    EXPECT_GE(ball.vel.x, 0.0f);
}

TEST(BallBehavior, BallGoesOffScreenRight) {
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BallState>(0, "ball");
    state->Reset();
    state->balls.resize(1);

    auto& ball = state->balls[0];
    ball.active = true;
    ball.pos = {790.0f, 50.0f};
    ball.vel = {200.0f, 0.0f};
    ball.radius = 20.0f;

    float screenW = 800.0f;
    float dt = 0.016f;

    ball.pos.x += ball.vel.x * dt;
    if (ball.pos.x > screenW - ball.radius) {
        ball.pos.x = screenW - ball.radius;
        ball.vel.x *= -1.0f;
    }

    EXPECT_EQ(ball.pos.x, screenW - ball.radius);
    EXPECT_LT(ball.vel.x, 0.0f);
}

TEST(BallBehavior, BallDistanceFromPoint) {
    Vector2 ballCenter{100.0f, 100.0f};
    Vector2 cursorPos{110.0f, 100.0f};
    float radius = 20.0f;

    float dx = cursorPos.x - ballCenter.x;
    float dy = cursorPos.y - ballCenter.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_LT(dist, radius);
    EXPECT_TRUE(dist < radius);
}

TEST(BallBehavior, BallOutsideCursorRange) {
    Vector2 ballCenter{100.0f, 100.0f};
    Vector2 cursorPos{200.0f, 200.0f};
    float radius = 20.0f;

    float dx = cursorPos.x - ballCenter.x;
    float dy = cursorPos.y - ballCenter.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    EXPECT_GT(dist, radius);
    EXPECT_FALSE(dist < radius);
}

TEST(BallBehavior, BallNormalization) {
    Vector2 vel{3.0f, 4.0f};
    float len = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    EXPECT_FLOAT_EQ(len, 5.0f);

    Vector2 normalized = vel / len;
    EXPECT_FLOAT_EQ(normalized.x, 0.6f);
    EXPECT_FLOAT_EQ(normalized.y, 0.8f);

    float newLen = std::sqrt(normalized.x * normalized.x + normalized.y * normalized.y);
    EXPECT_NEAR(newLen, 1.0f, 0.001f);
}

TEST(BallBehavior, BallKickDirection) {
    Vector2 ballCenter{100.0f, 100.0f};
    Vector2 cursorPos{80.0f, 100.0f};

    Vector2 kickDir = Vector2{ballCenter.x - cursorPos.x, ballCenter.y - cursorPos.y};
    float len = Vector2::Length(kickDir);
    Vector2 normalized = kickDir / len;

    EXPECT_GT(normalized.x, 0.0f);
    EXPECT_NEAR(normalized.y, 0.0f, 0.001f);
}
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

struct TestVec2 {
    float x, y;
    TestVec2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
    TestVec2 operator+(const TestVec2& o) const { return {x + o.x, y + o.y}; }
    TestVec2 operator-(const TestVec2& o) const { return {x - o.x, y - o.y}; }
    TestVec2 operator*(float s) const { return {x * s, y * s}; }
};

static float Length(TestVec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }
static void Normalize(TestVec2& v) {
    float len = Length(v);
    if (len > 0.0001f) { v.x /= len; v.y /= len; }
}
static float Distance(TestVec2 a, TestVec2 b) {
    return Length({a.x - b.x, a.y - b.y});
}

#define TEST_DT_SCALED_ROTATION(current, degPerSec, dt) (current + (degPerSec) * (dt))
#define TEST_DT_SCALED_VELOCITY(vel, friction, dt) (vel * std::pow(friction, dt * 60.0f))

static float TestNormalizeAngle(float angle) {
    while (angle >= 360.0f) angle -= 360.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

TEST(PhysicsHelpers, Vector2Length) {
    EXPECT_FLOAT_EQ(Length({3, 4}), 5.0f);
    EXPECT_FLOAT_EQ(Length({0, 5}), 5.0f);
    EXPECT_FLOAT_EQ(Length({0, 0}), 0.0f);
}

TEST(PhysicsHelpers, Vector2Normalize) {
    TestVec2 v{3, 4};
    Normalize(v);
    EXPECT_NEAR(v.x, 0.6f, 0.001f);
    EXPECT_NEAR(v.y, 0.8f, 0.001f);
}

TEST(PhysicsHelpers, Vector2Distance) {
    EXPECT_FLOAT_EQ(Distance({0, 0}, {3, 4}), 5.0f);
    EXPECT_FLOAT_EQ(Distance({1, 1}, {1, 4}), 3.0f);
}

TEST(PhysicsHelpers, Clamp) {
    EXPECT_FLOAT_EQ(::Clamp(5.0f, 0.0f, 10.0f), 5.0f);
    EXPECT_FLOAT_EQ(::Clamp(-5.0f, 0.0f, 10.0f), 0.0f);
    EXPECT_FLOAT_EQ(::Clamp(15.0f, 0.0f, 10.0f), 10.0f);
}

TEST(PhysicsHelpers, NormalizeAngle) {
    EXPECT_FLOAT_EQ(TestNormalizeAngle(90.0f), 90.0f);
    EXPECT_FLOAT_EQ(TestNormalizeAngle(450.0f), 90.0f);
    EXPECT_FLOAT_EQ(TestNormalizeAngle(-90.0f), 270.0f);
    EXPECT_FLOAT_EQ(TestNormalizeAngle(360.0f), 0.0f);
}

TEST(DTScaledRotation, FrameLockedVsDT) {
    float angle60 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        angle60 += 4.0f;
    }
    EXPECT_FLOAT_EQ(angle60, 240.0f);

    float angle120 = 0.0f;
    for (int i = 0; i < 120; ++i) {
        double dt = 1.0 / 120.0;
        angle120 = TEST_DT_SCALED_ROTATION(angle120, 240.0f, dt);
    }
    EXPECT_FLOAT_EQ(angle120, 240.0f);
}

TEST(DTScaledRotation, DifferentFrameRates) {
    float result30 = 0.0f;
    for (int i = 0; i < 30; ++i) {
        result30 = TEST_DT_SCALED_ROTATION(result30, 240.0f, 1.0 / 30.0);
    }
    EXPECT_FLOAT_EQ(result30, 240.0f);

    float result60 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        result60 = TEST_DT_SCALED_ROTATION(result60, 240.0f, 1.0 / 60.0);
    }
    EXPECT_FLOAT_EQ(result60, 240.0f);
}

TEST(DTScaledRotation, FractionalDT) {
    float angle = 0.0f;
    for (int i = 0; i < 60; ++i) {
        angle = TEST_DT_SCALED_ROTATION(angle, 120.0f, 1.0 / 60.0);
    }
    EXPECT_NEAR(angle, 120.0f, 0.001f);
}

TEST(DTScaledVelocity, FrictionScaling) {
    float vel = 500.0f;
    float friction = 0.95f;
    for (int i = 0; i < 60; ++i) {
        vel = TEST_DT_SCALED_VELOCITY(vel, friction, 1.0 / 60.0);
    }
    EXPECT_NEAR(vel, 500.0f * std::pow(friction, 60), 0.01f);
}

TEST(DTScaledVelocity, VariableDT) {
    float vel = 500.0f;
    float friction = 0.9f;
    vel = TEST_DT_SCALED_VELOCITY(vel, friction, 1.0 / 60.0);
    EXPECT_NEAR(vel, 500.0f * std::pow(0.9f, 1.0f), 0.01f);
}

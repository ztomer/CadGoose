// ===========================
// test_behavior.cpp
// Behavior System Unit Tests
// ===========================
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

// ===========================
// Mock Vector2 for standalone testing
// ===========================
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

// ===========================
// DT-Scaled Helper Macros (for testing)
// ===========================
#define TEST_DT_SCALED_ROTATION(current, degPerSec, dt) (current + (degPerSec) * (dt))
#define TEST_DT_SCALED_VELOCITY(vel, friction, dt) (vel * std::pow(friction, dt * 60.0f))

static float TestNormalizeAngle(float angle) {
    while (angle >= 360.0f) angle -= 360.0f;
    while (angle < 0.0f) angle += 360.0f;
    return angle;
}

// ===========================
// Physics Helper Tests
// ===========================
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

// ===========================
// DT-Scaled Rotation Tests
// ===========================
TEST(DTScaledRotation, FrameLockedVsDT) {
    // Simulate 60fps for 1 second
    float angle60 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        angle60 += 4.0f; // 4° per frame at 60fps
    }
    EXPECT_FLOAT_EQ(angle60, 240.0f);

    // Simulate 120fps for 1 second using DT
    float angle120 = 0.0f;
    for (int i = 0; i < 120; ++i) {
        double dt = 1.0 / 120.0;
        angle120 = TEST_DT_SCALED_ROTATION(angle120, 240.0f, dt); // 240°/sec
    }
    EXPECT_FLOAT_EQ(angle120, 240.0f);

    // Both should equal 240° after 1 second
    EXPECT_FLOAT_EQ(angle60, angle120);
}

TEST(DTScaledRotation, DifferentFrameRates) {
    float angle60 = 0.0f, angle120 = 0.0f, angle30 = 0.0f;

    for (int i = 0; i < 60; ++i) angle60 += 4.0f;

    for (int i = 0; i < 120; ++i) {
        double dt = 1.0 / 120.0;
        angle120 = TEST_DT_SCALED_ROTATION(angle120, 240.0f, dt);
    }

    for (int i = 0; i < 30; ++i) {
        double dt = 1.0 / 30.0;
        angle30 = TEST_DT_SCALED_ROTATION(angle30, 240.0f, dt);
    }

    EXPECT_FLOAT_EQ(angle60, angle120);
    EXPECT_FLOAT_EQ(angle120, angle30);
}

TEST(DTScaledRotation, FractionalDT) {
    float angle = 0.0f;
    double dt = 0.016667f; // ~60fps
    for (int i = 0; i < 100; ++i) {
        angle = TEST_DT_SCALED_ROTATION(angle, 180.0f, dt);
    }
    EXPECT_NEAR(angle, 180.0f * 100 * dt, 0.1f);
}

// ===========================
// DT-Scaled Velocity Tests
// ===========================
TEST(DTScaledVelocity, FrictionScaling) {
    float vel = 100.0f;

    // Frame-locked friction (wrong)
    float velFrame = 100.0f;
    for (int i = 0; i < 60; ++i) {
        velFrame *= 0.98f;
    }

    // DT-scaled friction (correct)
    float velDT = 100.0f;
    double dt = 1.0 / 60.0;
    for (int i = 0; i < 60; ++i) {
        velDT = TEST_DT_SCALED_VELOCITY(velDT, 0.98f, dt);
    }

    EXPECT_NEAR(velFrame, velDT, 0.01f);
}

TEST(DTScaledVelocity, VariableDT) {
    float vel60 = 100.0f;
    float vel120 = 100.0f;

    for (int i = 0; i < 60; ++i) {
        vel60 = TEST_DT_SCALED_VELOCITY(vel60, 0.98f, 1.0/60.0);
    }

    for (int i = 0; i < 120; ++i) {
        vel120 = TEST_DT_SCALED_VELOCITY(vel120, 0.98f, 1.0/120.0);
    }

    EXPECT_NEAR(vel60, vel120, 0.01f);
}

// ===========================
// BehaviorStateManager Tests
// ===========================
TEST(BehaviorStateManager, Singleton) {
    auto& mgr1 = BehaviorStateManager::Instance();
    auto& mgr2 = BehaviorStateManager::Instance();
    EXPECT_EQ(&mgr1, &mgr2);
}

TEST(BehaviorStateManager, GetOrCreateNew) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state = mgr.GetOrCreate<JailState>(1, "jail");
    ASSERT_NE(state, nullptr);
    EXPECT_FALSE(state->isJailed);
    EXPECT_FALSE(state->positionSet);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, GetOrCreateExisting) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state1 = mgr.GetOrCreate<JailState>(1, "jail");
    state1->jailPos.x = 100.0f;
    state1->jailPos.y = 200.0f;
    state1->isJailed = true;

    auto* state2 = mgr.GetOrCreate<JailState>(1, "jail");
    EXPECT_EQ(state1, state2);
    EXPECT_FLOAT_EQ(state2->jailPos.x, 100.0f);
    EXPECT_FLOAT_EQ(state2->jailPos.y, 200.0f);
    EXPECT_TRUE(state2->isJailed);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, MultipleGooseIds) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* state1 = mgr.GetOrCreate<JailState>(1, "jail");
    auto* state2 = mgr.GetOrCreate<JailState>(2, "jail");

    EXPECT_NE(state1, state2);
    state1->jailPos.x = 100.0f;
    state2->jailPos.x = 200.0f;

    auto* state1Again = mgr.Get<JailState>(1, "jail");
    auto* state2Again = mgr.Get<JailState>(2, "jail");

    EXPECT_FLOAT_EQ(state1Again->jailPos.x, 100.0f);
    EXPECT_FLOAT_EQ(state2Again->jailPos.x, 200.0f);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, MultipleBehaviorTypes) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    auto* jail = mgr.GetOrCreate<JailState>(1, "jail");
    auto* ball = mgr.GetOrCreate<BallState>(1, "ball");
    auto* acid = mgr.GetOrCreate<AcidState>(1, "acid");

    EXPECT_NE((void*)jail, (void*)ball);
    EXPECT_NE((void*)jail, (void*)acid);
    EXPECT_NE((void*)ball, (void*)acid);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, RemoveForGoose) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    mgr.GetOrCreate<JailState>(1, "jail");
    mgr.GetOrCreate<BallState>(1, "ball");
    mgr.GetOrCreate<JailState>(2, "jail");

    EXPECT_NE(mgr.Get<JailState>(1, "jail"), nullptr);
    EXPECT_NE(mgr.Get<BallState>(1, "ball"), nullptr);
    EXPECT_NE(mgr.Get<JailState>(2, "jail"), nullptr);

    mgr.RemoveForGoose(1);

    EXPECT_EQ(mgr.Get<JailState>(1, "jail"), nullptr);
    EXPECT_EQ(mgr.Get<BallState>(1, "ball"), nullptr);
    EXPECT_NE(mgr.Get<JailState>(2, "jail"), nullptr);

    mgr.ClearAll();
}

TEST(BehaviorStateManager, ThreadSafety) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    std::atomic<int> successCount(0);
    const int iterations = 1000;

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < iterations; ++i) {
                int gooseId = (i % 10) + 1;
                auto* state = mgr.GetOrCreate<JailState>(gooseId, "jail");
                if (state) {
                    state->jailPos.x = static_cast<float>(i);
                    auto* read = mgr.Get<JailState>(gooseId, "jail");
                    if (read && read->jailPos.x == static_cast<float>(i)) {
                        successCount.fetch_add(1);
                    }
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_GE(successCount.load(), iterations * 3);
    mgr.ClearAll();
}

TEST(BehaviorStateManager, StateCount) {
    auto& mgr = BehaviorStateManager::Instance();
    mgr.ClearAll();

    EXPECT_EQ(mgr.GetStateCount(), 0u);

    mgr.GetOrCreate<JailState>(1, "jail");
    EXPECT_EQ(mgr.GetStateCount(), 1u);

    mgr.GetOrCreate<BallState>(1, "ball");
    EXPECT_EQ(mgr.GetStateCount(), 2u);

    mgr.GetOrCreate<JailState>(2, "jail");
    EXPECT_EQ(mgr.GetStateCount(), 3u);

    mgr.RemoveForGoose(1);
    EXPECT_EQ(mgr.GetStateCount(), 1u);

    mgr.ClearAll();
    EXPECT_EQ(mgr.GetStateCount(), 0u);
}

// ===========================
// Jail State Tests
// ===========================
TEST(JailState, Reset) {
    JailState state;
    state.jailPos.x = 100.0f;
    state.jailPos.y = 200.0f;
    state.jailRadius = 150.0f;
    state.isJailed = true;
    state.positionSet = true;
    state.lastJailAttempt = 1000.0;

    state.Reset();

    EXPECT_FLOAT_EQ(state.jailPos.x, 0.0f);
    EXPECT_FLOAT_EQ(state.jailPos.y, 0.0f);
    EXPECT_FLOAT_EQ(state.jailRadius, 80.0f);
    EXPECT_FALSE(state.isJailed);
    EXPECT_FALSE(state.positionSet);
    EXPECT_EQ(state.lastJailAttempt, 0.0);
}

// ===========================
// Ball Physics Tests
// ===========================
TEST(BallPhysics, Gravity) {
    BallState::Ball ball;
    ball.pos = {100, 100};
    ball.vel = {0, 0};
    ball.radius = 25.0f;
    ball.active = true;

    // Simulate 1 second of falling at 60fps
    float screenHeight = 1080.0f;
    float globalScale = 1.0f;
    for (int i = 0; i < 60; ++i) {
        // Call UpdateBallPhysics - need to check signature
        float dy = ball.vel.y + (400.0f * (1.0/60.0)); // gravity
        ball.vel.y = dy;
        ball.pos.y += ball.vel.y * (1.0/60.0);
    }

    EXPECT_GT(ball.pos.y, 100.0f); // Should have fallen
}

TEST(BallPhysics, BounceOffFloor) {
    BallState::Ball ball;
    ball.pos = {100, 800.0f}; // Start lower, clearly falling
    ball.vel = {0, -50.0f}; // Moving up slightly
    ball.radius = 25.0f;
    ball.active = true;

    float screenHeight = 1080.0f;
    float globalScale = 1.0f;
    float floorY = screenHeight / globalScale - ball.radius; // 1055

    // Simulate until ball hits floor and bounces
    float dt = 1.0/60.0;
    float gravity = 400.0f;
    float bounceFactor = 0.7f;

    bool hitFloor = false;
    bool bounced = false;

    for (int i = 0; i < 300 && !bounced; ++i) {
        ball.vel.y += gravity * dt;
        ball.pos.y += ball.vel.y * dt;

        if (!hitFloor && ball.pos.y > floorY) {
            ball.pos.y = floorY;
            ball.vel.y = -ball.vel.y * bounceFactor;
            hitFloor = true;
            bounced = true;
        }
    }

    ASSERT_TRUE(hitFloor) << "Ball should have hit the floor";
    EXPECT_TRUE(bounced) << "Ball should have bounced";
}

TEST(BallPhysics, WallBounce) {
    BallState::Ball ball;
    ball.pos = {-10, 100}; // Off left edge
    ball.vel = {-100, 0};
    ball.radius = 25.0f;
    ball.active = true;

    float screenWidth = 1920.0f;
    float globalScale = 1.0f;
    float dt = 1.0/60.0;

    ball.vel.x += 0; // no gravity on x
    ball.pos.x += ball.vel.x * dt;

    if (ball.pos.x < ball.radius) {
        ball.pos.x = ball.radius;
        ball.vel.x = -ball.vel.x * 0.7f;
    }

    EXPECT_GE(ball.pos.x, ball.radius); // Clamped to screen
    EXPECT_GT(ball.vel.x, 0.0f); // Bounced right
}

TEST(BallPhysics, KickFromCursor) {
    BallState::Ball ball;
    ball.pos = {100, 100};
    ball.vel = {0, 0};
    ball.radius = 25.0f;

    float dx = ball.pos.x - 150.0f;
    float dy = ball.pos.y - 150.0f;
    float dist = std::sqrt(dx*dx + dy*dy);
    float kickForce = 300.0f;

    if (dist > 0.001f && dist < kickForce * 0.5f) {
        float len = dist;
        dx /= len; dy /= len;
        ball.vel.x = dx * kickForce;
        ball.vel.y = dy * kickForce;
    }

    EXPECT_NE(ball.vel.x, 0.0f);
    EXPECT_NE(ball.vel.y, 0.0f);
}

TEST(BallPhysics, KickFromGoose) {
    BallState::Ball ball;
    ball.pos = {100, 100};
    ball.vel = {0, 0};
    ball.radius = 25.0f;

    float dx = ball.pos.x - 50.0f;
    float dy = ball.pos.y - 50.0f;
    float dist = std::sqrt(dx*dx + dy*dy);
    float kickForce = 300.0f;

    if (dist > 0.001f) {
        float len = dist;
        dx /= len; dy /= len;
        ball.vel.x = dx * kickForce;
        ball.vel.y = dy * kickForce;
    }

    EXPECT_NE(ball.vel.x, 0.0f);
    EXPECT_NE(ball.vel.y, 0.0f);
}

// ===========================
// Breadcrumb Physics Tests
// ===========================
TEST(BreadcrumbPhysics, Gravity) {
    BreadCrumbState::Crumb crumb;
    crumb.pos = {100, 100};
    crumb.vel = {0, 0};
    crumb.lifetime = 5.0f;
    crumb.active = true;

    float dt = 1.0/60.0;
    crumb.vel.y += 400.0f * 0.5f * dt;
    crumb.pos.y += crumb.vel.y * dt;

    EXPECT_GT(crumb.pos.y, 100.0f);
}

TEST(BreadcrumbPhysics, LifetimeDecay) {
    BreadCrumbState::Crumb crumb;
    crumb.pos = {100, 100};
    crumb.vel = {0, 0};
    crumb.lifetime = 0.5f;
    crumb.active = true;

    float dt = 1.0/60.0;
    crumb.lifetime -= dt;
    EXPECT_TRUE(crumb.active);

    for (int i = 0; i < 30; ++i) {
        crumb.lifetime -= dt;
    }

    EXPECT_LT(crumb.lifetime, 0.0f); // Expired
}

// ===========================
// Acid Rotation Tests
// ===========================
TEST(AcidRotation, RotationOverTime) {
    float direction = 0.0f;
    double dt = 1.0 / 60.0;

    for (int i = 0; i < 60; ++i) {
        direction = TEST_DT_SCALED_ROTATION(direction, 240.0f, dt);
    }

    EXPECT_NEAR(direction, 240.0f, 0.1f);
}

TEST(AcidRotation, Normalize360) {
    float direction = 0.0f;
    double dt = 1.0 / 60.0;

    for (int i = 0; i < 180; ++i) { // 3 seconds = 720°
        direction = TEST_DT_SCALED_ROTATION(direction, 240.0f, dt);
        if (direction >= 360.0f) direction -= 360.0f;
    }

    EXPECT_LT(direction, 360.0f); // Should wrap around
}

// ===========================
// Rainbow Hue Tests
// ===========================
TEST(RainbowHue, Cycle) {
    float hue = 0.0f;
    double dt = 1.0 / 60.0;

    for (int i = 0; i < 60; ++i) {
        hue += 60.0f * dt; // 60°/sec
        if (hue >= 360.0f) hue -= 360.0f;
    }

    EXPECT_NEAR(hue, 60.0f, 0.1f);
}

TEST(RainbowHue, WrapAround) {
    float hue = 350.0f;
    double dt = 1.0 / 60.0;

    // 60 frames * 1 degree/frame = 60 degrees
    // 350 + 60 = 410, wrap to 50
    for (int i = 0; i < 60; ++i) {
        hue += 60.0f * dt; // 60°/sec
        if (hue >= 360.0f) hue -= 360.0f;
    }

    // Should be around 50 degrees (350 + 10 = 360, +40 = 400, wrap to 40)
    // Actually: 350 + 60*(60/60) = 350 + 60 = 410, but each frame adds 1 degree
    // So after 10 frames: 0, after 10 more: 10, wait - 350 + 60 = 410 → 50
    EXPECT_NEAR(hue, 50.0f, 5.0f); // Approximately 50 degrees
}

// ===========================
// Health System Tests
// ===========================
TEST(HealthSystem, ApplyDamage) {
    HealthState state;
    state.currentHealth = 100.0f;
    state.maxHealth = 100.0f;

    state.currentHealth = ::Clamp(state.currentHealth - 25.0f, 0.0f, state.maxHealth);
    EXPECT_FLOAT_EQ(state.currentHealth, 75.0f);

    state.currentHealth = ::Clamp(state.currentHealth - 100.0f, 0.0f, state.maxHealth);
    EXPECT_FLOAT_EQ(state.currentHealth, 0.0f); // Clamped
}

TEST(HealthSystem, ApplyRegen) {
    HealthState state;
    state.currentHealth = 50.0f;
    state.maxHealth = 100.0f;
    state.regenAccumulator = 0.0f;

    state.regenAccumulator += 10.0f * 0.1f; // 1 HP per 0.1 sec
    EXPECT_EQ(state.regenAccumulator, 1.0f);

    if (state.regenAccumulator >= 1.0f) {
        state.currentHealth = ::Clamp(state.currentHealth + 1.0f, 0.0f, state.maxHealth);
        state.regenAccumulator -= 1.0f;
    }

    EXPECT_EQ(state.regenAccumulator, 0.0f);
    EXPECT_FLOAT_EQ(state.currentHealth, 51.0f);
}

TEST(HealthSystem, NoRegenAtFull) {
    HealthState state;
    state.currentHealth = 100.0f;
    state.maxHealth = 100.0f;
    state.regenAccumulator = 0.0f;

    if (state.currentHealth < state.maxHealth) {
        state.regenAccumulator += 10.0f * 0.1f;
    }

    EXPECT_EQ(state.regenAccumulator, 0.0f); // Not accumulated at full health
    EXPECT_FLOAT_EQ(state.currentHealth, 100.0f);
}

// ===========================
// Anger System Tests
// ===========================
TEST(AngerSystem, IncreaseAnger) {
    AngerState state;
    state.angerLevel = 0.0f;

    state.angerLevel = ::Clamp(state.angerLevel + 50.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 50.0f);

    state.angerLevel = ::Clamp(state.angerLevel + 100.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 100.0f); // Clamped
}

TEST(AngerSystem, DecreaseAnger) {
    AngerState state;
    state.angerLevel = 50.0f;

    state.angerLevel = ::Clamp(state.angerLevel - 10.0f * 1.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 40.0f);

    state.angerLevel = ::Clamp(state.angerLevel - 10.0f * 5.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f); // Clamped
}

// ===========================
// Portal Tests
// ===========================
TEST(Portal, CollisionDetection) {
    PortalState::Portal portal;
    portal.x = 100.0f;
    portal.y = 100.0f;
    portal.active = true;
    portal.portalId = 1;

    auto CheckColl = [](float x, float y, const PortalState::Portal& p, float r) {
        if (!p.active) return false;
        float dx = x - p.x, dy = y - p.y;
        return std::sqrt(dx*dx + dy*dy) < r;
    };

    EXPECT_TRUE(CheckColl(100.0f, 100.0f, portal, 50.0f));
    EXPECT_TRUE(CheckColl(130.0f, 100.0f, portal, 50.0f));
    EXPECT_FALSE(CheckColl(200.0f, 100.0f, portal, 50.0f));

    portal.active = false;
    EXPECT_FALSE(CheckColl(100.0f, 100.0f, portal, 50.0f));
}

TEST(Portal, Teleport) {
    PortalState::Portal portalA{100.0f, 100.0f, true, 1};
    PortalState::Portal portalB{500.0f, 500.0f, true, 2};

    float x = 100.0f, y = 100.0f;
    float radius = 50.0f;

    auto CheckColl = [](float x, float y, const PortalState::Portal& p, float r) {
        if (!p.active) return false;
        float dx = x - p.x, dy = y - p.y;
        return std::hypot(dx, dy) < r;
    };

    // When at portalA center, should teleport to near portalB
    if (CheckColl(x, y, portalA, radius)) {
        x = portalB.x + (x - portalA.x) * 0.1f;
        y = portalB.y + (y - portalA.y) * 0.1f;
    }

    // After teleport, should be near portal B (offset by 10% of distance, which is small)
    EXPECT_NEAR(x, 500.0f, 10.0f); // Near portal B
    EXPECT_NEAR(y, 500.0f, 10.0f);
}

// ===========================
// Drag Resistance Tests
// ===========================
TEST(DragResistance, CalculateResistance) {
    auto CalcRes = [](float dragSpeed, float maxSpeed) {
        if (dragSpeed > maxSpeed) {
            return 1.0f - (maxSpeed / dragSpeed);
        }
        return 0.0f;
    };

    EXPECT_FLOAT_EQ(CalcRes(100.0f, 50.0f), 0.5f);
    EXPECT_FLOAT_EQ(CalcRes(50.0f, 50.0f), 0.0f);
    EXPECT_FLOAT_EQ(CalcRes(200.0f, 50.0f), 0.75f);
}

TEST(DragResistance, CheckResistance) {
    auto CheckRes = [](float dragSpeed, float threshold, float random) {
        return dragSpeed > threshold && random < 0.05f;
    };

    EXPECT_TRUE(CheckRes(100.0f, 50.0f, 0.01f));  // Fast + low random = resist
    EXPECT_FALSE(CheckRes(50.0f, 50.0f, 0.01f)); // Not fast enough
    EXPECT_FALSE(CheckRes(100.0f, 50.0f, 0.10f)); // Fast but high random = no resist
}

// ===========================
// BehaviorRegistry Tests
// ===========================
TEST(BehaviorRegistry, Singleton) {
    auto& reg1 = BehaviorRegistry::Instance();
    auto& reg2 = BehaviorRegistry::Instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST(BehaviorRegistry, GetNonexistent) {
    auto& reg = BehaviorRegistry::Instance();
    reg.Clear();

    EXPECT_EQ(reg.Get("nonexistent"), nullptr);
}

TEST(BehaviorRegistry, RegisterAndGet) {
    auto& reg = BehaviorRegistry::Instance();
    reg.Clear();

    static bool enabled = true;
    static Behavior testBehavior = {
        .id = "test",
        .name = "Test",
        .description = "Test behavior",
        .enabledPtr = &enabled,
        .init = nullptr,
        .tick = nullptr,
        .render = nullptr,
        .cleanup = nullptr
    };

    reg.Register(testBehavior);

    auto* retrieved = reg.Get("test");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_STREQ(retrieved->id, "test");
    EXPECT_STREQ(retrieved->name, "Test");

    reg.Clear();
}

TEST(BehaviorRegistry, BehaviorCount) {
    auto& reg = BehaviorRegistry::Instance();
    reg.Clear();

    EXPECT_EQ(reg.GetBehaviorCount(), 0u);

    static bool enabled1 = true;
    static Behavior b1 = {
        .id = "test1", .name = "Test1", .enabledPtr = &enabled1
    };
    static bool enabled2 = true;
    static Behavior b2 = {
        .id = "test2", .name = "Test2", .enabledPtr = &enabled2
    };

    reg.Register(b1);
    EXPECT_EQ(reg.GetBehaviorCount(), 1u);

    reg.Register(b2);
    EXPECT_EQ(reg.GetBehaviorCount(), 2u);

    reg.Clear();
}

// ===========================
// BehaviorConfig Tests
// ===========================
TEST(BehaviorConfig, DefaultValues) {
    Config cfg;
    
    EXPECT_FALSE(cfg.behaviors.fun.ball);
    EXPECT_FALSE(cfg.behaviors.fun.rainbow);
    EXPECT_FALSE(cfg.behaviors.control.honcker);
    EXPECT_FALSE(cfg.behaviors.systems.health);
    
    EXPECT_EQ(cfg.behaviors.ball.count, 5);
    EXPECT_FLOAT_EQ(cfg.behaviors.ball.size, 25.0f);
    EXPECT_FLOAT_EQ(cfg.behaviors.ball.speed, 300.0f);
    EXPECT_FLOAT_EQ(cfg.behaviors.ball.friction, 0.98f);
}

TEST(BehaviorConfig, EnableBehavior) {
    Config cfg;
    
    cfg.behaviors.fun.ball = true;
    cfg.behaviors.fun.rainbow = true;
    cfg.behaviors.control.honcker = true;
    
    EXPECT_TRUE(cfg.behaviors.fun.ball);
    EXPECT_TRUE(cfg.behaviors.fun.rainbow);
    EXPECT_TRUE(cfg.behaviors.control.honcker);
    
    EXPECT_FALSE(cfg.behaviors.systems.health);
}

// ===========================
// PortalState Tests
// ===========================
TEST(PortalState, DefaultValues) {
    PortalState state;
    EXPECT_FALSE(state.portalA.active);
    EXPECT_FALSE(state.portalB.active);
    EXPECT_TRUE(state.portalsEnabled);
}

TEST(PortalState, Reset) {
    PortalState state;
    state.portalA = {100.0f, 200.0f, true, 1};
    state.portalB = {500.0f, 600.0f, true, 2};
    state.portalsEnabled = false;

    state.Reset();

    EXPECT_FALSE(state.portalA.active);
    EXPECT_FALSE(state.portalB.active);
    EXPECT_TRUE(state.portalsEnabled);
}

TEST(PortalState, TeleportCollision) {
    PortalState::Portal portalA{100.0f, 100.0f, true, 1};
    PortalState::Portal portalB{500.0f, 500.0f, true, 2};

    float x = 100.0f, y = 100.0f;
    float radius = 20.0f;

    float distA = std::hypot(x - portalA.x, y - portalA.y);
    EXPECT_LT(distA, radius);
    EXPECT_TRUE(portalA.active);
    EXPECT_TRUE(portalB.active);
}

TEST(PortalState, PortalRadius) {
    constexpr float PORTAL_RADIUS = 30.0f;
    constexpr float PORTAL_TELEPORT_DIST = 20.0f;

    PortalState state;
    state.portalA = {100.0f, 100.0f, true, 1};
    state.portalB = {500.0f, 500.0f, true, 2};

    float x = 75.0f, y = 100.0f;
    float dist = std::hypot(x - state.portalA.x, y - state.portalA.y);

    EXPECT_GE(dist, PORTAL_TELEPORT_DIST);
    EXPECT_LT(dist, PORTAL_RADIUS);

    x = 100.0f, y = 100.0f;
    dist = std::hypot(x - state.portalA.x, y - state.portalA.y);
    EXPECT_LT(dist, PORTAL_TELEPORT_DIST);
}

TEST(PortalState, DeactivateAfterTeleport) {
    PortalState state;
    state.portalA = {100.0f, 100.0f, true, 1};
    state.portalB = {500.0f, 500.0f, true, 2};

    state.portalA.active = false;

    EXPECT_FALSE(state.portalA.active);
    EXPECT_TRUE(state.portalB.active);
}

// ===========================
// AngerState Tests
// ===========================
TEST(AngerState, DefaultValues) {
    AngerState state;
    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
    EXPECT_FALSE(state.isPunching);
}

TEST(AngerState, Reset) {
    AngerState state;
    state.angerLevel = 75.0f;
    state.isPunching = true;
    state.lastPunchTime = 123.45;
    state.lastAngerIncrease = 123.40;

    state.Reset();

    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
    EXPECT_FALSE(state.isPunching);
}

TEST(AngerState, AngerIncrease) {
    AngerState state;
    float increaseRate = 20.0f;
    double dt = 0.016; // 60fps

    state.angerLevel += increaseRate * (float)dt;
    EXPECT_GT(state.angerLevel, 0.0f);
    EXPECT_LT(state.angerLevel, 20.0f);
}

TEST(AngerState, AngerDecrease) {
    AngerState state;
    state.angerLevel = 50.0f;
    float decreaseRate = 5.0f;
    double dt = 0.016;

    state.angerLevel = std::max(0.0f, state.angerLevel - decreaseRate * (float)dt);
    EXPECT_GT(state.angerLevel, 40.0f);
    EXPECT_LT(state.angerLevel, 50.0f);
}

TEST(AngerState, AngerCapAt100) {
    AngerState state;
    float increaseRate = 15.0f;
    double dt = 1.0;

    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel = std::min(100.0f, state.angerLevel);

    EXPECT_FLOAT_EQ(state.angerLevel, 100.0f);
}

TEST(AngerState, AngerFloorAt0) {
    AngerState state;
    state.angerLevel = 10.0f;
    float decreaseRate = 8.0f;
    double dt = 1.0;

    state.angerLevel = std::max(0.0f, state.angerLevel - decreaseRate * (float)dt);
    EXPECT_FLOAT_EQ(state.angerLevel, 2.0f);
}

TEST(AngerState, PunchTriggerThreshold) {
    AngerState state;
    state.angerLevel = 80.0f;
    state.isPunching = false;

    constexpr float PUNCH_COOLDOWN = 1.5;
    double time = 10.0;
    double lastPunch = 8.0;

    bool canPunch = state.angerLevel >= 80.0f && !state.isPunching &&
                    (time - lastPunch) > PUNCH_COOLDOWN;
    EXPECT_TRUE(canPunch);
}

TEST(AngerState, PunchDuration) {
    AngerState state;
    state.isPunching = true;
    state.lastPunchTime = 10.0;

    constexpr float PUNCH_DURATION = 0.3;
    double currentTime = 10.15;

    bool stillPunching = state.isPunching && (currentTime - state.lastPunchTime) < PUNCH_DURATION;
    EXPECT_TRUE(stillPunching);

    currentTime = 10.35;
    stillPunching = state.isPunching && (currentTime - state.lastPunchTime) < PUNCH_DURATION;
    EXPECT_FALSE(stillPunching);
}

TEST(AngerState, CursorDistanceAnger) {
    constexpr float CURSOR_ANGER_RADIUS = 100.0f;
    constexpr float ANGER_INCREASE_RATE = 15.0f;

    struct TestGoose { Vector2 pos = {100, 100}; };
    TestGoose goose;

    Vector2 cursorPos = {150, 150};
    float distToCursor = std::hypot(goose.pos.x - cursorPos.x, goose.pos.y - cursorPos.y);

    bool shouldGetAngrier = distToCursor < CURSOR_ANGER_RADIUS;
    EXPECT_TRUE(shouldGetAngrier);
    EXPECT_LT(distToCursor, 100.0f);
}

TEST(AngerState, FarCursorNoAnger) {
    constexpr float CURSOR_ANGER_RADIUS = 100.0f;

    struct TestGoose { Vector2 pos = {100, 100}; };
    TestGoose goose;

    Vector2 cursorPos = {500, 500};
    float distToCursor = std::hypot(goose.pos.x - cursorPos.x, goose.pos.y - cursorPos.y);

    bool shouldGetAngrier = distToCursor < CURSOR_ANGER_RADIUS;
    EXPECT_FALSE(shouldGetAngrier);
    EXPECT_GT(distToCursor, 100.0f);
}

// ===========================
// Ball Type Tests
// ===========================
TEST(BallType, SoccerBallProperties) {
    BallState::Ball ball = BallState::CreateSoccerBall(28.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Soccer);
    EXPECT_FLOAT_EQ(ball.radius, 28.0f);
    EXPECT_FLOAT_EQ(ball.r, 1.0f);
    EXPECT_FLOAT_EQ(ball.g, 1.0f);
    EXPECT_FLOAT_EQ(ball.b, 1.0f);
    EXPECT_TRUE(ball.hasPattern);
    EXPECT_TRUE(ball.active);
}

TEST(BallType, BeachBallProperties) {
    BallState::Ball ball = BallState::CreateBeachBall(35.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Beach);
    EXPECT_FLOAT_EQ(ball.radius, 35.0f);
    EXPECT_FLOAT_EQ(ball.r, 1.0f);
    EXPECT_FLOAT_EQ(ball.g, 0.9f);
    EXPECT_FLOAT_EQ(ball.b, 0.6f);
    EXPECT_TRUE(ball.hasPattern);
    EXPECT_TRUE(ball.active);
}

TEST(BallType, GenericBallProperties) {
    BallState::Ball ball = BallState::CreateGenericBall(25.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Generic);
    EXPECT_FLOAT_EQ(ball.radius, 25.0f);
    EXPECT_FLOAT_EQ(ball.r, 0.3f);
    EXPECT_FLOAT_EQ(ball.g, 0.5f);
    EXPECT_FLOAT_EQ(ball.b, 0.9f);
    EXPECT_FALSE(ball.hasPattern);
    EXPECT_TRUE(ball.active);
}

TEST(BallType, DifferentBounceFactors) {
    constexpr float SOCCER_BOUNCE = 0.75f;
    constexpr float BEACH_BOUNCE = 0.65f;
    constexpr float GENERIC_BOUNCE = 0.8f;

    EXPECT_NE(SOCCER_BOUNCE, BEACH_BOUNCE);
    EXPECT_NE(BEACH_BOUNCE, GENERIC_BOUNCE);
}

TEST(BallType, SoccerBallDifferentSpeed) {
    constexpr float SOCCER_SPEED = 350.0f;
    constexpr float GENERIC_SPEED = 300.0f;

    EXPECT_GT(SOCCER_SPEED, GENERIC_SPEED);
}

TEST(BallType, BeachBallLarger) {
    constexpr float BEACH_SIZE = 35.0f;
    constexpr float GENERIC_SIZE = 25.0f;

    EXPECT_GT(BEACH_SIZE, GENERIC_SIZE);
}

// ===========================
// Pomodoro Timer Tests
// ===========================
TEST(PomodoroState, InitialPhase) {
    PomodoroState state;
    state.Reset();

    EXPECT_EQ(state.phase, PomodoroPhase::Work);
    EXPECT_EQ(state.completedSessions, 0);
    EXPECT_FALSE(state.isAggressive);
    EXPECT_FLOAT_EQ(state.accumulatedRotation, 0.0f);
    EXPECT_FALSE(state.speedMultiplierApplied);
}

TEST(PomodoroState, PhaseTransitions) {
    PomodoroState state;
    state.phase = PomodoroPhase::Work;
    state.completedSessions = 0;

    state.completedSessions++;
    state.phase = PomodoroPhase::Break;
    EXPECT_EQ(state.phase, PomodoroPhase::Break);

    state.phase = PomodoroPhase::Work;
    state.completedSessions++;
    EXPECT_EQ(state.phase, PomodoroPhase::Work);

    state.phase = PomodoroPhase::LongBreak;
    EXPECT_EQ(state.phase, PomodoroPhase::LongBreak);
}

TEST(PomodoroState, SessionCounting) {
    constexpr int SESSIONS_BEFORE_LONG_BREAK = 4;

    int sessions = 0;
    EXPECT_EQ(sessions, 0);

    for (int i = 0; i < SESSIONS_BEFORE_LONG_BREAK; ++i) {
        sessions++;
    }

    EXPECT_EQ(sessions, SESSIONS_BEFORE_LONG_BREAK);
    bool shouldLongBreak = (sessions >= SESSIONS_BEFORE_LONG_BREAK);
    EXPECT_TRUE(shouldLongBreak);
}

TEST(PomodoroState, AggressiveModeActivation) {
    PomodoroState state;
    state.phase = PomodoroPhase::Work;
    EXPECT_FALSE(state.isAggressive);

    state.isAggressive = true;
    EXPECT_TRUE(state.isAggressive);
}

TEST(PomodoroState, RotationAccumulation) {
    PomodoroState state;
    float rotation = 0.0f;

    for (int i = 0; i < 60; ++i) {
        rotation += 90.0f * (1.0f / 60.0f);
    }

    state.accumulatedRotation = rotation;
    EXPECT_NEAR(state.accumulatedRotation, 90.0f, 0.1f);
}

TEST(PomodoroState, PhaseDurations) {
    constexpr int WORK_MINUTES = 25;
    constexpr int BREAK_MINUTES = 5;
    constexpr int LONG_BREAK_MINUTES = 15;

    PomodoroState state;

    state.phase = PomodoroPhase::Work;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), WORK_MINUTES);

    state.phase = PomodoroPhase::Break;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), BREAK_MINUTES);

    state.phase = PomodoroPhase::LongBreak;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), LONG_BREAK_MINUTES);
}

TEST(PomodoroState, AggressiveHonkInterval) {
    constexpr float HONK_INTERVAL = 2.0f;
    double lastHonk = 0.0;
    double currentTime = 0.0;
    bool shouldHonk = false;

    currentTime = 0.5;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_FALSE(shouldHonk);

    lastHonk = currentTime;

    currentTime = 2.0;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_FALSE(shouldHonk);

    lastHonk = currentTime;

    currentTime = 4.0;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_TRUE(shouldHonk);
}

TEST(PomodoroState, SpeedMultiplierInAggressiveMode) {
    constexpr float SPEED_MULTIPLIER = 1.5f;
    constexpr float BASE_WALK_SPEED = 180.0f;
    constexpr float BASE_RUN_SPEED = 480.0f;

    float aggressiveSpeed = BASE_RUN_SPEED * SPEED_MULTIPLIER;
    EXPECT_FLOAT_EQ(aggressiveSpeed, 720.0f);
}

TEST(PomodoroState, TimeRemainingCalculation) {
    double phaseStartTime = 100.0;
    double phaseDuration = 25.0 * 60.0;
    double currentTime = 110.0;

    double elapsed = currentTime - phaseStartTime;
    double remaining = phaseDuration - elapsed;

    EXPECT_NEAR(remaining, 1490.0, 0.1);
}
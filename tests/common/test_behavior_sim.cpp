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
#include "behaviors/states/pomodoro_state.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/breadcrumb_state.h"
#include "behaviors/states/health_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/portal_state.h"

#define TEST_DT_SCALED_ROTATION(current, degPerSec, dt) (current + (degPerSec) * (dt))

TEST(PomodoroState, TextYFlipPosition) {
    auto FlipY = [](float y, float height) { return height - y; };

    EXPECT_FLOAT_EQ(FlipY(100.0f, 1080.0f), 980.0f);
    EXPECT_FLOAT_EQ(FlipY(0.0f, 1080.0f), 1080.0f);
    EXPECT_FLOAT_EQ(FlipY(1080.0f, 1080.0f), 0.0f);
}

TEST(PomodoroState, TextBackgroundRect) {
    struct Rect { float x, y, w, h; };
    auto CalcBgRect = [](float textWidth, float textHeight) -> Rect {
        return {2.0f, 2.0f, textWidth + 4.0f, textHeight + 4.0f};
    };

    Rect bg = CalcBgRect(100.0f, 30.0f);
    EXPECT_FLOAT_EQ(bg.x, 2.0f);
    EXPECT_FLOAT_EQ(bg.y, 2.0f);
    EXPECT_FLOAT_EQ(bg.w, 104.0f);
    EXPECT_FLOAT_EQ(bg.h, 34.0f);
}

TEST(BallPhysics, Gravity) {
    BallState::Ball ball;
    ball.pos = {100, 100};
    ball.vel = {0, 0};
    ball.radius = 25.0f;
    ball.active = true;

    float screenHeight = 1080.0f;
    float globalScale = 1.0f;
    for (int i = 0; i < 60; ++i) {
        float dy = ball.vel.y + (400.0f * (1.0/60.0));
        ball.vel.y = dy;
        ball.pos.y += ball.vel.y * (1.0/60.0);
    }

    EXPECT_GT(ball.pos.y, 100.0f);
}

TEST(BallPhysics, BounceOffFloor) {
    BallState::Ball ball;
    ball.pos = {100, 800.0f};
    ball.vel = {0, -50.0f};
    ball.radius = 25.0f;
    ball.active = true;

    float screenHeight = 1080.0f;
    float globalScale = 1.0f;
    float floorY = screenHeight / globalScale - ball.radius;

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
        }

        if (hitFloor && ball.vel.y > 0) {
            bounced = true;
        }
    }

    EXPECT_TRUE(hitFloor);
    EXPECT_TRUE(bounced);
    EXPECT_GT(ball.vel.y, 0);
}

TEST(BallPhysics, WallBounce) {
    BallState::Ball ball;
    ball.pos = {0, 400};
    ball.vel = {-200, 0};
    ball.radius = 25.0f;
    ball.active = true;

    float screenWidth = 1920.0f;
    float globalScale = 1.0f;
    float bounceFactor = 0.8f;

    float leftWall = ball.radius;
    float rightWall = screenWidth / globalScale - ball.radius;

    bool bounced = false;
    for (int i = 0; i < 10; ++i) {
        ball.pos.x += ball.vel.x * (1.0/60.0);
        if (ball.pos.x < leftWall) {
            ball.pos.x = leftWall;
            ball.vel.x = -ball.vel.x * bounceFactor;
            bounced = true;
        } else if (ball.pos.x > rightWall) {
            ball.pos.x = rightWall;
            ball.vel.x = -ball.vel.x * bounceFactor;
            bounced = true;
        }
    }

    EXPECT_TRUE(bounced);
    EXPECT_GE(ball.pos.x, leftWall);
    EXPECT_GT(ball.vel.x, 0);
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

    EXPECT_LT(crumb.lifetime, 0.0f);
}

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

    for (int i = 0; i < 180; ++i) {
        direction = TEST_DT_SCALED_ROTATION(direction, 240.0f, dt);
        if (direction >= 360.0f) direction -= 360.0f;
    }

    EXPECT_LT(direction, 360.0f);
}

TEST(RainbowHue, Cycle) {
    float hue = 0.0f;
    double dt = 1.0 / 60.0;

    for (int i = 0; i < 60; ++i) {
        hue += 60.0f * dt;
        if (hue >= 360.0f) hue -= 360.0f;
    }

    EXPECT_NEAR(hue, 60.0f, 0.1f);
}

TEST(RainbowHue, WrapAround) {
    float hue = 350.0f;
    double dt = 1.0 / 60.0;

    for (int i = 0; i < 60; ++i) {
        hue += 60.0f * dt;
        if (hue >= 360.0f) hue -= 360.0f;
    }

    EXPECT_NEAR(hue, 50.0f, 5.0f);
}

TEST(HealthSystem, ApplyDamage) {
    HealthState state;
    state.currentHealth = 100.0f;
    state.maxHealth = 100.0f;

    state.currentHealth = ::Clamp(state.currentHealth - 25.0f, 0.0f, state.maxHealth);
    EXPECT_FLOAT_EQ(state.currentHealth, 75.0f);

    state.currentHealth = ::Clamp(state.currentHealth - 100.0f, 0.0f, state.maxHealth);
    EXPECT_FLOAT_EQ(state.currentHealth, 0.0f);
}

TEST(HealthSystem, ApplyRegen) {
    HealthState state;
    state.currentHealth = 50.0f;
    state.maxHealth = 100.0f;
    state.regenAccumulator = 0.0f;

    state.regenAccumulator += 10.0f * 0.1f;
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

    EXPECT_EQ(state.regenAccumulator, 0.0f);
    EXPECT_FLOAT_EQ(state.currentHealth, 100.0f);
}

TEST(AngerSystem, IncreaseAnger) {
    AngerState state;
    state.angerLevel = 0.0f;

    state.angerLevel = ::Clamp(state.angerLevel + 50.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 50.0f);

    state.angerLevel = ::Clamp(state.angerLevel + 100.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 100.0f);
}

TEST(AngerSystem, DecreaseAnger) {
    AngerState state;
    state.angerLevel = 50.0f;

    state.angerLevel = ::Clamp(state.angerLevel - 10.0f * 1.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 40.0f);

    state.angerLevel = ::Clamp(state.angerLevel - 10.0f * 5.0f, 0.0f, 100.0f);
    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
}

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

    if (CheckColl(x, y, portalA, radius)) {
        x = portalB.x + (x - portalA.x) * 0.1f;
        y = portalB.y + (y - portalA.y) * 0.1f;
    }

    EXPECT_NEAR(x, 500.0f, 10.0f);
    EXPECT_NEAR(y, 500.0f, 10.0f);
}

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

    EXPECT_TRUE(CheckRes(100.0f, 50.0f, 0.01f));
    EXPECT_FALSE(CheckRes(50.0f, 50.0f, 0.01f));
    EXPECT_FALSE(CheckRes(100.0f, 50.0f, 0.10f));
}

TEST(BehaviorRegistry, Singleton) {
    auto& reg1 = BehaviorRegistry::Instance();
    auto& reg2 = BehaviorRegistry::Instance();
    EXPECT_EQ(&reg1, &reg2);
}

TEST(BehaviorRegistry, RegistryHasBehaviors) {
    auto& reg = BehaviorRegistry::Instance();
    EXPECT_GE(reg.GetBehaviorCount(), 16) << "Expected at least 16 behaviors";
    EXPECT_NE(reg.Get("honcker"), nullptr);
    EXPECT_NE(reg.Get("drag"), nullptr);
    EXPECT_NE(reg.Get("jail"), nullptr);
}

TEST(BehaviorRegistry, GetNonexistent) {
    auto& reg = BehaviorRegistry::Instance();
    EXPECT_EQ(reg.Get("nonexistent_behavior_xyz"), nullptr);
}

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

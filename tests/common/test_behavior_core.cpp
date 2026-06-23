#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include "behavior.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/breadcrumb_state.h"
#include "behaviors/states/health_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/portal_state.h"
#include "behaviors/states/jail_state.h"
#include "goose.h"
#include "goose_math.h"
#include "config.h"
#include "world.h"

// Declarations for functions defined in behavior.cpp
void UpdateBallPhysics(BallState::Ball& ball, float screenWidth, float screenHeight,
                       float globalScale, double dt);
void KickBallFromCursor(BallState::Ball& ball, float cursorX, float cursorY, float kickForce);
void KickBallFromGoose(BallState::Ball& ball, float gooseX, float gooseY, float kickForce);
void UpdateCrumbPhysics(BreadCrumbState::Crumb& crumb, float globalScale, double dt);
float CalculateAcidRotation(float currentDirection, float degreesPerSecond, double dt);
float CalculateRainbowHue(float currentHue, float degreesPerSecond, double dt);
void ApplyDamage(HealthState* state, float damage, double time);
void ApplyRegen(HealthState* state, float regenPerSecond, double dt);
void IncreaseAnger(AngerState* state, float amount, double time);
void DecreaseAnger(AngerState* state, float amount, double dt);
void ResetPunchCooldown(AngerState* state, double time);
bool CheckPortalCollision(float x, float y, const PortalState::Portal& portal, float radius);
void TeleportThroughPortal(float& x, float& y, PortalState::Portal& fromPortal,
                           PortalState::Portal& toPortal, float radius);
bool CheckDragResistance(float dragSpeed, float resistanceThreshold, float randomValue);
float CalculateDragResistance(float dragSpeed, float maxSpeed);

namespace {
    std::unordered_map<std::string, bool> g_testBehaviorEnabled;
    std::vector<std::unique_ptr<Behavior>> g_testBehaviors; // owns Behavior objects

    void RegisterTestBehavior(const char* id, bool initEnabled,
                              Behavior::InitFunc initFn = nullptr,
                              Behavior::TickFunc tickFn = nullptr,
                              Behavior::RenderFunc renderFn = nullptr,
                              Behavior::CleanupFunc cleanupFn = nullptr) {
        g_testBehaviorEnabled[id] = initEnabled;

        auto bhv = std::make_unique<Behavior>();
        bhv->id = id;
        bhv->name = id;
        bhv->enabledPtr = &g_testBehaviorEnabled[id];
        bhv->init = initFn;
        bhv->tick = tickFn;
        bhv->render = renderFn;
        bhv->cleanup = cleanupFn;
        BehaviorRegistry::Instance().Register(*bhv);
        g_testBehaviors.push_back(std::move(bhv));
    }

    void SetBehaviorEnabled(const char* id, bool enabled) {
        g_testBehaviorEnabled[id] = enabled;
    }

    void ClearTestRegistry() {
        BehaviorRegistry::Instance().Clear();
        BehaviorStateManager::Instance().ClearAll();
    }

    void RestoreRegistry() {
        BehaviorRegistry::Instance().RestoreOriginal();
    }

    // Fixture for tests that clear the registry — restores automatically.
    class ClearRegistryFixture : public ::testing::Test {
    protected:
        void SetUp() override {
            ClearTestRegistry();
        }
        void TearDown() override {
            RestoreRegistry();
        }
    };
}

// ===========================
// BehaviorRegistry lifecycle
// ===========================

TEST(BehaviorRegistryTest, RegisterAndGet) {
    auto& reg = BehaviorRegistry::Instance();
    auto* ball = reg.Get("ball");
    ASSERT_NE(ball, nullptr);
    EXPECT_STREQ(ball->id, "ball");

    auto* nonexistent = reg.Get("nonexistent");
    EXPECT_EQ(nonexistent, nullptr);
}

TEST(BehaviorRegistryTest, InitAllWithNullGoose) {
    auto& reg = BehaviorRegistry::Instance();
    reg.InitAll(nullptr);
}

TEST(BehaviorRegistryTest, TickAllWithNullGoose) {
    auto& reg = BehaviorRegistry::Instance();
    reg.TickAll(nullptr, 1.0/60.0, 100.0);
}

TEST(BehaviorRegistryTest, RenderAllWithNullGoose) {
    auto& reg = BehaviorRegistry::Instance();
    reg.RenderAll(nullptr, nullptr);
}

TEST(BehaviorRegistryTest, CleanupAllWithNullGoose) {
    auto& reg = BehaviorRegistry::Instance();
    reg.CleanupAll(nullptr);
}

TEST(BehaviorRegistryTest, ClearEmptiesBehaviors) {
    auto& reg = BehaviorRegistry::Instance();
    size_t before = reg.GetBehaviorCount();
    EXPECT_GT(before, 0u);

    reg.Clear();
    EXPECT_EQ(reg.GetBehaviorCount(), 0u);
    reg.Restore();
}

TEST(BehaviorRegistryTest, GetReturnsNullAfterClear) {
    auto& reg = BehaviorRegistry::Instance();
    reg.Clear();
    EXPECT_EQ(reg.Get("ball"), nullptr);
    reg.Restore();
}

// ===========================
// Ball Physics
// ===========================

class BallPhysicsTest : public ::testing::Test {
protected:
    BallState::Ball ball;
    void SetUp() override {
        ball.pos = {100, 100};
        ball.vel = {0, 0};
        ball.radius = 25.0f;
        ball.active = true;
    }
};

TEST_F(BallPhysicsTest, GravityPullsDown) {
    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);
    EXPECT_GT(ball.vel.y, 0);
    EXPECT_GT(ball.pos.y, 100);
}

TEST_F(BallPhysicsTest, InactiveNoChange) {
    ball.active = false;
    float oldY = ball.pos.y;
    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);
    EXPECT_EQ(ball.pos.y, oldY);
}

TEST_F(BallPhysicsTest, BounceOffFloor) {
    ball.pos.y = 1055.1;
    ball.vel.y = 300;

    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);

    EXPECT_LE(ball.pos.y, 1080 - ball.radius);
    EXPECT_LT(ball.vel.y, 0);
}

TEST_F(BallPhysicsTest, SmallBounceSetsVelToZero) {
    ball.pos.y = 1076;
    ball.vel.y = 0;

    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);

    EXPECT_LE(ball.pos.y, 1080 - ball.radius);
    EXPECT_FLOAT_EQ(ball.vel.y, 0);
}

TEST_F(BallPhysicsTest, BounceOffLeftWall) {
    ball.pos.x = 0;
    ball.vel.x = -200;

    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);

    EXPECT_GE(ball.pos.x, ball.radius);
    EXPECT_GT(ball.vel.x, 0);
}

TEST_F(BallPhysicsTest, BounceOffRightWall) {
    ball.pos.x = 1920;
    ball.vel.x = 200;

    UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);

    EXPECT_LE(ball.pos.x, 1920 - ball.radius);
    EXPECT_LT(ball.vel.x, 0);
}

TEST_F(BallPhysicsTest, KickFromCursorNearby) {
    KickBallFromCursor(ball, 90, 90, 500);
    EXPECT_GT(ball.vel.x, 0);
    EXPECT_GT(ball.vel.y, 0);
}

TEST_F(BallPhysicsTest, KickFromCursorFarAway) {
    KickBallFromCursor(ball, 0, 0, 10);
    EXPECT_EQ(ball.vel.x, 0);
    EXPECT_EQ(ball.vel.y, 0);
}

TEST_F(BallPhysicsTest, KickFromGoose) {
    KickBallFromGoose(ball, 90, 90, 500);
    EXPECT_GT(ball.vel.x, 0);
    EXPECT_GT(ball.vel.y, 0);
}

TEST_F(BallPhysicsTest, AirResistanceSlowsOverTime) {
    ball.vel = {500, 500};
    for (int i = 0; i < 100; i++) {
        UpdateBallPhysics(ball, 1920, 1080, 1.0f, 1.0/60.0);
    }
    EXPECT_LT(std::abs(ball.vel.x), 500);
    EXPECT_LT(std::abs(ball.vel.y), 500);
}

// ===========================
// Breadcrumb Physics
// ===========================

TEST(BreadcrumbPhysicsTest, GravityAndDecay) {
    BreadCrumbState::Crumb crumb;
    crumb.pos = {100, 100};
    crumb.vel = {0, 0};
    crumb.active = true;
    crumb.lifetime = 5.0f;

    UpdateCrumbPhysics(crumb, 1.0f, 1.0/60.0);
    EXPECT_GT(crumb.vel.y, 0);
    EXPECT_LT(crumb.lifetime, 5.0f);
}

TEST(BreadcrumbPhysicsTest, ExpiredBecomesInactive) {
    BreadCrumbState::Crumb crumb;
    crumb.active = true;
    crumb.lifetime = 0.001f;

    UpdateCrumbPhysics(crumb, 1.0f, 1.0);
    EXPECT_FALSE(crumb.active);
}

TEST(BreadcrumbPhysicsTest, InactiveDoesNotChange) {
    BreadCrumbState::Crumb crumb;
    crumb.active = false;
    crumb.lifetime = 5.0f;
    float oldY = crumb.pos.y;

    UpdateCrumbPhysics(crumb, 1.0f, 1.0/60.0);
    EXPECT_EQ(crumb.pos.y, oldY);
}

// ===========================
// Acid Rotation
// ===========================

TEST(AcidRotationTest, RotatesOverTime) {
    float result = CalculateAcidRotation(0.0f, 90.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 90.0f);
}

TEST(AcidRotationTest, WrapsAt360) {
    float result = CalculateAcidRotation(350.0f, 20.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);
}

TEST(AcidRotationTest, NoRotationWithZeroTime) {
    float result = CalculateAcidRotation(45.0f, 90.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 45.0f);
}

// ===========================
// Rainbow Hue Cycling
// ===========================

TEST(RainbowHueTest, CyclesOverTime) {
    float result = CalculateRainbowHue(0.0f, 60.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 60.0f);
}

TEST(RainbowHueTest, WrapsAt360) {
    float result = CalculateRainbowHue(350.0f, 20.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 10.0f);
}

TEST(RainbowHueTest, NoHueChangeWithZeroTime) {
    float result = CalculateRainbowHue(100.0f, 60.0f, 0.0f);
    EXPECT_FLOAT_EQ(result, 100.0f);
}

TEST(RainbowHueTest, ExactlyAt360Wraps) {
    float result = CalculateRainbowHue(300.0f, 60.0f, 1.0f);
    EXPECT_FLOAT_EQ(result, 0.0f);
}

// ===========================
// Health System
// ===========================

TEST(HealthSystemTest, ApplyDamageReducesHealth) {
    HealthState state;
    state.maxHealth = 100.0f;
    state.currentHealth = 100.0f;

    ApplyDamage(&state, 25.0f, 0.0);
    EXPECT_FLOAT_EQ(state.currentHealth, 75.0f);
}

TEST(HealthSystemTest, ApplyDamageClampsToZero) {
    HealthState state;
    state.maxHealth = 100.0f;
    state.currentHealth = 50.0f;

    ApplyDamage(&state, 100.0f, 0.0);
    EXPECT_FLOAT_EQ(state.currentHealth, 0.0f);
}

TEST(HealthSystemTest, ApplyDamageSetsLastDamageTime) {
    HealthState state;
    state.currentHealth = 100.0f;
    state.maxHealth = 100.0f;

    ApplyDamage(&state, 10.0f, 42.0);
    EXPECT_EQ(state.lastDamageTime, 42.0);
}

TEST(HealthSystemTest, ApplyDamageNullState) {
    ApplyDamage(nullptr, 10.0f, 0.0);
}

TEST(HealthSystemTest, ApplyRegenIncreasesHealth) {
    HealthState state;
    state.maxHealth = 100.0f;
    state.currentHealth = 50.0f;
    state.regenAccumulator = 0.0f;

    ApplyRegen(&state, 60.0f, 1.0);
    EXPECT_GE(state.currentHealth, 51.0f);
}

TEST(HealthSystemTest, ApplyRegenStopsAtMax) {
    HealthState state;
    state.maxHealth = 100.0f;
    state.currentHealth = 99.9f;
    state.regenAccumulator = 0.0f;

    ApplyRegen(&state, 60.0f, 1.0);
    EXPECT_FLOAT_EQ(state.currentHealth, 100.0f);
}

TEST(HealthSystemTest, ApplyRegenNullState) {
    ApplyRegen(nullptr, 60.0f, 1.0);
}

TEST(HealthSystemTest, NoRegenAtFullHealth) {
    HealthState state;
    state.maxHealth = 100.0f;
    state.currentHealth = 100.0f;
    state.regenAccumulator = 0.0f;

    ApplyRegen(&state, 60.0f, 1.0);
    EXPECT_FLOAT_EQ(state.regenAccumulator, 0.0f);
}

// ===========================
// Anger System
// ===========================

TEST(AngerSystemTest, IncreaseAnger) {
    AngerState state;
    state.angerLevel = 0.0f;

    IncreaseAnger(&state, 30.0f, 0.0);
    EXPECT_FLOAT_EQ(state.angerLevel, 30.0f);
}

TEST(AngerSystemTest, IncreaseAngerClampsTo100) {
    AngerState state;
    state.angerLevel = 90.0f;

    IncreaseAnger(&state, 30.0f, 0.0);
    EXPECT_FLOAT_EQ(state.angerLevel, 100.0f);
}

TEST(AngerSystemTest, IncreaseAngerSetsLastTime) {
    AngerState state;
    IncreaseAnger(&state, 10.0f, 55.0);
    EXPECT_EQ(state.lastAngerIncrease, 55.0);
}

TEST(AngerSystemTest, IncreaseAngerNullState) {
    IncreaseAnger(nullptr, 30.0f, 0.0);
}

TEST(AngerSystemTest, DecreaseAnger) {
    AngerState state;
    state.angerLevel = 100.0f;

    DecreaseAnger(&state, 10.0f, 1.0);
    EXPECT_FLOAT_EQ(state.angerLevel, 90.0f);
}

TEST(AngerSystemTest, DecreaseAngerClampsToZero) {
    AngerState state;
    state.angerLevel = 5.0f;

    DecreaseAnger(&state, 10.0f, 1.0);
    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
}

TEST(AngerSystemTest, DecreaseAngerNullState) {
    DecreaseAnger(nullptr, 10.0f, 1.0);
}

TEST(AngerSystemTest, ResetPunchCooldown) {
    AngerState state;
    state.isPunching = true;
    state.lastPunchTime = 0.0;

    ResetPunchCooldown(&state, 50.0);
    EXPECT_FALSE(state.isPunching);
    EXPECT_EQ(state.lastPunchTime, 50.0);
}

TEST(AngerSystemTest, ResetPunchCooldownNullState) {
    ResetPunchCooldown(nullptr, 50.0);
}

// ===========================
// Portal Teleport
// ===========================

TEST(PortalTest, CollisionDetection) {
    PortalState::Portal portal;
    portal.active = true;
    portal.x = 100;
    portal.y = 100;

    EXPECT_TRUE(CheckPortalCollision(105, 100, portal, 50));
    EXPECT_FALSE(CheckPortalCollision(200, 200, portal, 50));
}

TEST(PortalTest, CollisionWithInactivePortal) {
    PortalState::Portal portal;
    portal.active = false;
    portal.x = 100;
    portal.y = 100;

    EXPECT_FALSE(CheckPortalCollision(105, 100, portal, 50));
}

TEST(PortalTest, TeleportTransfersPosition) {
    PortalState::Portal from;
    from.active = true;
    from.x = 100;
    from.y = 100;

    PortalState::Portal to;
    to.active = true;
    to.x = 500;
    to.y = 500;

    float x = 105, y = 100;
    TeleportThroughPortal(x, y, from, to, 50);

    EXPECT_NE(x, 105);
    EXPECT_NE(y, 100);
}

TEST(PortalTest, TeleportNoOpWhenNotColliding) {
    PortalState::Portal from;
    from.active = true;
    from.x = 100;
    from.y = 100;

    PortalState::Portal to;
    to.active = true;
    to.x = 500;
    to.y = 500;

    float x = 0, y = 0;
    TeleportThroughPortal(x, y, from, to, 50);
    EXPECT_FLOAT_EQ(x, 0);
    EXPECT_FLOAT_EQ(y, 0);
}

TEST(PortalTest, TeleportNoOpWithInactiveFrom) {
    PortalState::Portal from;
    from.active = false;
    PortalState::Portal to;
    to.active = true;

    float x = 105, y = 100;
    TeleportThroughPortal(x, y, from, to, 50);
    EXPECT_FLOAT_EQ(x, 105);
}

TEST(PortalTest, TeleportNoOpWithInactiveTo) {
    PortalState::Portal from;
    from.active = true;
    from.x = 100;
    from.y = 100;
    PortalState::Portal to;
    to.active = false;

    float x = 105, y = 100;
    TeleportThroughPortal(x, y, from, to, 50);
    EXPECT_FLOAT_EQ(x, 105);
}

// ===========================
// Drag Resistance
// ===========================

TEST(DragResistanceTest, CheckResistanceAboveThreshold) {
    EXPECT_TRUE(CheckDragResistance(100.0f, 50.0f, 0.01f));
}

TEST(DragResistanceTest, CheckResistanceBelowThreshold) {
    EXPECT_FALSE(CheckDragResistance(30.0f, 50.0f, 0.01f));
}

TEST(DragResistanceTest, CheckResistanceHighRandom) {
    EXPECT_FALSE(CheckDragResistance(100.0f, 50.0f, 0.1f));
}

TEST(DragResistanceTest, CalculateResistanceAboveMax) {
    float r = CalculateDragResistance(200.0f, 100.0f);
    EXPECT_FLOAT_EQ(r, 0.5f);
}

TEST(DragResistanceTest, CalculateResistanceAtMax) {
    float r = CalculateDragResistance(100.0f, 100.0f);
    EXPECT_FLOAT_EQ(r, 0.0f);
}

TEST(DragResistanceTest, CalculateResistanceBelowMax) {
    float r = CalculateDragResistance(50.0f, 100.0f);
    EXPECT_FLOAT_EQ(r, 0.0f);
}

// ===========================
// Registry lifecycle (post-clear — empty registry, no real behaviors)
// ===========================

class StubRenderer : public IRenderer {
public:
    void SaveState() override {}
    void RestoreState() override {}
    void Translate(float, float) override {}
    void Scale(float, float) override {}
    void Rotate(float) override {}
    void DrawEllipse(RenderPoint, float, float, RenderColor) override {}
    void DrawEllipseOutline(RenderPoint, float, float, RenderColor, float) override {}
    void DrawLine(RenderPoint, RenderPoint, RenderColor, float) override {}
    void DrawRect(RenderRect, RenderColor) override {}
    void DrawRectOutline(RenderRect, RenderColor, float) override {}
    void DrawRoundedRect(RenderRect, float, RenderColor) override {}
    void DrawPolygon(const RenderPoint*, int, RenderColor) override {}
    void DrawImage(void*, RenderRect) override {}
    bool GetImageSize(void*, float*, float*) override { return false; }
    void DrawText(const char*, RenderPoint, RenderColor, float) override {}
    float MeasureText(const char*, float) override { return 0; }
    void SetAlpha(float) override {}
};

TEST_F(ClearRegistryFixture, InitAllWithCustomBehavior) {
    auto& reg = BehaviorRegistry::Instance();
    int initCount = 0;
    RegisterTestBehavior("custom_lifecycle", true,
        [&](BehaviorContext&) { initCount++; });
    Goose g(990, "CustomLifecycle", 1920, 1080);
    reg.InitAll(&g);
    EXPECT_EQ(initCount, 1);
}

TEST_F(ClearRegistryFixture, TickAllWithCustomBehavior) {
    auto& reg = BehaviorRegistry::Instance();
    int tickCount = 0;
    RegisterTestBehavior("custom_tick", true, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { tickCount++; });
    Goose g(991, "CustomTick", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    EXPECT_GT(tickCount, 0);
}

TEST_F(ClearRegistryFixture, RenderAllWithCustomBehavior) {
    auto& reg = BehaviorRegistry::Instance();
    int renderCount = 0;
    RegisterTestBehavior("custom_render", true, nullptr, nullptr,
        [&](Goose*, BehaviorContext&, IRenderer*) { renderCount++; });
    Goose g(992, "CustomRender", 1920, 1080);
    StubRenderer renderer;
    reg.RenderAll(&g, &renderer);
    EXPECT_EQ(renderCount, 1);
}

TEST_F(ClearRegistryFixture, CleanupAllWithCustomBehavior) {
    auto& reg = BehaviorRegistry::Instance();
    int cleanupCount = 0;
    RegisterTestBehavior("custom_cleanup", true, nullptr, nullptr, nullptr,
        [&](BehaviorContext&) { cleanupCount++; });
    Goose g(993, "CustomCleanup", 1920, 1080);
    reg.CleanupAll(&g);
    EXPECT_EQ(cleanupCount, 1);
}

TEST_F(ClearRegistryFixture, TickAllEnabledToDisabledTransition) {
    auto& reg = BehaviorRegistry::Instance();
    int tickCount = 0;
    int cleanupCount = 0;

    RegisterTestBehavior("test_transition", true, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { tickCount++; },
        nullptr,
        [&](BehaviorContext&) { cleanupCount++; });

    Goose g(994, "Transition", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    EXPECT_GT(tickCount, 0);
    EXPECT_EQ(cleanupCount, 0);

    SetBehaviorEnabled("test_transition", false);
    reg.TickAll(&g, 1.0/60.0, 200.0);
    EXPECT_EQ(cleanupCount, 1);

    reg.TickAll(&g, 1.0/60.0, 300.0);
    int finalTick = tickCount;
    reg.TickAll(&g, 1.0/60.0, 400.0);
    EXPECT_EQ(tickCount, finalTick);
}

TEST_F(ClearRegistryFixture, TickAllDisabledToEnabledTransition) {
    auto& reg = BehaviorRegistry::Instance();
    int initCount = 0;
    int tickCount = 0;

    RegisterTestBehavior("test_enable_transition", false,
        [&](BehaviorContext&) { initCount++; },
        [&](Goose*, BehaviorContext&, double, double) { tickCount++; });

    Goose g(995, "EnableTransition", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    EXPECT_EQ(initCount, 0);
    int ticksBeforeEnable = tickCount;

    SetBehaviorEnabled("test_enable_transition", true);
    reg.TickAll(&g, 1.0/60.0, 200.0);
    EXPECT_EQ(initCount, 1);
    EXPECT_GT(tickCount, ticksBeforeEnable);
}

TEST_F(ClearRegistryFixture, InitAllCatchesThrowingBehavior) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_throw_init", true,
        [&](BehaviorContext&) { throw std::runtime_error("init failure"); });
    Goose g(996, "ThrowInit", 1920, 1080);
    reg.InitAll(&g);
    SUCCEED();
}

TEST_F(ClearRegistryFixture, TickAllCatchesThrowingTick) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_throw_tick", true, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { throw std::runtime_error("tick failure"); });
    Goose g(997, "ThrowTick", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    SUCCEED();
}

TEST_F(ClearRegistryFixture, TickAllCatchesThrowingCleanup) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_throw_cleanup", true, nullptr, nullptr, nullptr,
        [&](BehaviorContext&) { throw std::runtime_error("cleanup failure"); });
    Goose g(998, "ThrowCleanup", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    SetBehaviorEnabled("test_throw_cleanup", false);
    reg.TickAll(&g, 1.0/60.0, 200.0);
    SUCCEED();
}

TEST_F(ClearRegistryFixture, TickAllCatchesThrowingInit) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_throw_disabled_init", false,
        [&](BehaviorContext&) { throw std::runtime_error("disabled init failure"); });
    Goose g(999, "ThrowDisabledInit", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    SetBehaviorEnabled("test_throw_disabled_init", true);
    reg.TickAll(&g, 1.0/60.0, 200.0);
    SUCCEED();
}

TEST_F(ClearRegistryFixture, RenderPassCatchesThrowingRender) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_throw_render", true, nullptr, nullptr,
        [&](Goose*, BehaviorContext&, IRenderer*) { throw std::runtime_error("render failure"); });
    Goose g(999, "ThrowRender", 1920, 1080);
    StubRenderer renderer;
    reg.RenderAll(&g, &renderer);
    SUCCEED();
}

TEST_F(ClearRegistryFixture, TickAllWithJailState) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_jail_behavior", true, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { });
    Goose g(999, "JailState", 1920, 1080);
    auto* jail = BehaviorStateManager::Instance().GetOrCreate<JailState>(g.id, "jail");
    ASSERT_NE(jail, nullptr);
    jail->isJailed = true;
    reg.TickAll(&g, 1.0/60.0, 100.0);
    SUCCEED();
    jail->isJailed = false;
}

TEST_F(ClearRegistryFixture, TickAllDisabledBehaviorDoesNothing) {
    auto& reg = BehaviorRegistry::Instance();
    int tickCount = 0;
    RegisterTestBehavior("test_never_enabled", false, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { tickCount++; });
    Goose g(999, "NeverEnabled", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    EXPECT_EQ(tickCount, 0);
}

TEST_F(ClearRegistryFixture, CleanupAllRemovesState) {
    auto& reg = BehaviorRegistry::Instance();
    RegisterTestBehavior("test_cleanup_removes_state", true, nullptr,
        [&](Goose*, BehaviorContext&, double, double) { });
    Goose g(999, "CleanupRemoves", 1920, 1080);
    reg.TickAll(&g, 1.0/60.0, 100.0);
    auto* state = BehaviorStateManager::Instance().Get<BehaviorState>(g.id, "test_cleanup_removes_state");
    EXPECT_NE(state, nullptr);
    reg.CleanupAll(&g);
    state = BehaviorStateManager::Instance().Get<BehaviorState>(g.id, "test_cleanup_removes_state");
    EXPECT_EQ(state, nullptr);
}

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

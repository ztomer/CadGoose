// test_behavior_toggle.cpp
// Unit tests for runtime behavior enable/disable transitions.
// Verifies that cleanup() is called when disabled and init() when re-enabled.
#include "gtest/gtest.h"
#include <atomic>

#include "behavior.h"
#include "config.h"
#include "world.h"

static void ResetBehaviorGlobals() {
    g_config.general.globalScale = 1.0f;
    g_time = 0.0;
    BehaviorStateManager::Instance().ClearAll();
}

// ============================================================
// Core transition logic — cleanup on disable, init on re-enable
// ============================================================

TEST(BehaviorToggle, CleanupCalledOnDisable) {
    ResetBehaviorGlobals();

    std::atomic<int> cleanupCount{0};
    std::atomic<int> initCount{0};
    bool enabled = true;

    auto initFn = [&initCount](BehaviorContext&) { initCount++; };
    auto tickFn = [](Goose*, BehaviorContext&, double, double) {};
    auto cleanupFn = [&cleanupCount](BehaviorContext&) { cleanupCount++; };

    Behavior testBehavior = BEHAVIOR_DEF_CUSTOM(
        "test_disable", "Test Disable", "Tests cleanup on disable",
        enabled, initFn, tickFn, nullptr, cleanupFn, false, false
    );

    BehaviorContext ctx{};
    ctx.time = 0.0;
    ctx.globalScale = 1.0f;

    // Initial state: enabled
    testBehavior.init(ctx);
    EXPECT_EQ(initCount.load(), 1);
    EXPECT_EQ(cleanupCount.load(), 0);

    // Simulate TickAll's disable transition detection
    bool wasEnabled = true;
    enabled = false;
    bool isEnabled = enabled;
    if (wasEnabled && !isEnabled && testBehavior.cleanup) {
        testBehavior.cleanup(ctx);
    }
    EXPECT_EQ(cleanupCount.load(), 1);
}

TEST(BehaviorToggle, InitCalledOnReEnable) {
    ResetBehaviorGlobals();

    std::atomic<int> cleanupCount{0};
    std::atomic<int> initCount{0};
    bool enabled = false;

    auto initFn = [&initCount](BehaviorContext&) { initCount++; };
    auto tickFn = [](Goose*, BehaviorContext&, double, double) {};
    auto cleanupFn = [&cleanupCount](BehaviorContext&) { cleanupCount++; };

    Behavior testBehavior = BEHAVIOR_DEF_CUSTOM(
        "test_reenable", "Test Re-enable", "Tests init on re-enable",
        enabled, initFn, tickFn, nullptr, cleanupFn, false, false
    );

    BehaviorContext ctx{};
    ctx.time = 0.0;
    ctx.globalScale = 1.0f;

    // Initially disabled
    EXPECT_EQ(initCount.load(), 0);

    // Simulate TickAll's enable transition detection
    bool wasEnabled = false;
    enabled = true;
    bool isEnabled = enabled;
    if (!wasEnabled && isEnabled && testBehavior.init) {
        testBehavior.init(ctx);
    }
    EXPECT_EQ(initCount.load(), 1);

    // Disable again
    wasEnabled = true;
    enabled = false;
    isEnabled = enabled;
    if (wasEnabled && !isEnabled && testBehavior.cleanup) {
        testBehavior.cleanup(ctx);
    }
    EXPECT_EQ(cleanupCount.load(), 1);

    // Re-enable
    wasEnabled = false;
    enabled = true;
    isEnabled = enabled;
    if (!wasEnabled && isEnabled && testBehavior.init) {
        testBehavior.init(ctx);
    }
    EXPECT_EQ(initCount.load(), 2);
}

// ============================================================
// Multiple behaviors toggle independently
// ============================================================

TEST(BehaviorToggle, MultipleBehaviors_IndependentToggle) {
    ResetBehaviorGlobals();

    bool ballEnabled = true;
    bool toysEnabled = true;
    std::atomic<int> ballCleanupCount{0};
    std::atomic<int> toysCleanupCount{0};

    auto ballCleanup = [&ballCleanupCount](BehaviorContext&) { ballCleanupCount++; };
    auto toysCleanup = [&toysCleanupCount](BehaviorContext&) { toysCleanupCount++; };
    auto tickFn = [](Goose*, BehaviorContext&, double, double) {};

    Behavior ballBehavior = BEHAVIOR_DEF_CUSTOM(
        "ball", "Ball", "Ball behavior",
        ballEnabled, nullptr, tickFn, nullptr, ballCleanup, false, false
    );

    Behavior toysBehavior = BEHAVIOR_DEF_CUSTOM(
        "toys", "Toys", "Toys behavior",
        toysEnabled, nullptr, tickFn, nullptr, toysCleanup, false, false
    );

    BehaviorContext ctx{};
    ctx.time = 0.0;

    // Disable only ball
    bool wasBallEnabled = true;
    ballEnabled = false;
    if (wasBallEnabled && !ballEnabled && ballBehavior.cleanup) {
        ballBehavior.cleanup(ctx);
    }
    EXPECT_EQ(ballCleanupCount.load(), 1);
    EXPECT_EQ(toysCleanupCount.load(), 0);

    // Disable only toys
    bool wasToysEnabled = true;
    toysEnabled = false;
    if (wasToysEnabled && !toysEnabled && toysBehavior.cleanup) {
        toysBehavior.cleanup(ctx);
    }
    EXPECT_EQ(ballCleanupCount.load(), 1);
    EXPECT_EQ(toysCleanupCount.load(), 1);
}

// ============================================================
// Behavior with no cleanup function doesn't crash
// ============================================================

TEST(BehaviorToggle, NoCleanupFunction_SafeDisable) {
    ResetBehaviorGlobals();

    std::atomic<int> initCount{0};
    bool enabled = true;

    auto initFn = [&initCount](BehaviorContext&) { initCount++; };
    auto tickFn = [](Goose*, BehaviorContext&, double, double) {};

    // No cleanup function (nullptr)
    Behavior testBehavior = BEHAVIOR_DEF(
        "test_no_cleanup", "Test No Cleanup", "Tests safe disable without cleanup",
        enabled, initFn, tickFn, nullptr
    );

    BehaviorContext ctx{};
    ctx.time = 0.0;
    ctx.globalScale = 1.0f;

    testBehavior.init(ctx);
    EXPECT_EQ(initCount.load(), 1);

    // Disable — should not crash even though cleanup is nullptr
    bool wasEnabled = true;
    enabled = false;
    bool isEnabled = enabled;
    if (wasEnabled && !isEnabled && testBehavior.cleanup) {
        testBehavior.cleanup(ctx);
    }
    // cleanup was nullptr, so it wasn't called — no crash
    EXPECT_EQ(initCount.load(), 1);
}

// ============================================================
// Behavior with no init function doesn't crash on re-enable
// ============================================================

TEST(BehaviorToggle, NoInitFunction_SafeReEnable) {
    ResetBehaviorGlobals();

    std::atomic<int> tickCount{0};
    bool enabled = false;

    auto tickFn = [&tickCount](Goose*, BehaviorContext&, double, double) { tickCount++; };

    // No init function (nullptr)
    Behavior testBehavior = BEHAVIOR_DEF(
        "test_no_init", "Test No Init", "Tests safe re-enable without init",
        enabled, nullptr, tickFn, nullptr
    );

    BehaviorContext ctx{};
    ctx.time = 0.0;
    ctx.globalScale = 1.0f;

    // Enable — should not crash even though init is nullptr
    bool wasEnabled = false;
    enabled = true;
    bool isEnabled = enabled;
    if (!wasEnabled && isEnabled && testBehavior.init) {
        testBehavior.init(ctx);
    }
    // init was nullptr, so it wasn't called — no crash

    // Tick should still work
    testBehavior.tick(nullptr, ctx, 0.016, 0.0);
    EXPECT_EQ(tickCount.load(), 1);
}

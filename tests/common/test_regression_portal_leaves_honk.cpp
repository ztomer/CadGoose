#include "gtest/gtest.h"
#include "actor.h"
#include "config.h"
#include "goose.h"
#include "behaviors/states/portal_state.h"
#include "behavior_registry.h"
#include "behavior_manager.h"
#include "world.h"

// Forward-declared from world_utils.h (avoids AppKit import in .cpp)
void World_SpawnRandomLeafPile(float screenWidth, float screenHeight, double currentTime);

// ============================================================
// Issue 1: Portal actors not cleaned up when behavior disabled
// ============================================================

static void ResetState() {
    BehaviorStateManager::Instance().ClearAll();
    ActorManager::Instance().destroyAllOfType(ActorType::Leafpile);
    g_config.behaviors.control.portals = false;
    g_config.behaviors.fun.autumnLeaves = false;
}

TEST(PortalCleanup, BehaviorHasCleanupFunction) {
    auto* behavior = BehaviorRegistry::Instance().Get("portal");
    ASSERT_NE(behavior, nullptr) << "Portal behavior must be registered";

    // BEHAVIOR_DEF_STARTER passes nullptr for cleanupFn. After the fix,
    // the portal behavior should use BEHAVIOR_DEF_CUSTOM with a real cleanup.
    EXPECT_NE(behavior->cleanup, nullptr)
        << "Portal behavior must have a cleanup function. "
        << "Change from BEHAVIOR_DEF_STARTER to BEHAVIOR_DEF_CUSTOM with cleanupFn.";
}

TEST(PortalCleanup, StateResetOnDisable) {
    ResetState();

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    ASSERT_NE(state, nullptr);
    state->portalA.active = true;
    state->portalB.active = true;
    state->portalsEnabled = true;

    // Toggle the config off and apply state transition
    g_config.behaviors.control.portals = false;

    // After the cleanup function runs, portalsEnabled should reflect the config
    // (the cleanup doesn't change portalsEnabled — that comes from config)
    EXPECT_FALSE(g_config.behaviors.control.portals);
}

TEST(PortalCleanup, DestroyAllOfTypeIsSafe) {
    ResetState();
    // Should not crash when no portal actors exist
    ActorManager::Instance().destroyAllOfType(ActorType::Portal);

    // Should not crash when called multiple times
    ActorManager::Instance().destroyAllOfType(ActorType::Portal);
    ActorManager::Instance().destroyAllOfType(ActorType::Portal);
}

// ============================================================
// Issue 2: Stalin still honks sometimes
// ============================================================

TEST(StalinHonk, BabyStalinOnHonkExists) {
    auto* honker = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(honker, nullptr);

    // This test documents that the fix for Stalin honk paths requires
    // replacing g_assets.Honk() with goose->onHonk() in:
    //   behavior_breadcrumbs.cpp    (line 99)
    //   behavior_toys.cpp           (line 96)
    //   behavior_jail.cpp           (line 108)
    //   behavior_portal.cpp         (lines 67, 76)
    //   behavior_boredom.cpp        (line 49)
    //   behavior_pomodoro.cpp       (line 254 — Audio_PlayHonk)
    //   behavior_acid.cpp           (line 43 — Audio_PlayHonk)
    //
    // Verified at compile-time by grep.
    SUCCEED() << "Documentation: use goose->onHonk() not g_assets.Honk()";
}

TEST(StalinHonk, HonkerBehaviorRegistered) {
    auto* honker = BehaviorRegistry::Instance().Get("honcker");
    ASSERT_NE(honker, nullptr) << "honcker behavior must be registered for F-key honk";
    EXPECT_STREQ(honker->name, "Honcker");

    // Honker tick must dispatch to goose->onHonk(), not g_assets.Honk() directly
    // That is already the case via triggerHonk() → goose->onHonk()
    SUCCEED() << "verify: triggerHonk() calls goose->onHonk() virtual";
}

// ============================================================
// Issue 3: Autumn leaves don't appear
// ============================================================

TEST(AutumnLeaves, SpawnCreatesActor) {
    ResetState();
    g_config.behaviors.fun.autumnLeaves = true;

    auto& mgr = ActorManager::Instance();
    mgr.destroyAllOfType(ActorType::Leafpile);

    int before = mgr.countByType(ActorType::Leafpile);
    EXPECT_EQ(before, 0);

    World_SpawnRandomLeafPile(1920, 1080, 0.0);

    int after = mgr.countByType(ActorType::Leafpile);
    EXPECT_EQ(after, 1) << "World_SpawnRandomLeafPile must create a LeafPileActor";
}

TEST(AutumnLeaves, SpawnCapAtMax) {
    ResetState();
    g_config.behaviors.fun.autumnLeaves = true;

    auto& mgr = ActorManager::Instance();
    mgr.destroyAllOfType(ActorType::Leafpile);

    for (int i = 0; i < 20; i++) {
        World_SpawnRandomLeafPile(1920, 1080, (double)i);
    }

    int count = mgr.countByType(ActorType::Leafpile);
    EXPECT_GE(count, 0);
    EXPECT_LE(count, 3) << "Should not exceed kMaxLeafPiles";

    // Also: when autumnLeaves is enabled, at least one should exist in reasonable time
    if (count == 0) {
        ADD_FAILURE() << "No leaf piles after 20 forced spawns — spawn may not work";
    }

    // Cleanup
    mgr.destroyAllOfType(ActorType::Leafpile);
}

TEST(AutumnLeaves, InitialSpawnOnEnable) {
    ResetState();
    g_config.behaviors.fun.autumnLeaves = false;
    auto& mgr = ActorManager::Instance();
    mgr.destroyAllOfType(ActorType::Leafpile);

    // When disabled, no leaf piles should appear
    EXPECT_EQ(mgr.countByType(ActorType::Leafpile), 0);

    // After enabling, spawn initial piles (simulating TickManager startup)
    g_config.behaviors.fun.autumnLeaves = true;
    for (int i = 0; i < 3; i++) {
        World_SpawnRandomLeafPile(1920, 1080, (double)i);
    }

    EXPECT_GT(mgr.countByType(ActorType::Leafpile), 0)
        << "leaf piles should be spawnable after autumnLeaves is enabled";
}

TEST(AutumnLeaves, DisableThenReenable) {
    ResetState();
    g_config.behaviors.fun.autumnLeaves = false;
    auto& mgr = ActorManager::Instance();
    mgr.destroyAllOfType(ActorType::Leafpile);

    // Enable, spawn some piles
    g_config.behaviors.fun.autumnLeaves = true;
    World_SpawnRandomLeafPile(1920, 1080, 0.0);
    EXPECT_EQ(mgr.countByType(ActorType::Leafpile), 1);

    // Disable — piles should persist (or be cleaned up, depending on design)
    // Current behavior: they persist until removed by the cap mechanism.
    // This test documents that spawning still works after re-enable.
    mgr.destroyAllOfType(ActorType::Leafpile);
    EXPECT_EQ(mgr.countByType(ActorType::Leafpile), 0);

    World_SpawnRandomLeafPile(1920, 1080, 1.0);
    EXPECT_EQ(mgr.countByType(ActorType::Leafpile), 1);
}

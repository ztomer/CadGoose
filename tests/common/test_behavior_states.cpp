// ===========================
// test_behavior_states.cpp
// Verifies all behavior states can be created
// ===========================
#include "behaviors/states/all.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"
#include "behaviors/states/drag_state.h"
#include "behaviors/states/jail_state.h"
#include "behaviors/states/portal_state.h"
#include "behaviors/states/rainbow_state.h"
#include "behaviors/states/health_state.h"
#include "behaviors/states/pomodoro_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/honcker_state.h"
#include "behaviors/states/acid_state.h"

TEST(BehaviorStates, AllBehaviorStatesExist) {
    Goose testGoose(0, "Test", 1920, 1080);
    auto& mgr = BehaviorStateManager::Instance();

    std::vector<std::pair<const char*, const char*>> stateDefs = {
        {"honcker", "honcker"}, {"drag", "drag"}, {"jail", "jail"},
        {"portal", "portal"}, {"anger", "anger"}, {"ball", "ball"},
        {"breadcrumbs", "breadcrumbs"}, {"health", "health"},
        {"acid", "acid"}, {"rainbow", "rainbow"},
        {"pomodoro", "pomodoro"},
        {"presence", "presence"}
    };

    for (const auto& [stateType, behaviorId] : stateDefs) {
        auto* state = mgr.Get<BehaviorState>(testGoose.id, behaviorId);
        if (!state) {
            mgr.GetOrCreate<BehaviorState>(testGoose.id, behaviorId);
            state = mgr.Get<BehaviorState>(testGoose.id, behaviorId);
        }
        EXPECT_NE(state, nullptr) << "State type '" << stateType << "' not found after create";
    }
}



TEST(BehaviorStates, DragStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    auto* drag = registry.Get("drag");
    ASSERT_NE(drag, nullptr) << "drag behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<DragState>(0, "drag");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isDragging);
}

TEST(BehaviorStates, JailStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("jail"), nullptr) << "jail behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<JailState>(0, "jail");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isJailed);
}

TEST(BehaviorStates, PortalStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("portal"), nullptr) << "portal behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<PortalState>(0, "portal");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->portalA.active);
    EXPECT_FALSE(state->portalB.active);
}

TEST(BehaviorStates, RainbowStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("rainbow"), nullptr) << "rainbow behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<RainbowState>(0, "rainbow");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->hue, 0.0f);
}

TEST(BehaviorStates, HealthStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("health"), nullptr) << "health behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<HealthState>(0, "health");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isDead);
}

TEST(BehaviorStates, PomodoroStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("pomodoro"), nullptr) << "pomodoro behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<PomodoroState>(0, "pomodoro");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->phase, PomodoroPhase::Work);
}

TEST(BehaviorStates, AngerStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("anger"), nullptr) << "anger behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<AngerState>(0, "anger");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->angerLevel, 0.0f);
}

TEST(BehaviorStates, BallStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("ball"), nullptr) << "ball behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<BallState>(0, "ball");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ(state->balls.size(), 0);
}

TEST(BehaviorStates, HonckerStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("honcker"), nullptr) << "honcker behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<HonckerState>(0, "honcker");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isOnCooldown);
}

TEST(BehaviorStates, AcidStateCreate) {
    auto& registry = BehaviorRegistry::Instance();
    ASSERT_NE(registry.Get("acid"), nullptr) << "acid behavior should be registered";

    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<AcidState>(0, "acid");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->isSpinning);
}

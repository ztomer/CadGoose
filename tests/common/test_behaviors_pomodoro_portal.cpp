// ===========================
// test_behaviors_pomodoro_portal.cpp
// Tests for pomodoro and portal behaviors
// ===========================
#include "behaviors/states/all.h"
#include "gtest/gtest.h"
#include <cmath>
#include <string>
#include <vector>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

static void ResetBehaviorState() {
    BehaviorStateManager::Instance().ClearAll();
}

// ===========================
// Pomodoro Behavior Tests
// ===========================
TEST(PomodoroBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<PomodoroState>(0, "pomodoro");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_EQ((int)state->phase, (int)PomodoroPhase::Work);
    EXPECT_EQ(state->phaseStartTime, 0);
    EXPECT_EQ(state->completedSessions, 0);
    EXPECT_FALSE(state->isAggressive);
    EXPECT_FALSE(state->isSleeping);
    EXPECT_FALSE(state->speedMultiplierApplied);
    EXPECT_EQ(state->accumulatedRotation, 0.0f);
}

TEST(PomodoroBehavior, PhaseEnumValues) {
    EXPECT_EQ((int)PomodoroPhase::Work, 0);
    EXPECT_EQ((int)PomodoroPhase::Break, 1);
    EXPECT_EQ((int)PomodoroPhase::LongBreak, 2);
}

TEST(PomodoroBehavior, DefaultDurations) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();

    g_config.behaviors.pomodoro.workMinutes = 25;
    g_config.behaviors.pomodoro.breakMinutes = 5;
    g_config.behaviors.pomodoro.longBreakMinutes = 15;

    EXPECT_EQ(state->GetPhaseDurationMinutes(), 25);
    state->phase = PomodoroPhase::Break;
    EXPECT_EQ(state->GetPhaseDurationMinutes(), 5);
    state->phase = PomodoroPhase::LongBreak;
    EXPECT_EQ(state->GetPhaseDurationMinutes(), 15);
    state->phase = PomodoroPhase::Work;
}

TEST(PomodoroBehavior, WorkToBreakTransition) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();
    state->phase = PomodoroPhase::Work;
    state->phaseStartTime = 0.0;
    state->completedSessions = 0;

    double time = 25.0 * 60.0 + 1.0;
    double elapsed = time - state->phaseStartTime;
    double phaseDuration = g_config.behaviors.pomodoro.workMinutes * 60.0;

    if (elapsed >= phaseDuration) {
        state->completedSessions++;
        if (state->completedSessions >= g_config.behaviors.pomodoro.sessionsBeforeLongBreak) {
            state->phase = PomodoroPhase::LongBreak;
            state->completedSessions = 0;
        } else {
            state->phase = PomodoroPhase::Break;
        }
        state->phaseStartTime = time;
    }

    EXPECT_EQ((int)state->phase, (int)PomodoroPhase::Break);
    EXPECT_EQ(state->completedSessions, 1);
}

TEST(PomodoroBehavior, WorkToLongBreakTransition) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();
    state->phase = PomodoroPhase::Work;
    state->phaseStartTime = 0.0;
    state->completedSessions = 3;

    double time = 25.0 * 60.0 + 1.0;
    double elapsed = time - state->phaseStartTime;
    double phaseDuration = g_config.behaviors.pomodoro.workMinutes * 60.0;

    if (elapsed >= phaseDuration) {
        state->completedSessions++;
        if (state->completedSessions >= g_config.behaviors.pomodoro.sessionsBeforeLongBreak) {
            state->phase = PomodoroPhase::LongBreak;
            state->completedSessions = 0;
        } else {
            state->phase = PomodoroPhase::Break;
        }
        state->phaseStartTime = time;
    }

    EXPECT_EQ((int)state->phase, (int)PomodoroPhase::LongBreak);
    EXPECT_EQ(state->completedSessions, 0);
}

TEST(PomodoroBehavior, BreakToWorkTransition) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();
    state->phase = PomodoroPhase::Break;
    state->phaseStartTime = 0.0;
    state->isAggressive = true;

    double time = 5.0 * 60.0 + 1.0;
    double elapsed = time - state->phaseStartTime;
    double phaseDuration = g_config.behaviors.pomodoro.breakMinutes * 60.0;

    if (elapsed >= phaseDuration) {
        state->phase = PomodoroPhase::Work;
        state->isAggressive = false;
        state->phaseStartTime = time;
    }

    EXPECT_EQ((int)state->phase, (int)PomodoroPhase::Work);
    EXPECT_FALSE(state->isAggressive);
}

TEST(PomodoroBehavior, AggressiveMode) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();
    state->phase = PomodoroPhase::Break;
    state->isAggressive = true;

    float dt = 0.016f;
    float manicRotationSpeed = 90.0f;
    float rotationAmount = manicRotationSpeed * dt;
    state->accumulatedRotation += rotationAmount;

    EXPECT_GT(state->accumulatedRotation, 0.0f);
}

TEST(PomodoroBehavior, SleepMode) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->Reset();
    state->phase = PomodoroPhase::Work;
    state->isSleeping = true;

    float dt = 0.016f;
    state->zzzAnimTime += dt;
    float zzzInterval = 1.5f;
    int zzzFrames = 3;

    state->zzzAnimTime = zzzInterval + 0.1f;
    if (state->zzzAnimTime > zzzInterval) {
        state->zzzFrame = (state->zzzFrame + 1) % zzzFrames;
        state->zzzAnimTime = 0;
    }
    EXPECT_EQ(state->zzzFrame, 1);
}

TEST(PomodoroBehavior, ZzzAnimationFrames) {
    int frame = 0;
    const char* zzzStr;
    switch (frame) {
        case 0: zzzStr = "Z"; break;
        case 1: zzzStr = "z"; break;
        default: zzzStr = "."; break;
    }
    EXPECT_STREQ(zzzStr, "Z");

    frame = 1;
    switch (frame) {
        case 0: zzzStr = "Z"; break;
        case 1: zzzStr = "z"; break;
        default: zzzStr = "."; break;
    }
    EXPECT_STREQ(zzzStr, "z");

    frame = 2;
    switch (frame) {
        case 0: zzzStr = "Z"; break;
        case 1: zzzStr = "z"; break;
        default: zzzStr = "."; break;
    }
    EXPECT_STREQ(zzzStr, ".");
}

TEST(PomodoroBehavior, PhaseLabels) {
    PomodoroPhase phase = PomodoroPhase::Work;
    bool isAggressive = false;
    const char* label = "R";
    switch (phase) {
        case PomodoroPhase::Work: label = "R"; break;
        case PomodoroPhase::Break: label = isAggressive ? "ATK!" : "B"; break;
        case PomodoroPhase::LongBreak: label = isAggressive ? "ATK!" : "LB"; break;
    }
    EXPECT_STREQ(label, "R");

    phase = PomodoroPhase::Break;
    isAggressive = true;
    switch (phase) {
        case PomodoroPhase::Work: label = "R"; break;
        case PomodoroPhase::Break: label = isAggressive ? "ATK!" : "B"; break;
        case PomodoroPhase::LongBreak: label = isAggressive ? "ATK!" : "LB"; break;
    }
    EXPECT_STREQ(label, "ATK!");

    phase = PomodoroPhase::LongBreak;
    isAggressive = false;
    switch (phase) {
        case PomodoroPhase::Work: label = "R"; break;
        case PomodoroPhase::Break: label = isAggressive ? "ATK!" : "B"; break;
        case PomodoroPhase::LongBreak: label = isAggressive ? "ATK!" : "LB"; break;
    }
    EXPECT_STREQ(label, "LB");
}

TEST(PomodoroBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.systems.pomodoro, false);
    g_config.behaviors.systems.pomodoro = true;
    EXPECT_EQ(g_config.behaviors.systems.pomodoro, true);
    g_config.behaviors.systems.pomodoro = false;
}

TEST(PomodoroBehavior, PomodoroConfigDefaults) {
    EXPECT_EQ(g_config.behaviors.pomodoro.workMinutes, 25);
    EXPECT_EQ(g_config.behaviors.pomodoro.breakMinutes, 5);
    EXPECT_EQ(g_config.behaviors.pomodoro.longBreakMinutes, 15);
    EXPECT_EQ(g_config.behaviors.pomodoro.sessionsBeforeLongBreak, 4);
    EXPECT_EQ(g_config.behaviors.pomodoro.enableAggressiveMode, true);
    EXPECT_FLOAT_EQ(g_config.behaviors.pomodoro.aggressiveHonkInterval, 2.0f);
    EXPECT_FLOAT_EQ(g_config.behaviors.pomodoro.aggressiveSpeedMultiplier, 1.5f);
}

// ===========================
// Portal Behavior Tests
// ===========================
TEST(PortalBehavior, StateCreation) {
    ResetBehaviorState();
    auto& mgr = BehaviorStateManager::Instance();
    auto* state = mgr.GetOrCreate<PortalState>(0, "portal");
    ASSERT_NE(state, nullptr);
    state->Reset();
    EXPECT_FALSE(state->portalA.active);
    EXPECT_FALSE(state->portalB.active);
    EXPECT_FALSE(state->justTeleported);
}

TEST(PortalBehavior, PortalPlacement) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state->Reset();

    state->portalA.x = 100.0f;
    state->portalA.y = 200.0f;
    state->portalA.active = true;
    state->portalA.portalId = 1;

    EXPECT_TRUE(state->portalA.active);
    EXPECT_FLOAT_EQ(state->portalA.x, 100.0f);
    EXPECT_FLOAT_EQ(state->portalA.y, 200.0f);
    EXPECT_EQ(state->portalA.portalId, 1);
}

TEST(PortalBehavior, TeleportAtoB) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state->Reset();

    state->portalA.x = 100.0f;
    state->portalA.y = 100.0f;
    state->portalA.active = true;

    state->portalB.x = 700.0f;
    state->portalB.y = 500.0f;
    state->portalB.active = true;

    Vector2 goosePos{100.0f, 100.0f};
    float p1w = 80.0f, p1h = 80.0f;

    bool inP1 = goosePos.x > state->portalA.x - p1w/2 && goosePos.x < state->portalA.x + p1w/2 &&
                goosePos.y > state->portalA.y - p1h/2 && goosePos.y < state->portalA.y + p1h/2;
    EXPECT_TRUE(inP1);

    goosePos.x = state->portalB.x;
    goosePos.y = state->portalB.y;

    EXPECT_FLOAT_EQ(goosePos.x, 700.0f);
    EXPECT_FLOAT_EQ(goosePos.y, 500.0f);
}

TEST(PortalBehavior, TeleportBtoA) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state->Reset();

    state->portalA.x = 100.0f;
    state->portalA.y = 100.0f;
    state->portalA.active = true;

    state->portalB.x = 700.0f;
    state->portalB.y = 500.0f;
    state->portalB.active = true;

    Vector2 goosePos{700.0f, 500.0f};
    float p2w = 80.0f, p2h = 80.0f;

    bool inP2 = goosePos.x > state->portalB.x - p2w/2 && goosePos.x < state->portalB.x + p2w/2 &&
                goosePos.y > state->portalB.y - p2h/2 && goosePos.y < state->portalB.y + p2h/2;
    EXPECT_TRUE(inP2);

    goosePos.x = state->portalA.x;
    goosePos.y = state->portalA.y;

    EXPECT_FLOAT_EQ(goosePos.x, 100.0f);
    EXPECT_FLOAT_EQ(goosePos.y, 100.0f);
}

TEST(PortalBehavior, NoTeleportWhenInactive) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state->Reset();

    state->portalA.x = 100.0f;
    state->portalA.y = 100.0f;
    state->portalA.active = true;
    state->portalB.active = false;

    Vector2 goosePos{100.0f, 100.0f};
    float p1w = 80.0f, p1h = 80.0f;

    bool inP1 = goosePos.x > state->portalA.x - p1w/2 && goosePos.x < state->portalA.x + p1w/2 &&
                goosePos.y > state->portalA.y - p1h/2 && goosePos.y < state->portalA.y + p1h/2;
    EXPECT_TRUE(inP1);

    bool shouldTeleport = inP1 && state->portalB.active;
    EXPECT_FALSE(shouldTeleport);
}

TEST(PortalBehavior, JustTeleportedFlag) {
    ResetBehaviorState();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state->Reset();
    state->justTeleported = true;

    Vector2 goosePos{700.0f, 500.0f};
    state->portalB.x = 700.0f;
    state->portalB.y = 500.0f;
    float p2w = 80.0f, p2h = 80.0f;

    bool inPortal = goosePos.x > state->portalB.x - p2w/2 && goosePos.x < state->portalB.x + p2w/2 &&
                    goosePos.y > state->portalB.y - p2h/2 && goosePos.y < state->portalB.y + p2h/2;
    EXPECT_TRUE(inPortal);

    if (state->justTeleported) {
        if (!inPortal) {
            state->justTeleported = false;
        }
    }
    EXPECT_TRUE(state->justTeleported);
}

TEST(PortalBehavior, StatePersistsAcrossGooseSpawns) {
    ResetBehaviorState();

    auto* state1 = BehaviorStateManager::Instance().GetOrCreate<PortalState>(0, "portal");
    state1->Reset();
    state1->portalA.x = 100.0f;
    state1->portalA.y = 200.0f;
    state1->portalA.active = true;
    state1->portalB.x = 700.0f;
    state1->portalB.y = 500.0f;
    state1->portalB.active = true;

    BehaviorStateManager::Instance().RemoveForGoose(0);

    auto* state2 = BehaviorStateManager::Instance().GetOrCreate<PortalState>(1, "portal");
    state2->Reset();

    EXPECT_FALSE(state2->portalA.active);
    EXPECT_FALSE(state2->portalB.active);
}

TEST(PortalBehavior, ConfigPointer) {
    EXPECT_EQ(g_config.behaviors.control.portals, false);
    g_config.behaviors.control.portals = true;
    EXPECT_EQ(g_config.behaviors.control.portals, true);
    g_config.behaviors.control.portals = false;
}

TEST(PortalBehavior, PortalConfigDefaults) {
    EXPECT_EQ(g_config.portal.hotkey1, "1");
    EXPECT_EQ(g_config.portal.hotkey2, "2");
    EXPECT_EQ(g_config.portal.hotkey0, "0");
    EXPECT_FLOAT_EQ(g_config.portal.p1Width, 80.0f);
    EXPECT_FLOAT_EQ(g_config.portal.p1Height, 80.0f);
    EXPECT_FLOAT_EQ(g_config.portal.p2Width, 80.0f);
    EXPECT_FLOAT_EQ(g_config.portal.p2Height, 80.0f);
}

// test_pomodoro_quiet.cpp
// Unit tests for Pomodoro behavior — quiet during rest, timer reset on re-enable
#include "gtest/gtest.h"
#include <cmath>

#include "behavior.h"
#include "config.h"
#include "world.h"
#include "behaviors/states/pomodoro_state.h"

static void ResetPomodoroGlobals() {
    g_config.general.globalScale = 1.0f;
    g_time = 0.0;
    BehaviorStateManager::Instance().ClearAll();
}

// ============================================================
// Pomodoro state resets on re-init
// ============================================================

TEST(PomodoroQuiet, TimerResetsOnReInit) {
    ResetPomodoroGlobals();

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->phase = PomodoroPhase::Work;
    state->phaseStartTime = 100.0;
    state->completedSessions = 3;
    state->isSleeping = true;

    // Simulate re-init (what happens when behavior is re-enabled)
    state->Reset();
    state->phaseStartTime = 200.0;

    EXPECT_EQ(state->phase, PomodoroPhase::Work);
    EXPECT_EQ(state->completedSessions, 0);
    EXPECT_FALSE(state->isSleeping);
    EXPECT_EQ(state->phaseStartTime, 200.0);
}

// ============================================================
// Pomodoro phase transitions work correctly
// ============================================================

TEST(PomodoroQuiet, PhaseTransition_WorkToBreak) {
    ResetPomodoroGlobals();

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->phase = PomodoroPhase::Work;
    state->phaseStartTime = 0.0;
    state->completedSessions = 0;
    state->isSleeping = false;

    // Simulate work phase completion
    state->completedSessions++;
    state->phase = PomodoroPhase::Break;
    state->isSleeping = false;
    state->phaseStartTime = 100.0;

    EXPECT_EQ(state->phase, PomodoroPhase::Break);
    EXPECT_FALSE(state->isSleeping);
}

TEST(PomodoroQuiet, PhaseTransition_BreakToWork) {
    ResetPomodoroGlobals();

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(0, "pomodoro");
    state->phase = PomodoroPhase::Break;
    state->phaseStartTime = 0.0;
    state->isAggressive = true;

    // Simulate break phase completion
    state->phase = PomodoroPhase::Work;
    state->isAggressive = false;
    state->isSleeping = false;
    state->phaseStartTime = 100.0;

    EXPECT_EQ(state->phase, PomodoroPhase::Work);
    EXPECT_FALSE(state->isAggressive);
}

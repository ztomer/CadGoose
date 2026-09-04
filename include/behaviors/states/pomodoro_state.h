#ifndef POMODORO_STATE_H
#define POMODORO_STATE_H

#include "behavior_state.h"
#include "goose_math.h"

enum class PomodoroPhase : int {
    Work = 0,
    Break = 1,
    LongBreak = 2
};

struct PomodoroState : public BehaviorState {
    PomodoroPhase phase = PomodoroPhase::Work;
    double phaseStartTime = 0;
    int completedSessions = 0;
    double lastHonkTime = 0;
    bool isAggressive = false;
    float accumulatedRotation = 0.0f;
    bool speedMultiplierApplied = false;

    // Sleep mode fields
    bool isSleeping = false;
    Vector2 bedPosition{0, 0};
    double zzzAnimTime = 0;
    int zzzFrame = 0;
    double slowRotateDir = 1.0f;
    double slowRotateTimer = 0;

    void Reset() override {
        phase = PomodoroPhase::Work;
        phaseStartTime = 0;
        completedSessions = 0;
        lastHonkTime = 0;
        isAggressive = false;
        accumulatedRotation = 0.0f;
        speedMultiplierApplied = false;
        isSleeping = false;
        bedPosition = {0, 0};
        zzzAnimTime = 0;
        zzzFrame = 0;
        slowRotateDir = 1.0f;
        slowRotateTimer = 0;
    }

    int GetPhaseDurationMinutes() const {
        switch (phase) {
            case PomodoroPhase::Work: return 25;
            case PomodoroPhase::Break: return 5;
            case PomodoroPhase::LongBreak: return 15;
        }
        return 25;
    }
};

#endif

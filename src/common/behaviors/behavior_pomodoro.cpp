// ===========================
// behavior_pomodoro.cpp
// Pomodoro Timer Behavior
// Work for 25 minutes, break for 5, long break after 4 sessions
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <cmath>

static bool s_enabled = true;

extern void Audio_PlayHonk();

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(ctx.goose->id, "pomodoro");
    state->Reset();
    state->phaseStartTime = ctx.time;
}

static double GetPhaseDuration(PomodoroPhase phase) {
    const auto& cfg = g_config.behaviors.pomodoro;
    switch (phase) {
        case PomodoroPhase::Work: return cfg.workMinutes * 60.0;
        case PomodoroPhase::Break: return cfg.breakMinutes * 60.0;
        case PomodoroPhase::LongBreak: return cfg.longBreakMinutes * 60.0;
    }
    return 25 * 60.0;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.pomodoro.enabled) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<PomodoroState>(goose->id, "pomodoro");
    const auto& cfg = g_config.behaviors.pomodoro;

    double elapsed = time - state->phaseStartTime;
    double phaseDuration = GetPhaseDuration(state->phase);

    if (elapsed >= phaseDuration) {
        if (state->phase == PomodoroPhase::Work) {
            state->completedSessions++;
            if (state->completedSessions >= cfg.sessionsBeforeLongBreak) {
                state->phase = PomodoroPhase::LongBreak;
                state->completedSessions = 0;
                state->isAggressive = false;
            } else {
                state->phase = PomodoroPhase::Break;
                state->isAggressive = false;
            }
        } else {
            state->phase = PomodoroPhase::Work;
            state->isAggressive = cfg.enableAggressiveMode;
        }
        state->phaseStartTime = time;
        elapsed = 0;
    }

    if (state->phase == PomodoroPhase::Work && state->isAggressive) {
        float rotationAmount = 90.0f * (float)dt;
        goose->dir += rotationAmount;
        state->accumulatedRotation += rotationAmount;

        if (time - state->lastHonkTime >= cfg.aggressiveHonkInterval) {
            Audio_PlayHonk();
            state->lastHonkTime = time;
        }

        if (!state->speedMultiplierApplied) {
            goose->currentSpeed = g_config.movement.baseRunSpeed * cfg.aggressiveSpeedMultiplier;
            state->speedMultiplierApplied = true;
        }
    } else {
        if (state->accumulatedRotation > 0) {
            goose->dir -= state->accumulatedRotation;
            state->accumulatedRotation = 0;
        }
        if (state->speedMultiplierApplied) {
            goose->currentSpeed = g_config.movement.baseWalkSpeed;
            state->speedMultiplierApplied = false;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.pomodoro.enabled) return;

    auto* state = BehaviorStateManager::Instance().Get<PomodoroState>(goose->id, "pomodoro");
    if (!state) return;

    double elapsed = ctx.time - state->phaseStartTime;
    double remaining = GetPhaseDuration(state->phase) - elapsed;
    int minutes = (int)(remaining / 60.0);
    int seconds = (int)fmod(remaining, 60.0);

    const char* phaseLabel = "WORK";
    float r = 0.8f, g = 0.2f, b = 0.2f;

    switch (state->phase) {
        case PomodoroPhase::Work:
            phaseLabel = state->isAggressive ? "WORK - ATTACK!" : "WORK";
            r = 0.8f; g = 0.2f; b = 0.2f;
            break;
        case PomodoroPhase::Break:
            phaseLabel = "BREAK";
            r = 0.2f; g = 0.8f; b = 0.2f;
            break;
        case PomodoroPhase::LongBreak:
            phaseLabel = "LONG BREAK";
            r = 0.2f; g = 0.4f; b = 0.8f;
            break;
    }

    fprintf(stderr, "[POMODORO] %s - %02d:%02d (%d/4 sessions)\n",
            phaseLabel, minutes, seconds, state->completedSessions + 1);
}

static Behavior g_pomodoroBehavior = {
    .id = "pomodoro",
    .name = "Pomodoro",
    .description = "Work/rest timer: 25 min work, 5 min break, 15 min long break after 4 sessions",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 10,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_pomodoroBehavior);

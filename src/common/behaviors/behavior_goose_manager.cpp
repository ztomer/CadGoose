// ===========================
// behavior_goose_manager.cpp
// GooseManager Behavior - Control goose tasks and speeds
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<GooseManagerState>(ctx.goose->id, "gooseManager");
    state->tasksEnabled[0] = g_config.gooseManager.taskWander;
    state->tasksEnabled[1] = g_config.gooseManager.taskFetch;
    state->tasksEnabled[2] = g_config.gooseManager.taskChase;
    state->tasksEnabled[3] = g_config.gooseManager.taskSnatch;
    state->speedsEnabled[0] = g_config.gooseManager.speedWalk;
    state->speedsEnabled[1] = g_config.gooseManager.speedRun;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.gooseManager) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<GooseManagerState>(goose->id, "gooseManager");

    if (!g_config.gooseManager.taskWander && !g_config.gooseManager.taskFetch &&
        !g_config.gooseManager.taskChase && !g_config.gooseManager.taskSnatch) {
        for (int i = 0; i < 4; i++) {
            state->tasksEnabled[i] = true;
        }
    } else {
        state->tasksEnabled[0] = g_config.gooseManager.taskWander;
        state->tasksEnabled[1] = g_config.gooseManager.taskFetch;
        state->tasksEnabled[2] = g_config.gooseManager.taskChase;
        state->tasksEnabled[3] = g_config.gooseManager.taskSnatch;
    }

    state->speedsEnabled[0] = g_config.gooseManager.speedWalk;
    state->speedsEnabled[1] = g_config.gooseManager.speedRun;

    if (!state->tasksEnabled[0] && goose->state == GooseState::WANDER) {
        if (state->tasksEnabled[1]) goose->state = GooseState::FETCHING;
        else if (state->tasksEnabled[2]) goose->state = GooseState::CHASE_CURSOR;
        else if (state->tasksEnabled[3]) goose->state = GooseState::SNATCH_CURSOR;
    }

    if (!state->speedsEnabled[0] && goose->currentSpeed < 100.0f) {
        goose->currentSpeed = 0.0f;
    }
    if (!state->speedsEnabled[1] && goose->currentSpeed >= 100.0f) {
        goose->currentSpeed = 50.0f;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_gooseManagerBehavior = {
    .id = "gooseManager",
    .name = "GooseManager",
    .description = "Control which tasks and speeds the goose can use",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_gooseManagerBehavior);
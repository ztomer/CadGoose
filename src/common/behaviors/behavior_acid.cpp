// ===========================
// behavior_acid.cpp
// Acid Behavior - Spin with honks
// Based on AcidGoose by F!NN
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"

#ifdef __APPLE__
extern void Audio_PlayHonk();
#define PLAY_HONK() Audio_PlayHonk()
#else
#define PLAY_HONK() fprintf(stderr, "[ACID] Honk!\n")
#endif

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AcidState>(ctx.goose->id, "acid");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.acid) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AcidState>(goose->id, "acid");

    if (!state->isSpinning) {
        if (rand() % g_config.behaviors.acid.triggerChance == 0) {
            state->isSpinning = true;
            state->rotationAccumulator = 0.0f;
        }
        return;
    }

    float spinSpeed = g_config.behaviors.acid.spinSpeed;
    state->rotationAccumulator += spinSpeed * dt;
    goose->dir += spinSpeed * dt;
    if (goose->dir >= 360.0f) goose->dir -= 360.0f;

    if (time - state->lastHonkTime >= g_config.behaviors.acid.honkInterval) {
        PLAY_HONK();
        state->lastHonkTime = time;
    }

    if (state->rotationAccumulator >= g_config.behaviors.acid.rotationTotal) {
        state->isSpinning = false;
        state->rotationAccumulator = 0.0f;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_acidBehavior = {
    .id = "acid",
    .name = "Acid",
    .description = "Goose spins wildly with honks. Based on AcidGoose by F!NN",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.fun.acid,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_acidBehavior);
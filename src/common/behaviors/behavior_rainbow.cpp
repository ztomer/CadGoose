// ===========================
// behavior_rainbow.cpp
// Rainbow Behavior - Cycle through rainbow colors
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(ctx.goose->id, "rainbow");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.rainbow) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(goose->id, "rainbow");
    
    state->hue += g_config.behaviors.rainbow.hueSpeed * dt;
    if (state->hue >= 360.0f) {
        state->hue -= 360.0f;
    }
    state->lastUpdate = time;
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

float Rainbow_GetHue(int gooseId) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(gooseId, "rainbow");
    if (!g_config.behaviors.fun.rainbow) return 0.0f;
    return state->hue;
}

void Rainbow_SetHue(int gooseId, float hue) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(gooseId, "rainbow");
    state->hue = hue;
}

static Behavior g_rainbowBehavior = {
    .id = "rainbow",
    .name = "Rainbow",
    .description = "Goose cycles through rainbow colors",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_rainbowBehavior);
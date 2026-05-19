// ===========================
// behavior_rainbow.cpp
// Rainbow Behavior - Cycle through rainbow colors
// ===========================
#include "behavior.h"
#include "behaviors/states/rainbow_state.h"
#include "goose.h"
#include "config.h"


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(ctx.goose->id, "rainbow");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<RainbowState>(goose->id, "rainbow");
    
    state->hue += g_config.behaviors.rainbow.hueSpeed * dt;
    if (state->hue >= 360.0f) {
        state->hue -= 360.0f;
    }
    state->lastUpdate = time;
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
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

static Behavior g_rainbowBehavior = BEHAVIOR_DEF_STARTER(
    "rainbow", "Rainbow", "Goose cycles through rainbow colors",
    g_config.behaviors.fun.rainbow, init, tick, render
);

REGISTER_BEHAVIOR(g_rainbowBehavior);

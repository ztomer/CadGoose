// ===========================
// behavior_presence.cpp
// Presence Behavior - Menu bar status showing goose state
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.presence) return;
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.info.presence) return;
}

static Behavior g_presenceBehavior = {
    .id = "presence",
    .name = "Presence",
    .description = "Shows goose state in menu bar status",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_presenceBehavior);
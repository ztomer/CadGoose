// ===========================
// behavior_presence.cpp
// Presence Behavior - Menu bar status showing goose state
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

extern "C" void Presence_UpdateStatusFromBehavior(const char* status);

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.presence) return;
    
    // Only update presence every 0.5s to avoid spamming the UI thread
    static double lastUpdate = 0.0;
    if (time - lastUpdate < 0.5) return;
    lastUpdate = time;

    const char* status = "Wandering";
    switch (goose->state) {
        case GooseState::WANDER: status = "Wandering"; break;
        case GooseState::FETCHING: status = "Fetching"; break;
        case GooseState::RETURNING: status = "Returning"; break;
        case GooseState::CHASE_CURSOR: status = "Chasing Cursor"; break;
        case GooseState::SNATCH_CURSOR: status = "Snatching Cursor"; break;
    }
    
    char fullStatus[64];
    snprintf(fullStatus, sizeof(fullStatus), "Goose: %s", status);
    Presence_UpdateStatusFromBehavior(fullStatus);
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
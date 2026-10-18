// ===========================
// behavior_presence.cpp
// Presence Behavior - Menu bar status + goose window visibility
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

extern "C" void Presence_UpdateStatusFromBehavior(const char* status);
extern "C" void Presence_SetGooseWindowVisible(bool visible);

static bool s_lastVisible = true;
// The status throttle. It was a function-local static that init never reset,
// so a re-enabled presence (or the next test) inherited the last tick's time.
static double s_lastUpdate = 0.0;

static void ResetState() {
    s_lastVisible = true;
    s_lastUpdate = 0.0;
}

static void init(BehaviorContext& ctx) {
    (void)ctx;
    ResetState();
}

#ifdef CG_TEST_SEAMS
void Presence_ResetForTest() { ResetState(); }
#endif

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    (void)dt;
    if (time - s_lastUpdate < 0.5) return;
    s_lastUpdate = time;

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

    bool targetVisible = g_config.behaviors.info.visible;
    if (targetVisible != s_lastVisible) {
        s_lastVisible = targetVisible;
        Presence_SetGooseWindowVisible(targetVisible);
    }
}

static void render(Goose*, BehaviorContext&, IRenderer*) {
}

static Behavior g_presenceBehavior = BEHAVIOR_DEF(
    "presence", "Presence", "Shows goose state in menu bar and controls goose window visibility",
    g_config.behaviors.info.presence, init, tick, render
);

REGISTER_BEHAVIOR(g_presenceBehavior);
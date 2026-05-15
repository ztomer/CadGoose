#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "ai_text_meme.h"
#include <ctime>

static bool s_enabled = true;

static const char* s_defaultMessages[] = {
    "You're doing great!",
    "Drink some water!",
    "Take a break!",
    "You got this!",
    "Stay awesome!",
    "You are enough!",
    "Keep going!",
    "You matter!",
    "Breathe!",
    "Have a snack!"
};

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "affirmations");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.affirmations) return;
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(goose->id, "affirmations");

    if (goose->heldItem) return;
    if (goose->state != GooseState::WANDER) return;

    double lastDrop = -1.0;
    if (goose->lastDropTime > 0) lastDrop = goose->lastDropTime;
    if (time - lastDrop < g_config.behaviors.affirmations.interval) return;

    if ((rand() % 500) != 0) return;

    std::string msg;
    if (!g_config.behaviors.affirmations.customMessage.empty()) {
        msg = g_config.behaviors.affirmations.customMessage;
        size_t pos = msg.find("{}");
        if (pos != std::string::npos) {
            msg.replace(pos, 2, goose->name);
        }
    } else {
        int idx = rand() % (sizeof(s_defaultMessages) / sizeof(s_defaultMessages[0]));
        msg = s_defaultMessages[idx];
    }

    ItemData* item = g_assets.CreateTextItem(msg);
    if (item) {
        DroppedItem drop;
        drop.data = item;
        drop.pos = goose->GetBeakTipDevice() - WorldCoord::ItemHalfSize(item);
        drop.rotation = 0;
        drop.timeDropped = time;
        drop.pinned = false;
        g_droppedItems.push_back(drop);
        g_assets.Bite();
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_affirmationsBehavior = {
    .id = "affirmations",
    .name = "Custom Affirmations",
    .description = "Goose drops positive affirmation messages",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.fun.affirmations,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_affirmationsBehavior);

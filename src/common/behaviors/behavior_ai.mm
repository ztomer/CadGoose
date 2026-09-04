// ===========================
// behavior_ai.cpp
// AI Behavior - Chat with goose via OpenAI
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "ai_mcp_bridge.h"
#include "actor.h"
#include <string>
#include <vector>

#ifdef __APPLE__
// The chat window UI lives in ai_chat_window.mm (split out along its seams);
// this file only registers the behavior.
#endif

#include "ai_chat_window_internal.h"



static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AIState>(ctx.goose->id, "ai");
    state->Reset();
}

static void tick(Goose*, BehaviorContext&, double, double) {
}

static void render(Goose*, BehaviorContext&, IRenderer*) {
}

static Behavior g_aiBehavior = BEHAVIOR_DEF(
    "ai", "AI", "Chat with the goose using AI. Enable and click goose to open chat.",
    g_config.behaviors.systems.ai, init, tick, render
);

REGISTER_BEHAVIOR(g_aiBehavior);
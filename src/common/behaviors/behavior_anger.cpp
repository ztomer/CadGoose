// ===========================
// behavior_anger.cpp
// Anger Behavior - Goose gets angry and punches
// Based on OnePunchGoose by VisualError
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "cursor_io.h"
#include <cmath>

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(ctx.goose->id, "anger");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.anger) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(goose->id, "anger");

    float distToCursor = 1000.0f;
    if (g_cursorProvider) {
        CursorState curState = g_cursorProvider->Read();
        if (curState.hasPos()) {
            distToCursor = std::hypot(goose->pos.x - curState.position.x, goose->pos.y - curState.position.y);
        }
    }

    if (distToCursor < g_config.behaviors.anger.cursorRadius) {
        state->angerLevel = std::min(100.0f, state->angerLevel + g_config.behaviors.anger.increaseRate * (float)dt);
        state->lastAngerIncrease = time;
    } else {
        state->angerLevel = std::max(0.0f, state->angerLevel - g_config.behaviors.anger.decreaseRate * (float)dt);
    }

    if (state->angerLevel >= 80.0f && !state->isPunching) {
        if (time - state->lastPunchTime > g_config.behaviors.anger.punchCooldown) {
            state->isPunching = true;
            state->lastPunchTime = time;
        }
    }

    if (state->isPunching && time - state->lastPunchTime > g_config.behaviors.anger.punchDuration) {
        state->isPunching = false;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(goose->id, "anger");
    if (state->angerLevel < 10.0f) return;

    float alpha = state->angerLevel / 100.0f * 0.5f;
    float radius = state->angerLevel / 100.0f * 20.0f + 10.0f;

    CGContextSetRGBFillColor(cg, 1.0f, 0.0f, 0.0f, alpha);
    CGContextFillEllipseInRect(cg, CGRectMake(
        goose->pos.x - radius, goose->pos.y - radius,
        radius * 2, radius * 2));
#endif
}

static Behavior g_angerBehavior = {
    .id = "anger",
    .name = "Anger",
    .description = "Goose gets angry near the cursor and punches. Based on OnePunchGoose by VisualError",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_angerBehavior);
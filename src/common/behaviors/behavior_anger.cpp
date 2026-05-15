#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "cursor_io.h"
#include <cmath>


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(ctx.goose->id, "anger");
    state->Reset();
}

// Export anger state so goose drawing can tint body red
float Anger_GetLevel(int gooseId) {
    auto* state = BehaviorStateManager::Instance().Get<AngerState>(gooseId, "anger");
    return state ? state->angerLevel : 0.0f;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
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

    if (state->isPunching) {
        goose->currentSpeed = g_config.movement.baseRunSpeed * 1.3f;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(goose->id, "anger");
    if (state->angerLevel < 10.0f) return;

    float intensity = state->angerLevel / 100.0f;
    bool punching = state->isPunching;

    if (punching) {
        float flashAlpha = 0.4f + 0.3f * sin(ctx.time * 60.0f);
        float flashRadius = 40.0f + 20.0f * intensity;
        CGContextSetRGBFillColor(cg, 1.0f, 0.3f, 0.0f, flashAlpha);
        CGContextFillEllipseInRect(cg, CGRectMake(
            goose->pos.x - flashRadius, goose->pos.y - flashRadius,
            flashRadius * 2, flashRadius * 2));
    }

    float auraAlpha = intensity * 0.3f;
    float auraRadius = 15.0f + intensity * 20.0f;
    CGContextSetRGBFillColor(cg, 1.0f, 0.1f, 0.0f, auraAlpha);
    CGContextFillEllipseInRect(cg, CGRectMake(
        goose->pos.x - auraRadius, goose->pos.y - auraRadius,
        auraRadius * 2, auraRadius * 2));
#endif
}

static Behavior g_angerBehavior = BEHAVIOR_DEF_STARTER(
    "anger", "Anger", "Goose gets angry near the cursor and punches. Based on OnePunchGoose by VisualError",
    g_config.behaviors.fun.anger, init, tick, render
);

REGISTER_BEHAVIOR(g_angerBehavior);

// ===========================
// behavior_sonic.cpp
// Sonic Mode - 2.5x speed boost + blue trail + frequent honks
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>

static constexpr float SONIC_SPEED_MULTIPLIER = 2.5f;
static constexpr float TRAIL_CIRCLE_RADIUS = 6.0f;
static constexpr float TRAIL_SPAWN_INTERVAL = 0.03f;
static constexpr float TRAIL_LIFETIME = 0.5f;
static constexpr float HONK_INTERVAL = 0.8f;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<SonicState>(ctx.goose->id, "sonic");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<SonicState>(goose->id, "sonic");

    // Apply speed multiplier once per tick (not compounded)
    // Store original speed before first multiplication, then scale from base
    if (state->trails.empty() || state->lastTrailTime == 0) {
        state->baseSpeed = goose->currentSpeed;
    }
    goose->currentSpeed = state->baseSpeed * SONIC_SPEED_MULTIPLIER;

    // Spawn trail circles behind goose
    if (time - state->lastTrailTime >= TRAIL_SPAWN_INTERVAL) {
        SonicTrail trail;
        trail.pos = goose->pos;
        trail.time = time;
        state->trails.push_back(trail);
        state->lastTrailTime = time;
    }

    // Clean up expired trails
    while (!state->trails.empty() && (time - state->trails.front().time) > TRAIL_LIFETIME) {
        state->trails.erase(state->trails.begin());
    }

    // Honk more frequently
    if (time - state->lastHonkTime >= HONK_INTERVAL) {
        g_assets.Honk();
        state->lastHonkTime = time;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<SonicState>(goose->id, "sonic");
    if (state->trails.empty()) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    for (const auto& trail : state->trails) {
        float age = (float)(ctx.time - trail.time);
        float alpha = std::max(0.0f, 1.0f - age / TRAIL_LIFETIME);
        float radius = TRAIL_CIRCLE_RADIUS * ctx.globalScale * alpha;

        CGRect rect = CGRectMake(trail.pos.x - radius, trail.pos.y - radius, radius * 2.0f, radius * 2.0f);

        CGContextSetRGBFillColor(cg, 0.2f, 0.5f, 1.0f, alpha * 0.6f);
        CGContextFillEllipseInRect(cg, rect);
    }

    CGContextRestoreGState(cg);
}

static void cleanup(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().Get<SonicState>(ctx.goose->id, "sonic");
    if (state) {
        state->trails.clear();
        state->baseSpeed = 0;
        state->lastTrailTime = 0;
        state->lastHonkTime = 0;
    }
}

static Behavior g_sonicBehavior = BEHAVIOR_DEF_CUSTOM(
    "sonic", "Sonic Mode", "Goose moves at 2.5x speed with a blue trail effect and frequent honks",
    g_config.behaviors.fun.sonicMode, init, tick, render, cleanup, false, false
);

REGISTER_BEHAVIOR(g_sonicBehavior);

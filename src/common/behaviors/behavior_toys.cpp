// ===========================
// behavior_toys.cpp
// Toys Enabled - Spawns stick toys that goose can fetch
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "goose_math.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>


static constexpr int MAX_TOYS = 5;
static constexpr float TOY_SPAWN_INTERVAL = 5.0f;
static constexpr float TOY_LIFETIME = 30.0f;
static constexpr float TOY_FETCH_DISTANCE = 20.0f;
static constexpr float STICK_LENGTH = 24.0f;
static constexpr float STICK_WIDTH = 4.0f;
static constexpr float BALL_RADIUS = 15.0f;

static int FindInactiveToy(ToysState* state) {
    for (int i = 0; i < MAX_TOYS; ++i) {
        if (!state->toys[i].active) return i;
    }
    return -1;
}

static void SpawnToy(ToysState* state, const Vector2& pos, Toy::Type type) {
    int idx = FindInactiveToy(state);
    if (idx < 0) return;

    state->toys[idx].pos = pos;
    state->toys[idx].angle = (float)(rand() % 360);
    state->toys[idx].time = g_time;
    state->toys[idx].active = true;
    state->toys[idx].type = type;
    state->activeCount++;
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<ToysState>(ctx.goose->id, "toys");
    state->Reset();
    state->lastSpawnTime = ctx.time;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<ToysState>(goose->id, "toys");

    // Spawn new toys periodically
    if (time - state->lastSpawnTime >= TOY_SPAWN_INTERVAL && state->activeCount < MAX_TOYS) {
        float screenW = (float)g_screenWidth / ctx.globalScale;
        float screenH = (float)g_screenHeight / ctx.globalScale;
        float margin = 100.0f;

        Vector2 spawnPos{
            margin + (float)(rand() % (int)(screenW - margin * 2)),
            margin + (float)(rand() % (int)(screenH - margin * 2))
        };

        Toy::Type type = (rand() % 2 == 0) ? Toy::Type::Stick : Toy::Type::Ball;
        SpawnToy(state, spawnPos, type);
        state->lastSpawnTime = time;
    }

    // Clean up expired toys
    state->activeCount = 0;
    for (int i = 0; i < MAX_TOYS; ++i) {
        if (state->toys[i].active) {
            if (time - state->toys[i].time > TOY_LIFETIME) {
                state->toys[i].active = false;
            } else {
                state->activeCount++;
            }
        }
    }

    // Find nearest active toy
    int nearestIdx = -1;
    float nearestDist = 1e9f;
    for (int i = 0; i < MAX_TOYS; ++i) {
        if (!state->toys[i].active) continue;
        float dist = Vector2::Distance(goose->pos, state->toys[i].pos);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestIdx = i;
        }
    }

    if (nearestIdx < 0) return;

    // Goose reached toy — pick it up and transition to RETURNING
    if (nearestDist < TOY_FETCH_DISTANCE) {
        Toy::Type toyType = state->toys[nearestIdx].type;
        state->toys[nearestIdx].active = false;
        state->activeCount--;

        if (!goose->heldItem) {
            goose->heldItem = g_assets.CreateToyItem(toyType == Toy::Type::Stick);
            goose->state = GooseState::RETURNING;
            float screenW = (float)g_screenWidth / ctx.globalScale;
            float screenH = (float)g_screenHeight / ctx.globalScale;
            float margin = 100.0f;
            goose->target = {
                margin + (float)(rand() % (int)(screenW - margin * 2)),
                margin + (float)(rand() % (int)(screenH - margin * 2))
            };
            g_assets.Bite();
            g_assets.Honk();
        }
        return;
    }

    // Goose chases nearby toys when wandering
    if (goose->state == GooseState::WANDER && nearestDist < 200.0f) {
        goose->target = state->toys[nearestIdx].pos;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<ToysState>(goose->id, "toys");
    if (state->activeCount == 0) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    for (int i = 0; i < MAX_TOYS; ++i) {
        if (!state->toys[i].active) continue;

        Vector2 devPos = WorldCoord::ToDevice(state->toys[i].pos);
        float age = (float)(ctx.time - state->toys[i].time);
        float alpha = std::min(1.0f, age / 0.5f);

        if (state->toys[i].type == Toy::Type::Stick) {
            float rad = state->toys[i].angle * (float)PI / 180.0f;
            float halfLen = STICK_LENGTH * ctx.globalScale / 2.0f;
            float halfWidth = STICK_WIDTH * ctx.globalScale / 2.0f;

            float cosA = std::cos(rad);
            float sinA = std::sin(rad);

            CGPoint corners[4] = {
                {devPos.x + (-halfLen * cosA + halfWidth * sinA), devPos.y + (-halfLen * sinA - halfWidth * cosA)},
                {devPos.x + (halfLen * cosA + halfWidth * sinA), devPos.y + (halfLen * sinA - halfWidth * cosA)},
                {devPos.x + (halfLen * cosA - halfWidth * sinA), devPos.y + (halfLen * sinA + halfWidth * cosA)},
                {devPos.x + (-halfLen * cosA - halfWidth * sinA), devPos.y + (-halfLen * sinA + halfWidth * cosA)},
            };

            CGContextSetRGBFillColor(cg, 0.55f, 0.35f, 0.15f, alpha);
            CGContextBeginPath(cg);
            CGContextAddLines(cg, corners, 4);
            CGContextClosePath(cg);
            CGContextFillPath(cg);
        } else {
            float radius = BALL_RADIUS * ctx.globalScale;
            CGRect rect = CGRectMake(devPos.x - radius, devPos.y - radius, radius * 2.0f, radius * 2.0f);
            CGContextSetRGBFillColor(cg, 0.8f, 0.2f, 0.2f, alpha);
            CGContextFillEllipseInRect(cg, rect);
        }
    }

    CGContextRestoreGState(cg);
}

static Behavior g_toysBehavior = BEHAVIOR_DEF(
    "toys", "Toys", "Spawns stick and ball toys that the goose can chase and fetch",
    g_config.behaviors.fun.toysEnabled, init, tick, render
);

REGISTER_BEHAVIOR(g_toysBehavior);

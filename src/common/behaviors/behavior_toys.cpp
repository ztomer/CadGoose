// ===========================
// behavior_toys.cpp
// Toys Enabled - Spawns stick toys that goose can fetch
// Each toy is a ToyActor with its own window.
// ===========================
#include "behavior.h"
#include "behaviors/states/toys_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "goose_math.h"
#include "actor.h"
#include "actor_toy.h"
#include <cmath>
#include <vector>
#include <typeinfo>

static constexpr int MAX_TOYS = 5;
static constexpr float TOY_SPAWN_INTERVAL = 5.0f;
static constexpr float TOY_FETCH_DISTANCE = 20.0f;
static constexpr float kToySpawnMargin = 100.0f;
static constexpr float kToyWanderDetectRange = 200.0f;

static int s_nextToyId = 0;

static void init(BehaviorContext& ctx) {
    s_nextToyId = 0;
    (void)ctx;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto& mgr = ActorManager::Instance();

    // Count active toys
    int activeCount = mgr.countByType("toy");

    // Spawn new toy if needed
    static double lastSpawnTime = 0;
    if (time - lastSpawnTime >= TOY_SPAWN_INTERVAL && activeCount < MAX_TOYS) {
        float margin = kToySpawnMargin;
        float screenW = (float)g_world.screenWidth;
        float screenH = (float)g_world.screenHeight;

        Vector2 spawnPos{
            margin + (float)(rand() % (int)(screenW - margin * 2)),
            margin + (float)(rand() % (int)(screenH - margin * 2))
        };

        ToyActor::Type type = (rand() % 2 == 0) ? ToyActor::Stick : ToyActor::Ball;
        ToyActor* toy = new ToyActor(type, spawnPos, s_nextToyId++);
        mgr.add(toy);
        lastSpawnTime = time;
    }

    // Find nearest active toy
    ToyActor* nearestToy = nullptr;
    float nearestDist = 1e9f;

    for (int i = 0; i < mgr.totalCount(); i++) {
        Actor* a = mgr.getByIndex(i);
        if (!a) continue;
        if (strcmp(a->type(), "toy") != 0) continue;

        ToyActor* toy = dynamic_cast<ToyActor*>(a);
        if (!toy) continue;
        if (!toy->isAlive()) continue;

        float dist = Vector2::Distance(goose->pos, toy->position);
        if (dist < nearestDist) {
            nearestDist = dist;
            nearestToy = toy;
        }
    }

    if (!nearestToy) return;

    // Pick up toy if close enough
    if (nearestDist < TOY_FETCH_DISTANCE) {
        bool isStick = (nearestToy->toyType() == ToyActor::Stick);

        mgr.remove(nearestToy);
        delete nearestToy;

        if (!goose->heldItem) {
            goose->heldItem = g_assets.CreateToyItem(isStick);
            goose->state = GooseState::RETURNING;
            float margin = kToySpawnMargin;
            goose->target = {
                margin + (float)(rand() % (int)(g_world.screenWidth - margin * 2)),
                margin + (float)(rand() % (int)(g_world.screenHeight - margin * 2))
            };
            g_assets.Bite();
            g_assets.Honk();
        }
        return;
    }

    // Wander toward toy if close enough
    if (goose->state == GooseState::WANDER && nearestDist < kToyWanderDetectRange) {
        goose->target = nearestToy->position;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    // Toys render via their own BehaviorElementWindow
    (void)goose; (void)ctx; (void)renderCtx;
}

static Behavior g_toysBehavior = BEHAVIOR_DEF(
    "toys", "Toys", "Spawns stick and ball toys that the goose can chase and fetch",
    g_config.behaviors.fun.toysEnabled, init, tick, render
);

REGISTER_BEHAVIOR(g_toysBehavior);

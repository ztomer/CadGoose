#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include "actor.h"
#include "actor_flower.h"
#include <ctime>
#include <cmath>
#include <vector>

static constexpr int kInteractiveDropProbability = 400;

static void init(BehaviorContext& ctx) {
    (void)ctx;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    (void)ctx;

    if (goose->heldItem) return;
    if (goose->state != GooseState::WANDER) return;

    double lastDrop = goose->lastDropTime;
    bool shouldDrop = (time - lastDrop >= g_config.behaviors.interactiveDrops.dropInterval) && ((rand() % kInteractiveDropProbability) == 0);

    if (shouldDrop) {
        Vector2 dropPos = goose->GetBeakTipDevice();
        float hue = (rand() % 360) / 360.0f * 360.0f;
        FlowerActor* flower = new FlowerActor(dropPos, hue, time);
        ActorManager::Instance().add(flower);
        g_assets.Bite();
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    (void)goose; (void)ctx; (void)renderCtx;
}

static Behavior g_interactiveDropsBehavior = BEHAVIOR_DEF_GROUND(
    "interactive_drops", "Interactive Drops", "Goose drops puddles that splash and flowers that grow",
    g_config.behaviors.fun.interactiveDrops, init, tick, render
);

REGISTER_BEHAVIOR(g_interactiveDropsBehavior);

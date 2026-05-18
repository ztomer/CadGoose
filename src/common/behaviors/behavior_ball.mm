// behavior_ball.mm
// Ball behavior — thin wrapper that creates BallActor and delegates to it.
// The actual ball logic lives in actor_ball.mm (Actor, not Behavior).

#include "behavior.h"
#include "behaviors/states/ball_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "actor.h"
#include "actor_ball.h"
#include "cursor_backend.h"
#include <cmath>

static BallActor* s_ballActor = nullptr;

static void init(BehaviorContext& ctx) {
    if (!s_ballActor) {
        s_ballActor = new BallActor();
        ActorManager::Instance().add(s_ballActor);
    }
    // Reset ball position on goose init
    s_ballActor->position = {300.0f, 300.0f};
    s_ballActor->velocity = {0, 0};
    s_ballActor->speed = 0.0f;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!s_ballActor || !s_ballActor->active) return;

    // Check goose kick
    float gooseFootSize = g_config.render.footSize;
    s_ballActor->onGooseKick(goose->pos, gooseFootSize, time);

    // If ball was kicked by goose, update goose target
    if (s_ballActor->wasKicked()) {
        s_ballActor->clearKickedFlag();
        if (goose->state == GooseState::WANDER) {
            goose->target = s_ballActor->position;
            goose->currentSpeed = g_config.movement.baseWalkSpeed * 0.7f;
        }
    }

    // Check cursor kick
    auto* backend = g_backendManager.GetActiveBackend();
    if (backend) {
        Vector2 cursorPos = backend->GetCursorPos();
        s_ballActor->onCursorKick(cursorPos, time);

        // If ball was kicked by cursor, update goose state
        if (s_ballActor->wasKicked()) {
            s_ballActor->clearKickedFlag();
            if (goose->state == GooseState::WANDER) {
                goose->state = GooseState::CHASE_CURSOR;
            }
        }
    }

    // Chase ball if it's moving
    if (goose->state == GooseState::CHASE_CURSOR && s_ballActor->speed > 5.0f) {
        goose->target = s_ballActor->position;
    }

    // Tick ball physics (position, velocity, animation, bounds)
    s_ballActor->tick(dt, time);
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!s_ballActor || !s_ballActor->active) return;
    // Ball renders via its own BehaviorElementWindow
    (void)goose; (void)ctx; (void)renderCtx;
}

static void cleanup(BehaviorContext& ctx) {
    if (s_ballActor) {
        ActorManager::Instance().remove(s_ballActor);
        delete s_ballActor;
        s_ballActor = nullptr;
    }
}

static Behavior g_ballBehavior = BEHAVIOR_DEF_CUSTOM(
    "ball", "Ball", "Ball that goose chases and kicks. Based on BallMod by TheOrlando",
    g_config.behaviors.fun.ball, init, tick, render, cleanup, false, false
);

REGISTER_BEHAVIOR(g_ballBehavior);

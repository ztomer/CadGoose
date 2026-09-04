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
#include "cursor_io.h"
#include <cmath>

static BallActor* s_ballActor = nullptr;

static void init(BehaviorContext& ctx) {
    if (!s_ballActor) {
        s_ballActor = new BallActor();
        ActorManager::Instance().add(s_ballActor);
    }
    // Only reset position on first creation, not on subsequent goose spawns.
    // The ball is a singleton shared across all geese — resetting it on each
    // goose init causes the ball to snap back to origin when a new goose appears.
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!s_ballActor || !s_ballActor->isActive()) return;

    // Check goose kick
    float gooseFootSize = g_config.render.footSize;
    s_ballActor->onGooseKick(goose->pos, gooseFootSize, time);

    // If ball was kicked by goose, update goose target
    if (s_ballActor->wasKicked()) {
        s_ballActor->clearKickedFlag();
        if (goose->state == GooseState::WANDER) {
            goose->target = s_ballActor->position().toVector2();
            goose->currentSpeed = g_config.movement.baseWalkSpeed * 0.7f;
        }
    }

    // Check cursor kick
    if (g_cursorProvider) {
        CursorState cs = g_cursorProvider->Read();
        if (cs.hasPos()) {
            s_ballActor->onCursorKick(cs.position, time);

            // If ball was kicked by cursor, update goose state
            if (s_ballActor->wasKicked()) {
                s_ballActor->clearKickedFlag();
                if (goose->state == GooseState::WANDER) {
                    goose->state = GooseState::CHASE_CURSOR;
                }
            }
        }
    }

    // Chase ball if it's moving
    if (goose->state == GooseState::CHASE_CURSOR && s_ballActor->speed > 5.0f) {
        goose->target = s_ballActor->position().toVector2();
    }

    // Tick ball physics (position, velocity, animation, bounds)
    s_ballActor->tick(*ctx.world, dt, time);
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    if (!s_ballActor || !s_ballActor->isActive()) return;
    // Ball renders via its own BehaviorElementWindow
    (void)goose; (void)ctx; (void)irenderer;
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

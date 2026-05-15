#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BoredomState>(ctx.goose->id, "boredom");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BoredomState>(goose->id, "boredom");

    if (state->isLyingDown) {
        double lieDuration = time - state->lieDownStartTime;
        if (lieDuration > 8.0) {
            state->isLyingDown = false;
            state->isSighing = false;
            state->idleStartTime = time;
        }
        return;
    }

    if (goose->currentSpeed < 10.0f) {
        if (!state->isSighing) {
            if (state->idleStartTime == 0) {
                state->idleStartTime = time;
            } else if (time - state->idleStartTime > 600.0 && (rand() % 600) == 0) {
                state->isSighing = true;
                state->sighStartTime = time;
                g_assets.Honk();
            }
        } else {
            double sighDuration = time - state->sighStartTime;
            if (sighDuration > 2.0 && !state->isLyingDown) {
                state->isLyingDown = true;
                state->lieDownStartTime = time;
                goose->currentSpeed = 0;
                goose->vel = {0, 0};
            }
            if (sighDuration > 12.0) {
                state->isSighing = false;
                state->isLyingDown = false;
                state->idleStartTime = time;
            }
        }
    } else {
        state->isSighing = false;
        state->isLyingDown = false;
        state->idleStartTime = 0;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BoredomState>(goose->id, "boredom");

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    if (state->isLyingDown) {
        CGContextSaveGState(cg);
        Vector2 bodyPos = WorldCoord::RigBody(*goose);

        CGContextSetRGBFillColor(cg, 0.8f, 0.7f, 0.7f, 0.6f);
        CGContextFillEllipseInRect(cg, CGRectMake(bodyPos.x - 20.0f, bodyPos.y - 6.0f, 40.0f, 12.0f));

        Vector2 neckPos = WorldCoord::RigNeckHead(*goose);
        CGContextSetRGBFillColor(cg, 0.8f, 0.7f, 0.7f, 0.6f);
        CGContextFillEllipseInRect(cg, CGRectMake(neckPos.x - 8.0f, neckPos.y - 4.0f, 16.0f, 8.0f));

        CGContextSetRGBFillColor(cg, 0.3f, 0.3f, 0.3f, 0.5f);
        CGContextSetLineWidth(cg, 1.5f);
        CGContextMoveToPoint(cg, neckPos.x - 4.0f, neckPos.y - 2.0f);
        CGContextAddLineToPoint(cg, neckPos.x + 4.0f, neckPos.y - 2.0f);
        CGContextStrokePath(cg);

        CGContextRestoreGState(cg);
        return;
    }

    if (state->isSighing) {
        CGContextSaveGState(cg);
        Vector2 headPos = WorldCoord::RigNeckHead(*goose);
        double elapsed = goose->lastUpdateTime - state->sighStartTime;

        if (elapsed < 2.0) {
            float sighPuff = (float)elapsed * 10.0f;
            CGContextSetRGBFillColor(cg, 0.7f, 0.7f, 0.7f, 0.4f);
            CGContextFillEllipseInRect(cg, CGRectMake(
                headPos.x + 5.0f + sighPuff,
                headPos.y - 5.0f - sighPuff * 0.3f,
                8.0f + sighPuff * 0.5f,
                4.0f + sighPuff * 0.3f));
        }

        CGContextRestoreGState(cg);
    }
#endif
}

static Behavior g_boredomBehavior = BEHAVIOR_DEF(
    "boredom", "Boredom Sigh", "Goose sighs dramatically and lies down after 10+ minutes idle",
    g_config.behaviors.fun.boredom, init, tick, render
);

REGISTER_BEHAVIOR(g_boredomBehavior);

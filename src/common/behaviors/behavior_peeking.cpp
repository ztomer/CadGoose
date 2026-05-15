#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(ctx.goose->id, "peeking");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(goose->id, "peeking");

    if (state->isPeeking) {
        double peekDuration = time - state->peekStartTime;
        if (peekDuration > 1.5) {
            state->isPeeking = false;
            state->nextPeekTime = time + 8.0 + (rand() % 10);
        }
        return;
    }

    if (time < state->nextPeekTime) return;

    Vector2 posDev = WorldCoord::GoosePos(*goose);
    int screenW = g_screenWidth;
    float margin = 30.0f;

    bool atEdge = false;
    if (posDev.x < margin) { atEdge = true; state->peekSide = -1; }
    else if (posDev.x > screenW - margin) { atEdge = true; state->peekSide = 1; }
    else { state->peekSide = 0; }

    if (atEdge && (rand() % 120) == 0) {
        state->isPeeking = true;
        state->peekStartTime = time;
    }

    if (!atEdge) {
        state->nextPeekTime = time + 5.0;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(goose->id, "peeking");
    if (!state->isPeeking) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);
    Vector2 headPos = WorldCoord::RigNeckHead(*goose);
    float peekDir = (float)state->peekSide;

    CGContextSetRGBFillColor(cg, 0.8f, 0.7f, 0.6f, 0.8f);
    float ex = headPos.x + peekDir * 12.0f;
    float ey = headPos.y - 5.0f;
    CGContextFillEllipseInRect(cg, CGRectMake(ex - 5.0f, ey - 4.0f, 10.0f, 8.0f));

    CGContextSetRGBFillColor(cg, 0.1f, 0.1f, 0.1f, 0.9f);
    CGContextFillEllipseInRect(cg, CGRectMake(ex + peekDir * 2.0f - 1.5f, ey - 1.0f, 3.0f, 3.0f));

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_peekingBehavior = BEHAVIOR_DEF(
    "peeking", "Window Peeking", "Goose peeks head around monitor bezel at screen edges",
    g_config.behaviors.fun.peeking, init, tick, render
);

REGISTER_BEHAVIOR(g_peekingBehavior);

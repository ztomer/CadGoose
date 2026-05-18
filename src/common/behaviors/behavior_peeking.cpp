#include "behavior.h"
#include "behaviors/states/peeking_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include "cg_renderer.h"
#include <cmath>

static constexpr float kPeekMaxDuration = 1.5f;
static constexpr float kPeekIntervalMin = 8.0f;
static constexpr int kPeekIntervalMax = 10;
static constexpr float kPeekEdgeMargin = 30.0f;
static constexpr int kPeekProbabilityDivisor = 120;
static constexpr float kPeekResetTime = 5.0f;
static constexpr float kPeekEyeOffset = 12.0f;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(ctx.goose->id, "peeking");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(goose->id, "peeking");

    if (state->isPeeking) {
        double peekDuration = time - state->peekStartTime;
        if (peekDuration > kPeekMaxDuration) {
            state->isPeeking = false;
            state->nextPeekTime = time + kPeekIntervalMin + (rand() % kPeekIntervalMax);
        }
        return;
    }

    if (time < state->nextPeekTime) return;

    Vector2 posDev = goose->pos;
    int screenW = g_world.screenWidth;
    float margin = kPeekEdgeMargin;

    bool atEdge = false;
    if (posDev.x < margin) { atEdge = true; state->peekSide = -1; }
    else if (posDev.x > screenW - margin) { atEdge = true; state->peekSide = 1; }
    else { state->peekSide = 0; }

    if (atEdge && (rand() % kPeekProbabilityDivisor) == 0) {
        state->isPeeking = true;
        state->peekStartTime = time;
    }

    if (!atEdge) {
        state->nextPeekTime = time + kPeekResetTime;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<PeekingState>(goose->id, "peeking");
    if (!state->isPeeking) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGRenderer renderer(cg);

    Vector2 headPos = goose->rig.neckHead;
    float peekDir = (float)state->peekSide;

    float ex = headPos.x + peekDir * kPeekEyeOffset;
    float ey = headPos.y - 5.0f;
    renderer.DrawEllipse({ex, ey}, 5.0f, 4.0f, MakePeekEyeSkin(0.8f));
    renderer.DrawEllipse({ex + peekDir * 2.0f - 1.5f, ey - 1.0f}, 1.5f, 1.5f,
                        RenderColor{0.1f, 0.1f, 0.1f, 0.9f});
#endif
}

static Behavior g_peekingBehavior = BEHAVIOR_DEF(
    "peeking", "Window Peeking", "Goose peeks head around monitor bezel at screen edges",
    g_config.behaviors.fun.peeking, init, tick, render
);

REGISTER_BEHAVIOR(g_peekingBehavior);

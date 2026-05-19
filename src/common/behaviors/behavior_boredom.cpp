#include "behavior.h"
#include "behaviors/states/boredom_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "goose_math.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <cmath>

static constexpr float kBoredomIdleSpeedThreshold = 10.0f;
static constexpr float kBoredomIdleStartTime = 600.0f; // 10 minutes
static constexpr int kBoredomSighProbability = 600;
static constexpr float kBoredomSighToLieDuration = 2.0f;
static constexpr float kBoredomMaxSighDuration = 12.0f;
static constexpr float kBoredomBodyEllipseRx = 20.0f;
static constexpr float kBoredomBodyEllipseRy = 6.0f;
static constexpr float kBoredomSighPuffRate = 10.0f;
static constexpr float kBoredomSighPuffYRate = 0.25f;
static constexpr float kBoredomSighPuffSizeRate = 0.15f;

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

    if (goose->currentSpeed < kBoredomIdleSpeedThreshold) {
        if (!state->isSighing) {
            if (state->idleStartTime == 0) {
                state->idleStartTime = time;
            } else if (time - state->idleStartTime > kBoredomIdleStartTime && (rand() % kBoredomSighProbability) == 0) {
                state->isSighing = true;
                state->sighStartTime = time;
                g_assets.Honk();
            }
        } else {
            double sighDuration = time - state->sighStartTime;
            if (sighDuration > kBoredomSighToLieDuration && !state->isLyingDown) {
                state->isLyingDown = true;
                state->lieDownStartTime = time;
                goose->currentSpeed = 0;
                goose->vel = {0, 0};
            }
            if (sighDuration > kBoredomMaxSighDuration) {
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

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BoredomState>(goose->id, "boredom");

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)(irenderer ? irenderer->nativeContext() : nullptr);
    if (!cg) return;

    CGRenderer renderer(cg);

    if (state->isLyingDown) {
        renderer.SaveState();
        Vector2 bodyPos = goose->rig.body;

        renderer.DrawEllipse({bodyPos.x, bodyPos.y - 6.0f}, kBoredomBodyEllipseRx, kBoredomBodyEllipseRy,
                            RenderColor{0.8f, 0.7f, 0.7f, 0.6f});

        Vector2 neckPos = goose->rig.neckHead;
        renderer.DrawEllipse({neckPos.x, neckPos.y - 4.0f}, 8.0f, 4.0f,
                            RenderColor{0.8f, 0.7f, 0.7f, 0.6f});

        renderer.DrawLine({neckPos.x - 4.0f, neckPos.y - 2.0f},
                         {neckPos.x + 4.0f, neckPos.y - 2.0f},
                         RenderColor{0.3f, 0.3f, 0.3f, 0.5f}, 1.5f);

        renderer.RestoreState();
        return;
    }

    if (state->isSighing) {
        renderer.SaveState();
        Vector2 headPos = goose->rig.neckHead;
        double elapsed = goose->lastUpdateTime - state->sighStartTime;

        if (elapsed < 2.0) {
            float sighPuff = (float)elapsed * kBoredomSighPuffRate;
            renderer.DrawEllipse({headPos.x + 5.0f + sighPuff, headPos.y - 5.0f - sighPuff * kBoredomSighPuffYRate},
                                4.0f + sighPuff * kBoredomSighPuffSizeRate, 2.0f + sighPuff * 0.15f,
                                RenderColor{0.7f, 0.7f, 0.7f, 0.4f});
        }

        renderer.RestoreState();
    }
#endif
}

static Behavior g_boredomBehavior = BEHAVIOR_DEF(
    "boredom", "Boredom Sigh", "Goose sighs dramatically and lies down after 10+ minutes idle",
    g_config.behaviors.fun.boredom, init, tick, render
);

REGISTER_BEHAVIOR(g_boredomBehavior);

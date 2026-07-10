#include "behavior.h"
#include "behaviors/states/anger_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "cursor_io.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include "event_bus.h"
#include "assets.h"
#include "behavior_api.h"
#include "goose_math.h"
#include <cmath>

static constexpr float kAngerMaxLevel = 100.0f;
static constexpr float kAngerHonkIncrease = 15.0f;
static constexpr float kAngerDefaultDistToCursor = 1000.0f;
static constexpr float kAngerFlashBaseAlpha = 0.4f;
static constexpr float kAngerFlashAlphaAmp = 0.3f;
static constexpr float kAngerFlashFreq = 60.0f;
static constexpr float kAngerFlashBaseRadius = 40.0f;
static constexpr float kAngerFlashRadiusAmp = 20.0f;
static constexpr float kAngerFlashR = 1.0f;
static constexpr float kAngerFlashG = 0.3f;
static constexpr float kAngerFlashB = 0.0f;
static constexpr float kAngerAuraAlphaScale = 0.3f;
static constexpr float kAngerAuraBaseRadius = 15.0f;
static constexpr float kAngerAuraRadiusAmp = 20.0f;


static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(ctx.goose->id, "anger");
    state->Reset();
}

static void ensureSubscriptions(Goose* goose, AngerState* state) {
    if (state->honkSub == 0) {
        state->honkSub = EventBus::Instance().Subscribe<GooseHonkedEvent>([goose](const GooseHonkedEvent& e) {
            if (e.gooseId == goose->id) {
                auto* s = BehaviorStateManager::Instance().Get<AngerState>(goose->id, "anger");
                if (s) {
                    s->angerLevel = std::min(kAngerMaxLevel, s->angerLevel + kAngerHonkIncrease);
                    s->lastAngerIncrease = e.time;
                }
            }
        });
    }
    if (state->cursorSub == 0) {
        state->cursorSub = EventBus::Instance().Subscribe<CursorFastMoveEvent>([goose](const CursorFastMoveEvent& e) {
            auto* s = BehaviorStateManager::Instance().Get<AngerState>(goose->id, "anger");
            if (s) {
                float dist = std::hypot(goose->pos.x - e.x, goose->pos.y - e.y);
                if (dist < g_config.behaviors.anger.cursorRadius) {
                    s->angerLevel = std::min(100.0f, s->angerLevel + 5.0f);
                }
            }
        });
    }
}

// Export anger state so goose drawing can tint body red
float Anger_GetLevel(int gooseId) {
    auto* state = BehaviorStateManager::Instance().Get<AngerState>(gooseId, "anger");
    return state ? state->angerLevel : 0.0f;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(goose->id, "anger");
    ensureSubscriptions(goose, state);

    float distToCursor = kAngerDefaultDistToCursor;
    if (g_cursorProvider) {
        CursorState curState = g_cursorProvider->Read();
        if (curState.hasPos()) {
            distToCursor = std::hypot(goose->pos.x - curState.position.x, goose->pos.y - curState.position.y);
        }
    }

    if (distToCursor < g_config.behaviors.anger.cursorRadius) {
        state->angerLevel = std::min(100.0f, state->angerLevel + g_config.behaviors.anger.increaseRate * (float)dt);
        state->lastAngerIncrease = time;
    } else {
        state->angerLevel = std::max(0.0f, state->angerLevel - g_config.behaviors.anger.decreaseRate * (float)dt);
    }

    if (state->angerLevel >= 80.0f && !state->isPunching) {
        if (time - state->lastPunchTime > g_config.behaviors.anger.punchCooldown) {
            state->isPunching = true;
            state->lastPunchTime = time;
        }
    }

    if (state->isPunching && time - state->lastPunchTime > g_config.behaviors.anger.punchDuration) {
        state->isPunching = false;
    }

    if (state->isPunching) {
        goose->currentSpeed = g_config.movement.baseRunSpeed * 1.3f;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    if (!irenderer) return;
    IRenderer& renderer = *irenderer;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<AngerState>(goose->id, "anger");
    if (state->angerLevel < 10.0f) return;

    float intensity = state->angerLevel / 100.0f;
    bool punching = state->isPunching;

    if (punching) {
        float flashAlpha = kAngerFlashBaseAlpha + kAngerFlashAlphaAmp * sin(ctx.time * kAngerFlashFreq);
        float flashRadius = kAngerFlashBaseRadius + kAngerFlashRadiusAmp * intensity;
        renderer.DrawEllipse({goose->pos.x, goose->pos.y}, flashRadius, flashRadius,
                            RenderColor{kAngerFlashR, kAngerFlashG, kAngerFlashB, flashAlpha});

        #ifdef __APPLE__
        static void* s_punchImage = nullptr;
        if (!s_punchImage) {
            s_punchImage = (void*)g_assets.GetBehaviorImage("Assets/Images/OtherGfx/punch_hand.png");
        }

        CGContextRef cgCtx = (CGContextRef)renderer.nativeContext();
        if (cgCtx && s_punchImage) {
            double elapsed = ctx.time - state->lastPunchTime;
            float duration = g_config.behaviors.anger.punchDuration;
            float t = Clamp((float)(elapsed / duration), 0.0f, 1.0f);
            
            // Smooth punch wave: starts at 0, peaks at 1 (middle of punch), returns to 0
            float punchFactor = sin(t * M_PI);
            float maxExtend = 35.0f * g_config.general.globalScale;
            float extend = punchFactor * maxExtend;
            
            float angleRad = goose->dir * (float)(M_PI / 180.0);
            float dx = cos(angleRad);
            float dy = sin(angleRad);
            float px = -dy;
            float py = dx;
            
            // Alternate left/right wing for successive punches
            bool leftSide = (static_cast<int>(state->lastPunchTime * 10.0) % 2 == 0);
            float sideSign = leftSide ? 1.0f : -1.0f;
            
            // Spawn hand relative to body (wing area), extending in the facing direction
            float sideDist = 12.0f * g_config.general.globalScale;
            float startOffset = 15.0f * g_config.general.globalScale;
            RenderPoint handPos = {
                goose->rig.body.x + px * (sideDist * sideSign) + dx * (startOffset + extend),
                goose->rig.body.y + py * (sideDist * sideSign) + dy * (startOffset + extend)
            };
            
            // Retrieve body color
            float bodyColorR = g_config.color.currentBody.r;
            float bodyColorG = g_config.color.currentBody.g;
            float bodyColorB = g_config.color.currentBody.b;
            
            if (g_config.behaviors.fun.rainbow) {
                float hue = Rainbow_GetHue(goose->id);
                float r, g, b;
                HSV_to_RGB(hue, 1.0f, 0.85f, &r, &g, &b);
                bodyColorR = r; bodyColorG = g; bodyColorB = b;
            }

            CGContextSaveGState(cgCtx);
            CGContextTranslateCTM(cgCtx, handPos.x, handPos.y);
            
            // Align the punch hand pointing direction to the goose's angle.
            // Fist points up in the PNG (+Y down in flipped view is -90 deg), so add M_PI_2 (90 deg) to face right (0 deg).
            CGContextRotateCTM(cgCtx, angleRad + M_PI_2);
            
            float handSize = 28.0f * g_config.general.globalScale;
            CGRect rect = CGRectMake(-handSize / 2.0f, -handSize / 2.0f, handSize, handSize);
            
            // Flip inside local space for CGContextDrawImage
            CGContextTranslateCTM(cgCtx, 0, handSize);
            CGContextScaleCTM(cgCtx, 1.0f, -1.0f);
            CGContextTranslateCTM(cgCtx, 0, -handSize);
            
            CGContextDrawImage(cgCtx, rect, (CGImageRef)s_punchImage);
            
            // Color mask with the goose's current body color
            CGContextSetBlendMode(cgCtx, kCGBlendModeSourceIn);
            CGContextSetRGBFillColor(cgCtx, bodyColorR, bodyColorG, bodyColorB, 1.0f);
            CGContextFillRect(cgCtx, rect);
            
            CGContextRestoreGState(cgCtx);
        }
        #endif
    }

    float auraAlpha = intensity * kAngerAuraAlphaScale;
    float auraRadius = kAngerAuraBaseRadius + intensity * kAngerAuraRadiusAmp;
    renderer.DrawEllipse({goose->pos.x, goose->pos.y}, auraRadius, auraRadius,
                        RenderColor{1.0f, 0.1f, 0.0f, auraAlpha});
}

static void cleanup(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().Get<AngerState>(ctx.goose->id, "anger");
    if (state) {
        if (state->honkSub) EventBus::Instance().Unsubscribe(state->honkSub);
        if (state->cursorSub) EventBus::Instance().Unsubscribe(state->cursorSub);
    }
}

static Behavior g_angerBehavior = BEHAVIOR_DEF_CUSTOM(
    "anger", "Anger", "Goose gets angry near the cursor and punches. Based on OnePunchGoose by VisualError",
    g_config.behaviors.fun.anger, init, tick, render, cleanup, true, false
);

REGISTER_BEHAVIOR(g_angerBehavior);

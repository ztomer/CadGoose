// ===========================
// behavior_honcker.cpp
// Honcker Behavior - Press F to make goose honk
// Based on Honcker by DesktopGooseUnofficial
// Reference: uses GetAsyncKeyState(0x46) for F key detection
// ===========================
#include "behavior.h"
#include "behaviors/states/honcker_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "hotkey.h"

static constexpr float kHonkYOffset = 20.0f;
static constexpr float kHonkBubbleRadius = 15.0f;
static constexpr float kHonkBubbleR = 1.0f;
static constexpr float kHonkBubbleG = 0.8f;
static constexpr float kHonkBubbleB = 0.0f;
static constexpr float kHonkBubbleAlpha = 0.8f;

#ifdef __APPLE__
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <ApplicationServices/ApplicationServices.h>

static bool s_wasPressed = false;
static CGImageRef s_honkImage = nullptr;

static bool IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    if (!s_honkImage) {
        s_honkImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/honk.png");
    }
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    int keyCode = KeyNameToKeyCode(g_config.behaviors.honcker.hotkey);
    bool pressed = IsKeyPressed(keyCode);

    if (pressed && !s_wasPressed) {
        g_assets.Honk();
        auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(goose->id, "honcker");
        state->visible = true;
        state->lastShowTime = time;
    }

    s_wasPressed = pressed;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(goose->id, "honcker");
    if (state->visible && time - state->lastShowTime > 0.5) {
        state->visible = false;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(goose->id, "honcker");
    if (!state || !state->visible) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGRenderer renderer(cg);

    Vector2 headPos = goose->rig.neckHead;
    float size = g_config.behaviors.honcker.size;

    if (s_honkImage) {
        float imgW = (float)CGImageGetWidth(s_honkImage);
        float imgH = (float)CGImageGetHeight(s_honkImage);
        float scale = size / imgW;
        float drawW = imgW * scale;
        float drawH = imgH * scale;
        float yOffset = kHonkYOffset;
        renderer.DrawImage(s_honkImage, RenderRect{headPos.x - drawW/2, headPos.y - drawH - yOffset, drawW, drawH});
    } else {
        renderer.DrawEllipse({headPos.x, headPos.y - kHonkYOffset}, kHonkBubbleRadius, kHonkBubbleRadius,
                            RenderColor{kHonkBubbleR, kHonkBubbleG, kHonkBubbleB, kHonkBubbleAlpha});
    }
}

static Behavior g_honckerBehavior = BEHAVIOR_DEF_STARTER(
    "honcker", "Honcker", "Press F to make the goose honk. Based on Honcker by DesktopGooseUnofficial",
    g_config.behaviors.control.honcker, init, tick, render
);

REGISTER_BEHAVIOR(g_honckerBehavior);

void Honcker_Honk(Goose* goose, double time) {
    g_assets.Honk();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(goose->id, "honcker");
    state->visible = true;
    state->lastShowTime = time;
}
#else
// Linux stub
void Honcker_Honk(Goose*, double) {}
#endif

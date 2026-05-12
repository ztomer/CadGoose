// ===========================
// behavior_honcker.cpp
// Honcker Behavior - Press F to make goose honk
// Based on Honcker by DesktopGooseUnofficial
// Reference: uses GetAsyncKeyState(0x46) for F key detection
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>

static bool s_enabled = true;
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
    if (!g_config.behaviors.control.honcker) return;

    int keyCode = g_config.behaviors.honcker.key;
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
    if (!g_config.behaviors.control.honcker) return;

    auto* state = BehaviorStateManager::Instance().Get<HonckerState>(goose->id, "honcker");
    if (!state || !state->visible) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    Vector2 headPos = goose->rig.neckHead;
    float size = g_config.behaviors.honcker.size;

    if (s_honkImage) {
        float imgW = (float)CGImageGetWidth(s_honkImage);
        float imgH = (float)CGImageGetHeight(s_honkImage);
        float scale = size / imgW;
        float drawW = imgW * scale;
        float drawH = imgH * scale;
        CGRect rect = CGRectMake(headPos.x - drawW/2, headPos.y - drawH - 20, drawW, drawH);
        CGContextDrawImage(cg, rect, s_honkImage);
    } else {
        CGContextSetRGBFillColor(cg, 1.0f, 0.8f, 0.0f, 0.8f);
        CGContextFillEllipseInRect(cg, CGRectMake(headPos.x - 15, headPos.y - 35, 30, 30));
    }
}

static Behavior g_honckerBehavior = {
    .id = "honcker",
    .name = "Honcker",
    .description = "Press F to make the goose honk. Based on Honcker by DesktopGooseUnofficial",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_honckerBehavior);

void Honcker_Honk(Goose* goose, double time) {
    g_assets.Honk();
    auto* state = BehaviorStateManager::Instance().GetOrCreate<HonckerState>(goose->id, "honcker");
    state->visible = true;
    state->lastShowTime = time;
}
// ===========================
// behavior_honcker.cpp
// Honcker Behavior - Press F to make goose honk
// Based on Honcker by DesktopGooseUnofficial
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <ApplicationServices/ApplicationServices.h>

static bool s_enabled = true;
static bool s_wasPressed = false;

static bool IsKeyHeld(int keyCode) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return false;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return false;
    }

    int64_t eventKey = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    bool pressed = (eventKey == keyCode);

    CFRelease(event);
    CFRelease(source);
    return pressed;
}

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.honcker) return;

    int keyCode = g_config.behaviors.honcker.key;
    bool pressed = IsKeyHeld(keyCode);

    if (pressed && !s_wasPressed) {
        g_assets.Honk();
    }

    s_wasPressed = pressed;
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
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
}
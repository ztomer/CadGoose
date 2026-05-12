// ===========================
// behavior_banish.cpp
// Banish Behavior - Banish goose to the shadow realm
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>

static bool s_enabled = true;
static bool s_banished = false;
static double s_banishTime = 0.0;
static constexpr float FADE_DURATION = 0.5f;
static constexpr float RESPAWN_DELAY = 30.0f;

static bool IsMiddleMouseDown() {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return false;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return false;
    }

    CGEventType type = CGEventGetType(event);
    bool isMiddle = (type == kCGEventOtherMouseDown);
    CFRelease(event);
    CFRelease(source);
    return isMiddle;
}

static bool AreModifiersPressed() {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return false;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return false;
    }

    CGEventFlags flags = CGEventGetFlags(event);
    bool ctrlAlt = (flags & kCGEventFlagMaskControl) && (flags & kCGEventFlagMaskAlternate);
    CFRelease(event);
    CFRelease(source);
    return ctrlAlt;
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BanishState>(ctx.goose->id, "banish");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.banish) {
        s_banished = false;
        return;
    }

    auto* state = BehaviorStateManager::Instance().GetOrCreate<BanishState>(goose->id, "banish");

    if (AreModifiersPressed() && IsMiddleMouseDown() && !s_banished) {
        state->originalPos = goose->pos;
        s_banishTime = time;
        s_banished = true;
        state->fadeProgress = 0.0f;
    }

    if (s_banished) {
        float elapsed = (float)(time - s_banishTime);

        if (elapsed < FADE_DURATION) {
            state->fadeProgress = elapsed / FADE_DURATION;
        } else if (elapsed < RESPAWN_DELAY) {
            state->fadeProgress = 1.0f;
        } else {
            float screenW = (float)g_screenWidth;
            float screenH = (float)g_screenHeight;
            goose->pos.x = 100.0f + (float)(rand() % (int)(screenW - 200.0f));
            goose->pos.y = 100.0f + (float)(rand() % (int)(screenH - 200.0f));
            goose->vel = {0, 0};
            s_banished = false;
            state->fadeProgress = 0.0f;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.banish || !s_banished) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<BanishState>(goose->id, "banish");

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float fadeOut = state->fadeProgress;
    float screenW = (float)g_screenWidth;
    float screenH = (float)g_screenHeight;

    CGContextSetRGBFillColor(cg, 0.0f, 0.0f, 0.0f, fadeOut * 0.8f);
    CGContextFillRect(cg, CGRectMake(0, 0, screenW, screenH));
}

static Behavior g_banishBehavior = {
    .id = "banish",
    .name = "Banish",
    .description = "Press Ctrl+Alt+Middle Click to banish goose to the shadow realm. Respawns after 30s.",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_banishBehavior);
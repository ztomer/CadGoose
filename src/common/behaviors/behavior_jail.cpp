// ===========================
// behavior_jail.cpp
// Jail Behavior - Trap the goose in a cage
// Based on GooseJail by WackyModer
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

static bool s_enabled = true;
static bool s_jailToggle = false;
static Vector2 s_jailPos{-250.0f, -250.0f};
static Vector2 s_jailPosition{300.0f, 400.0f};

static bool IsKeyPressed(int keyCode) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return false;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return false;
    }

    CGKeyCode key = (CGKeyCode)keyCode;
    bool pressed = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode) == key;

    CFRelease(event);
    CFRelease(source);
    return pressed;
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.jail) {
        s_jailToggle = false;
        return;
    }

    Vector2 cursorPos{-1, -1};
    if (g_cursorProvider) {
        CursorState state = g_cursorProvider->Read();
        if (state.hasPos()) {
            cursorPos = state.position;
        }
    }

    if (IsKeyPressed(g_config.behaviors.jail.keyO)) {
        s_jailPosition = cursorPos;
    }

    if (IsKeyPressed(g_config.behaviors.jail.keyP)) {
        s_jailToggle = !s_jailToggle;
        goose->state = GooseState:: WANDER;
        g_assets.Honk();
    }

    if (s_jailToggle) {
        goose->target = s_jailPosition;
        s_jailPos = goose->pos;

        goose->pos = s_jailPosition;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.jail || !s_jailToggle) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float jailSize = g_config.behaviors.jail.size;
    CGRect rect = CGRectMake(s_jailPos.x - jailSize/2, s_jailPos.y - jailSize/2, jailSize, jailSize);

    CGContextSetRGBStrokeColor(cg, 0.5f, 0.5f, 0.5f, 1.0f);
    CGContextSetLineWidth(cg, 4.0f);
    CGContextStrokeRect(cg, rect);

    CGContextSetRGBFillColor(cg, 0.3f, 0.3f, 0.3f, 0.3f);
    CGContextFillRect(cg, rect);
#endif
}

static Behavior g_jailBehavior = {
    .id = "jail",
    .name = "Jail",
    .description = "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_jailBehavior);
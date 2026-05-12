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
static bool s_wasKeyDown = false;

static bool IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.jail) {
        auto* state = BehaviorStateManager::Instance().Get<JailState>(goose->id, "jail");
        if (state) state->isJailed = false;
        return;
    }

    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    Vector2 cursorPos{-1, -1};
    if (g_cursorProvider) {
        CursorState cs = g_cursorProvider->Read();
        if (cs.hasPos()) {
            cursorPos = cs.position;
        }
    }

    if (IsKeyPressed(g_config.behaviors.jail.keyO)) {
        state->jailPos = cursorPos;
        state->positionSet = true;
    }

    bool keyDown = IsKeyPressed(g_config.behaviors.jail.keyP);
    if (keyDown && !s_wasKeyDown) {
        state->isJailed = !state->isJailed;
        goose->state = GooseState::WANDER;
        g_assets.Honk();
    }
    s_wasKeyDown = keyDown;

    if (state->isJailed && state->positionSet) {
        goose->target = state->jailPos;
        goose->pos = state->jailPos;
        goose->vel = {0, 0};
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.jail) return;

    auto* state = BehaviorStateManager::Instance().Get<JailState>(goose->id, "jail");
    if (!state || !state->isJailed || !state->positionSet) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float jailSize = g_config.behaviors.jail.size;
    CGRect rect = CGRectMake(state->jailPos.x - jailSize/2, state->jailPos.y - jailSize/2, jailSize, jailSize);

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
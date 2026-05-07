// ===========================
// behavior_drag.cpp
// Drag Behavior - Drag the goose with your cursor
// Based on DragGoose by euandeas/Straaft
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

static bool s_enabled = true;

static constexpr float DRAG_RADIUS = 45.0f;

static void init(BehaviorContext& ctx) {
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.drag) return;

    Vector2 cursorPos{-1, -1};
    if (g_cursorProvider) {
        CursorState state = g_cursorProvider->Read();
        if (state.hasPos()) {
            cursorPos = state.position;
        }
    }

    float dx = goose->pos.x - cursorPos.x;
    float dy = goose->pos.y - cursorPos.y;
    bool onGoose = (dx > -g_config.behaviors.drag.radius && dx < g_config.behaviors.drag.radius &&
                    dy > -g_config.behaviors.drag.radius && dy < g_config.behaviors.drag.radius);

    bool mouseDown = false;
#ifdef __APPLE__
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (source) {
        CGEventRef event = CGEventCreate(source);
        if (event) {
            CGEventType type = CGEventGetType(event);
            mouseDown = (type == kCGEventLeftMouseDown || type == kCGEventLeftMouseDragged);
            CFRelease(event);
        }
        CFRelease(source);
    }
#endif

    if (onGoose && mouseDown && goose->state != SNATCH_CURSOR) {
        goose->pos.x = cursorPos.x - 5.0f;
        goose->pos.y = cursorPos.y;

        goose->dir += (float)(rand() % 21 - 10);
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_dragBehavior = {
    .id = "drag",
    .name = "Drag",
    .description = "Drag the goose with your cursor. Be careful - he may bite! Based on DragGoose by Straaft",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_dragBehavior);
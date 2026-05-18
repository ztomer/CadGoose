// ===========================
// behavior_drag.cpp
// Drag Behavior - Drag the goose with your cursor
// Based on DragGoose by euandeas/Straaft
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
#endif

static constexpr float DRAG_RADIUS = 45.0f;
static constexpr int kDragDirectionJitterMax = 10;

static void init(BehaviorContext& ctx) {}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
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
    mouseDown = CGEventSourceButtonState(kCGEventSourceStateHIDSystemState, kCGMouseButtonLeft);
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (dpy) {
        Window root, child;
        int rootX, rootY, winX, winY;
        unsigned int mask;
        if (XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child, &rootX, &rootY, &winX, &winY, &mask)) {
            mouseDown = (mask & Button1Mask) != 0;
        }
        XCloseDisplay(dpy);
    }
#endif

    if (onGoose && mouseDown && goose->state != GooseState:: SNATCH_CURSOR) {
        goose->pos.x = cursorPos.x - 5.0f;
        goose->pos.y = cursorPos.y;
        goose->vel.x = 0;
        goose->vel.y = 0;

        goose->dir += (float)(rand() % (kDragDirectionJitterMax * 2 + 1) - kDragDirectionJitterMax);
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
}

static Behavior g_dragBehavior = BEHAVIOR_DEF_STARTER(
    "drag", "Drag", "Drag the goose with your cursor. Be careful - he may bite! Based on DragGoose by Straaft",
    g_config.behaviors.control.drag, init, tick, render
);

REGISTER_BEHAVIOR(g_dragBehavior);

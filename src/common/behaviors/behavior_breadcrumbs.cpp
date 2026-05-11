// ===========================
// behavior_breadcrumbs.cpp
// Bread Crumbs Behavior - Leave trail at cursor position
// Based on BreadCrumbs by Straaft
// Reference: BreadCrumbs.dll decompiled
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "cursor_backend.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <TargetConditionals.h>

static bool s_enabled = true;
static CGImageRef s_crumbImage = nullptr;
static bool s_following = false;
static Vector2 s_droppedPos{0, 0};
static double s_lastKeyCheck = 0;

static int GetTriggerKeyCode() {
    const std::string& keyName = g_config.behaviors.breadCrumbs.triggerKey;

    if (keyName == "RightShift") {
#if defined(__APPLE__)
        return 60;
#elif defined(__linux__)
        return 50;
#endif
    } else if (keyName == "LeftShift") {
#if defined(__APPLE__)
        return 56;
#elif defined(__linux__)
        return 50;
#endif
    }

#if defined(__APPLE__)
    return 60;
#elif defined(__linux__)
    return 50;
#else
    return 60;
#endif
}

static short GetAsyncKeyState(int keyCode) {
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) return 0;

    CGEventRef event = CGEventCreate(source);
    if (!event) {
        CFRelease(source);
        return 0;
    }

    int64_t eventKey = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    short result = (eventKey == keyCode) ? -32767 : 0;

    CFRelease(event);
    CFRelease(source);
    return result;
}

static void init(BehaviorContext& ctx) {
    if (!s_crumbImage) {
        s_crumbImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/crumbs.png");
    }
    s_following = false;
    s_lastKeyCheck = 0;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.breadCrumbs) return;
    if (time - s_lastKeyCheck < 0.016) return;
    s_lastKeyCheck = time;

    auto* backend = g_backendManager.GetActiveBackend();
    if (!backend) return;

    Vector2 cursorPos = backend->GetCursorPos();
    int keyCode = GetTriggerKeyCode();
    short keyState = GetAsyncKeyState(keyCode);
    bool isPressed = (keyState != 0);

    if (isPressed && !s_following) {
        s_following = true;
        s_droppedPos = cursorPos;
        g_assets.Honk();
    } else if (!isPressed && s_following) {
        s_following = false;
    }

    if (s_following) {
        s_droppedPos = cursorPos;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.breadCrumbs) return;
    if (!s_following) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    if (!s_crumbImage) {
        CGContextSetRGBFillColor(cg, 0.9f, 0.7f, 0.4f, 0.8f);
        float size = g_config.behaviors.breadCrumbs.size;
        CGContextFillEllipseInRect(cg, CGRectMake(s_droppedPos.x - size/2, s_droppedPos.y - size/2, size, size));
        return;
    }

    float imgWidth = (float)CGImageGetWidth(s_crumbImage);
    float imgHeight = (float)CGImageGetHeight(s_crumbImage);

    float x = s_droppedPos.x - imgWidth / 2.0f;
    float y = s_droppedPos.y - imgHeight / 2.0f;

    CGRect rect = CGRectMake(x, y, imgWidth, imgHeight);
    CGContextDrawImage(cg, rect, s_crumbImage);
}

static Behavior g_breadcrumbBehavior = {
    .id = "breadcrumbs",
    .name = "Bread Crumbs",
    .description = "Leave trail at cursor. Based on BreadCrumbs by Straaft",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_breadcrumbBehavior);
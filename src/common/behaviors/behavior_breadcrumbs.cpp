#include "behavior.h"
#include "behaviors/states/breadcrumb_state.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "cursor_backend.h"
#include "hotkey.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include "ring_buffer.h"
#include "actor.h"
#include "actor_breadcrumb.h"
#include <cmath>



#ifdef __APPLE__
#include "cg_renderer.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <TargetConditionals.h>
#elif defined(__linux__)
#include "linux_renderer.h"
#include <X11/Xlib.h>
#endif

#ifdef __APPLE__
static CGImageRef s_crumbImage = nullptr;
#elif defined(__linux__)
static void* s_crumbImage = nullptr;
#else
static void* s_crumbImage = nullptr;
#endif
static bool s_wasKeyDown = false;
static double s_lastKeyCheck = 0;
static int s_nextCrumbId = 0;

static bool IsKeyDown(int keyCode) {
#ifdef __APPLE__
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
#elif defined(__linux__)
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) return false;
    char keys[32];
    XQueryKeymap(dpy, keys);
    int keyIndex = keyCode / 8;
    int keyBit = keyCode % 8;
    bool pressed = (keys[keyIndex] & (1 << keyBit)) != 0;
    XCloseDisplay(dpy);
    return pressed;
#else
    (void)keyCode;
    return false;
#endif
}

static void LogCrumb(const char* msg) {
    fprintf(stderr, "[Breadcrumbs] %s\n", msg);
}

static void init(BehaviorContext& ctx) {
    if (!s_crumbImage) {
        s_crumbImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/crumbs.png");
    }
    s_wasKeyDown = false;
    s_lastKeyCheck = 0;
    s_nextCrumbId = 0;
    g_world.crumbs.clear();
    (void)ctx;
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    (void)ctx;

    if (time - s_lastKeyCheck < 0.016) return;
    s_lastKeyCheck = time;

    auto* backend = g_backendManager.GetActiveBackend();
    if (!backend) return;

    Vector2 cursorPos = backend->GetCursorPos();
    int keyCode = KeyNameToKeyCode(g_config.behaviors.breadCrumbs.hotkey);
    bool keyDown = IsKeyDown(keyCode);

    if (keyDown && !s_wasKeyDown) {
        s_wasKeyDown = true;
        Crumbs crumb;
        crumb.pos = cursorPos;
        crumb.time = time;
        crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
        g_world.crumbs.push(crumb);
        // Create BreadcrumbActor
        BreadcrumbActor* actor = new BreadcrumbActor(cursorPos, time, crumb.lifetime);
        ActorManager::Instance().add(actor);
        LogCrumb("first crumb dropped");
        g_assets.Honk();
    } else if (!keyDown) {
        if (s_wasKeyDown) LogCrumb("key released");
        s_wasKeyDown = false;
    }

    if (keyDown && !g_world.crumbs.empty()) {
        Crumbs& last = g_world.crumbs.back();
        float dist = std::hypot(cursorPos.x - last.pos.x, cursorPos.y - last.pos.y);
        if (dist >= g_config.behaviors.breadCrumbs.spawnDist) {
            Crumbs crumb;
            crumb.pos = cursorPos;
            crumb.time = time;
            crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
            g_world.crumbs.push(crumb);
            // Create BreadcrumbActor
            BreadcrumbActor* actor = new BreadcrumbActor(cursorPos, time, crumb.lifetime);
            ActorManager::Instance().add(actor);
        }
    }

    int maxCrumbs = g_config.behaviors.breadCrumbs.maxCrumbs;
    while (g_world.crumbs.size() > (size_t)maxCrumbs) {
        g_world.crumbs.pop();
    }

    while (!g_world.crumbs.empty() && time - g_world.crumbs.front().time > g_world.crumbs.front().lifetime) {
        g_world.crumbs.pop();
    }

    float eatRadius = g_config.render.footSize * 2.0f;
    for (size_t i = 0; i < g_world.crumbs.size(); ++i) {
        Crumbs& crumb = g_world.crumbs[i];
        if (crumb.eaten) continue;
        float dist = std::hypot(goose->pos.x - crumb.pos.x, goose->pos.y - crumb.pos.y);
        if (dist < eatRadius) {
            crumb.eaten = true;
            g_assets.Bite();
            goose->isChewing = true;
            goose->chewingStartTime = time;
            break;
        }
    }

    while (!g_world.crumbs.empty() && g_world.crumbs.front().eaten) {
        g_world.crumbs.pop();
    }
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    (void)goose; (void)ctx; (void)irenderer;
}

static Behavior g_breadcrumbBehavior = BEHAVIOR_DEF(
    "breadcrumbs", "Bread Crumbs", "Hold hotkey to drop breadcrumbs at cursor. Based on BreadCrumbs by Straaft",
    g_config.behaviors.fun.breadCrumbs, init, tick, render
);

REGISTER_BEHAVIOR(g_breadcrumbBehavior);

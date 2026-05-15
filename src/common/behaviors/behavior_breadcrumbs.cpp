#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "cursor_backend.h"
#include "hotkey.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <TargetConditionals.h>
#include <cmath>

#include "ring_buffer.h"

static CGImageRef s_crumbImage = nullptr;
static bool s_wasKeyDown = false;
static double s_lastKeyCheck = 0;

struct Crumbs {
    Vector2 pos;
    double time;
    float lifetime;
    bool eaten = false;
};
static constexpr size_t kMaxCrumbs = 200;
static RingBuffer<Crumbs, kMaxCrumbs> s_crumbs;

static bool IsKeyDown(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
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
    s_crumbs.clear();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
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
        s_crumbs.push(crumb);
        LogCrumb("first crumb dropped");
        g_assets.Honk();
    } else if (!keyDown) {
        if (s_wasKeyDown) LogCrumb("key released");
        s_wasKeyDown = false;
    }

    if (keyDown && !s_crumbs.empty()) {
        Crumbs& last = s_crumbs.back();
        float dist = std::hypot(cursorPos.x - last.pos.x, cursorPos.y - last.pos.y);
        if (dist >= g_config.behaviors.breadCrumbs.spawnDist) {
            Crumbs crumb;
            crumb.pos = cursorPos;
            crumb.time = time;
            crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
            s_crumbs.push(crumb);
        }
    }

    int maxCrumbs = g_config.behaviors.breadCrumbs.maxCrumbs;
    while (s_crumbs.size() > (size_t)maxCrumbs) {
        s_crumbs.pop();
    }

    while (!s_crumbs.empty() && time - s_crumbs.front().time > s_crumbs.front().lifetime) {
        s_crumbs.pop();
    }

    float eatRadius = g_config.render.footSize * 2.0f;
    for (size_t i = 0; i < s_crumbs.size(); ++i) {
        Crumbs& crumb = s_crumbs[i];
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

    while (!s_crumbs.empty() && s_crumbs.front().eaten) {
        s_crumbs.pop();
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (s_crumbs.empty()) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    if (!s_crumbImage) {
        float size = g_config.behaviors.breadCrumbs.size;
        for (const auto& crumb : s_crumbs) {
            if (crumb.eaten) continue;
            float alpha = std::max(0.0f, 1.0f - (float)(ctx.time - crumb.time) / crumb.lifetime);
            CGContextSetRGBFillColor(cg, 0.9f, 0.7f, 0.4f, alpha * 0.8f);
            CGContextFillEllipseInRect(cg, CGRectMake(crumb.pos.x - size/2, crumb.pos.y - size/2, size, size));
        }
    } else {
        float imgWidth = (float)CGImageGetWidth(s_crumbImage);
        float imgHeight = (float)CGImageGetHeight(s_crumbImage);
        for (const auto& crumb : s_crumbs) {
            if (crumb.eaten) continue;
            float alpha = std::max(0.0f, 1.0f - (float)(ctx.time - crumb.time) / crumb.lifetime);
            CGContextSetAlpha(cg, alpha);
            CGRect rect = CGRectMake(crumb.pos.x - imgWidth / 2.0f, crumb.pos.y - imgHeight / 2.0f, imgWidth, imgHeight);
            CGContextDrawImage(cg, rect, s_crumbImage);
        }
    }

    CGContextRestoreGState(cg);
}

static Behavior g_breadcrumbBehavior = BEHAVIOR_DEF_GROUND(
    "breadcrumbs", "Bread Crumbs", "Hold hotkey to drop breadcrumbs at cursor. Based on BreadCrumbs by Straaft",
    g_config.behaviors.fun.breadCrumbs, init, tick, render
);

REGISTER_BEHAVIOR(g_breadcrumbBehavior);

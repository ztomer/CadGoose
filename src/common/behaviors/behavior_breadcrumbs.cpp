#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "cursor_backend.h"
#include <CoreGraphics/CoreGraphics.h>
#include <ApplicationServices/ApplicationServices.h>
#include <TargetConditionals.h>
#include <vector>
#include <cmath>

static bool s_enabled = true;
static CGImageRef s_crumbImage = nullptr;
static bool s_wasKeyDown = false;
static double s_lastKeyCheck = 0;

struct Crumbs {
    Vector2 pos;
    double time;
    float lifetime;
};
static std::vector<Crumbs> s_crumbs;

static int GetTriggerKeyCode() {
    const std::string& keyName = g_config.behaviors.breadCrumbs.triggerKey;
    if (keyName == "RightShift") return 60;
    if (keyName == "LeftShift") return 56;
    return 60;
}

static bool IsKeyDown(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
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
    if (!g_config.behaviors.fun.breadCrumbs) return;
    if (time - s_lastKeyCheck < 0.016) return;
    s_lastKeyCheck = time;

    auto* backend = g_backendManager.GetActiveBackend();
    if (!backend) return;

    Vector2 cursorPos = backend->GetCursorPos();
    int keyCode = GetTriggerKeyCode();
    bool keyDown = IsKeyDown(keyCode);

    if (keyDown && !s_wasKeyDown) {
        s_wasKeyDown = true;
        Crumbs crumb;
        crumb.pos = cursorPos;
        crumb.time = time;
        crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
        s_crumbs.push_back(crumb);
        g_assets.Honk();
    } else if (!keyDown) {
        s_wasKeyDown = false;
    }

    if (keyDown && s_crumbs.size() > 0) {
        Crumbs& last = s_crumbs.back();
        float dist = std::hypot(cursorPos.x - last.pos.x, cursorPos.y - last.pos.y);
        if (dist >= g_config.behaviors.breadCrumbs.spawnDist) {
            Crumbs crumb;
            crumb.pos = cursorPos;
            crumb.time = time;
            crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
            s_crumbs.push_back(crumb);
        }
    }

    int maxCrumbs = g_config.behaviors.breadCrumbs.maxCrumbs;
    while ((int)s_crumbs.size() > maxCrumbs) {
        s_crumbs.erase(s_crumbs.begin());
    }

    for (auto it = s_crumbs.begin(); it != s_crumbs.end(); ) {
        if (time - it->time > it->lifetime) {
            it = s_crumbs.erase(it);
        } else {
            ++it;
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.breadCrumbs) return;
    if (s_crumbs.empty()) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    if (!s_crumbImage) {
        float size = g_config.behaviors.breadCrumbs.size;
        for (const auto& crumb : s_crumbs) {
            float alpha = std::max(0.0f, 1.0f - (float)(ctx.time - crumb.time) / crumb.lifetime);
            CGContextSetRGBFillColor(cg, 0.9f, 0.7f, 0.4f, alpha * 0.8f);
            CGContextFillEllipseInRect(cg, CGRectMake(crumb.pos.x - size/2, crumb.pos.y - size/2, size, size));
        }
    } else {
        float imgWidth = (float)CGImageGetWidth(s_crumbImage);
        float imgHeight = (float)CGImageGetHeight(s_crumbImage);
        for (const auto& crumb : s_crumbs) {
            float alpha = std::max(0.0f, 1.0f - (float)(ctx.time - crumb.time) / crumb.lifetime);
            CGContextSetAlpha(cg, alpha);
            CGRect rect = CGRectMake(crumb.pos.x - imgWidth / 2.0f, crumb.pos.y - imgHeight / 2.0f, imgWidth, imgHeight);
            CGContextDrawImage(cg, rect, s_crumbImage);
        }
    }

    CGContextRestoreGState(cg);
}

static Behavior g_breadcrumbBehavior = {
    .id = "breadcrumbs",
    .name = "Bread Crumbs",
    .description = "Hold RightShift to drop breadcrumbs at cursor. Based on BreadCrumbs by Straaft",
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

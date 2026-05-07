// ===========================
// behavior_breadcrumbs.cpp
// Bread Crumbs Behavior - Leave trail of crumbs
// Based on BreadCrumbs by Straaft
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BreadCrumbState>(ctx.goose->id, "breadcrumbs");
    state->Reset();
    state->crumbs.resize(g_config.behaviors.breadCrumbs.maxCrumbs);
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.breadCrumbs) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<BreadCrumbState>(goose->id, "breadcrumbs");
    if (state->crumbs.empty()) return;

    float speed = std::hypot(goose->vel.x, goose->vel.y);
    if (speed < 10.0f) return;

    Vector2 lastPos{0, 0};
    bool foundLast = false;

    for (auto& crumb : state->crumbs) {
        if (crumb.active) {
            crumb.lifetime -= dt;
            if (crumb.lifetime <= 0) {
                crumb.active = false;
            }
        }
    }

    for (auto& crumb : state->crumbs) {
        if (crumb.active) {
            lastPos = crumb.pos;
            foundLast = true;
            break;
        }
    }

    float spawnDist = g_config.behaviors.breadCrumbs.spawnDist;
    float dist = foundLast ? std::hypot(goose->pos.x - lastPos.x, goose->pos.y - lastPos.y) : spawnDist + 1.0f;

    if (dist >= spawnDist) {
        for (auto& crumb : state->crumbs) {
            if (!crumb.active) {
                crumb.active = true;
                crumb.pos.x = goose->pos.x;
                crumb.pos.y = goose->pos.y;
                crumb.vel.x = goose->vel.x;
                crumb.vel.y = goose->vel.y;
                crumb.lifetime = g_config.behaviors.breadCrumbs.lifetime;
                break;
            }
        }
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.breadCrumbs) return;

    auto* state = BehaviorStateManager::Instance().GetOrCreate<BreadCrumbState>(goose->id, "breadcrumbs");

    CGContextRef ctx_ = (CGContextRef)renderCtx;
    if (!ctx_) return;

    float crumbSize = g_config.behaviors.breadCrumbs.size;

    for (auto& crumb : state->crumbs) {
        if (!crumb.active) continue;

        float alpha = std::min(1.0f, (float)crumb.lifetime / 2.0f);
        CGContextSetRGBFillColor(ctx_, 0.9f, 0.7f, 0.4f, alpha);
        CGContextFillEllipseInRect(ctx_, CGRectMake(
            crumb.pos.x - crumbSize/2, crumb.pos.y - crumbSize/2,
            crumbSize, crumbSize));
    }
}

static Behavior g_breadcrumbBehavior = {
    .id = "breadcrumbs",
    .name = "Bread Crumbs",
    .description = "Leave a trail of breadcrumbs. Based on BreadCrumbs by Straaft",
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
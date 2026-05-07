// ===========================
// behavior_nametag.cpp
// Nametag Behavior - Show goose name above head
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cstring>

static bool s_enabled = true;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "nametag");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.nametag) return;
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.info.nametag) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    const char* name = goose->name.empty() ? "Goose" : goose->name.c_str();

    CGContextSaveGState(cg);

    CGContextSetRGBFillColor(cg, 0.0f, 0.0f, 0.0f, 0.7f);

    float nameWidth = (float)strlen(name) * 8.0f;
    float boxHeight = 18.0f;
    Vector2 headPos = goose->rig.head2;

    float boxX = headPos.x - nameWidth / 2.0f - 4.0f;
    float boxY = headPos.y - 40.0f;

    CGRect bgRect = CGRectMake(boxX, boxY, nameWidth + 8.0f, boxHeight);
    CGContextFillRect(cg, bgRect);

    CGContextSetRGBFillColor(cg, 1.0f, 1.0f, 1.0f, 1.0f);

    CGContextSelectFont(cg, "Helvetica", 12.0f, kCGEncodingMacRoman);
    CGContextSetTextDrawingMode(cg, kCGTextFill);
    CGContextShowTextAtPoint(cg, headPos.x - nameWidth / 2.0f, headPos.y - 33.0f, name, strlen(name));

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_nametagBehavior = {
    .id = "nametag",
    .name = "Nametag",
    .description = "Shows the goose's name above its head",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_nametagBehavior);
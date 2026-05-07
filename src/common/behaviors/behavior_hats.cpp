// ===========================
// behavior_hats.cpp
// Hats Behavior - Put hats on geese
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>

static bool s_enabled = true;

struct HatStyle {
    float r, g, b;
    float hatRimWidth;
    float hatBodyHeight;
};

static const HatStyle STYLES[] = {
    {0.2f, 0.2f, 0.4f, 8.0f, 20.0f},   // Blue top hat
    {0.8f, 0.6f, 0.2f, 10.0f, 12.0f},   // Gold crown
    {0.4f, 0.8f, 0.4f, 6.0f, 16.0f},    // Green cap
    {0.9f, 0.1f, 0.1f, 7.0f, 14.0f},    // Red party hat
    {0.5f, 0.3f, 0.7f, 9.0f, 18.0f},    // Purple wizard
};

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "hats");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.fun.hats) return;
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.hats) return;

    CGContextRef ctx_ = (CGContextRef)renderCtx;
    if (!ctx_) return;

    const HatStyle& style = STYLES[goose->id % 5];

    float hatWidth = g_config.behaviors.hats.sizeX;
    float hatOffsetX = g_config.behaviors.hats.offsetX;
    float hatOffsetY = g_config.behaviors.hats.offsetY;

    Vector2 hatPos = goose->rig.head2 + Vector2{hatOffsetX, hatOffsetY};

    float dirRad = goose->dir * (float)(3.14159265358979323846 / 180.0);
    float cosA = std::cos(dirRad);
    float sinA = std::sin(dirRad);

    CGContextSaveGState(ctx_);
    CGContextTranslateCTM(ctx_, hatPos.x, hatPos.y);
    CGContextRotateCTM(ctx_, -dirRad);

    float halfW = hatWidth / 2.0f;
    float halfH = style.hatBodyHeight / 2.0f;

    CGContextSetRGBFillColor(ctx_, style.r, style.g, style.b, 1.0f);
    CGContextFillRect(ctx_, CGRectMake(-halfW, -halfH, hatWidth, style.hatBodyHeight));

    CGContextSetRGBFillColor(ctx_, style.r * 0.7f, style.g * 0.7f, style.b * 0.7f, 1.0f);
    CGContextFillRect(ctx_, CGRectMake(-halfW - 2, halfH - 4, hatWidth + 4, 4));

    CGContextSetRGBStrokeColor(ctx_, style.r * 0.5f, style.g * 0.5f, style.b * 0.5f, 1.0f);
    CGContextSetLineWidth(ctx_, 1.0f);
    CGContextStrokeRect(ctx_, CGRectMake(-halfW, -halfH, hatWidth, style.hatBodyHeight));

    CGContextRestoreGState(ctx_);
}

static Behavior g_hatsBehavior = {
    .id = "hats",
    .name = "Hats",
    .description = "Put hats on geese",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_hatsBehavior);
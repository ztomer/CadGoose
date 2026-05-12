#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include <CoreGraphics/CoreGraphics.h>
#include <cmath>

static bool s_enabled = true;
static CGImageRef s_hatImage = nullptr;

static void init(BehaviorContext& ctx) {
    if (!s_hatImage) {
        s_hatImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/hat_default.png");
    }
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.fun.hats) return;

    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;
    if (!s_hatImage) return;

    float hatSize = g_config.behaviors.hats.sizeX;
    float offsetX = g_config.behaviors.hats.offsetX;
    float offsetY = g_config.behaviors.hats.offsetY;
    float dir = goose->dir;
    float gs = ctx.globalScale;

    float imgWidth = (float)CGImageGetWidth(s_hatImage);
    float imgHeight = (float)CGImageGetHeight(s_hatImage);

    float scale = hatSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

    Vector2 headDevice = WorldCoord::RigNeckHead(*goose);
    float screenX = headDevice.x + offsetX * gs;
    float screenY = headDevice.y + offsetY * gs;

    CGContextSaveGState(cg);

    CGContextTranslateCTM(cg, screenX, screenY);
    CGContextRotateCTM(cg, -dir * M_PI / 180.0f);

    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    if (facingLeft) {
        CGContextScaleCTM(cg, -1.0, 1.0);
    }

    CGRect rect = CGRectMake(-drawW / 2.0f, -drawH / 2.0f, drawW, drawH);
    CGContextSaveGState(cg);
    CGContextTranslateCTM(cg, 0, drawH / 2.0f);
    CGContextScaleCTM(cg, 1.0, -1.0);
    CGContextTranslateCTM(cg, 0, drawH / 2.0f);
    CGContextDrawImage(cg, rect, s_hatImage);
    CGContextRestoreGState(cg);

    CGContextRestoreGState(cg);
}

static Behavior g_hatsBehavior = {
    .id = "hats",
    .name = "Hats",
    .description = "Put hats on geese. Based on HatGoos by DaNike",
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

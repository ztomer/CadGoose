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

    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    if (facingLeft) {
        CGContextScaleCTM(cg, -1.0, 1.0);
    }

    float halfW = drawW / 2.0f;
    float halfH = drawH / 2.0f;

    // Pre-render hat to bitmap at exact size to avoid Metal shader compilation
    if (drawW > 0 && drawH > 0) {
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef bmp = CGBitmapContextCreate(nullptr, (size_t)drawW, (size_t)drawH, 8, 0, cs, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
        CGColorSpaceRelease(cs);
        if (bmp) {
            CGContextDrawImage(bmp, CGRectMake(0, 0, drawW, drawH), s_hatImage);
            CGImageRef scaled = CGBitmapContextCreateImage(bmp);
            CGContextRelease(bmp);
            if (scaled) {
                CGContextTranslateCTM(cg, -halfW, halfH);
                CGContextScaleCTM(cg, 1.0, -1.0);
                CGContextDrawImage(cg, CGRectMake(0, 0, drawW, drawH), scaled);
                CGImageRelease(scaled);
            }
        }
    }
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

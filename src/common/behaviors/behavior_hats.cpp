#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "renderer_interface.h"
#include <cmath>

static constexpr float kFacingLeftMin = 90.0f;
static constexpr float kFacingLeftMax = 270.0f;

#ifdef __APPLE__
#include "cg_renderer.h"

static CGImageRef s_hatImage = nullptr;
static CGImageRef s_hatScaled = nullptr;
static float s_hatScaledSize = 0;

static void cleanupHat(BehaviorContext&) {
    if (s_hatScaled) { CGImageRelease(s_hatScaled); s_hatScaled = nullptr; }
}

static void init(BehaviorContext& ctx) {
    if (!s_hatImage) {
        s_hatImage = g_assets.GetBehaviorImage("Assets/Images/OtherGfx/hat_default.png");
    }
}
#else
static void cleanupHat(BehaviorContext&) {}
static void init(BehaviorContext& ctx) {}
#endif

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;
    if (!s_hatImage) return;

    CGRenderer renderer(cg);

    float hatSize = g_config.behaviors.hats.sizeX;
    float offsetX = g_config.behaviors.hats.offsetX;
    float offsetY = g_config.behaviors.hats.offsetY;
    float dir = goose->dir;

    float imgWidth = (float)CGImageGetWidth(s_hatImage);
    float imgHeight = (float)CGImageGetHeight(s_hatImage);

    float scale = hatSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

    if (s_hatScaledSize != hatSize) {
        if (s_hatScaled) { CGImageRelease(s_hatScaled); s_hatScaled = nullptr; }
        if (drawW > 0 && drawH > 0) {
            CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
            CGContextRef bmp = CGBitmapContextCreate(nullptr, (size_t)drawW, (size_t)drawH, 8, 0, cs, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
            CGColorSpaceRelease(cs);
            if (bmp) {
                CGContextDrawImage(bmp, CGRectMake(0, 0, drawW, drawH), s_hatImage);
                s_hatScaled = CGBitmapContextCreateImage(bmp);
                CGContextRelease(bmp);
                s_hatScaledSize = hatSize;
            }
        }
    }

    Vector2 headDevice = goose->rig.neckHead;
    float screenX = headDevice.x + offsetX;
    float screenY = headDevice.y + offsetY;

    renderer.SaveState();
    renderer.Translate(screenX, screenY);

    bool facingLeft = (dir > kFacingLeftMin && dir < kFacingLeftMax);
    if (facingLeft) {
        renderer.Scale(-1.0f, 1.0f);
    }

    float halfW = drawW / 2.0f;
    float halfH = drawH / 2.0f;

    if (s_hatScaled) {
        renderer.Translate(-halfW, halfH);
        renderer.Scale(1.0f, -1.0f);
        renderer.DrawImage(s_hatScaled, RenderRect{0, 0, drawW, drawH});
    }
    renderer.RestoreState();
#endif
}

static Behavior g_hatsBehavior = BEHAVIOR_DEF_CUSTOM(
    "hats", "Hats", "Put hats on geese. Based on HatGoos by DaNike",
    g_config.behaviors.fun.hats, init, tick, render, cleanupHat, false, false
);

REGISTER_BEHAVIOR(g_hatsBehavior);

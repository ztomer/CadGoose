#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "renderer_interface.h"
#include <cmath>
#include <cstring>

static constexpr float kFacingLeftMin = 90.0f;
static constexpr float kFacingLeftMax = 270.0f;

static void* s_hatImage = nullptr;

static void cleanupHat(BehaviorContext&) {}

static void init(BehaviorContext& ctx) {
    if (!s_hatImage) {
        s_hatImage = (void*)g_assets.GetBehaviorImage("Assets/Images/OtherGfx/hat_default.png");
    }
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    if (!irenderer || !s_hatImage) return;
    IRenderer& renderer = *irenderer;

    float headScale = (std::strcmp(goose->type(), "baby_stalin") == 0) ? 2.0f : 1.0f;
    float hatSize = g_config.behaviors.hats.sizeX * headScale;
    float offsetX = g_config.behaviors.hats.offsetX * headScale;
    float offsetY = g_config.behaviors.hats.offsetY * headScale;
    float dir = goose->dir;

    float imgWidth = 0, imgHeight = 0;
    if (!renderer.GetImageSize(s_hatImage, &imgWidth, &imgHeight) || imgWidth <= 0) return;

    float scale = hatSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

    // Position the hat centered above the goose's head.
    // The renderer's coordinate system has +Y going down (top-left origin via
    // the goose view's flipped CTM), so "above" means subtracting halfH from
    // the head Y. offsetY is applied as-is (config-author's convention).
    DevicePoint headDevice = WorldCoord::RigNeckHead(*goose);
    float halfW = drawW / 2.0f;
    float halfH = drawH / 2.0f;
    float drawX = headDevice.x + offsetX - halfW;
    float drawY = headDevice.y + offsetY - halfH;

    bool facingLeft = (dir > kFacingLeftMin && dir < kFacingLeftMax);
    if (facingLeft) {
        // Mirror around the hat's vertical center so the hat itself flips,
        // not just its position on screen. Also Y-flip for CoreGraphics.
        renderer.SaveState();
        renderer.Translate(headDevice.x + offsetX, drawY + drawH);
        renderer.Scale(-1.0f, -1.0f);
        renderer.DrawImage(s_hatImage, RenderRect{-halfW, 0, drawW, drawH});
        renderer.RestoreState();
    } else {
        renderer.SaveState();
        renderer.Translate(drawX, drawY + drawH);
        renderer.Scale(1.0f, -1.0f);
        renderer.DrawImage(s_hatImage, RenderRect{0, 0, drawW, drawH});
        renderer.RestoreState();
    }
}

static Behavior g_hatsBehavior = BEHAVIOR_DEF_CUSTOM(
    "hats", "Hats", "Put hats on geese. Based on HatGoos by DaNike",
    g_config.behaviors.fun.hats, init, tick, render, cleanupHat, false, false
);

REGISTER_BEHAVIOR(g_hatsBehavior);

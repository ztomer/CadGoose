#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "assets.h"
#include "renderer_interface.h"
#include <cmath>

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

    float hatSize = g_config.behaviors.hats.sizeX;
    float offsetX = g_config.behaviors.hats.offsetX;
    float offsetY = g_config.behaviors.hats.offsetY;
    float dir = goose->dir;

    float imgWidth = 0, imgHeight = 0;
    if (!renderer.GetImageSize(s_hatImage, &imgWidth, &imgHeight) || imgWidth <= 0) return;

    float scale = hatSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

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

    renderer.Translate(-halfW, halfH);
    renderer.Scale(1.0f, -1.0f);
    renderer.DrawImage(s_hatImage, RenderRect{0, 0, drawW, drawH});

    renderer.RestoreState();
}

static Behavior g_hatsBehavior = BEHAVIOR_DEF_CUSTOM(
    "hats", "Hats", "Put hats on geese. Based on HatGoos by DaNike",
    g_config.behaviors.fun.hats, init, tick, render, cleanupHat, false, false
);

REGISTER_BEHAVIOR(g_hatsBehavior);

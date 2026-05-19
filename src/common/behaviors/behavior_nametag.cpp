// ===========================
// behavior_nametag.cpp
// Nametag Behavior - Show goose name above head
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "render_colors.h"
#include <cstring>

static constexpr float kNametagCharWidth = 8.0f;
static constexpr float kNametagBoxHeight = 18.0f;
static constexpr float kNametagBoxXPad = 4.0f;
static constexpr float kNametagBoxYOffset = 40.0f;
static constexpr float kNametagBoxPad = 2.0f;
static constexpr float kNametagBoxWidthPad = 12.0f;
static constexpr float kNametagBoxHeightPad = 4.0f;
static constexpr float kNametagCornerRadius = 6.0f;
static constexpr float kNametagHighlightPad = 1.0f;
static constexpr float kNametagHighlightWidthPad = 10.0f;
static constexpr float kNametagHighlightCornerRadius = 4.0f;
static constexpr float kNametagHighlightAlpha = 0.08f;
static constexpr float kNametagTextYOffset = 25.0f;

static void cleanupNameFont(BehaviorContext&) {}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "nametag");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
    if (!irenderer || goose->name.empty()) return;
    IRenderer& renderer = *irenderer;
    renderer.SaveState();

    Vector2 headPos = goose->rig.neckHead;

    const char* name = goose->name.c_str();
    size_t nameLen = strlen(name);
    float fontSize = g_config.behaviors.nametag.size;

    float measured = renderer.MeasureText(name, fontSize);
    float nameWidth = measured > 0.0f ? measured : (float)nameLen * kNametagCharWidth;
    float boxHeight = kNametagBoxHeight;
    float boxX = headPos.x - nameWidth / 2.0f - kNametagBoxXPad;
    float boxY = headPos.y - kNametagBoxYOffset;

    // Liquid glass background
    renderer.DrawRoundedRect({boxX - kNametagBoxPad, boxY - kNametagBoxPad, nameWidth + kNametagBoxWidthPad, boxHeight + kNametagBoxHeightPad},
                             kNametagCornerRadius,
                             MakeNametagBg(0.85f));
    // Subtle highlight for liquid glass effect
    renderer.DrawRoundedRect({boxX - kNametagHighlightPad, boxY - kNametagHighlightPad, nameWidth + kNametagHighlightWidthPad, (boxHeight + kNametagBoxHeightPad) * 0.5f},
                             kNametagHighlightCornerRadius,
                             RenderColor{1.0f, 1.0f, 1.0f, kNametagHighlightAlpha});

    float textX = headPos.x - nameWidth / 2.0f;
    renderer.DrawText(name, {textX, headPos.y - kNametagTextYOffset}, RenderColor::White(), fontSize);

    renderer.RestoreState();
}

static Behavior g_nametagBehavior = BEHAVIOR_DEF_CUSTOM(
    "nametag", "Nametag", "Shows the goose's name above its head",
    g_config.behaviors.info.nametag, init, tick, render, cleanupNameFont, false, false
);

REGISTER_BEHAVIOR(g_nametagBehavior);

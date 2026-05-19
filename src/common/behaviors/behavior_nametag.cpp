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
#include "cg_renderer.h"
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

static CTFontRef s_nameFont = nullptr;
static CGColorRef s_nameWhite = nullptr;
static float s_nameFontSize = 0;

static void cleanupNameFont(BehaviorContext&) {
    if (s_nameFont) { CFRelease(s_nameFont); s_nameFont = nullptr; }
    if (s_nameWhite) { CGColorRelease(s_nameWhite); s_nameWhite = nullptr; }
    s_nameFontSize = 0;
}

static void ensureNameFont(float fontSize) {
    if (s_nameFont && s_nameFontSize == fontSize) return;
    if (s_nameFont) { CFRelease(s_nameFont); s_nameFont = nullptr; }
    s_nameFont = CTFontCreateWithName(CFSTR("Helvetica"), fontSize, NULL);
    s_nameFontSize = fontSize;
    if (!s_nameWhite) s_nameWhite = CGColorCreateGenericRGB(1.0f, 1.0f, 1.0f, 1.0f);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "nametag");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
}

static void render(Goose* goose, BehaviorContext& ctx, IRenderer* irenderer) {
#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)(irenderer ? irenderer->nativeContext() : nullptr);
    if (!cg) return;

    if (goose->name.empty()) return;

    CGRenderer renderer(cg);
    renderer.SaveState();

    Vector2 headPos = goose->rig.neckHead;

    const char* name = goose->name.c_str();
    size_t nameLen = strlen(name);

    float nameWidth = (float)nameLen * kNametagCharWidth;
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

    float fontSize = g_config.behaviors.nametag.size;
    ensureNameFont(fontSize);
    if (s_nameFont && s_nameWhite) {
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { s_nameFont, s_nameWhite };
        CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        CGContextSetTextMatrix(cg, CGAffineTransformMakeScale(1.0, -1.0));

        CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)name, nameLen, kCFStringEncodingUTF8, false);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);

        if (line) {
            // Get actual text width for precise centering
            CFIndex stringLength = CTLineGetStringRange(line).length;
            double textWidth = CTLineGetOffsetForStringIndex(line, stringLength, NULL);

            float boxCenterX = headPos.x;
            float textX = boxCenterX - (float)textWidth / 2.0f;
            CGContextSetTextPosition(cg, textX, headPos.y - kNametagTextYOffset);
            CTLineDraw(line, cg);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attributes);
    }

    renderer.RestoreState();
#endif
}

static Behavior g_nametagBehavior = BEHAVIOR_DEF_CUSTOM(
    "nametag", "Nametag", "Shows the goose's name above its head",
    g_config.behaviors.info.nametag, init, tick, render, cleanupNameFont, false, false
);

REGISTER_BEHAVIOR(g_nametagBehavior);

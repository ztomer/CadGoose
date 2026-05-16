// ===========================
// behavior_nametag.cpp
// Nametag Behavior - Show goose name above head
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "renderer_interface.h"
#include "cg_renderer.h"
#include <cstring>

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

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    if (goose->name.empty()) return;

    CGRenderer renderer(cg);
    renderer.SaveState();

    Vector2 headPos = goose->rig.neckHead;

    const char* name = goose->name.c_str();
    size_t nameLen = strlen(name);

    float nameWidth = (float)nameLen * 8.0f;
    float boxHeight = 18.0f;
    float boxX = headPos.x - nameWidth / 2.0f - 4.0f;
    float boxY = headPos.y - 40.0f;

    // Liquid glass background
    renderer.DrawRoundedRect({boxX - 2.0f, boxY - 2.0f, nameWidth + 12.0f, boxHeight + 4.0f},
                             6.0f,
                             RenderColor{0.15f, 0.15f, 0.15f, 0.85f});
    // Subtle highlight for liquid glass effect
    renderer.DrawRoundedRect({boxX - 1.0f, boxY - 1.0f, nameWidth + 10.0f, (boxHeight + 4.0f) * 0.5f},
                             4.0f,
                             RenderColor{1.0f, 1.0f, 1.0f, 0.08f});

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
            CGContextSetTextPosition(cg, headPos.x - nameWidth / 2.0f, headPos.y - 25.0f);
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

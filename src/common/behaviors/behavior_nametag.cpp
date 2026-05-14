// ===========================
// behavior_nametag.cpp
// Nametag Behavior - Show goose name above head
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
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

    if (goose->name.empty()) return;

    Vector2 headPos = WorldCoord::RigNeckHead(*goose);

    const char* name = goose->name.c_str();
    size_t nameLen = strlen(name);

    CGContextSaveGState(cg);

    float nameWidth = (float)nameLen * 8.0f;
    float boxHeight = 18.0f;
    float boxX = headPos.x - nameWidth / 2.0f - 4.0f;
    float boxY = headPos.y - 40.0f;
    
    CGContextSetRGBFillColor(cg, 0.2f, 0.2f, 0.2f, 0.9f);
    CGContextFillRect(cg, CGRectMake(boxX - 2.0f, boxY - 2.0f, nameWidth + 12.0f, boxHeight + 4.0f));
    
    CGContextSetRGBFillColor(cg, 1.0f, 1.0f, 1.0f, 1.0f);

    float fontSize = g_config.behaviors.nametag.size;
    CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica"), fontSize, NULL);
    if (font) {
        CGColorRef white = CGColorCreateGenericRGB(1.0f, 1.0f, 1.0f, 1.0f);
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { font, white };
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
        CGColorRelease(white);
        CFRelease(font);
    }

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_nametagBehavior = {
    .id = "nametag",
    .name = "Nametag",
    .description = "Shows the goose's name above its head",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.info.nametag,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_nametagBehavior);

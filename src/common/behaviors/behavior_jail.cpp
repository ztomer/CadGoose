// ===========================
// behavior_jail.cpp
// Jail Behavior - Trap the goose in a cage
// Based on GooseJail by WackyModer
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "hotkey.h"
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#endif

static bool s_enabled = true;
static bool s_oWasKeyDown = false;
static bool s_pWasKeyDown = false;

static bool IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.control.jail) {
        auto* state = BehaviorStateManager::Instance().Get<JailState>(goose->id, "jail");
        if (state) state->isJailed = false;
        return;
    }

    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    Vector2 cursorPos{-1, -1};
    if (g_cursorProvider) {
        CursorState cs = g_cursorProvider->Read();
        if (cs.hasPos()) {
            cursorPos = cs.position;
        }
    }

    bool oDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO));
    if (oDown && !s_oWasKeyDown) {
        state->jailPos = cursorPos;
        state->positionSet = true;
    }
    s_oWasKeyDown = oDown;

    bool pDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP));
    if (pDown && !s_pWasKeyDown) {
        state->isJailed = !state->isJailed;
        goose->state = GooseState::WANDER;
        g_assets.Honk();
    }
    s_pWasKeyDown = pDown;

    if (state->isJailed && state->positionSet) {
        goose->target = state->jailPos;
        goose->pos = state->jailPos;
        goose->vel = {0, 0};
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.control.jail) return;

    auto* state = BehaviorStateManager::Instance().Get<JailState>(goose->id, "jail");
    if (!state || !state->positionSet) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float jailSize = g_config.behaviors.jail.size;
    CGRect rect = CGRectMake(state->jailPos.x - jailSize/2, state->jailPos.y - jailSize/2, jailSize, jailSize);

    // Pulsing alpha for visibility
    float pulse = 0.6f + 0.4f * sin(ctx.time * 3.0f);
    bool jailed = state->isJailed;

    // Draw cage
    CGContextSetRGBStrokeColor(cg, 1.0f, 0.6f, 0.0f, jailed ? 1.0f : pulse * 0.6f);
    CGContextSetLineWidth(cg, jailed ? 4.0f : 2.0f);
    CGContextStrokeRect(cg, rect);

    CGContextSetRGBFillColor(cg, 1.0f, 0.6f, 0.0f, jailed ? 0.2f : 0.05f);
    CGContextFillRect(cg, rect);

    // Draw "JAIL" or "SET" label
    const char* label = jailed ? "JAIL" : "SET";
    CTFontRef font = CTFontCreateWithName(CFSTR("Helvetica-Bold"), 14.0f, NULL);
    if (font) {
        CGColorRef textColor = CGColorCreateGenericRGB(1.0f, 0.6f, 0.0f, jailed ? 1.0f : pulse * 0.6f);
        CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
        CFTypeRef values[] = { font, textColor };
        CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

        CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)label, strlen(label), kCFStringEncodingUTF8, false);
        CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
        CTLineRef line = CTLineCreateWithAttributedString(attrStr);

        if (line) {
            float textX = state->jailPos.x - jailSize/2;
            float textY = state->jailPos.y - jailSize/2 - 20.0f;
            CGContextSaveGState(cg);
            CGContextTranslateCTM(cg, textX, textY);
            CGContextScaleCTM(cg, 1.0, -1.0);
            CGContextSetTextPosition(cg, 0, 0);
            CTLineDraw(line, cg);
            CGContextRestoreGState(cg);
            CFRelease(line);
        }

        CFRelease(attrStr);
        CFRelease(string);
        CFRelease(attributes);
        CGColorRelease(textColor);
        CFRelease(font);
    }
#endif
}

static Behavior g_jailBehavior = {
    .id = "jail",
    .name = "Jail",
    .description = "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    .enabledPtr = &s_enabled,
    .configPtr = &g_config.behaviors.control.jail,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = true }
};

REGISTER_BEHAVIOR(g_jailBehavior);
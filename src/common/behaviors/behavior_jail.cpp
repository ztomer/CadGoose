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
#include "ring_buffer.h"
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#endif

static bool s_oWasKeyDown = false;
static bool s_pWasKeyDown = false;
static constexpr size_t kMaxJails = 10;
static RingBuffer<Vector2, kMaxJails> s_jails;
static bool s_jailsActive = false;
static double s_lastInputTime = 0;

#ifdef __APPLE__
static CTFontRef s_jailFont = nullptr;
static void cleanupJailFont() {
    if (s_jailFont) { CFRelease(s_jailFont); s_jailFont = nullptr; }
}
#endif

static bool IsKeyPressed(int keyCode) {
    return CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, (CGKeyCode)keyCode);
}

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(ctx.goose->id, "jail");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<JailState>(goose->id, "jail");

    if (!g_config.behaviors.control.jail) {
        state->isJailed = false;
        s_jailsActive = false;
        s_jails.clear();
        return;
    }

    if (time > s_lastInputTime) {
        s_lastInputTime = time;

        Vector2 cursorPos{-1, -1};
        if (g_cursorProvider) {
            CursorState cs = g_cursorProvider->Read();
            if (cs.hasPos()) {
                cursorPos = cs.position;
            }
        }

        bool oDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyO));
        if (oDown && !s_oWasKeyDown) {
            if (s_jailsActive) {
                // Reset functionality: if active, placing a new trap clears the old ones
                s_jails.clear();
                s_jailsActive = false;
            }
            s_jails.push(cursorPos);
        }
        s_oWasKeyDown = oDown;

        bool pDown = IsKeyPressed(KeyNameToKeyCode(g_config.behaviors.jail.hotkeyP));
        if (pDown && !s_pWasKeyDown && !s_jails.empty()) {
            s_jailsActive = !s_jailsActive;
            g_assets.Honk();
        }
        s_pWasKeyDown = pDown;
    }

    state->isJailed = s_jailsActive && !s_jails.empty();
    ctx.isJailed = state->isJailed;

    if (state->isJailed) {
        // Find nearest trap
        Vector2 nearest = s_jails[0];
        float minDist = Vector2::Distance(goose->pos, nearest);
        for (size_t i = 1; i < s_jails.size(); ++i) {
            float d = Vector2::Distance(goose->pos, s_jails[i]);
            if (d < minDist) {
                minDist = d;
                nearest = s_jails[i];
            }
        }
        
        goose->target = nearest;
        goose->pos = nearest;
        goose->vel = {0, 0};
        goose->state = GooseState::WANDER;
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (goose->id != 0 || s_jails.empty()) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    float jailSize = g_config.behaviors.jail.size;
    float pulse = 0.6f + 0.4f * sin(ctx.time * 3.0f);
    bool jailed = s_jailsActive;

    for (const auto& jailPos : s_jails) {
        CGRect rect = CGRectMake(jailPos.x - jailSize/2, jailPos.y - jailSize/2, jailSize, jailSize);

        CGContextSetRGBStrokeColor(cg, 1.0f, 0.6f, 0.0f, jailed ? 1.0f : pulse * 0.6f);
        CGContextSetLineWidth(cg, jailed ? 4.0f : 2.0f);
        CGContextStrokeRect(cg, rect);

        CGContextSetRGBFillColor(cg, 1.0f, 0.6f, 0.0f, jailed ? 0.2f : 0.05f);
        CGContextFillRect(cg, rect);

        const char* label = jailed ? "JAIL" : "SET";
        if (!s_jailFont) s_jailFont = CTFontCreateWithName(CFSTR("Helvetica-Bold"), 14.0f, NULL);
        if (s_jailFont) {
            CGColorRef textColor = CGColorCreateGenericRGB(1.0f, 0.6f, 0.0f, jailed ? 1.0f : pulse * 0.6f);
            CFTypeRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
            CFTypeRef values[] = { s_jailFont, textColor };
            CFDictionaryRef attributes = CFDictionaryCreate(NULL, (const void**)keys, (const void**)values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

            CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)label, strlen(label), kCFStringEncodingUTF8, false);
            CFAttributedStringRef attrStr = CFAttributedStringCreate(NULL, string, attributes);
            CTLineRef line = CTLineCreateWithAttributedString(attrStr);

            if (line) {
                float textX = jailPos.x - jailSize/2;
                float textY = jailPos.y - jailSize/2 - 20.0f;
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
        }
    }
#endif
}

static Behavior g_jailBehavior = BEHAVIOR_DEF_CUSTOM(
    "jail", "Jail", "Press O to set jail position, P to trap goose. Based on GooseJail by WackyModer",
    g_config.behaviors.control.jail, init, tick, render,
    [](BehaviorContext&) { cleanupJailFont(); }, true, false
);

REGISTER_BEHAVIOR(g_jailBehavior);
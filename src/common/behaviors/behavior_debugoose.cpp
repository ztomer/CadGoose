// ===========================
// behavior_debugoose.cpp
// Debugoose Behavior - Debug overlay
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <cstring>
#include <vector>
#include <string>

static bool s_enabled = true;
static const int MAX_LOGS = 10;
static std::vector<std::string> s_recentLogs;

static void init(BehaviorContext& ctx) {
    auto* state = BehaviorStateManager::Instance().GetOrCreate<BehaviorState>(ctx.goose->id, "debugoose");
    state->Reset();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.debugoose) return;

    char buf[256];
    snprintf(buf, sizeof(buf), "T=%.1f S=%d P=(%.0f,%.0f) V=%.0f",
             time, (int)goose->state, goose->pos.x, goose->pos.y, goose->currentSpeed);
    s_recentLogs.push_back(buf);
    if ((int)s_recentLogs.size() > MAX_LOGS) {
        s_recentLogs.erase(s_recentLogs.begin());
    }
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.info.debugoose) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    int logCount = (int)s_recentLogs.size();
    CGContextSetRGBFillColor(cg, 0.1f, 0.1f, 0.1f, 0.8f);
    CGContextFillRect(cg, CGRectMake(10, 10, 300, (float)(logCount * 20 + 30)));

    CTFontRef font = CTFontCreateWithName(CFSTR("Courier"), 12.0f, NULL);
    if (font) {
        CGContextSetRGBFillColor(cg, 0.0f, 1.0f, 0.0f, 1.0f);

        for (int i = 0; i < logCount; i++) {
            const std::string& log = s_recentLogs[i];
            CFStringRef string = CFStringCreateWithBytes(NULL, (const UInt8*)log.c_str(), log.length(), kCFStringEncodingUTF8, false);
            CTLineRef line = CTLineCreateWithAttributedString(CFAttributedStringCreate(NULL, string, NULL));

            if (line) {
                CGContextSetTextPosition(cg, 20, 25 + i * 20);
                CTLineDraw(line, cg);
                CFRelease(line);
            }
            CFRelease(string);
        }
        CFRelease(font);
    }

    CGContextRestoreGState(cg);
#endif
}

static Behavior g_debugooseBehavior = {
    .id = "debugoose",
    .name = "Debugoose",
    .description = "Shows debug overlay with recent log entries and goose state",
    .enabledPtr = &s_enabled,
    .init = init,
    .tick = tick,
    .render = render,
    .cleanup = nullptr,
    .conflicts = nullptr,
    .priority = 0,
    .config = { .requiresAccessibility = false, .isStarter = false }
};

REGISTER_BEHAVIOR(g_debugooseBehavior);

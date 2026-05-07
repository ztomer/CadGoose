// ===========================
// behavior_debugoose.cpp
// Debugoose Behavior - Debug overlay with log entries
// ===========================
#include "behavior.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include <CoreGraphics/CoreGraphics.h>
#include <vector>
#include <string>
#include <deque>

static bool s_enabled = true;
static std::deque<std::string> s_recentLogs;
static constexpr size_t MAX_LOGS = 10;

void Debugoose_Log(const char* message) {
    s_recentLogs.push_back(message);
    if (s_recentLogs.size() > MAX_LOGS) {
        s_recentLogs.pop_front();
    }
}

static void init(BehaviorContext& ctx) {
    s_recentLogs.clear();
}

static void tick(Goose* goose, BehaviorContext& ctx, double dt, double time) {
    if (!g_config.behaviors.info.debugoose) {
        s_recentLogs.clear();
        return;
    }

    char buf[256];
    snprintf(buf, sizeof(buf), "State: %d, Speed: %.0f", (int)goose->state, goose->currentSpeed);
    Debugoose_Log(buf);
}

static void render(Goose* goose, BehaviorContext& ctx, void* renderCtx) {
    if (!g_config.behaviors.info.debugoose) return;

#ifdef __APPLE__
    CGContextRef cg = (CGContextRef)renderCtx;
    if (!cg) return;

    CGContextSaveGState(cg);

    CGContextSetRGBFillColor(cg, 0.1f, 0.1f, 0.1f, 0.8f);
    CGContextFillRect(cg, CGRectMake(10, 10, 300, (float)(s_recentLogs.size() * 20 + 30)));

    CGContextSetRGBFillColor(cg, 0.0f, 1.0f, 0.0f, 1.0f);
    CGContextSelectFont(cg, "Courier", 12.0f, kCGEncodingMacRoman);
    CGContextSetTextDrawingMode(cg, kCGTextFill);

    int i = 0;
    for (const auto& log : s_recentLogs) {
        CGContextShowTextAtPoint(cg, 20, 25 + i * 20, log.c_str(), log.length());
        i++;
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
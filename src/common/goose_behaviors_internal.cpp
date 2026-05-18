// goose_behaviors_internal.cpp
// Internal state management and helper functions
#include "goose.h"
#include "world.h"
#include "config.h"
#include "goose_math.h"
#include "event_bus.h"
#include <cstdio>

static inline double Rand01() { return static_cast<double>(rand() % 1000) / 1000.0; }

static FILE* GetDebugLog() {
    static FILE* f = nullptr;
    if (!f) {
        f = fopen("/tmp/goose_debug.log", "a");
        if (!f) f = stderr;
    }
    return f;
}

Vector2 GetSnatchForward(float dir, const Vector2& isoScale) {
    float rad = dir * DEG_TO_RAD;
    return {std::cos(rad) * isoScale.x, std::sin(rad) * isoScale.y};
}

void triggerHonk(Goose& g, double time, double cd, double& lastBucket) {
    auto& hs = g.honkState;
    if ((time - hs.lastAny) < g_config.honk.minGap) return;
    if ((time - lastBucket) < cd) return;
    g_assets.Honk();
    hs.lastAny = time;
    lastBucket = time;
    EventBus::Instance().Publish(GooseHonkedEvent{g.id, g.pos.x, g.pos.y, time});
}

void initHonkState(Goose::HonkState& hs, double time) {
    if (hs.init) return;
    hs.init = true;
    hs.lastAny = -1e9;
    hs.lastChase = -1e9;
    hs.lastFetch = -1e9;
    hs.lastGeneric = -1e9;
    hs.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
}

void updateIdleHonk(Goose& g, double time, double cd, double& lastGeneric) {
    auto& hs = g.honkState;
    if (hs.nextIdleHonk > time + g_config.honk.idleCheckAhead) return;
    triggerHonk(g, time, cd, lastGeneric);
    hs.nextIdleHonk = time + g_config.honk.idleMin + Rand01() * (g_config.honk.idleMax - g_config.honk.idleMin);
}

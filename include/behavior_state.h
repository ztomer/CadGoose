#ifndef BEHAVIOR_STATE_H
#define BEHAVIOR_STATE_H

#include <vector>
#include "goose_math.h"
#include "event_bus.h"
#include "world.h"

struct BehaviorContext {
    Goose* goose = nullptr;
    double time = 0;
    float globalScale = 1.0f;
    bool isJailed = false;
    void* stats = nullptr;
    IRenderer* renderer = nullptr; // Platform-agnostic rendering interface
    WorldContext* world = nullptr; // injected per tick; non-null inside Goose::tick / behavior dispatch
};

struct BehaviorStats {
    int activeCount = 0;
    int errorCount = 0;
    double lastTickTime = 0;
};

struct BehaviorState {
    virtual ~BehaviorState() = default;
    virtual void Reset() {}
};

#endif

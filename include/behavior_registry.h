#ifndef BEHAVIOR_REGISTRY_H
#define BEHAVIOR_REGISTRY_H

#include <vector>
#include <functional>
#include "behavior_state.h"
#include "renderer_interface.h"

struct Behavior {
    const char* id;
    const char* name;
    const char* description;

    bool* enabledPtr;
    bool* configPtr = nullptr; // optional config bool to sync with enabledPtr (socket toggle)

    using InitFunc = std::function<void(BehaviorContext&)>;
    using TickFunc = std::function<void(Goose*, BehaviorContext&, double dt, double time)>;
    using RenderFunc = std::function<void(Goose*, BehaviorContext&, IRenderer* renderer)>;
    using CleanupFunc = std::function<void(BehaviorContext&)>;

    InitFunc init;
    TickFunc tick;
    RenderFunc render;
    CleanupFunc cleanup;

    bool renderOnGround = false;
    const char** conflicts = nullptr;
    int priority = 0;

    struct Config {
        bool requiresAccessibility = false;
        bool requiresNetwork = false;
        bool isStarter = false;
    } config;
};

class BehaviorRegistry {
public:
    static BehaviorRegistry& Instance() {
        static BehaviorRegistry inst;
        return inst;
    }

    void Register(Behavior& behavior);

    void InitAll(Goose* goose);
    void TickAll(Goose* goose, double dt, double time);
    void RenderAll(Goose* goose, IRenderer* renderer);
    void RenderPass(Goose* goose, IRenderer* renderer, bool groundPass);
    void CleanupAll(Goose* goose);

    Behavior* Get(const char* id);
    const std::vector<Behavior*>& GetAll() const { return behaviors; }
    size_t GetBehaviorCount() const { return behaviors.size(); }

    void Clear();

private:
    std::vector<Behavior*> behaviors;
};

#define REGISTER_BEHAVIOR(b) \
    static void _register_##b##_helper() { \
        BehaviorRegistry::Instance().Register(b); \
    } \
    static struct _register_##b##_ctor { \
        _register_##b##_ctor() { _register_##b##_helper(); } \
    } _register_##b##_instance;

// Centralized behavior constructor — enabledPtr and configPtr always
// point to the same config bool, preventing the toggle-desync bug.
#define BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag) \
    Behavior { \
        .id = bid, \
        .name = bname, \
        .description = bdesc, \
        .enabledPtr = &configBool, \
        .configPtr = &configBool, \
        .init = initFn, \
        .tick = tickFn, \
        .render = renderFn, \
        .cleanup = cleanupFn, \
        .renderOnGround = groundFlag, \
        .conflicts = nullptr, \
        .priority = 0, \
        .config = { .requiresAccessibility = false, .isStarter = starterFlag } \
    }

#define BEHAVIOR_DEF(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, false, false)

#define BEHAVIOR_DEF_STARTER(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, true, false)

#define BEHAVIOR_DEF_GROUND(bid, bname, bdesc, configBool, initFn, tickFn, renderFn) \
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, nullptr, false, true)

#define BEHAVIOR_DEF_CUSTOM(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag) \
    BEHAVIOR_DEF_FULL(bid, bname, bdesc, configBool, initFn, tickFn, renderFn, cleanupFn, starterFlag, groundFlag)

#define BEHAVIOR_ENABLED(goose, name) \
    ([]() { \
        auto* _b = BehaviorRegistry::Instance().Get(name); \
        return _b && *_b->enabledPtr && (goose) && (goose)->behaviorsEnabled; \
    }())

#endif

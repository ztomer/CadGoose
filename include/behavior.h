// ===========================
// behavior.h
// Multi-Entity Behavior System with per-instance state management
// ===========================
#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <shared_mutex>

#include "goose_math.h"

struct Goose;

struct BehaviorContext {
    Goose* goose = nullptr;
    double time = 0;
    float globalScale = 1.0f;
    bool isJailed = false;
    void* stats = nullptr;
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

struct JailState : public BehaviorState {
    bool isJailed = false;

    void Reset() override {
        isJailed = false;
    }
};

struct BallState : public BehaviorState {
    enum class BallType : int {
        Generic = 0,
        Soccer = 1,
        Beach = 2
    };

    struct Ball {
        Vector2 pos{0, 0};
        Vector2 vel{0, 0};
        float radius = 25.0f;
        bool active = true;
        BallType type = BallType::Generic;
        int currentFrame = 0;

        float r = 0.3f, g = 0.5f, b = 0.9f;
        float strokeR = 0.2f, strokeG = 0.4f, strokeB = 0.7f;
        float strokeWidth = 2.0f;
        bool hasPattern = false;
    };
    std::vector<Ball> balls;

    void Reset() override {
        for (auto& ball : balls) {
            ball.active = false;
        }
    }

    static Ball CreateSoccerBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Soccer;
        ball.r = 1.0f; ball.g = 1.0f; ball.b = 1.0f;
        ball.strokeR = 0.15f; ball.strokeG = 0.15f; ball.strokeB = 0.15f;
        ball.strokeWidth = 2.0f;
        ball.hasPattern = true;
        return ball;
    }

    static Ball CreateBeachBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Beach;
        ball.r = 1.0f; ball.g = 0.9f; ball.b = 0.6f;
        ball.strokeR = 0.9f; ball.strokeG = 0.7f; ball.strokeB = 0.3f;
        ball.strokeWidth = 1.0f;
        ball.hasPattern = true;
        return ball;
    }

    static Ball CreateGenericBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Generic;
        ball.r = 0.3f; ball.g = 0.5f; ball.b = 0.9f;
        ball.strokeR = 0.2f; ball.strokeG = 0.4f; ball.strokeB = 0.7f;
        ball.strokeWidth = 2.0f;
        ball.hasPattern = false;
        return ball;
    }
};

struct AcidState : public BehaviorState {
    float rotationAccumulator = 0.0f;
    double lastHonkTime = 0;
    bool isSpinning = false;

    void Reset() override {
        rotationAccumulator = 0.0f;
        lastHonkTime = 0;
        isSpinning = false;
    }
};

struct HealthState : public BehaviorState {
    float currentHealth = 100.0f;
    float maxHealth = 100.0f;
    double lastDamageTime = 0;
    float regenAccumulator = 0.0f;
    bool isDead = false;

    void Reset() override {
        currentHealth = 100.0f;
        maxHealth = 100.0f;
        lastDamageTime = 0;
        regenAccumulator = 0.0f;
        isDead = false;
    }
};

struct DragState : public BehaviorState {
    bool isDragging = false;
    float dragAnchorX = 0, dragAnchorY = 0;
    double dragStartTime = 0;
    float resistanceChance = 0.0f;

    void Reset() override {
        isDragging = false;
        dragAnchorX = 0;
        dragAnchorY = 0;
        dragStartTime = 0;
        resistanceChance = 0.0f;
    }
};

enum class PomodoroPhase : int {
    Work = 0,
    Break = 1,
    LongBreak = 2
};

struct PomodoroState : public BehaviorState {
    PomodoroPhase phase = PomodoroPhase::Work;
    double phaseStartTime = 0;
    int completedSessions = 0;
    double lastHonkTime = 0;
    bool isAggressive = false;
    float accumulatedRotation = 0.0f;
    bool speedMultiplierApplied = false;

    void Reset() override {
        phase = PomodoroPhase::Work;
        phaseStartTime = 0;
        completedSessions = 0;
        lastHonkTime = 0;
        isAggressive = false;
        accumulatedRotation = 0.0f;
        speedMultiplierApplied = false;
    }

    int GetPhaseDurationMinutes() const {
        switch (phase) {
            case PomodoroPhase::Work: return 25;
            case PomodoroPhase::Break: return 5;
            case PomodoroPhase::LongBreak: return 15;
        }
        return 25;
    }
};

struct PortalState : public BehaviorState {
    struct Portal {
        float x = 0, y = 0;
        bool active = false;
        int portalId = 0;
        float width = 80.0f;
        float height = 80.0f;
    };
    Portal portalA;
    Portal portalB;
    bool portalsEnabled = true;
    bool justTeleported = false;

    void Reset() override {
        portalA.active = false;
        portalB.active = false;
        portalsEnabled = true;
        justTeleported = false;
    }
};

struct BreadCrumbState : public BehaviorState {
    struct Crumb {
        Vector2 pos{0, 0};
        Vector2 vel{0, 0};
        float lifetime = 0;
        bool active = true;
    };
    std::vector<Crumb> crumbs;
    double lastThrowTime = 0;
    bool isThrowing = false;

    void Reset() override {
        for (auto& crumb : crumbs) {
            crumb.active = false;
        }
        lastThrowTime = 0;
        isThrowing = false;
    }
};

struct AngerState : public BehaviorState {
    float angerLevel = 0.0f;
    double lastAngerIncrease = 0;
    double lastPunchTime = 0;
    bool isPunching = false;

    void Reset() override {
        angerLevel = 0.0f;
        lastAngerIncrease = 0;
        lastPunchTime = 0;
        isPunching = false;
    }
};

struct RainbowState : public BehaviorState {
    float hue = 0.0f;
    double lastUpdate = 0;

    void Reset() override {
        hue = 0.0f;
        lastUpdate = 0;
    }
};

struct HonckerState : public BehaviorState {
    double lastHonkTime = 0;
    double lastShowTime = 0;
    bool isOnCooldown = false;
    bool visible = false;

    void Reset() override {
        lastHonkTime = 0;
        lastShowTime = 0;
        isOnCooldown = false;
        visible = false;
    }
};

struct BanishState : public BehaviorState {
    float fadeProgress = 0.0f;
    Vector2 originalPos{0, 0};

    void Reset() override {
        fadeProgress = 0.0f;
        originalPos = {0, 0};
    }
};

class BehaviorStateManager {
public:
    static BehaviorStateManager& Instance() {
        static BehaviorStateManager inst;
        return inst;
    }

    template<typename T>
    T* GetOrCreate(int gooseId, const char* behaviorId) {
        int key = MakeKey(gooseId, behaviorId);
        std::unique_lock<std::shared_mutex> lock(mutex_);

        auto it = states.find(key);
        if (it != states.end()) {
            T* ptr = dynamic_cast<T*>(it->second.get());
            if (ptr) return ptr;
            states.erase(it);
        }
        auto state = std::make_unique<T>();
        T* ptr = state.get();
        states[key] = std::move(state);
        return ptr;
    }

    template<typename T>
    T* Get(int gooseId, const char* behaviorId) {
        int key = MakeKey(gooseId, behaviorId);
        std::shared_lock<std::shared_mutex> lock(mutex_);

        auto it = states.find(key);
        if (it != states.end()) {
            return dynamic_cast<T*>(it->second.get());
        }
        return nullptr;
    }

    bool Has(int gooseId, const char* behaviorId) {
        return Get<BehaviorState>(gooseId, behaviorId) != nullptr;
    }

    void RemoveForGoose(int gooseId) {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        for (auto it = states.begin(); it != states.end(); ) {
            int storedId = (it->first >> 16);
            if (storedId == gooseId) {
                it = states.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ClearAll() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        states.clear();
    }

    size_t GetStateCount() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return states.size();
    }

private:
    BehaviorStateManager() = default;

    int MakeKey(int gooseId, const char* behaviorId) {
        size_t hash = std::hash<std::string>{}(behaviorId);
        return (gooseId << 16) | static_cast<int>(hash & 0xFFFF);
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<int, std::unique_ptr<BehaviorState>> states;
};

struct Behavior {
    const char* id;
    const char* name;
    const char* description;

    bool* enabledPtr;
    bool* configPtr = nullptr; // optional config bool to sync with enabledPtr (socket toggle)

    using InitFunc = std::function<void(BehaviorContext&)>;
    using TickFunc = std::function<void(Goose*, BehaviorContext&, double dt, double time)>;
    using RenderFunc = std::function<void(Goose*, BehaviorContext&, void* ctx)>;
    using CleanupFunc = std::function<void(BehaviorContext&)>;

    InitFunc init;
    TickFunc tick;
    RenderFunc render;
    CleanupFunc cleanup;

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
    void RenderAll(Goose* goose, void* ctx);
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

#define BEHAVIOR_ENABLED(goose, name) \
    ([]() { \
        auto* _b = BehaviorRegistry::Instance().Get(name); \
        return _b && *_b->enabledPtr && (goose) && (goose)->behaviorsEnabled; \
    }())

// API functions for behaviors
struct Goose;
float Rainbow_GetHue(int gooseId);
void Rainbow_SetHue(int gooseId, float hue);
void Health_Damage(Goose* goose, float amount, double time);
void Health_Heal(Goose* goose, float amount);
void Honcker_Honk(Goose* goose, double time);

#endif // BEHAVIOR_H
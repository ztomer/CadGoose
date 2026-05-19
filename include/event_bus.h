#pragma once

// Lightweight event bus for decoupled behavior signaling.
// Behaviors publish events; other behaviors subscribe without direct coupling.

#include <functional>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <shared_mutex>

#include "goose_math.h"

// ============================================================
// Event types
// ============================================================

enum class EventType : uint8_t {
    GooseHonked = 0,
    GooseDamaged = 1,
    ItemDropped = 2,
    ItemEaten = 3,
    GooseJailed = 4,
    GooseFreed = 5,
    PomodoroPhaseChanged = 6,
    GooseStuck = 7,
    CursorFastMove = 8,
    ToySpawned = 9,
    BallKicked = 10,
    BreadcrumbDropped = 11,
    GooseTeleported = 12,
    COUNT
};

struct GooseHonkedEvent {
    int gooseId;
    float x, y;
    double time;
};

struct GooseDamagedEvent {
    int gooseId;
    float amount;
    double time;
};

struct ItemDroppedEvent {
    int gooseId;
    float x, y;
    const char* itemType; // "meme", "text", "toy"
};

struct ItemEatenEvent {
    int gooseId;
    float x, y;
    const char* itemType;
};

struct GooseJailedEvent {
    int gooseId;
    float x, y;
};

struct GooseFreedEvent {
    int gooseId;
};

struct PomodoroPhaseChangedEvent {
    int gooseId;
    int phase; // 0=Work, 1=Break, 2=LongBreak
    double time;
};

struct GooseStuckEvent {
    int gooseId;
    float x, y;
    double stuckDuration;
};

struct CursorFastMoveEvent {
    float vx, vy;
    float x, y;
};

struct ToySpawnedEvent {
    float x, y;
    int type; // 0=stick, 1=ball
};

struct BallKickedEvent {
    int gooseId;
    float ballX, ballY;
    float ballVx, ballVy;
};

struct BreadcrumbDroppedEvent {
    float x, y;
};

struct GooseTeleportedEvent {
    int gooseId;
    int portalId;
    float fromX, fromY;
    float toX, toY;
};

// ============================================================
// EventBus
// ============================================================

using SubscriptionId = uint64_t;

class EventBus {
public:
    static EventBus& Instance() {
        static EventBus inst;
        return inst;
    }

    // Subscribe to an event. Returns a subscription ID for unsubscribing.
    template<typename T>
    SubscriptionId Subscribe(std::function<void(const T&)> handler);

    // Unsubscribe by ID
    void Unsubscribe(SubscriptionId id);

    // Publish an event (calls all subscribers)
    template<typename T>
    void Publish(const T& event);

    // Clear all subscriptions (e.g. on goose cleanup)
    void Clear();

private:
    EventBus() = default;

    mutable std::shared_mutex mutex_;
    std::unordered_map<EventType, std::vector<std::pair<SubscriptionId, std::function<void(const void*)>>>> handlers_;
    std::unordered_map<SubscriptionId, EventType> subscriptionMap_;
    SubscriptionId nextId_ = 1;
};

// ============================================================
// Inline implementations
// ============================================================

namespace detail {
    template<typename T> struct EventTypeInfo;
    template<> struct EventTypeInfo<GooseHonkedEvent> { static constexpr EventType type = EventType::GooseHonked; };
    template<> struct EventTypeInfo<GooseDamagedEvent> { static constexpr EventType type = EventType::GooseDamaged; };
    template<> struct EventTypeInfo<ItemDroppedEvent> { static constexpr EventType type = EventType::ItemDropped; };
    template<> struct EventTypeInfo<ItemEatenEvent> { static constexpr EventType type = EventType::ItemEaten; };
    template<> struct EventTypeInfo<GooseJailedEvent> { static constexpr EventType type = EventType::GooseJailed; };
    template<> struct EventTypeInfo<GooseFreedEvent> { static constexpr EventType type = EventType::GooseFreed; };
    template<> struct EventTypeInfo<PomodoroPhaseChangedEvent> { static constexpr EventType type = EventType::PomodoroPhaseChanged; };
    template<> struct EventTypeInfo<GooseStuckEvent> { static constexpr EventType type = EventType::GooseStuck; };
    template<> struct EventTypeInfo<CursorFastMoveEvent> { static constexpr EventType type = EventType::CursorFastMove; };
    template<> struct EventTypeInfo<ToySpawnedEvent> { static constexpr EventType type = EventType::ToySpawned; };
    template<> struct EventTypeInfo<BallKickedEvent> { static constexpr EventType type = EventType::BallKicked; };
    template<> struct EventTypeInfo<BreadcrumbDroppedEvent> { static constexpr EventType type = EventType::BreadcrumbDropped; };
    template<> struct EventTypeInfo<GooseTeleportedEvent> { static constexpr EventType type = EventType::GooseTeleported; };
}

template<typename T>
SubscriptionId EventBus::Subscribe(std::function<void(const T&)> handler) {
    constexpr EventType type = detail::EventTypeInfo<T>::type;
    std::unique_lock<std::shared_mutex> lock(mutex_);

    SubscriptionId id = nextId_++;
    auto wrapper = [handler](const void* data) {
        handler(*static_cast<const T*>(data));
    };
    handlers_[type].push_back({id, wrapper});
    subscriptionMap_[id] = type;
    return id;
}

template<typename T>
void EventBus::Publish(const T& event) {
    constexpr EventType type = detail::EventTypeInfo<T>::type;

    // Snapshot the handler list under the lock, then invoke unlocked so
    // a handler that re-publishes or (un)subscribes can't deadlock or
    // mutate the vector under our iterator.
    std::vector<std::function<void(const void*)>> snapshot;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto it = handlers_.find(type);
        if (it == handlers_.end()) return;
        snapshot.reserve(it->second.size());
        for (const auto& [id, handler] : it->second) {
            snapshot.push_back(handler);
        }
    }
    for (const auto& handler : snapshot) {
        handler(&event);
    }
}

#ifndef BEHAVIOR_MANAGER_H
#define BEHAVIOR_MANAGER_H

#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <string>
#include "behavior_state.h"

class BehaviorStateManager {
public:
    static BehaviorStateManager& Instance() {
        static BehaviorStateManager inst;
        return inst;
    }

    template<typename T>
    T* GetOrCreate(int gooseId, const char* behaviorId) {
        uint64_t key = MakeKey(gooseId, behaviorId);
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
        uint64_t key = MakeKey(gooseId, behaviorId);
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
            int storedId = static_cast<int>(it->first >> 32);
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

    uint64_t MakeKey(int gooseId, const char* behaviorId) {
        size_t hash = std::hash<std::string>{}(behaviorId);
        return (static_cast<uint64_t>(gooseId) << 32) | static_cast<uint32_t>(hash);
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, std::unique_ptr<BehaviorState>> states;
};


#endif

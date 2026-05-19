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

        uint16_t targetId = static_cast<uint16_t>(gooseId);
        for (auto it = states.begin(); it != states.end(); ) {
            uint16_t storedId = static_cast<uint16_t>(it->first >> 48);
            if (storedId == targetId) {
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
        // 16 bits for gooseId (supports up to 65535), 48 bits for behavior hash.
        // FNV-1a 64-bit hash truncated to 48 bits — far lower collision risk than 32-bit.
        uint64_t hash = Fnv1a64(behaviorId);
        return (static_cast<uint64_t>(static_cast<uint16_t>(gooseId)) << 48) | (hash & 0xFFFFFFFFFFFFULL);
    }

    static uint64_t Fnv1a64(const char* str) {
        uint64_t hash = 0xCBF29CE484222325ULL; // FNV-1a 64-bit basis
        while (*str) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(*str++));
            hash *= 0x100000001B3ULL; // FNV-1a 64-bit prime
        }
        return hash;
    }

    mutable std::shared_mutex mutex_;
    std::unordered_map<uint64_t, std::unique_ptr<BehaviorState>> states;
};


#endif

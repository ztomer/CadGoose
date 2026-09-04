#include "actor.h"
#include "actor_dropped_item.h"
#include "goose.h"
#include "world.h"
#include <algorithm>
#include <unordered_set>

ActorManager& ActorManager::Instance() {
    static ActorManager instance;
    return instance;
}

void ActorManager::add(Actor* actor) {
    if (!actor) return;
    actors.push_back(actor);
    liveSet.insert(actor);   // H1: O(1) membership tracking
    invalidateCaches();
}

void ActorManager::remove(Actor* actor) {
    actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());
    liveSet.erase(actor);    // H1: keep in sync
    invalidateCaches();
}

void ActorManager::tickAll(WorldContext& ctx, double dt, double time) {
    // H1: Snapshot actors vector; use liveSet for O(1) "still alive" check
    // instead of O(N) std::find, making tickAll O(N) total instead of O(N²).
    std::vector<Actor*> snapshot = actors;

    // H5: Precompute goose positions into a flat cache so each Goose's
    // CalculateSeparationForce reads plain structs instead of chasing
    // pointers through the actor list N times.
    GooseSeparationCache_Update(getGeese());

    for (auto* actor : snapshot) {
        if (liveSet.count(actor)) {
            if (actor && actor->isActive() && actor->isAlive()) {
                actor->tick(ctx, dt, time);
            }
        }
    }
}

void ActorManager::renderAll(IRenderer* renderer) {
    // H1: Same O(1) live-set check as tickAll.
    std::vector<Actor*> snapshot = actors;
    for (auto* actor : snapshot) {
        if (liveSet.count(actor)) {
            if (actor && actor->isActive() && actor->isAlive()) {
                actor->render(renderer);
            }
        }
    }
}

void ActorManager::cleanup() {
    auto partition = std::stable_partition(actors.begin(), actors.end(),
        [](Actor* a) { return a->isAlive(); });
    bool changed = (partition != actors.end());
    std::vector<Actor*> dead(partition, actors.end());
    actors.erase(partition, actors.end());
    for (auto* a : dead) {
        liveSet.erase(a);    // H1: remove from live set before deletion
        delete a;
    }
    if (changed) invalidateCaches();
}

Actor* ActorManager::findByType(const char* type, int id) {
    for (auto* actor : actors) {
        if (actor->isActive() && strcmp(actor->type(), type) == 0) {
            if (id < 0 || actor->id() == id) {
                return actor;
            }
        }
    }
    return nullptr;
}

Actor* ActorManager::findByType(ActorType type, int id) {
    for (auto* actor : actors) {
        if (actor->isActive() && actor->actorType() == type) {
            if (id < 0 || actor->id() == id) {
                return actor;
            }
        }
    }
    return nullptr;
}

int ActorManager::countByType(const char* type) const {
    int count = 0;
    for (auto* actor : actors) {
        if (actor->isActive() && strcmp(actor->type(), type) == 0) {
            count++;
        }
    }
    return count;
}

int ActorManager::countByType(ActorType type) const {
    int count = 0;
    for (auto* actor : actors) {
        if (actor->isActive() && actor->actorType() == type) {
            count++;
        }
    }
    return count;
}

void ActorManager::destroyAllOfType(const char* type) {
    std::vector<Actor*> dead;
    for (auto it = actors.begin(); it != actors.end();) {
        Actor* a = *it;
        if (a && strcmp(a->type(), type) == 0) {
            a->setActive(false);
            dead.push_back(a);
            it = actors.erase(it);
        } else {
            ++it;
        }
    }
    for (auto* a : dead) {
        liveSet.erase(a);  // must precede delete — see note in the ActorType overload
        delete a;
    }
    if (!dead.empty()) invalidateCaches();
}

void ActorManager::destroyAllOfType(ActorType type) {
    std::vector<Actor*> dead;
    for (auto it = actors.begin(); it != actors.end();) {
        Actor* a = *it;
        if (a && a->actorType() == type) {
            a->setActive(false);
            dead.push_back(a);
            it = actors.erase(it);
        } else {
            ++it;
        }
    }
    for (auto* a : dead) {
        // liveSet is the O(1) "is this actor still alive" check used by
        // tickAll/renderAll. add/remove/cleanup all keep it in sync; these two
        // destroyAllOfType overloads did not, so every actor destroyed here
        // left a DANGLING pointer behind. Since the allocator readily recycles
        // a just-freed block, the next actor allocated could land on exactly
        // that address and be reported live by a stale entry — after which
        // tickAll/renderAll would call into it. Erase before delete.
        liveSet.erase(a);
        delete a;
    }
    if (!dead.empty()) invalidateCaches();
}

const std::vector<Goose*>& ActorManager::getGeese() const {
    if (geeseCacheDirty) {
        geeseCache.clear();
        geeseCache.reserve(actors.size());
        for (auto* actor : actors) {
            if (actor->isActive() && actor->isGoose()) {
                geeseCache.push_back(static_cast<Goose*>(actor));
            }
        }
        geeseCacheDirty = false;
    }
    return geeseCache;
}

const std::vector<DroppedItemActor*>& ActorManager::getDroppedItems() const {
    if (droppedItemsCacheDirty) {
        droppedItemsCache.clear();
        droppedItemsCache.reserve(actors.size());
        for (auto* actor : actors) {
            if (actor->isActive() && actor->actorType() == ActorType::DroppedItem) {
                droppedItemsCache.push_back(static_cast<DroppedItemActor*>(actor));
            }
        }
        droppedItemsCacheDirty = false;
    }
    return droppedItemsCache;
}

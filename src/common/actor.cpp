#include "actor.h"
#include "actor_dropped_item.h"
#include "goose.h"
#include "world.h"
#include <algorithm>

ActorManager& ActorManager::Instance() {
    static ActorManager instance;
    return instance;
}

void ActorManager::add(Actor* actor) {
    if (!actor) return;
    actors.push_back(actor);
    invalidateCaches();
}

void ActorManager::remove(Actor* actor) {
    actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());
    invalidateCaches();
}

void ActorManager::tickAll(WorldContext& ctx, double dt, double time) {
    std::vector<Actor*> snapshot = actors;
    for (auto* actor : snapshot) {
        if (std::find(actors.begin(), actors.end(), actor) != actors.end()) {
            if (actor && actor->isActive() && actor->isAlive()) {
                actor->tick(ctx, dt, time);
            }
        }
    }
}

void ActorManager::renderAll(IRenderer* renderer) {
    std::vector<Actor*> snapshot = actors;
    for (auto* actor : snapshot) {
        if (std::find(actors.begin(), actors.end(), actor) != actors.end()) {
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

int ActorManager::countByType(const char* type) const {
    int count = 0;
    for (auto* actor : actors) {
        if (actor->isActive() && strcmp(actor->type(), type) == 0) {
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
            if (actor->isActive() && strcmp(actor->type(), "dropped_item") == 0) {
                droppedItemsCache.push_back(static_cast<DroppedItemActor*>(actor));
            }
        }
        droppedItemsCacheDirty = false;
    }
    return droppedItemsCache;
}

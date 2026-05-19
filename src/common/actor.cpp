// actor.cpp
// ActorManager implementation.

#include "actor.h"
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
    for (auto* actor : actors) {
        if (actor->active) {
            actor->tick(ctx, dt, time);
        }
    }
}

void ActorManager::renderAll(IRenderer* renderer) {
    for (auto* actor : actors) {
        if (actor->active && actor->isAlive()) {
            actor->render(renderer);
        }
    }
}

void ActorManager::cleanup() {
    auto partition = std::stable_partition(actors.begin(), actors.end(),
        [](Actor* a) { return a->isAlive(); });
    bool changed = (partition != actors.end());
    for (auto it = partition; it != actors.end(); ++it) {
        delete *it;
    }
    actors.erase(partition, actors.end());
    if (changed) invalidateCaches();
}

Actor* ActorManager::findByType(const char* type, int id) {
    for (auto* actor : actors) {
        if (actor->active && strcmp(actor->type(), type) == 0) {
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
        if (actor->active && strcmp(actor->type(), type) == 0) {
            count++;
        }
    }
    return count;
}

void ActorManager::destroyAllOfType(const char* type) {
    auto newEnd = std::remove_if(actors.begin(), actors.end(),
        [type](Actor* a) {
            if (a && strcmp(a->type(), type) == 0) {
                delete a;
                return true;
            }
            return false;
        });
    bool changed = (newEnd != actors.end());
    actors.erase(newEnd, actors.end());
    if (changed) invalidateCaches();
}

const std::vector<Goose*>& ActorManager::getGeese() const {
    if (geeseCacheDirty) {
        geeseCache.clear();
        geeseCache.reserve(actors.size());
        for (auto* actor : actors) {
            if (actor->active && strcmp(actor->type(), "goose") == 0) {
                geeseCache.push_back(static_cast<Goose*>(actor));
            }
        }
        geeseCacheDirty = false;
    }
    return geeseCache;
}

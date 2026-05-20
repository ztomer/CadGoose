// actor.mm
// ActorManager implementation.

#include "actor.h"
#include "actor_dropped_item.h"
#include "goose.h"
#include "world.h"
#include <algorithm>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

void Actor::closeWindowOnMainThread(void (^closeBlock)()) {
#ifdef __APPLE__
    if (!closeBlock) return;
    // dispatch_async copies the block under ARC — just dispatch it.
    dispatch_async(dispatch_get_main_queue(), closeBlock);
#else
    (void)closeBlock;
#endif
}

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
    // Snapshot the actor pointers before iterating. Several behavior ticks
    // call ActorManager::add (jail, flower, portal, breadcrumb, dropped-item,
    // leafpile) which is a std::vector::push_back; if the vector reallocates
    // mid-iteration the range-for iterator is invalidated and the next deref
    // crashes (`ActorManager::tickAll` was the top frame in the
    // EXC_BAD_ACCESS crash report from 2026-05-19 22:01).
    std::vector<Actor*> snapshot = actors;
    for (auto* actor : snapshot) {
        if (actor && actor->isActive()) {
            actor->tick(ctx, dt, time);
        }
    }
}

void ActorManager::renderAll(IRenderer* renderer) {
    // Same snapshot pattern as tickAll — defensive against reentrant
    // add/remove (less critical here since render() shouldn't mutate the
    // actor set, but cheap insurance).
    std::vector<Actor*> snapshot = actors;
    for (auto* actor : snapshot) {
        if (actor && actor->isActive() && actor->isAlive()) {
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
            if (actor->isActive() && strcmp(actor->type(), "goose") == 0) {
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

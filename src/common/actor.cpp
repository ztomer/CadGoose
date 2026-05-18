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
}

void ActorManager::remove(Actor* actor) {
    actors.erase(std::remove(actors.begin(), actors.end(), actor), actors.end());
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
    actors.erase(
        std::remove_if(actors.begin(), actors.end(),
            [](Actor* a) { return !a->isAlive(); }),
        actors.end()
    );
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

std::vector<Goose*> ActorManager::getGeese() const {
    std::vector<Goose*> geese;
    for (auto* actor : actors) {
        if (actor->active && strcmp(actor->type(), "goose") == 0) {
            geese.push_back(static_cast<Goose*>(actor));
        }
    }
    return geese;
}

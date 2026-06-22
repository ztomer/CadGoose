// actor_manager.h
// ActorManager — owns all actors, ticks/renders/cleans them up.
// Separated from actor.h to keep the Actor base class lightweight.

#pragma once

#include <vector>

class Actor;
class Goose;
class DroppedItemActor;
struct WorldContext;
class IRenderer;

enum class ActorType;

class ActorManager {
public:
    static ActorManager& Instance();

    void add(Actor* actor);
    void remove(Actor* actor);

    void tickAll(WorldContext& ctx, double dt, double time);
    void renderAll(IRenderer* renderer);
    void cleanup();

    Actor* findByType(const char* type, int id = -1);
    Actor* findByType(ActorType type, int id = -1);

    int countByType(const char* type) const;
    int countByType(ActorType type) const;

    int totalCount() const { return (int)actors.size(); }

    Actor* getByIndex(int index) const {
        if (index < 0 || index >= (int)actors.size()) return nullptr;
        return actors[index];
    }

    const std::vector<Goose*>& getGeese() const;

    const std::vector<DroppedItemActor*>& getDroppedItems() const;

    void removeAllDroppedItems() { destroyAllOfType(ActorType::DroppedItem); }

    void destroyAllOfType(const char* type);
    void destroyAllOfType(ActorType type);

    void invalidateDroppedItemsCache() { droppedItemsCacheDirty = true; }

private:
    ActorManager() = default;
    std::vector<Actor*> actors;
    mutable std::vector<Goose*> geeseCache;
    mutable std::vector<DroppedItemActor*> droppedItemsCache;
    mutable bool geeseCacheDirty = true;
    mutable bool droppedItemsCacheDirty = true;

    void invalidateCaches() {
        geeseCacheDirty = true;
        droppedItemsCacheDirty = true;
    }
};

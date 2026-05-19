// actor.h
// Base Actor class — entities that exist in the world.
// Distinct from Behavior (logic attached to actors).

#pragma once

#include "coordinate_system.h"
#include "renderer_interface.h"
#include <vector>
#include <cstring>

class Goose; // forward declaration
struct WorldContext; // forward declaration

class Actor {
public:
    virtual ~Actor() = default;

    // Type identifier ("goose", "ball", "toy", "flower", etc.)
    virtual const char* type() const = 0;

    // Unique ID within type (0 for singletons, index for multiples)
    virtual int id() const { return 0; }

    // Lifecycle
    virtual void tick(WorldContext& ctx, double dt, double time) = 0;
    virtual void render(IRenderer* renderer) = 0;
    virtual bool isAlive() const = 0;  // false = remove from manager

    // Position in device coordinates
    DevicePoint position;

    // Visual/collision radius
    float radius;

    // Active flag
    bool active;

protected:
    Actor() : position{0, 0}, radius(0), active(true) {}
};

// ActorManager — owns all actors, ticks/renders/cleans them up.
class ActorManager {
public:
    static ActorManager& Instance();

    void add(Actor* actor);
    void remove(Actor* actor);

    void tickAll(WorldContext& ctx, double dt, double time);
    void renderAll(IRenderer* renderer);
    void cleanup();  // remove dead actors

    // Find actors by type (returns first match, or nullptr)
    Actor* findByType(const char* type, int id = -1);

    // Count actors by type
    int countByType(const char* type) const;

    // Total actor count
    int totalCount() const { return (int)actors.size(); }

    // Get actor by index (for iteration)
    Actor* getByIndex(int index) const {
        if (index < 0 || index >= (int)actors.size()) return nullptr;
        return actors[index];
    }

    // Get all Goose actors. Returns a reference to a cached vector that's
    // rebuilt only when the actor set changes — safe to call many times
    // per frame without per-call allocation.
    const std::vector<Goose*>& getGeese() const;

    // Delete and remove all actors of a given type (owning cleanup)
    void destroyAllOfType(const char* type);

private:
    ActorManager() = default;
    std::vector<Actor*> actors;
    mutable std::vector<Goose*> geeseCache;
    mutable bool geeseCacheDirty = true;

    void invalidateCaches() { geeseCacheDirty = true; }
};

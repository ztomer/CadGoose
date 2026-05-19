// actor.h
// Base Actor class — entities that exist in the world.
// Distinct from Behavior (logic attached to actors).

#pragma once

#include "coordinate_system.h"
#include "renderer_interface.h"
#include <vector>
#include <cstring>

class Goose; // forward declaration

class Actor {
public:
    virtual ~Actor() = default;

    // Type identifier ("goose", "ball", "toy", "flower", etc.)
    virtual const char* type() const = 0;

    // Unique ID within type (0 for singletons, index for multiples)
    virtual int id() const { return 0; }

    // Lifecycle
    virtual void tick(double dt, double time) = 0;
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
class DroppedItemActor; // forward declaration

class ActorManager {
public:
    static ActorManager& Instance();

    void add(Actor* actor);
    void remove(Actor* actor);

    void tickAll(double dt, double time);
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

    // Get all Goose actors
    std::vector<Goose*> getGeese() const;

    // Get all DroppedItem actors
    std::vector<DroppedItemActor*> getDroppedItems() const;

    // Remove all DroppedItem actors
    void removeAllDroppedItems();

private:
    ActorManager() = default;
    std::vector<Actor*> actors;
};

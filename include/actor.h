// actor.h
// Base Actor class — entities that exist in the world.
// Distinct from Behavior (logic attached to actors).

#pragma once

#include "coordinate_system.h"
#include "renderer_interface.h"
#include <cstring>

struct WorldContext; // forward declaration

enum class ActorType {
    Goose, BabyStalin, Ball, Breadcrumb, DroppedItem,
    Flower, Jail, Leafpile, Portal, Toy
};

class Actor {
public:
    virtual ~Actor() = default;

    // Type identifier for debug/logging ("goose", "ball", etc.)
    virtual const char* type() const = 0;

    // Compile-time type ID (fast integer comparison, replaces strcmp)
    virtual ActorType actorType() const = 0;

    // Unique ID within type (0 for singletons, index for multiples)
    virtual int id() const { return 0; }

    // Type helpers (avoid RTTI / dynamic_cast overhead)
    virtual bool isGoose() const { return false; }

    // Lifecycle
    virtual void tick(WorldContext& ctx, double dt, double time) = 0;
    virtual void render(IRenderer* renderer) = 0;
    virtual bool isAlive() const = 0;  // false = remove from manager

    // Accessors — fields are protected so external code goes through these.
    // (Subclasses can touch the underlying members directly.)
    DevicePoint position() const { return m_position; }
    void setPosition(DevicePoint p) { m_position = p; }
    float radius() const { return m_radius; }
    void setRadius(float r) { m_radius = r; }
    bool isActive() const { return m_active; }
    void setActive(bool a) { m_active = a; }

protected:
    Actor() : m_position{0, 0}, m_radius(0), m_active(true) {}

#ifdef __APPLE__
    // Safely dispatch a cleanup block to the main thread (call from destructor).
    void closeWindowOnMainThread(void (^cleanupBlock)());
#endif

    DevicePoint m_position;
    float m_radius;
    bool m_active;
};

#include "actor_manager.h"

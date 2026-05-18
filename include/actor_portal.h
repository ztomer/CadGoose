// actor_portal.h
// Portal actor — teleportation point (A or B).
// Each portal instance has its own window.

#pragma once

#include "actor.h"

class PortalActor : public Actor {
public:
    enum Type { PortalA, PortalB };

    PortalActor(Type type, const Vector2& pos);
    ~PortalActor() override;

    const char* type() const override { return "portal"; }
    int id() const override { return m_portalId; }
    void tick(double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return active; }

    Type portalType() const { return m_type; }
    int portalId() const { return m_portalId; }

private:
    static constexpr float PORTAL_SIZE = 40.0f;

    Type m_type;
    int m_portalId;
    void* m_image; // CGImageRef

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

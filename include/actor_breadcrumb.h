// actor_breadcrumb.h
// Breadcrumb actor — dropped at cursor, expires after lifetime.
// Each breadcrumb instance has its own window.

#pragma once

struct WorldContext;
#include "actor.h"

class BreadcrumbActor : public Actor {
public:
    BreadcrumbActor(const Vector2& pos, double spawnTime, float lifetime);
    ~BreadcrumbActor() override;

    const char* type() const override { return "breadcrumb"; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return m_active; }

    double spawnTime() const { return m_spawnTime; }
    float lifetime() const { return m_lifetime; }

private:
    static constexpr float CRUMB_SIZE = 10.0f;

    double m_spawnTime;
    float m_lifetime;
    void* m_image; // CGImageRef

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

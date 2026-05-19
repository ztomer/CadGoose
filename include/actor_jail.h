// actor_jail.h
// Jail actor — cage that traps the goose.
// Each jail instance has its own window.

#pragma once

struct WorldContext;
#include "actor.h"

class JailActor : public Actor {
public:
    JailActor(const Vector2& pos);
    ~JailActor() override;

    const char* type() const override { return "jail"; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return m_active; }

private:
    static constexpr float JAIL_SIZE = 40.0f;

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

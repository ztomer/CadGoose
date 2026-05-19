// actor_toy.h
// Toy actor — independent entity on the ground (stick or ball).
// Each toy instance has its own window.

#pragma once

#include "actor.h"

class ToyActor : public Actor {
public:
    enum Type { Stick, Ball };

    ToyActor(Type type, const Vector2& pos, int instanceId);
    ~ToyActor() override;

    const char* type() const override { return "toy"; }
    int id() const override { return m_instanceId; }
    void tick(double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return active; }

    Type toyType() const { return m_type; }
    float angle() const { return m_angle; }
    double spawnTime() const { return m_spawnTime; }

private:
    static constexpr float TOY_LIFETIME = 30.0f;
    static constexpr float STICK_LENGTH = 24.0f;
    static constexpr float STICK_WIDTH = 4.0f;
    static constexpr float BALL_RADIUS = 15.0f;

    Type m_type;
    float m_angle;
    double m_spawnTime;
    int m_instanceId;

#ifdef __APPLE__
    void* m_window;  // BehaviorElementWindow* as opaque pointer
    void* m_windowKey;  // NSNumber* as opaque pointer
    void* m_images[2]; // stick image, ball image
    void initWindow();
    void updateWindow();
#endif
};

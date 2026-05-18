// actor_flower.h
// Flower actor — grows from ground with stem and petals.
// Each flower instance has its own window.

#pragma once

struct WorldContext;
#include "actor.h"

class FlowerActor : public Actor {
public:
    FlowerActor(const Vector2& pos, float hue, double spawnTime);
    ~FlowerActor() override;

    const char* type() const override { return "flower"; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return active; }

    float growth() const { return m_growth; }
    float stemHeight() const { return m_stemHeight; }
    float petalSize() const { return m_petalSize; }
    float hue() const { return m_hue; }

private:
    static constexpr float FLOWER_LIFETIME = 30.0f;
    static constexpr float MAX_STEM_HEIGHT = 15.0f;
    static constexpr float MAX_PETAL_SIZE = 5.0f;
    static constexpr float GROWTH_THRESHOLD = 0.01f;

    float m_hue;
    float m_growth;
    float m_stemHeight;
    float m_petalSize;
    double m_spawnTime;

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

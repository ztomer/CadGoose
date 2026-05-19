// actor_leafpile.h
// Leaf pile actor — autumn leaves that scatter when goose approaches.
// Each leaf pile instance has its own window.

#pragma once

struct WorldContext;
#include "actor.h"
#include <vector>

struct LeafParticle {
    Vector2 curPosPlanar;
    float curPosZ;
    Vector2 velPlanar;
    float velZ;
    int colorIndex;
};

class LeafPileActor : public Actor {
public:
    LeafPileActor(const Vector2& pos, float radius, float height, double currentTime);
    ~LeafPileActor() override;

    const char* type() const override { return "leafpile"; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return m_active; }

    void kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage);

private:
    static constexpr int LEAVES_PER_PILE = 128;
    static constexpr float LEAF_PILE_SIZE = 60.0f;

    // radius is inherited from Actor (m_radius / radius() / setRadius())
    float m_height;
    double m_timeCreated;
    double m_timeSinceKicked;
    std::vector<LeafParticle> m_leaves;

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

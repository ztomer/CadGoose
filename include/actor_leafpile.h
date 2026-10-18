// actor_leafpile.h
// Leaf pile actor — autumn leaves that scatter when goose approaches.
// Each leaf pile instance has its own window.

#pragma once

struct WorldContext;
class IRenderer;
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
    ActorType actorType() const override { return ActorType::Leafpile; }
    void tick(WorldContext& ctx, double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return m_active; }

    void kick(Vector2 kickVelocity, double currentTime, float gooseSpeedPercentage);

    double timeCreated() const { return m_timeCreated; }
    float alphaMult() const { return m_alphaMult; }
    const std::vector<LeafParticle>& leaves() const { return m_leaves; }

    // Where the leaves are, at the current globalScale, relative to the pile
    // centre (y up the screen = negative, height folded in). render() sizes
    // the window from it and drawLeaves() lays out inside it, so the two
    // cannot disagree about the scale: they once did, because the window's
    // draw block captured the scale at creation and render() read it fresh.
    struct LeafBounds {
        float minX, maxX, minY, maxY;
        float centerX() const { return (minX + maxX) * 0.5f; }
        float centerY() const { return (minY + maxY) * 0.5f; }
        // Square, covering the farthest leaf in either axis, plus padding.
        float windowSize() const;
    };
    LeafBounds leafBounds() const;

    // Draws every leaf into a window of leafBounds().windowSize(), origin at
    // its top-left corner. The pile's window draw block is exactly this call.
    void drawLeaves(IRenderer& r) const;

private:
    static constexpr int LEAVES_PER_PILE = 128;
    static constexpr float LEAF_PILE_SIZE = 60.0f;

    // radius is inherited from Actor (m_radius / radius() / setRadius())
    float m_height;
    double m_timeCreated;
    double m_timeSinceKicked;
    float m_alphaMult = 1.0f;
    // Redraw gating: a resting pile (not kicked, not fading) draws nothing new,
    // so skip the per-frame window update + 128-leaf redraw. m_dirty is set while
    // leaves are in motion; alpha changes are detected separately in render().
    bool m_dirty = true;
    float m_lastRenderedAlpha = -1.0f;
    std::vector<LeafParticle> m_leaves;

#ifdef __APPLE__
    void* m_window;    // BehaviorElementWindow*
    void* m_windowKey; // NSNumber*
#endif
};

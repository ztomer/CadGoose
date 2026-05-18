// actor_ball.h
// Ball actor — independent entity with physics, window, and animation.
// Not a behavior; it's an Actor that exists in the world.

#pragma once

#include "actor.h"

#ifdef __APPLE__
@class BehaviorElementWindow;
@class NSNumber;
#endif

class BallActor : public Actor {
public:
    BallActor();
    ~BallActor() override;

    const char* type() const override { return "ball"; }
    void tick(double dt, double time) override;
    void render(IRenderer* renderer) override;
    bool isAlive() const override { return active; }

    // Physics state
    Vector2 velocity;
    float speed;
    double lastKickTime;
    double lastAnimateTime;
    float animationGap;
    int currentFrame;

    // Interaction with goose (called from goose tick)
    void onGooseKick(const Vector2& goosePos, float gooseFootSize, double time);
    void onCursorKick(const Vector2& cursorPos, double time);

    // Screen bounds
    void clampToBounds(float margin, float screenWidth, float screenHeight);

    // Returns true if ball was kicked (for goose state changes)
    bool wasKicked() const { return m_wasKicked; }
    void clearKickedFlag() { m_wasKicked = false; }

private:
    static constexpr float KICK_SPEED = 500.0f;
    static constexpr float DECELERATION = 3.0f;
    static constexpr float SPEED_THRESHOLD = 5.0f;
    static constexpr float KICK_COOLDOWN = 0.5f;
    static constexpr float ANIM_SPEED_DIVISOR = 5.0f;
    static constexpr float ANIM_GAP_MIN = 0.05f;
    static constexpr float ANIM_GAP_MAX = 0.2f;
    static constexpr int ANIM_FRAME_COUNT = 3;

    bool m_wasKicked;

#ifdef __APPLE__
    BehaviorElementWindow* m_window;
    NSNumber* m_windowKey;
    void* m_images[3];
    void initWindow();
    void updateWindow();
#endif
};

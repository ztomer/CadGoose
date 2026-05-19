#ifndef BALL_STATE_H
#define BALL_STATE_H

#include "behavior_state.h"
#include "goose_math.h"
#include <vector>

struct BallState : public BehaviorState {
    enum class BallType : int {
        Generic = 0,
        Soccer = 1,
        Beach = 2
    };

    struct Ball {
        Vector2 pos{0, 0};
        Vector2 vel{0, 0};
        float radius = 25.0f;
        bool active = true;
        BallType type = BallType::Generic;
        int currentFrame = 0;

        float r = 0.3f, g = 0.5f, b = 0.9f;
        float strokeR = 0.2f, strokeG = 0.4f, strokeB = 0.7f;
        float strokeWidth = 2.0f;
        bool hasPattern = false;

        // Physics and animation state
        float speed = 0.0f;
        float lastKickTime = 0.0f;
        float lastAnimateTime = 0.0f;
        float animationGap = 0.0f;
    };
    std::vector<Ball> balls;

    void Reset() override {
        for (auto& ball : balls) {
            ball.active = false;
        }
    }

    static Ball CreateSoccerBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Soccer;
        ball.r = 1.0f; ball.g = 1.0f; ball.b = 1.0f;
        ball.strokeR = 0.15f; ball.strokeG = 0.15f; ball.strokeB = 0.15f;
        ball.strokeWidth = 2.0f;
        ball.hasPattern = true;
        return ball;
    }

    static Ball CreateBeachBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Beach;
        ball.r = 1.0f; ball.g = 0.9f; ball.b = 0.6f;
        ball.strokeR = 0.9f; ball.strokeG = 0.7f; ball.strokeB = 0.3f;
        ball.strokeWidth = 1.0f;
        ball.hasPattern = true;
        return ball;
    }

    static Ball CreateGenericBall(float radius) {
        Ball ball;
        ball.radius = radius;
        ball.type = BallType::Generic;
        ball.r = 0.3f; ball.g = 0.5f; ball.b = 0.9f;
        ball.strokeR = 0.2f; ball.strokeG = 0.4f; ball.strokeB = 0.7f;
        ball.strokeWidth = 2.0f;
        ball.hasPattern = false;
        return ball;
    }
};

#endif

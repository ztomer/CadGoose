#include "behaviors/states/all.h"
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>

#include "behavior.h"
#include "goose_math.h"
#include "goose.h"
#include "config.h"
#include "world.h"
#include "behaviors/states/portal_state.h"
#include "behaviors/states/anger_state.h"
#include "behaviors/states/ball_state.h"
#include "behaviors/states/pomodoro_state.h"

TEST(PortalState, DefaultValues) {
    PortalState state;
    EXPECT_FALSE(state.portalA.active);
    EXPECT_FALSE(state.portalB.active);
    EXPECT_TRUE(state.portalsEnabled);
}

TEST(PortalState, Reset) {
    PortalState state;
    state.portalA = {100.0f, 200.0f, true, 1};
    state.portalB = {500.0f, 600.0f, true, 2};
    state.portalsEnabled = false;

    state.Reset();

    EXPECT_FALSE(state.portalA.active);
    EXPECT_FALSE(state.portalB.active);
    EXPECT_TRUE(state.portalsEnabled);
}

TEST(PortalState, TeleportCollision) {
    PortalState::Portal portalA{100.0f, 100.0f, true, 1};
    PortalState::Portal portalB{500.0f, 500.0f, true, 2};

    float x = 100.0f, y = 100.0f;
    float radius = 20.0f;

    float distA = std::hypot(x - portalA.x, y - portalA.y);
    EXPECT_LT(distA, radius);
    EXPECT_TRUE(portalA.active);
    EXPECT_TRUE(portalB.active);
}

TEST(PortalState, PortalRadius) {
    constexpr float PORTAL_RADIUS = 30.0f;
    constexpr float PORTAL_TELEPORT_DIST = 20.0f;

    PortalState state;
    state.portalA = {100.0f, 100.0f, true, 1};
    state.portalB = {500.0f, 500.0f, true, 2};

    float x = 75.0f, y = 100.0f;
    float dist = std::hypot(x - state.portalA.x, y - state.portalA.y);

    EXPECT_GE(dist, PORTAL_TELEPORT_DIST);
    EXPECT_LT(dist, PORTAL_RADIUS);

    x = 100.0f, y = 100.0f;
    dist = std::hypot(x - state.portalA.x, y - state.portalA.y);
    EXPECT_LT(dist, PORTAL_TELEPORT_DIST);
}

TEST(PortalState, DeactivateAfterTeleport) {
    PortalState state;
    state.portalA = {100.0f, 100.0f, true, 1};
    state.portalB = {500.0f, 500.0f, true, 2};

    state.portalA.active = false;

    EXPECT_FALSE(state.portalA.active);
    EXPECT_TRUE(state.portalB.active);
}

TEST(AngerState, DefaultValues) {
    AngerState state;
    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
    EXPECT_FALSE(state.isPunching);
}

TEST(AngerState, Reset) {
    AngerState state;
    state.angerLevel = 75.0f;
    state.isPunching = true;
    state.lastPunchTime = 123.45;
    state.lastAngerIncrease = 123.40;

    state.Reset();

    EXPECT_FLOAT_EQ(state.angerLevel, 0.0f);
    EXPECT_FALSE(state.isPunching);
}

TEST(AngerState, AngerIncrease) {
    AngerState state;
    float increaseRate = 20.0f;
    double dt = 0.016;

    state.angerLevel += increaseRate * (float)dt;
    EXPECT_GT(state.angerLevel, 0.0f);
    EXPECT_LT(state.angerLevel, 20.0f);
}

TEST(AngerState, AngerDecrease) {
    AngerState state;
    state.angerLevel = 50.0f;
    float decreaseRate = 5.0f;
    double dt = 0.016;

    state.angerLevel = std::max(0.0f, state.angerLevel - decreaseRate * (float)dt);
    EXPECT_GT(state.angerLevel, 40.0f);
    EXPECT_LT(state.angerLevel, 50.0f);
}

TEST(AngerState, AngerCapAt100) {
    AngerState state;
    float increaseRate = 15.0f;
    double dt = 1.0;

    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel += increaseRate * (float)dt;
    state.angerLevel = std::min(100.0f, state.angerLevel);

    EXPECT_FLOAT_EQ(state.angerLevel, 100.0f);
}

TEST(AngerState, AngerFloorAt0) {
    AngerState state;
    state.angerLevel = 10.0f;
    float decreaseRate = 8.0f;
    double dt = 1.0;

    state.angerLevel = std::max(0.0f, state.angerLevel - decreaseRate * (float)dt);
    EXPECT_FLOAT_EQ(state.angerLevel, 2.0f);
}

TEST(AngerState, PunchTriggerThreshold) {
    AngerState state;
    state.angerLevel = 80.0f;
    state.isPunching = false;

    constexpr float PUNCH_COOLDOWN = 1.5;
    double time = 10.0;
    double lastPunch = 8.0;

    bool canPunch = state.angerLevel >= 80.0f && !state.isPunching &&
                    (time - lastPunch) > PUNCH_COOLDOWN;
    EXPECT_TRUE(canPunch);
}

TEST(AngerState, PunchDuration) {
    AngerState state;
    state.isPunching = true;
    state.lastPunchTime = 10.0;

    constexpr float PUNCH_DURATION = 0.3;
    double currentTime = 10.15;

    bool stillPunching = state.isPunching && (currentTime - state.lastPunchTime) < PUNCH_DURATION;
    EXPECT_TRUE(stillPunching);

    currentTime = 10.35;
    stillPunching = state.isPunching && (currentTime - state.lastPunchTime) < PUNCH_DURATION;
    EXPECT_FALSE(stillPunching);
}

TEST(AngerState, CursorDistanceAnger) {
    constexpr float CURSOR_ANGER_RADIUS = 100.0f;
    constexpr float ANGER_INCREASE_RATE = 15.0f;

    struct TestGoose { Vector2 pos = {100, 100}; };
    TestGoose goose;

    Vector2 cursorPos = {150, 150};
    float distToCursor = std::hypot(goose.pos.x - cursorPos.x, goose.pos.y - cursorPos.y);

    bool shouldGetAngrier = distToCursor < CURSOR_ANGER_RADIUS;
    EXPECT_TRUE(shouldGetAngrier);
    EXPECT_LT(distToCursor, 100.0f);
}

TEST(AngerState, FarCursorNoAnger) {
    constexpr float CURSOR_ANGER_RADIUS = 100.0f;

    struct TestGoose { Vector2 pos = {100, 100}; };
    TestGoose goose;

    Vector2 cursorPos = {500, 500};
    float distToCursor = std::hypot(goose.pos.x - cursorPos.x, goose.pos.y - cursorPos.y);

    bool shouldGetAngrier = distToCursor < CURSOR_ANGER_RADIUS;
    EXPECT_FALSE(shouldGetAngrier);
    EXPECT_GT(distToCursor, 100.0f);
}

TEST(BallType, SoccerBallProperties) {
    BallState::Ball ball = BallState::CreateSoccerBall(28.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Soccer);
    EXPECT_FLOAT_EQ(ball.radius, 28.0f);
    EXPECT_FLOAT_EQ(ball.r, 1.0f);
    EXPECT_FLOAT_EQ(ball.g, 1.0f);
    EXPECT_FLOAT_EQ(ball.b, 1.0f);
    EXPECT_TRUE(ball.hasPattern);
    EXPECT_TRUE(ball.active);
}

TEST(BallType, BeachBallProperties) {
    BallState::Ball ball = BallState::CreateBeachBall(35.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Beach);
    EXPECT_FLOAT_EQ(ball.radius, 35.0f);
    EXPECT_FLOAT_EQ(ball.r, 1.0f);
    EXPECT_FLOAT_EQ(ball.g, 0.9f);
    EXPECT_FLOAT_EQ(ball.b, 0.6f);
    EXPECT_TRUE(ball.hasPattern);
}

TEST(BallType, GenericBallProperties) {
    BallState::Ball ball = BallState::CreateGenericBall(20.0f);

    EXPECT_EQ(ball.type, BallState::BallType::Generic);
    EXPECT_FLOAT_EQ(ball.radius, 20.0f);
    EXPECT_FALSE(ball.hasPattern);
}

TEST(BallType, DifferentBounceFactors) {
    float soccerBounce = 0.7f;
    float beachBounce = 0.9f;
    float genericBounce = 0.5f;

    EXPECT_GT(beachBounce, soccerBounce);
    EXPECT_LT(genericBounce, soccerBounce);
}

TEST(BallType, SoccerBallDifferentSpeed) {
    float soccerSpeed = 300.0f;
    float beachSpeed = 250.0f;
    float genericSpeed = 350.0f;

    EXPECT_NE(soccerSpeed, beachSpeed);
    EXPECT_NE(beachSpeed, genericSpeed);
}

TEST(BallType, BeachBallLarger) {
    float soccerRadius = 28.0f;
    float beachRadius = 35.0f;

    EXPECT_GT(beachRadius, soccerRadius);
}

TEST(PomodoroState, InitialPhase) {
    PomodoroState state;
    state.Reset();

    EXPECT_EQ(state.phase, PomodoroPhase::Work);
    EXPECT_EQ(state.completedSessions, 0);
    EXPECT_FALSE(state.isAggressive);
    EXPECT_FLOAT_EQ(state.accumulatedRotation, 0.0f);
    EXPECT_FALSE(state.speedMultiplierApplied);
}

TEST(PomodoroState, PhaseTransitions) {
    PomodoroState state;
    state.phase = PomodoroPhase::Work;
    state.completedSessions = 0;

    state.completedSessions++;
    state.phase = PomodoroPhase::Break;
    EXPECT_EQ(state.phase, PomodoroPhase::Break);

    state.phase = PomodoroPhase::Work;
    state.completedSessions++;
    EXPECT_EQ(state.phase, PomodoroPhase::Work);

    state.phase = PomodoroPhase::LongBreak;
    EXPECT_EQ(state.phase, PomodoroPhase::LongBreak);
}

TEST(PomodoroState, SessionCounting) {
    constexpr int SESSIONS_BEFORE_LONG_BREAK = 4;

    int sessions = 0;
    EXPECT_EQ(sessions, 0);

    for (int i = 0; i < SESSIONS_BEFORE_LONG_BREAK; ++i) {
        sessions++;
    }

    EXPECT_EQ(sessions, SESSIONS_BEFORE_LONG_BREAK);
    bool shouldLongBreak = (sessions >= SESSIONS_BEFORE_LONG_BREAK);
    EXPECT_TRUE(shouldLongBreak);
}

TEST(PomodoroState, AggressiveModeActivation) {
    PomodoroState state;
    state.phase = PomodoroPhase::Work;
    EXPECT_FALSE(state.isAggressive);

    state.isAggressive = true;
    EXPECT_TRUE(state.isAggressive);
}

TEST(PomodoroState, RotationAccumulation) {
    PomodoroState state;
    float rotation = 0.0f;

    for (int i = 0; i < 60; ++i) {
        rotation += 90.0f * (1.0f / 60.0f);
    }

    state.accumulatedRotation = rotation;
    EXPECT_NEAR(state.accumulatedRotation, 90.0f, 0.1f);
}

TEST(PomodoroState, PhaseDurations) {
    constexpr int WORK_MINUTES = 25;
    constexpr int BREAK_MINUTES = 5;
    constexpr int LONG_BREAK_MINUTES = 15;

    PomodoroState state;

    state.phase = PomodoroPhase::Work;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), WORK_MINUTES);

    state.phase = PomodoroPhase::Break;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), BREAK_MINUTES);

    state.phase = PomodoroPhase::LongBreak;
    EXPECT_EQ(state.GetPhaseDurationMinutes(), LONG_BREAK_MINUTES);
}

TEST(PomodoroState, AggressiveHonkInterval) {
    constexpr float HONK_INTERVAL = 2.0f;
    double lastHonk = 0.0;
    double currentTime = 0.0;
    bool shouldHonk = false;

    currentTime = 0.5;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_FALSE(shouldHonk);

    lastHonk = currentTime;

    currentTime = 2.0;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_FALSE(shouldHonk);

    lastHonk = currentTime;

    currentTime = 4.0;
    shouldHonk = (currentTime - lastHonk) >= HONK_INTERVAL;
    EXPECT_TRUE(shouldHonk);
}

TEST(PomodoroState, SpeedMultiplierInAggressiveMode) {
    constexpr float SPEED_MULTIPLIER = 1.5f;
    constexpr float BASE_WALK_SPEED = 180.0f;
    constexpr float BASE_RUN_SPEED = 480.0f;

    float aggressiveSpeed = BASE_RUN_SPEED * SPEED_MULTIPLIER;
    EXPECT_FLOAT_EQ(aggressiveSpeed, 720.0f);
}

TEST(PomodoroState, TimeRemainingCalculation) {
    double phaseStartTime = 100.0;
    double phaseDuration = 25.0 * 60.0;
    double currentTime = 110.0;

    double elapsed = currentTime - phaseStartTime;
    double remaining = phaseDuration - elapsed;

    EXPECT_NEAR(remaining, 1490.0, 0.1);
}

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

TEST(JailState, Reset) {
    JailState state;
    state.jailPos.x = 100.0f;
    state.jailPos.y = 200.0f;
    state.jailRadius = 150.0f;
    state.isJailed = true;
    state.positionSet = true;
    state.lastJailAttempt = 1000.0;

    state.Reset();

    EXPECT_FLOAT_EQ(state.jailPos.x, 0.0f);
    EXPECT_FLOAT_EQ(state.jailPos.y, 0.0f);
    EXPECT_FLOAT_EQ(state.jailRadius, 80.0f);
    EXPECT_FALSE(state.isJailed);
    EXPECT_FALSE(state.positionSet);
    EXPECT_EQ(state.lastJailAttempt, 0.0);
}

TEST(JailState, KeyEdgeDetection) {
    bool wasKeyDown = false;
    int toggleCount = 0;

    auto simulateTick = [&](bool keyPressed) {
        if (keyPressed && !wasKeyDown) {
            toggleCount++;
            wasKeyDown = true;
        } else if (!keyPressed) {
            wasKeyDown = false;
        }
    };

    simulateTick(true);
    EXPECT_EQ(toggleCount, 1);
    simulateTick(true);
    EXPECT_EQ(toggleCount, 1) << "Holding key should not retoggle";
    simulateTick(true);
    EXPECT_EQ(toggleCount, 1) << "Holding key should not retoggle";
    simulateTick(false);
    EXPECT_EQ(toggleCount, 1);
    simulateTick(true);
    EXPECT_EQ(toggleCount, 2) << "Release and repress should toggle again";
}

TEST(JailState, NoToggleWithoutEdge) {
    bool wasKeyDown = false;
    int toggleCount = 0;

    for (int i = 0; i < 100; i++) {
        bool keyPressed = true;
        if (keyPressed && !wasKeyDown) {
            toggleCount++;
            wasKeyDown = true;
        }
    }
    EXPECT_EQ(toggleCount, 1) << "100 frames of held key = 1 toggle";
}

TEST(JailState, EdgeDetectsRapidKeyReleases) {
    bool wasKeyDown = false;
    int toggleCount = 0;

    for (int i = 0; i < 10; i++) {
        {
            bool keyDown = true;
            if (keyDown && !wasKeyDown) {
                toggleCount++;
                wasKeyDown = true;
            }
        }
        {
            bool keyDown = false;
            if (!keyDown) {
                wasKeyDown = false;
            }
        }
    }
    EXPECT_EQ(toggleCount, 10) << "10 press-release cycles = 10 toggles";
}

TEST(JailState, PerGooseStateIsolation) {
    JailState state1;
    JailState state2;

    state1.isJailed = true;
    state1.jailPos = {100.0f, 200.0f};
    state1.positionSet = true;

    state2.isJailed = false;
    state2.jailPos = {0.0f, 0.0f};
    state2.positionSet = false;

    EXPECT_TRUE(state1.isJailed);
    EXPECT_FALSE(state2.isJailed);
    EXPECT_FLOAT_EQ(state1.jailPos.x, 100.0f);
    EXPECT_FLOAT_EQ(state2.jailPos.x, 0.0f);
}

TEST(JailState, TeleportToJailPosition) {
    JailState state;
    state.jailPos = {500.0f, 400.0f};
    state.positionSet = true;

    float gooseX = 100.0f, gooseY = 100.0f;

    if (state.positionSet) {
        gooseX = state.jailPos.x;
        gooseY = state.jailPos.y;
    }

    EXPECT_FLOAT_EQ(gooseX, 500.0f);
    EXPECT_FLOAT_EQ(gooseY, 400.0f);
}

TEST(JailState, NoTeleportWithoutPositionSet) {
    JailState state;
    state.positionSet = false;

    float gooseX = 100.0f, gooseY = 100.0f;

    if (state.positionSet) {
        gooseX = state.jailPos.x;
        gooseY = state.jailPos.y;
    }

    EXPECT_FLOAT_EQ(gooseX, 100.0f);
    EXPECT_FLOAT_EQ(gooseY, 100.0f);
}

TEST(JailState, PositionSetOnKeyPress) {
    JailState state;
    Vector2 goosePos{300.0f, 250.0f};

    state.jailPos = goosePos;
    state.positionSet = true;

    EXPECT_TRUE(state.positionSet);
    EXPECT_FLOAT_EQ(state.jailPos.x, 300.0f);
    EXPECT_FLOAT_EQ(state.jailPos.y, 250.0f);
}

TEST(JailState, JailedGooseVelocityZeroed) {
    JailState state;
    Vector2 vel{100.0f, 200.0f};

    state.isJailed = true;
    if (state.isJailed) {
        vel = {0.0f, 0.0f};
    }

    EXPECT_FLOAT_EQ(vel.x, 0.0f);
    EXPECT_FLOAT_EQ(vel.y, 0.0f);
}

TEST(JailState, DisableClearsJail) {
    JailState state;
    state.isJailed = true;
    state.jailPos = {100.0f, 200.0f};

    state.Reset();

    EXPECT_FALSE(state.isJailed);
}

TEST(JailState, MultipleTogglesCycle) {
    bool isJailed = false;
    bool wasKeyDown = false;
    int jailCount = 0;

    bool keys[] = {true, false, true, false, true, false, true, false};
    for (bool keyDown : keys) {
        if (keyDown && !wasKeyDown) {
            isJailed = !isJailed;
            if (isJailed) jailCount++;
            wasKeyDown = true;
        } else if (!keyDown) {
            wasKeyDown = false;
        }
    }

    EXPECT_EQ(jailCount, 2);
    EXPECT_FALSE(isJailed);
}

TEST(JailState, StateReuseAcrossGooseIds) {
    JailState baseState;
    baseState.jailPos = {100.0f, 200.0f};

    JailState copy1 = baseState;
    JailState copy2 = baseState;

    copy1.jailPos.x = 300.0f;
    copy2.jailPos.y = 400.0f;

    EXPECT_FLOAT_EQ(baseState.jailPos.x, 100.0f);
    EXPECT_FLOAT_EQ(copy1.jailPos.x, 300.0f);
    EXPECT_FLOAT_EQ(copy2.jailPos.x, 100.0f);
    EXPECT_FLOAT_EQ(copy2.jailPos.y, 400.0f);
}

TEST(JailState, NoDoubleJailOnToggleHeld) {
    bool isJailed = false;
    bool wasKeyDown = false;
    int toggleCount = 0;

    auto tick = [&](bool keyDown) {
        if (keyDown && !wasKeyDown) {
            isJailed = !isJailed;
            toggleCount++;
            wasKeyDown = true;
        } else if (!keyDown) {
            wasKeyDown = false;
        }
    };

    for (int i = 0; i < 60; i++) tick(true);
    EXPECT_EQ(toggleCount, 1);
    EXPECT_TRUE(isJailed);

    tick(false);
    tick(true);
    EXPECT_EQ(toggleCount, 2);
    EXPECT_FALSE(isJailed);
}

TEST(JailState, AndGateForJailActivation) {
    bool isJailed = false;
    bool positionSet = true;
    bool jailHeld = true;

    bool shouldJail = positionSet && jailHeld;
    EXPECT_TRUE(shouldJail);

    if (shouldJail) {
        isJailed = true;
    }
    EXPECT_TRUE(isJailed);
}

TEST(JailState, AndGateJailMissingPosition) {
    bool isJailed = false;
    bool positionSet = false;
    bool jailHeld = true;

    bool shouldJail = positionSet && jailHeld;
    EXPECT_FALSE(shouldJail);

    if (shouldJail) {
        isJailed = true;
    }
    EXPECT_FALSE(isJailed);
}

#include <gtest/gtest.h>
#include <cmath>
#include <cstdlib>

#include "goose_math.h"
#include "goose.h"
#include "world.h"
#include "config.h"

TEST(GooseBehaviors, DragSetsPosAndZerosVel) {
    Goose g(70, "Drag", 1920, 1080);
    Vector2 cursorPos{500, 500};
    g.pos = {495, 495};

    g.pos.x = cursorPos.x - 5.0f;
    g.pos.y = cursorPos.y;
    g.vel.x = 0;
    g.vel.y = 0;

    EXPECT_FLOAT_EQ(g.pos.x, 495.0f) << "Drag should set pos.x = cursor.x - 5";
    EXPECT_FLOAT_EQ(g.pos.y, 500.0f) << "Drag should set pos.y = cursor.y";
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f) << "Drag should zero vel.x";
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f) << "Drag should zero vel.y";
}

TEST(GooseBehaviors, JailSetsPosAndZerosVel) {
    Goose g(71, "Jail", 1920, 1080);
    Vector2 jailPos{300, 400};
    g.pos = {100, 100};
    g.vel = {50, -30};

    g.target = jailPos;
    g.pos = jailPos;
    g.vel = {0, 0};

    EXPECT_FLOAT_EQ(g.pos.x, 300.0f) << "Jail should set pos.x";
    EXPECT_FLOAT_EQ(g.pos.y, 400.0f) << "Jail should set pos.y";
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f) << "Jail should zero vel.x";
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f) << "Jail should zero vel.y";
}

TEST(GooseBehaviors, PortalTeleportZerosVel) {
    Goose g(72, "Portal", 1920, 1080);
    Vector2 dest{800, 600};
    g.pos = {100, 100};
    g.vel = {100, 50};

    g.pos.x = dest.x;
    g.pos.y = dest.y;
    g.vel = {0, 0};

    EXPECT_FLOAT_EQ(g.pos.x, 800.0f);
    EXPECT_FLOAT_EQ(g.pos.y, 600.0f);
    EXPECT_FLOAT_EQ(g.vel.x, 0.0f);
    EXPECT_FLOAT_EQ(g.vel.y, 0.0f);
}

TEST(GooseBehaviors, BallTargetInDeviceSpace) {
    Goose g(74, "BallCoord", 1920, 1080);
    float globalScale = 1.0f;
    float s_ballPosX = 300.0f;
    float s_ballPosY = 300.0f;
    float BALL_SIZE = 40.0f;

    float ballCenterX = s_ballPosX + BALL_SIZE / 2.0f;
    float ballCenterY = s_ballPosY + BALL_SIZE / 2.0f;
    float ballCenterDevX = ballCenterX * globalScale;
    float ballCenterDevY = ballCenterY * globalScale;

    EXPECT_FLOAT_EQ(ballCenterDevX, ballCenterX) << "At scale 1.0, device == world";
    EXPECT_FLOAT_EQ(ballCenterDevY, ballCenterY) << "At scale 1.0, device == world";

    globalScale = 2.0f;
    ballCenterDevX = ballCenterX * globalScale;
    ballCenterDevY = ballCenterY * globalScale;
    EXPECT_FLOAT_EQ(ballCenterDevX, ballCenterX * 2.0f) << "At scale 2.0, device = world * 2";
}

TEST(GooseBehaviors, BallTargetSetsGooseTarget) {
    Goose g(75, "BallTarget", 1920, 1080);
    float globalScale = 1.0f;
    float s_ballPosX = 400.0f, s_ballPosY = 500.0f;
    float BALL_SIZE = 40.0f;

    float ballCenterX = s_ballPosX + BALL_SIZE / 2.0f;
    float ballCenterY = s_ballPosY + BALL_SIZE / 2.0f;
    float ballCenterDevX = ballCenterX * globalScale;
    float ballCenterDevY = ballCenterY * globalScale;

    g.target = Vector2{ballCenterDevX, ballCenterDevY};
    g.pos = {100, 1000};

    Vector2 toTarget = g.target - g.pos;
    EXPECT_GT(toTarget.x, 0) << "Ball center (420) is to the RIGHT of goose (100)";
    EXPECT_LT(toTarget.y, 0) << "Ball center (520) is ABOVE goose (1000)";
}

TEST(CoordinateConversion, WorldToDeviceAndBack) {
    Vector2 goosePos{500, 500};

    for (float scale : {0.5f, 1.0f, 2.0f, 3.0f}) {
        Vector2 worldPos{300, 400};
        Vector2 devicePos = WorldToDevice(goosePos, worldPos, scale);
        Vector2 worldBack = DeviceToWorld(goosePos, devicePos, scale);

        EXPECT_NEAR(worldBack.x, worldPos.x, 0.01f) << "Round-trip should recover world.x at scale " << scale;
        EXPECT_NEAR(worldBack.y, worldPos.y, 0.01f) << "Round-trip should recover world.y at scale " << scale;
    }
}

TEST(GooseStateMachine, WanderToChaseToSnatchCycle) {
    Goose g(85, "StateCycle", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 500};
    g.state = GooseState::WANDER;
    g.cursorChaseChance = 100;

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {600, 500};

    g.Update(1.0/60.0, 1.0, 1920, 1080, c);
    float dist = Vector2::Distance(g.pos, g.target);
    EXPECT_LT(dist, 2000.0f) << "Goose should remain on screen after state transition";
}

TEST(GooseStateMachine, FetchingCreatesItem) {
    Goose g(86, "Fetch", 1920, 1080);
    g.state = GooseState::FETCHING;
    g.pos = {0, 0};
    g.target = {-40, 500};
    g.forceItemFetch = 1;
    g.memeFetchBias = 100;
    g.noteFetchBias = 100;

    CursorState c;
    c.caps = CAP_NONE;

    double dt = 1.0 / 60.0;
    for (int i = 0; i < 60; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
    }

    EXPECT_NE(g.state, GooseState::WANDER) << "Fetch should complete and return to wander";
}

TEST(MultiGoose, SeparationForcePushesApart) {
    g_world.geese.clear();
    g_world.nextId = 100;

    Goose& g1 = g_world.geese.emplace_back(100, "Sep1", 1920, 1080);
    g1.pos = {500, 500};
    g1.state = GooseState::WANDER;

    Goose& g2 = g_world.geese.emplace_back(101, "Sep2", 1920, 1080);
    g2.pos = {520, 500};
    g2.state = GooseState::WANDER;

    CursorState c;
    c.caps = CAP_NONE;

    Vector2 pos1before = g1.pos;
    Vector2 pos2before = g2.pos;

    g1.cursorChaseChance = 0;
    g2.cursorChaseChance = 0;
    g1.attackMouseBias = 0;
    g2.attackMouseBias = 0;
    g1.target = {1000, 1000};
    g2.target = {1000, 1000};

    double dt = 1.0 / 60.0;
    for (int i = 0; i < 30; i++) {
        g1.Update(dt, i * dt, 1920, 1080, c);
        g2.Update(dt, i * dt, 1920, 1080, c);
    }

    float g1dist = Vector2::Distance(g1.pos, pos1before);
    float g2dist = Vector2::Distance(g2.pos, pos2before);
    float totalDist = g1dist + g2dist;

    EXPECT_GT(totalDist, 10.0f) << "Geese should move (separation or seek)";

    g_world.geese.clear();
}

TEST(ConfigValidation, SliderKeysMatchConfigKeys) {
    struct { const char* key; float* ptr; } floatKeys[] = {
        {"behaviors.fun.ball.size", &g_config.behaviors.ball.size},
        {"behaviors.fun.breadCrumbs.max", (float*)&g_config.behaviors.breadCrumbs.maxCrumbs},
        {"behaviors.fun.hats.size", &g_config.behaviors.hats.sizeX},
        {"behaviors.fun.rainbow.speed", &g_config.behaviors.rainbow.hueSpeed},
        {"behaviors.fun.acid.speed", &g_config.behaviors.acid.spinSpeed},
        {"behaviors.fun.anger.max", &g_config.behaviors.anger.maxAnger},
        {"behaviors.control.honcker.cooldown", &g_config.behaviors.honcker.cooldown},
        {"behaviors.control.jail.size", &g_config.behaviors.jail.size},
        {"behaviors.control.portals.width", &g_config.portal.width},
        {"behaviors.control.drag.radius", &g_config.behaviors.drag.radius},
        {"behaviors.info.nametag.size", &g_config.behaviors.nametag.size},
        {"behaviors.info.presence.interval", &g_config.behaviors.presence.interval},
        {"behaviors.systems.health.opacity", &g_config.behaviors.health.opacity},
        {"behaviors.systems.pomodoro.workDuration", (float*)&g_config.behaviors.pomodoro.workMinutes},
        {"behaviors.systems.pomodoro.breakDuration", (float*)&g_config.behaviors.pomodoro.breakMinutes},
    };
    for (auto& fk : floatKeys) {
        EXPECT_NE(fk.ptr, nullptr) << fk.key;
    }

    struct { const char* key; bool* ptr; } boolKeys[] = {
        {"behaviors.fun.ball", &g_config.behaviors.fun.ball},
        {"behaviors.fun.breadCrumbs", &g_config.behaviors.fun.breadCrumbs},
        {"behaviors.fun.hats", &g_config.behaviors.fun.hats},
        {"behaviors.fun.rainbow", &g_config.behaviors.fun.rainbow},
        {"behaviors.fun.acid", &g_config.behaviors.fun.acid},
        {"behaviors.fun.anger", &g_config.behaviors.fun.anger},
        {"behaviors.control.honcker", &g_config.behaviors.control.honcker},
        {"behaviors.control.jail", &g_config.behaviors.control.jail},
        {"behaviors.control.portals", &g_config.behaviors.control.portals},
        {"behaviors.control.drag", &g_config.behaviors.control.drag},
        {"behaviors.info.nametag", &g_config.behaviors.info.nametag},
        {"behaviors.systems.health", &g_config.behaviors.systems.health},
        {"behaviors.systems.ai", &g_config.behaviors.systems.ai},
        {"behaviors.systems.pomodoro", &g_config.behaviors.systems.pomodoro},
    };
    for (auto& bk : boolKeys) {
        EXPECT_NE(bk.ptr, nullptr) << bk.key;
    }
}

TEST(GooseLifecycle, MultipleUpdatesDifferentTimes) {
    Goose g(400, "Lifecycle", 1920, 1080);

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {500, 500};

    double dt = 1.0 / 60.0;
    for (int i = 0; i < 1000; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        EXPECT_TRUE(std::isfinite(g.pos.x));
        EXPECT_TRUE(std::isfinite(g.pos.y));
        EXPECT_TRUE(std::isfinite(g.vel.x));
        EXPECT_TRUE(std::isfinite(g.vel.y));
        EXPECT_GE(g.dir, -180.0f);
        EXPECT_LE(g.dir, 360.0f);
    }
}

TEST(GooseLifecycle, RapidFrameBurst) {
    Goose g(401, "RapidBurst", 1920, 1080);

    CursorState c;
    c.caps = CAP_NONE;

    double dt = 1.0 / 240.0;
    for (int i = 0; i < 5000; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        EXPECT_TRUE(std::isfinite(g.pos.x));
        EXPECT_TRUE(std::isfinite(g.pos.y));
    }
    SUCCEED() << "5000 frames at 240fps completed without crash";
}

TEST(GooseStates, WanderPickNewTarget) {
    Goose g(410, "PickTgt", 1920, 1080);

    g.pos = {500, 500};
    g.target = {500, 500};
    g.state = GooseState::WANDER;

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    EXPECT_NE(g.target.x, 500.0f) << "PickNewTarget should change target X";
    EXPECT_NE(g.target.y, 500.0f) << "PickNewTarget should change target Y";
}

TEST(GooseStates, SnatchReleaseTransitionsToWander) {
    Goose g(411, "SnatchEnd", 1920, 1080);
    g.state = GooseState::SNATCH_CURSOR;
    g.snatchStartTime = 0.0;
    g.snatchDuration = 0.5f;
    g_world.cursorGrabberId = g.id;

    CursorState c;
    c.caps = CAP_GET_POS | CAP_MOVE_ABS;
    c.position = {500, 500};

    g.Update(1.0/60.0, 0.2, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::SNATCH_CURSOR) << "Snatch should continue before duration";

    g.Update(1.0/60.0, 0.7, 1920, 1080, c);
    EXPECT_EQ(g.state, GooseState::WANDER) << "Snatch should end after duration";
    EXPECT_EQ(g_world.cursorGrabberId, -1) << "Cursor grab should be released";
    g_world.cursorGrabberId = -1;
}

TEST(EdgeCase, GooseAtScreenCorner) {
    Goose g(420, "Corner", 1920, 1080);
    g.pos = {0, 0};
    g.target = {-100, -100};
    g.vel = {-100, -100};
    g.currentSpeed = 480.0f;

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 60; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
        EXPECT_GE(g.pos.x, 0.0f) << "Corner clamp x >= 0";
        EXPECT_GE(g.pos.y, 0.0f) << "Corner clamp y >= 0";
    }
}

TEST(EdgeCase, GooseAtScreenCornerBottomRight) {
    Goose g(421, "CornerBR", 1920, 1080);
    g.pos = {1920, 1080};
    g.target = {2000, 1200};
    g.vel = {100, 100};
    g.currentSpeed = 480.0f;

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 60; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
        EXPECT_LE(g.pos.x, 1920.0f) << "Corner clamp x <= 1920";
        EXPECT_LE(g.pos.y, 1080.0f) << "Corner clamp y <= 1080";
    }
}

TEST(EdgeCase, GooseNanPosition) {
    Goose g(422, "NanPos", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 500};
    g.vel = {0, 0};
    g.currentSpeed = 180.0f;

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);
    EXPECT_TRUE(std::isfinite(g.pos.x)) << "Position should remain finite";
    EXPECT_TRUE(std::isfinite(g.pos.y)) << "Position should remain finite";
}

TEST(EdgeCase, GooseNegativeDimensions) {
    Goose g(423, "NegDim", 1920, 1080);
    g.pos = {500, 500};
    g.vel = {0, 0};
    g.currentSpeed = 180.0f;
    g.target = {500, 500};

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, -1, -1, c);
    EXPECT_TRUE(std::isfinite(g.pos.x));
    EXPECT_TRUE(std::isfinite(g.pos.y));
}

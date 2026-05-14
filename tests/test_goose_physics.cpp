#include <gtest/gtest.h>
#include <cmath>
#include <cstdlib>

#include "goose_math.h"
#include "goose.h"
#include "world.h"

TEST(GoosePhysics, SeekForcePointsTowardTarget) {
    Goose g(60, "Seek", 1920, 1080);
    g.pos = {100, 100};
    g.target = {500, 400};
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    g.Update(dt, 0.0, 1920, 1080, c);

    EXPECT_GT(g.vel.x, 0.0f) << "Velocity X should point toward target (positive)";
    EXPECT_GT(g.vel.y, 0.0f) << "Velocity Y should point toward target (positive)";

    float dirRatio = g.vel.y / (g.vel.x + 1e-6f);
    float targetRatio = 300.0f / 400.0f;
    EXPECT_NEAR(dirRatio, targetRatio, 0.5f) << "Velocity ratio should roughly match target ratio";
}

TEST(GoosePhysics, SeekForceReversesWhenTargetIsBehind) {
    Goose g(61, "SeekReverse", 1920, 1080);
    g.pos = {500, 500};
    g.target = {100, 100};
    g.currentSpeed = 180.0f;
    g.vel = {200, 0};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 120; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
    }

    EXPECT_LT(g.vel.x, 0.0f) << "After 2s, velocity X should reverse toward target";
    EXPECT_LT(g.vel.y, 0.0f) << "After 2s, velocity Y should point toward target";
}

TEST(GoosePhysics, VelocityStaysBelowCurrentSpeed) {
    Goose g(62, "SpeedCap", 1920, 1080);
    g.pos = {0, 0};
    g.target = {5000, 0};
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 600; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        float speed = Vector2::Length(g.vel);
        EXPECT_LE(speed, g.currentSpeed + 1.0f) << "Velocity magnitude should not exceed currentSpeed";
    }
}

TEST(GoosePhysics, ClampToScreenKeepsGooseOnScreen) {
    Goose g(63, "Clamp", 1920, 1080);
    g.pos = {0, 0};
    g.target = {1920, 1080};
    g.vel = {-500, -500};

    double dt = 1.0 / 60.0;
    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 10; i++) {
        g.Update(dt, i * dt, 1920, 1080, c);
        EXPECT_GE(g.pos.x, 0.0f) << "ClampToScreen should keep x >= 0";
        EXPECT_GE(g.pos.y, 0.0f) << "ClampToScreen should keep y >= 0";
        EXPECT_LE(g.pos.x, 1920.0f) << "ClampToScreen should keep x <= screenW";
        EXPECT_LE(g.pos.y, 1080.0f) << "ClampToScreen should keep y <= screenH";
    }
}

TEST(GoosePhysics, SeekForceSettlesAtArrival) {
    Goose g(200, "Arrival", 1920, 1080);
    g.pos = {100, 100};
    g.target = {1900, 100};
    g.currentSpeed = 480.0f;
    g.vel = {0, 0};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 120; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
    }

    EXPECT_GT(g.vel.x, 100.0f) << "Goose should be moving right at high speed";
    EXPECT_NEAR(g.vel.y, 0.0f, 20.0f) << "Lateral velocity should be near zero";
}

TEST(GoosePhysics, SeekTargetAtCurrentPos) {
    Goose g(202, "SeekNone", 1920, 1080);
    g.pos = {500, 500};
    g.target = {535, 500};
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};
    g.cursorChaseChance = 0;
    g.attackMouseBias = 0;

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    float dist = Vector2::Distance(g.pos, g.target);
    EXPECT_GT(dist, 25.0f) << "Goose should not have reached target yet";
    EXPECT_GT(g.vel.x, 0.0f) << "Seek should push RIGHT toward target";
}

TEST(GoosePhysics, SeekBothAxesIndependent) {
    Goose g(201, "SeekAxes", 1920, 1080);
    g.pos = {500, 500};
    g.target = {1500, 100};
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    EXPECT_GT(g.vel.x, 0.0f) << "Seek should push RIGHT (positive X)";
    EXPECT_LT(g.vel.y, 0.0f) << "Seek should push UP (negative Y)";
}

TEST(GoosePhysics, EdgeAvoidMarginActivates) {
    Goose g(203, "EdgeAvoid", 1920, 1080);
    g.pos = {10, 500};
    g.target = {-100, 500};
    g.currentSpeed = 180.0f;
    g.vel = {-180, 0};

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 60; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
    }

    EXPECT_GE(g.pos.x, 0.0f) << "Edge avoid should keep goose.x >= 0";
}

TEST(GoosePhysics, SeekFromMultipleDirections) {
    CursorState c;
    c.caps = CAP_NONE;

    srand(42);
    for (int trial = 0; trial < 50; trial++) {
        Goose g(300 + trial, "SeekRnd", 1920, 1080);
        g.pos = {(float)(rand() % 1920), (float)(rand() % 1080)};
        g.target = {(float)(rand() % 1920), (float)(rand() % 1080)};
        g.currentSpeed = 180.0f;
        g.vel = {0, 0};

        g.Update(1.0/60.0, 0.0, 1920, 1080, c);

        Vector2 toTarget = g.target - g.pos;
        float dot = toTarget.x * g.vel.x + toTarget.y * g.vel.y;
        EXPECT_GT(dot, -1.0f) << "Seek should never push away from target";
    }
}

TEST(GooseFeet, SolveFeetProducesConsistentPositions) {
    Goose g(110, "Feet", 1920, 1080);
    g.pos = {500, 500};
    g.dir = 0.0f;
    g.currentSpeed = 180.0f;
    g.vel = {180, 0};

    CursorState c;
    c.caps = CAP_NONE;
    c.position = {1920, 1080};

    for (int i = 0; i < 30; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);

        EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.x));
        EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.y));
        EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.x));
        EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.y));

        float lDist = Vector2::Distance(g.rig.lFoot.currentPos, g.pos);
        float rDist = Vector2::Distance(g.rig.rFoot.currentPos, g.pos);
        EXPECT_LT(lDist, 100.0f) << "Left foot should be near goose";
        EXPECT_LT(rDist, 100.0f) << "Right foot should be near goose";
    }
}

TEST(GooseFeet, SolveFeetFootPositionsSymmetric) {
    Goose g(111, "FeetSym", 1920, 1080);
    g.pos = {960, 540};
    g.dir = 0.0f;
    g.currentSpeed = 0.0f;
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0 / 60.0, 0.0, 1920, 1080, c);

    EXPECT_NE(g.rig.lFoot.currentPos.x, 0.0f) << "Left foot should be initialized";
    EXPECT_NE(g.rig.rFoot.currentPos.x, 0.0f) << "Right foot should be initialized";

    float diffX = std::abs(g.rig.rFoot.currentPos.x - g.rig.lFoot.currentPos.x);
    float diffY = std::abs(g.rig.rFoot.currentPos.y - g.rig.lFoot.currentPos.y);
    float totalDiff = diffX + diffY;
    EXPECT_GT(totalDiff, 0.01f) << "Feet should be at different positions";
}

TEST(GooseFeet, FeetFollowGoosePosition) {
    Goose g(220, "FeetFollow", 1920, 1080);
    g.pos = {100, 100};
    g.vel = {0, 0};
    g.currentSpeed = 0.0f;

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    g.pos = {200, 100};
    g.target = {500, 100};

    g.Update(1.0/60.0, 0.017, 1920, 1080, c);

    float lfDist = Vector2::Distance(g.rig.lFoot.currentPos, g.pos);
    float rfDist = Vector2::Distance(g.rig.rFoot.currentPos, g.pos);
    EXPECT_LT(lfDist, 200.0f) << "Left foot should stay near goose after movement";
    EXPECT_LT(rfDist, 200.0f) << "Right foot should stay near goose after movement";
}

TEST(GooseFeet, SolveFeetWithHighCurrentSpeed) {
    Goose g(221, "FeetSpeed", 1920, 1080);
    g.pos = {500, 500};
    g.currentSpeed = 480.0f;
    g.vel = {480, 0};

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 30; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
        EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.x));
        EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.x));
        EXPECT_LT(Vector2::Distance(g.rig.lFoot.currentPos, g.pos), 100.0f);
        EXPECT_LT(Vector2::Distance(g.rig.rFoot.currentPos, g.pos), 100.0f);
    }
}

TEST(GooseDirection, UpdateDirectionFromVelocity) {
    Goose g(210, "DirUpdate", 1920, 1080);
    g.pos = {500, 500};
    g.vel = {100, 0};
    g.dir = 0.0f;

    CursorState c;
    c.caps = CAP_NONE;

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    EXPECT_NEAR(g.dir, 0.0f, 30.0f) << "Direction should face right when vel is rightward";
}

TEST(GooseDirection, DirectionBlendsSmoothly) {
    Goose g(211, "DirBlend", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 0};
    g.vel = {0, 0};
    g.dir = 0.0f;

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 120; i++) {
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
    }
    float expectedDir = -90.0f;
    EXPECT_NEAR(g.dir, expectedDir, 30.0f) << "Direction should converge toward target direction";
}

TEST(FeetStability, ConsecutiveUpdatesNoNanFeet) {
    Goose g(500, "FeetStress", 1920, 1080);
    g.pos = {500, 500};
    g.target = {800, 600};
    g.currentSpeed = 180.0f;
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    for (int batch = 0; batch < 500; batch++) {
        for (int call = 0; call < 3; call++) {
            g.Update(1.0/60.0, batch / 60.0, 1920, 1080, c);

            EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.x))
                << "lFoot.x NaN at batch " << batch << " call " << call;
            EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.y))
                << "lFoot.y NaN at batch " << batch << " call " << call;
            EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.x))
                << "rFoot.x NaN at batch " << batch << " call " << call;
            EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.y))
                << "rFoot.y NaN at batch " << batch << " call " << call;

            EXPECT_LT(Vector2::Distance(g.rig.lFoot.currentPos, g.pos), 200.0f)
                << "lFoot too far at batch " << batch;
            EXPECT_LT(Vector2::Distance(g.rig.rFoot.currentPos, g.pos), 200.0f)
                << "rFoot too far at batch " << batch;
        }
    }
}

TEST(GoosePhysics, NoInternalDoubleUpdateGuard) {
    Goose g(600, "NoGuard", 1920, 1080);
    g.pos = {500, 500};
    g.target = {800, 500};
    g.currentSpeed = 480.0f;
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    for (int i = 0; i < 3; i++) {
        float before = g.pos.x;
        g.Update(1.0/60.0, i / 60.0, 1920, 1080, c);
        EXPECT_GT(g.pos.x, before) << "Update " << i << " must move goose (no internal guard)";
    }
}

TEST(FeetStability, SolveFeetNanGuardRecovers) {
    Goose g(501, "FeetRecover", 1920, 1080);
    g.pos = {500, 500};
    g.target = {500, 500};
    g.currentSpeed = 0.0f;
    g.vel = {0, 0};

    CursorState c;
    c.caps = CAP_NONE;

    g.rig.lFoot.currentPos = {NAN, NAN};
    g.rig.rFoot.currentPos = {NAN, NAN};

    g.Update(1.0/60.0, 0.0, 1920, 1080, c);

    EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.x));
    EXPECT_TRUE(std::isfinite(g.rig.lFoot.currentPos.y));
    EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.x));
    EXPECT_TRUE(std::isfinite(g.rig.rFoot.currentPos.y));

    EXPECT_LT(Vector2::Distance(g.rig.lFoot.currentPos, g.pos), 100.0f);
    EXPECT_LT(Vector2::Distance(g.rig.rFoot.currentPos, g.pos), 100.0f);
}

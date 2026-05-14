#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>

#include "goose_math.h"
#include "goose.h"
#include "world.h"

TEST(SoakTest, TenMinuteGoose) {
    Goose g(42, "Soak", 2560, 1440);

    CursorState c;
    c.caps = CAP_GET_POS;
    c.position = {1280, 720};

    double dt = 1.0 / 60.0;
    double time = 0.0;
    double maxFrames = 36000;

    double lastLogTime = 0;
    const double LOG_INTERVAL = 5.0;

    Vector2 startPos = g.pos;
    Vector2 prevPos = g.pos;
    double cumulativeDist = 0;
    double stuckDuration = 0;
    bool wasStuck = false;
    double maxStuckDuration = 0;
    int stateChanges = 0;
    int prevState = -1;
    int framesWithVelZero = 0;

    for (int frame = 0; frame < maxFrames; frame++, time += dt) {
        g.Update(dt, time, 2560, 1440, c);

        double speed = Vector2::Length(g.vel);
        double moveMag = Vector2::Distance(g.pos, prevPos);
        cumulativeDist += moveMag;

        if (speed < 1.0 && moveMag < 0.1) {
            stuckDuration += dt;
            maxStuckDuration = std::max(maxStuckDuration, stuckDuration);
            if (stuckDuration > 5.0 && !wasStuck) {
                fprintf(stderr, "[SOAK] STUCK t=%.0fs pos(%.0f,%.0f) target(%.0f,%.0f) state=%d\n",
                        time, g.pos.x, g.pos.y, g.target.x, g.target.y, (int)g.state);
                wasStuck = true;
            }
            framesWithVelZero++;
        } else {
            stuckDuration = 0;
            wasStuck = false;
        }

        if ((int)g.state != prevState) {
            stateChanges++;
            prevState = (int)g.state;
        }

        if (time - lastLogTime >= LOG_INTERVAL) {
            lastLogTime = time;
            const char* stateName = (g.state == GooseState::WANDER ? "W" :
                                     g.state == GooseState::FETCHING ? "F" :
                                     g.state == GooseState::RETURNING ? "R" :
                                     g.state == GooseState::CHASE_CURSOR ? "C" : "S");
            fprintf(stderr, "[SOAK] t=%.0f pos(%.0f,%.0f) vel(%.1f,%.1f) spd=%.1f cs=%.1f %s target(%.0f,%.0f) dist=%.0f cumul=%.0f\n",
                    time, g.pos.x, g.pos.y, g.vel.x, g.vel.y,
                    Vector2::Length(g.vel), g.currentSpeed, stateName,
                    g.target.x, g.target.y, Vector2::Distance(g.pos, g.target),
                    cumulativeDist);
        }

        prevPos = g.pos;
    }

    float traveled = Vector2::Distance(g.pos, startPos);
    fprintf(stderr, "[SOAK] Final: pos(%.0f,%.0f) vel(%.1f,%.1f) speed=%.1f state=%d\n",
            g.pos.x, g.pos.y, g.vel.x, g.vel.y, Vector2::Length(g.vel), (int)g.state);
    fprintf(stderr, "[SOAK] Net traveled: %.0f px, Cumulative: %.0f px, stateChanges: %d, maxStuck: %.0fs, zeroVelFrames: %d\n",
            traveled, cumulativeDist, stateChanges, maxStuckDuration, framesWithVelZero);

    EXPECT_GT(cumulativeDist, 2560.0f * 10.0f) << "Goose didn't move enough cumulatively in 10 minutes";
    EXPECT_LT(maxStuckDuration, 20.0) << "Goose was stuck for more than 20 seconds";
    EXPECT_GT(stateChanges, 5) << "Goose should change state at least 5 times in 10 minutes";
}

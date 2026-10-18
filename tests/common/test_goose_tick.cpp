// Goose paths that earlier tests reached only through state they leaked
// (ROADMAP G0.16): the per-frame tick's use of the cursor provider, the
// night-time walking speed, and the Stalin honk. Each now has a test that
// sets up its own precondition and asserts the outcome.
#include <gtest/gtest.h>

#include <ctime>
#include <vector>

#include "audio.h"
#include "config.h"
#include "cursor_io.h"
#include "goose.h"
#include "test_seams.h"
#include "world.h"

namespace {

struct RecordingCursor : public ICursorProvider {
    CursorState state;
    int reads = 0;
    std::vector<CursorAction> executed;
    CursorState Read() override {
        ++reads;
        return state;
    }
    void Execute(const CursorAction& action) override { executed.push_back(action); }
};

TEST(GooseTick, ASnatchingGooseMovesTheCursorThroughTheProvider) {
    RecordingCursor cursor;
    cursor.state.caps = CAP_GET_POS | CAP_MOVE_ABS;
    cursor.state.position = {500, 500};
    g_cursorProvider = &cursor;  // the reset listener restores the original

    Goose g(511, "Snatcher", 1920, 1080);
    g.state = GooseState::SNATCH_CURSOR;
    g.snatchStartTime = 0.0;
    g.snatchDuration = 5.0f;
    g_world.cursorGrabberId = g.id;

    WorldContext world = g_world;
    g.tick(world, 1.0 / 60.0, 0.2);

    EXPECT_EQ(cursor.reads, 1);
    ASSERT_EQ(cursor.executed.size(), 1u);
    EXPECT_EQ(cursor.executed[0].type, CursorAction::MOVE_ABS);
}

TEST(GooseTick, AWanderingGooseLeavesTheCursorAlone) {
    RecordingCursor cursor;
    cursor.state.caps = CAP_GET_POS | CAP_MOVE_ABS;
    cursor.state.position = {1800, 1000};
    g_cursorProvider = &cursor;

    Goose g(512, "Wanderer", 1920, 1080);
    g.state = GooseState::WANDER;
    g.cursorChaseChance = 0;
    g.pos = {200, 200};
    g.target = {400, 200};

    WorldContext world = g_world;
    g.tick(world, 1.0 / 60.0, 1.0);

    EXPECT_EQ(cursor.reads, 1);
    EXPECT_TRUE(cursor.executed.empty());
}

// One frame of walking toward a target well inside the walk range; with the
// lerp rate at 1 the goose is at its target speed after that frame.
float WalkSpeedAfterOneFrame(bool night) {
    Nighttime_SetForTest(night);
    g_config.movement.speedLerpRate = 1.0f;
    Goose g(513, "Walker", 1920, 1080);
    g.state = GooseState::WANDER;
    g.cursorChaseChance = 0;
    g.pos = {500, 500};
    g.target = {800, 500};
    CursorState none{};
    g.Update(1.0 / 60.0, 1.0, 1920, 1080, none);
    return g.currentSpeed;
}

TEST(GooseNight, GeeseWalkAtSixTenthsSpeedAtNight) {
    const float day = WalkSpeedAfterOneFrame(false);
    const float night = WalkSpeedAfterOneFrame(true);
    EXPECT_FLOAT_EQ(day, g_config.movement.baseWalkSpeed);
    EXPECT_FLOAT_EQ(night, day * 0.6f);
}

TEST(GooseNight, UnpinnedItFollowsTheLocalClock) {
    Nighttime_SetForTest(std::nullopt);
    const time_t now = std::time(nullptr);
    tm local{};
    ASSERT_NE(localtime_r(&now, &local), nullptr);
    const bool expected = local.tm_hour >= 23 || local.tm_hour < 6;
    EXPECT_EQ(Nighttime_IsNowForTest(), expected);
    // Asked again inside the minute, it answers from its cache, unchanged.
    EXPECT_EQ(Nighttime_IsNowForTest(), expected);
}

TEST(GooseHonk, TheStalinGooseSoundsTheGulagNotAHonk) {
    Audio_SetMuted(false);  // the silent sink keeps this off the speakers
    Goose g(514, "Comrade", 1920, 1080);

    g_config.general.appearanceMode = APPEARANCE_STALIN;
    g.onHonk();
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Gulag), 1);
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 0);

    g_config.general.appearanceMode = 0;
    g.onHonk();
    EXPECT_EQ(Audio_StartedForTest(AudioClip::Honk), 1);
}

}  // namespace

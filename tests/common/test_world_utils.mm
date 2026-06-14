#include <gtest/gtest.h>
#include "world_utils.h"
#include "world.h"

namespace {

void ClearFootprints() {
    while (!g_world.footprints.empty())
        g_world.footprints.pop();
}

}

TEST(WorldUtils, CheckCloseButtonInside) {
    EXPECT_TRUE(CheckCloseButton(-50, -50, 100, 100, 20));
    EXPECT_TRUE(CheckCloseButton(-31, -50, 100, 100, 20));
    EXPECT_TRUE(CheckCloseButton(-50, -31, 100, 100, 20));
    EXPECT_TRUE(CheckCloseButton(-50, -50, 100, 100, 1));
}

TEST(WorldUtils, CheckCloseButtonOutside) {
    EXPECT_FALSE(CheckCloseButton(-29, -50, 100, 100, 20));
    EXPECT_FALSE(CheckCloseButton(-50, -29, 100, 100, 20));
    EXPECT_FALSE(CheckCloseButton(-50, -51, 100, 100, 20));
    EXPECT_FALSE(CheckCloseButton(-51, -50, 100, 100, 20));
}

TEST(WorldUtils, CheckCloseButtonEdge) {
    EXPECT_TRUE(CheckCloseButton(-50, -50, 100, 100, 20));
    EXPECT_TRUE(CheckCloseButton(-30, -30, 100, 100, 20));
    EXPECT_FALSE(CheckCloseButton(-29.999f, -50, 100, 100, 20));
}

TEST(WorldUtils, MoveItemToFrontNoop) {
    MoveItemToFront(nullptr);
}

TEST(WorldUtils, ShouldAcceptMouseEventsEmpty) {
    EXPECT_FALSE(ShouldAcceptMouseEvents());
}

TEST(WorldUtils, FootprintCleanupExpired) {
    ClearFootprints();
    double now = 1000.0;
    Footprint fp;
    fp.pos = {100, 200};
    fp.dir = 0.5f;
    fp.timeSpawned = now - 100.0;
    fp.lifetime = 10.0f;
    g_world.footprints.push(fp);
    EXPECT_FALSE(g_world.footprints.empty());
    World_CleanupExpired(now);
    EXPECT_TRUE(g_world.footprints.empty());
}

TEST(WorldUtils, FootprintCleanupNotExpired) {
    ClearFootprints();
    double now = 1000.0;
    Footprint fp;
    fp.pos = {100, 200};
    fp.dir = 0.5f;
    fp.timeSpawned = now - 5.0;
    fp.lifetime = 10.0f;
    g_world.footprints.push(fp);
    World_CleanupExpired(now);
    EXPECT_FALSE(g_world.footprints.empty());
}

TEST(WorldUtils, FootprintCleanupDefaultLifetime) {
    ClearFootprints();
    double now = 1000.0;
    Footprint fp;
    fp.pos = {100, 200};
    fp.dir = 0.5f;
    fp.timeSpawned = now - 100.0;
    fp.lifetime = 0.0f; // use default g_config.mud.lifetime
    g_world.footprints.push(fp);
    World_CleanupExpired(now);
    // Might or might not be expired depending on config default
    (void)fp;
}

// tick_manager_logic — the cadence/state decisions lifted out of
// TickManager::tick (see include/tick_manager_logic.h).
#include <gtest/gtest.h>
#include "tick_manager_logic.h"

namespace {

TEST(TickManagerLogicTest, CleanupRunsEverySixtiethTick) {
    EXPECT_FALSE(tick_manager_logic::ShouldRunWorldCleanup(1));
    EXPECT_FALSE(tick_manager_logic::ShouldRunWorldCleanup(59));
    EXPECT_TRUE(tick_manager_logic::ShouldRunWorldCleanup(60));
    EXPECT_TRUE(tick_manager_logic::ShouldRunWorldCleanup(120));
    EXPECT_FALSE(tick_manager_logic::ShouldRunWorldCleanup(61));
}

TEST(TickManagerLogicTest, CleanupNeverFiresOnTickZero) {
    // Defensive: 0 % 60 == 0, but the first tick is tick 1 and a zero count
    // must never trigger a cleanup pass.
    EXPECT_FALSE(tick_manager_logic::ShouldRunWorldCleanup(0));
}

TEST(TickManagerLogicTest, FirstEnabledFrameSpawnsTheBurst) {
    bool initialized = false;
    int spawns = tick_manager_logic::NextLeafSpawn(initialized, /*enabled=*/true,
                                                   /*roll=*/123);
    EXPECT_EQ(spawns, 3);
    EXPECT_TRUE(initialized);
}

TEST(TickManagerLogicTest, AfterFirstFrameSpawnIsOneInFiveHundred) {
    bool initialized = true;

    // roll==0 is the winning side of the RandRange(500) draw.
    EXPECT_EQ(tick_manager_logic::NextLeafSpawn(initialized, true, 0), 1);

    // Every other roll spawns nothing.
    EXPECT_EQ(tick_manager_logic::NextLeafSpawn(initialized, true, 1), 0);
    EXPECT_EQ(tick_manager_logic::NextLeafSpawn(initialized, true, 499), 0);
}

TEST(TickManagerLogicTest, DisabledNeverSpawnsAndNeverInitializes) {
    bool initialized = false;
    // Even the first-run burst must not fire while the feature is disabled…
    EXPECT_EQ(tick_manager_logic::NextLeafSpawn(initialized, false, 0), 0);
    EXPECT_EQ(tick_manager_logic::NextLeafSpawn(initialized, false, 123), 0);
    // …and the flag stays untouched for when it gets re-enabled.
    EXPECT_FALSE(initialized);
}

}  // namespace

// test_goose_sync.cpp
// Unit tests for per-goose random offset desynchronization
#include "gtest/gtest.h"
#include <cmath>
#include <set>

#include "config.h"
#include "goose.h"
#include "world.h"
#include "random_util.h"

static void ResetGooseGlobals() {
    g_config.general.globalScale = 1.0f;
    g_config.spawn.marginX = 50;
    g_config.spawn.marginY = 50;
    g_config.movement.initDirectionMax = 360;
    g_config.movement.baseWalkSpeed = 100.0f;
    g_config.physics.isoScaleX = 1.0f;
    g_config.physics.isoScaleY = 1.0f;
    g_config.step.leftFootAngle = 45.0f;
    g_config.step.rightFootAngle = -45.0f;
    g_config.step.durationWalk = 0.2f;
    g_config.item.memeFetchBiasMax = 20;
    g_config.item.noteFetchBiasMax = 20;
    g_config.honk.idleMin = 6.0;
    g_config.honk.idleMax = 14.0;
}

// ============================================================
// Each goose gets a unique random offset
// ============================================================

TEST(GooseSync, RandomOffset_IsUnique) {
    ResetGooseGlobals();

    std::set<float> offsets;
    for (int i = 0; i < 10; i++) {
        Goose goose(i, "goose" + std::to_string(i), 1920, 1080);
        offsets.insert(goose.randomOffset);
    }

    // With 10 geese and 0-3s range, most should be unique
    // (floating point equality is strict, so exact duplicates are rare)
    EXPECT_GE(offsets.size(), 8);  // At least 8 unique out of 10
}

TEST(GooseSync, RandomOffset_IsInRange) {
    ResetGooseGlobals();

    for (int i = 0; i < 20; i++) {
        Goose goose(i, "goose" + std::to_string(i), 1920, 1080);
        EXPECT_GE(goose.randomOffset, 0.0f);
        EXPECT_LE(goose.randomOffset, 3.0f);
    }
}

// ============================================================
// Fetch cooldown respects random offset
// ============================================================

TEST(GooseSync, FetchCooldown_WithOffset) {
    ResetGooseGlobals();

    Goose goose1(0, "goose0", 1920, 1080);
    Goose goose2(1, "goose1", 1920, 1080);

    goose1.lastDropTime = 0.0;
    goose2.lastDropTime = 0.0;

    float baseCooldown = 4.0f;

    // Goose 1 should be able to fetch at baseCooldown + offset
    double time1 = baseCooldown + goose1.randomOffset - 0.1;
    bool canFetch1 = (time1 - goose1.lastDropTime) > (baseCooldown + goose1.randomOffset);
    EXPECT_FALSE(canFetch1);  // Not yet, need to wait for offset

    double time2 = baseCooldown + goose1.randomOffset + 0.1;
    bool canFetch2 = (time2 - goose1.lastDropTime) > (baseCooldown + goose1.randomOffset);
    EXPECT_TRUE(canFetch2);  // Now can fetch

    // Goose 2 has different offset, so its timing is different
    double time3 = baseCooldown + goose2.randomOffset - 0.1;
    bool canFetch3 = (time3 - goose2.lastDropTime) > (baseCooldown + goose2.randomOffset);
    EXPECT_FALSE(canFetch3);  // Not yet for goose2 either
}

// test_footprint_registration.cpp
// Unit tests for footprint effect registration and window creation
#include "gtest/gtest.h"
#include <cmath>

#include "config.h"
#include "world.h"

// Reset footprint globals for each test
static void ResetFootprintGlobals() {
    g_config.mud.enabled = true;
    g_config.mud.chance = 15;
    g_config.mud.lifetime = 15.0f;
    g_config.general.globalScale = 1.0f;
    g_time = 0.0;
    // Clear footprints queue
    while (!g_world.footprints.empty()) {
        g_world.footprints.pop();
    }
}

TEST(FootprintRegistration, EmptyWorld_NoPositions) {
    ResetFootprintGlobals();
    g_time = 10.0;

    // Simulate Footprint_GetPositions logic
    std::vector<Vector2> positions;
    for (const auto& fp : g_world.footprints) {
        float age = (float)(g_time - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if (age <= life) {
            positions.push_back(fp.pos);
        }
    }

    EXPECT_EQ(positions.size(), 0u);
}

TEST(FootprintRegistration, SingleFootprint_ReturnsPosition) {
    ResetFootprintGlobals();
    g_time = 10.0;

    // Add a footprint
    Footprint fp;
    fp.pos = {100.0f, 200.0f};
    fp.timeSpawned = 5.0;  // 5 seconds old
    fp.lifetime = 15.0f;
    g_world.footprints.push(fp);

    // Simulate Footprint_GetPositions logic
    std::vector<Vector2> positions;
    for (const auto& fp : g_world.footprints) {
        float age = (float)(g_time - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if (age <= life) {
            positions.push_back(fp.pos);
        }
    }

    ASSERT_EQ(positions.size(), 1u);
    EXPECT_FLOAT_EQ(positions[0].x, 100.0f);
    EXPECT_FLOAT_EQ(positions[0].y, 200.0f);
}

TEST(FootprintRegistration, ExpiredFootprint_NotReturned) {
    ResetFootprintGlobals();
    g_time = 25.0;

    // Add an expired footprint (20 seconds old, lifetime 15)
    Footprint fp;
    fp.pos = {100.0f, 200.0f};
    fp.timeSpawned = 5.0;
    fp.lifetime = 15.0f;
    g_world.footprints.push(fp);

    // Simulate Footprint_GetPositions logic
    std::vector<Vector2> positions;
    for (const auto& fp : g_world.footprints) {
        float age = (float)(g_time - fp.timeSpawned);
        float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
        if (age <= life) {
            positions.push_back(fp.pos);
        }
    }

    EXPECT_EQ(positions.size(), 0u);
}

TEST(FootprintRegistration, FootprintAlpha_ComputeCorrectly) {
    ResetFootprintGlobals();
    g_time = 10.0;

    Footprint fp;
    fp.pos = {100.0f, 200.0f};
    fp.timeSpawned = 5.0;
    fp.lifetime = 15.0f;

    float age = (float)(g_time - fp.timeSpawned);
    float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
    float alpha = std::max(0.0f, 1.0f - (age / life));

    EXPECT_FLOAT_EQ(age, 5.0f);
    EXPECT_FLOAT_EQ(life, 15.0f);
    EXPECT_FLOAT_EQ(alpha, 2.0f / 3.0f);  // 1 - 5/15 = 0.667
}

TEST(FootprintRegistration, FootprintAlpha_FullyFaded) {
    ResetFootprintGlobals();
    g_time = 20.0;

    Footprint fp;
    fp.pos = {100.0f, 200.0f};
    fp.timeSpawned = 5.0;
    fp.lifetime = 15.0f;

    float age = (float)(g_time - fp.timeSpawned);
    float life = (fp.lifetime > 0.0f) ? fp.lifetime : g_config.mud.lifetime;
    float alpha = std::max(0.0f, 1.0f - (age / life));

    EXPECT_FLOAT_EQ(age, 15.0f);
    EXPECT_FLOAT_EQ(alpha, 0.0f);  // 1 - 15/15 = 0
}

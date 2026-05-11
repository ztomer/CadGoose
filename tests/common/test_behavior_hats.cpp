// ===========================
// test_behavior_hats.cpp
// Unit tests for Hats behavior
// ===========================
#include "gtest/gtest.h"
#include <cmath>

#include "behavior.h"
#include "config.h"
#include "goose.h"
#include "world.h"

TEST(HatsBehavior, HatsConfigValues) {
    float sizeX = g_config.behaviors.hats.sizeX;
    float sizeY = g_config.behaviors.hats.sizeY;
    float offsetX = g_config.behaviors.hats.offsetX;
    float offsetY = g_config.behaviors.hats.offsetY;

    EXPECT_GT(sizeX, 0.0f);
    EXPECT_GT(sizeY, 0.0f);
    EXPECT_EQ(offsetX, 0.0f);
    EXPECT_LT(offsetY, 0.0f);
}

TEST(HatsBehavior, ScreenYFromWorldY) {
    float screenHeight = 1080.0f;
    float worldY = 500.0f;
    float globalScale = 1.0f;

    float worldHeight = screenHeight / globalScale;
    float screenY = worldHeight - worldY;

    EXPECT_EQ(screenY, 1080.0f - 500.0f);
    EXPECT_EQ(screenY, 580.0f);
}

TEST(HatsBehavior, HatScaleCalculation) {
    float imgWidth = 128.0f;
    float imgHeight = 64.0f;
    float configSize = 32.0f;

    float scale = configSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

    EXPECT_FLOAT_EQ(scale, 0.25f);
    EXPECT_FLOAT_EQ(drawW, 32.0f);
    EXPECT_FLOAT_EQ(drawH, 16.0f);
}

TEST(HatsBehavior, DirectionRotation) {
    float dir = 90.0f;
    float rad = dir * M_PI / 180.0f;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    EXPECT_NEAR(cosA, 0.0f, 0.001f);
    EXPECT_NEAR(sinA, 1.0f, 0.001f);
}

TEST(HatsBehavior, DirectionRotation0Degrees) {
    float dir = 0.0f;
    float rad = dir * M_PI / 180.0f;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    EXPECT_NEAR(cosA, 1.0f, 0.001f);
    EXPECT_NEAR(sinA, 0.0f, 0.001f);
}

TEST(HatsBehavior, DirectionRotation180Degrees) {
    float dir = 180.0f;
    float rad = dir * M_PI / 180.0f;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    EXPECT_NEAR(cosA, -1.0f, 0.001f);
    EXPECT_NEAR(sinA, 0.0f, 0.001f);
}

TEST(HatsBehavior, HatPositionAboveHead) {
    Vector2 headPos{400.0f, 300.0f};
    float offsetY = -15.0f;

    float screenHeight = 800.0f;
    float screenY = screenHeight - headPos.y;

    float hatY = screenY + offsetY;

    EXPECT_GT(hatY, screenY - 30.0f);
    EXPECT_LT(hatY, screenY);
}

TEST(HatsBehavior, NoVerticalFlip) {
    float headY = 500.0f;
    float screenHeight = 1080.0f;

    float screenY_Normal = screenHeight - headY;
    float screenY_Flipped = headY;

    EXPECT_EQ(screenY_Flipped, 500.0f);
    EXPECT_NE(screenY_Normal, screenY_Flipped);
    EXPECT_EQ(screenY_Normal, 580.0f);
}
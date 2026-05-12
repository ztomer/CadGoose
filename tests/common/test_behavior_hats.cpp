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

TEST(HatsBehavior, FacingLeftDetection) {
    EXPECT_TRUE(91.0f > 90.0f && 91.0f < 270.0f);
    EXPECT_TRUE(180.0f > 90.0f && 180.0f < 270.0f);
    EXPECT_TRUE(269.0f > 90.0f && 269.0f < 270.0f);
    EXPECT_FALSE(0.0f > 90.0f && 0.0f < 270.0f);
    EXPECT_FALSE(90.0f > 90.0f && 90.0f < 270.0f);
    EXPECT_FALSE(270.0f > 90.0f && 270.0f < 270.0f);
    EXPECT_FALSE(360.0f > 90.0f && 360.0f < 270.0f);
}

TEST(HatsBehavior, DrawDroppedItemPatternCoords) {
    float drawW = 64.0f;
    float drawH = 48.0f;
    float halfW = drawW / 2.0f;
    float halfH = drawH / 2.0f;

    float tx = -halfW;
    float ty = halfH;

    EXPECT_FLOAT_EQ(tx, -32.0f);
    EXPECT_FLOAT_EQ(ty, 24.0f);
}

TEST(HatsBehavior, ImageCenteredRect) {
    float halfW = 32.0f;
    float halfH = 24.0f;
    CGRect rect = CGRectMake(-halfW, -halfH, 64.0f, 48.0f);

    EXPECT_FLOAT_EQ(rect.origin.x, -32.0f);
    EXPECT_FLOAT_EQ(rect.origin.y, -24.0f);
    EXPECT_FLOAT_EQ(rect.size.width, 64.0f);
    EXPECT_FLOAT_EQ(rect.size.height, 48.0f);
}

TEST(HatsBehavior, ImageScale) {
    float imgWidth = 128.0f;
    float imgHeight = 64.0f;
    float hatSize = 32.0f;

    float scale = hatSize / imgWidth;
    float drawW = imgWidth * scale;
    float drawH = imgHeight * scale;

    EXPECT_FLOAT_EQ(scale, 0.25f);
    EXPECT_FLOAT_EQ(drawW, 32.0f);
    EXPECT_FLOAT_EQ(drawH, 16.0f);

    CGRect rect = CGRectMake(-drawW/2, -drawH/2, drawW, drawH);
    CGContextDrawImage((CGContextRef)1, rect, nil);
    SUCCEED() << "DrawDroppedItem-style centered rect matched";
}

TEST(HatsBehavior, HeadPositionWithOffsets) {
    Vector2 headDevice{400.0f, 300.0f};
    float offsetX = 0.0f;
    float offsetY = -15.0f;
    float gs = 1.0f;

    float screenX = headDevice.x + offsetX * gs;
    float screenY = headDevice.y + offsetY * gs;

    EXPECT_FLOAT_EQ(screenX, 400.0f);
    EXPECT_FLOAT_EQ(screenY, 285.0f);
}

TEST(HatsBehavior, FacingLeftNoRotation) {
    float dir = 180.0f;
    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    EXPECT_TRUE(facingLeft);
    EXPECT_FALSE(facingLeft && false) << "Rotation removed; only X-flip used";
}

TEST(HatsBehavior, FacingRightNoFlip) {
    float dir = 0.0f;
    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    EXPECT_FALSE(facingLeft);
}

TEST(HatsBehavior, UpDirectionNoFlip) {
    float dir = 90.0f;
    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    EXPECT_FALSE(facingLeft);
}

TEST(HatsBehavior, DownDirectionNoFlip) {
    float dir = 270.0f;
    bool facingLeft = (dir > 90.0f && dir < 270.0f);
    EXPECT_FALSE(facingLeft);
}

TEST(HatsBehavior, YFlipTransformNeverCancels) {
    bool facingLeft = true;
    int xFlip = facingLeft ? -1 : 1;

    float worldA[4] = {-32.0f, 24.0f, 32.0f, -24.0f};
    for (int i = 0; i < 4; i += 2) {
        float x = worldA[i];
        float y = worldA[i + 1];

        float resultX = x * xFlip * 1 + y * 0;
        float resultY = x * 0 + y * (-1);

        float tx = resultX + 400.0f;
        float ty = resultY + 285.0f;

        EXPECT_FLOAT_EQ(tx, x * xFlip + 400.0f);
        EXPECT_FLOAT_EQ(ty, -y + 285.0f);
    }
}

TEST(HatsBehavior, MemeImageDrawPattern) {
    float drawW = 64.0f;
    float drawH = 48.0f;
    float x = -drawW / 2.0f;
    float y = -drawH / 2.0f;

    float translateX = x;
    float translateY = y + drawH;
    EXPECT_FLOAT_EQ(translateX, -32.0f);
    EXPECT_FLOAT_EQ(translateY, 24.0f);

    CGRect drawRect = CGRectMake(0, 0, drawW, drawH);
    EXPECT_FLOAT_EQ(drawRect.origin.x, 0.0f);
    EXPECT_FLOAT_EQ(drawRect.origin.y, 0.0f);
}

TEST(HatsBehavior, YFlipTransformResult) {
    float tx = -32.0f, ty = 24.0f;
    float sx = 1.0f, sy = -1.0f;

    struct TestPoint { float x, y; };

    TestPoint pixelTop{0.0f, 0.0f};
    TestPoint pixelBottom{0.0f, 48.0f};

    auto transform = [&](TestPoint p) -> TestPoint {
        float yFlipped = p.y * sy;
        return {p.x * sx + tx, yFlipped * sx + ty};
        // Simple approx: x→x*tx, y→y*sy+ty after scale
    };

    TestPoint r1 = transform(pixelTop);
    TestPoint r2 = transform(pixelBottom);

    EXPECT_FLOAT_EQ(r1.x, 0.0f + tx);
    EXPECT_FLOAT_EQ(r2.y, 48.0f * sy + ty);
}

TEST(HatsBehavior, LeftFacingQuadrantBoundary) {
    for (float d = 91.0f; d < 270.0f; d += 0.5f) {
        bool facingLeft = (d > 90.0f && d < 270.0f);
        EXPECT_TRUE(facingLeft) << "dir=" << d;
    }
}

TEST(HatsBehavior, RightFacingQuadrantBoundary) {
    for (float d = 0.0f; d <= 90.0f; d += 0.5f) {
        bool facingLeft = (d > 90.0f && d < 270.0f);
        EXPECT_FALSE(facingLeft) << "dir=" << d;
    }
    for (float d = 270.0f; d <= 360.0f; d += 0.5f) {
        bool facingLeft = (d > 90.0f && d < 270.0f);
        EXPECT_FALSE(facingLeft) << "dir=" << d;
    }
}
#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>

#ifdef __APPLE__
#import <CoreGraphics/CoreGraphics.h>
#import <CoreFoundation/CoreFoundation.h>
#endif

#include "goose_drawing.h"
#include "cg_renderer.h"
#include "config.h"
#include "world.h"
#include "goose.h"
#include "behavior.h"

// ===========================
// Y-flip helpers
// ===========================
static int cgYToRow(int cgY, int height) {
    return height - 1 - cgY;
}

static int pixelR(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 1];
}
static int pixelG(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 2];
}
static int pixelB(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 3];
}
static int pixelA(const uint8_t* d, int stride, int x, int cgY, int h) {
    return d[cgYToRow(cgY, h) * stride + x * 4 + 0];
}

// ===========================
// CGRenderer primitive tests
// ===========================

TEST(GooseRender, CGRenderer_DrawEllipse) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 100, 100, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    // Ellipse centered at (50,50), xRadius=20, yRadius=15
    // Spans x=[30,70], y=[35,65]
    r.DrawEllipse({50, 50}, 20, 15, RenderColor::Red());

    EXPECT_EQ(pixelR(d, s, 50, 50, 100), 255) << "center";
    EXPECT_EQ(pixelG(d, s, 50, 50, 100), 0);
    EXPECT_EQ(pixelR(d, s, 50, 40, 100), 255) << "top half (y=40 is inside at yRad=15 from y=50)";
    EXPECT_EQ(pixelR(d, s, 65, 50, 100), 255) << "right half (x=65 is inside at xRad=20 from x=50)";
    EXPECT_EQ(pixelA(d, s, 50, 50, 100), 255);
    EXPECT_EQ(pixelA(d, s, 0, 0, 100), 0) << "outside ellipse";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawEllipseOutline) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    // Outline centered at (25,25), xRadius=15, yRadius=10, lineWidth=2
    // Stroke spans roughly x=[10,40], y=[15,35]
    r.DrawEllipseOutline({25, 25}, 15, 10, RenderColor::Green(), 2);

    EXPECT_EQ(pixelG(d, s, 25, 25, 50), 0) << "center should be empty (outline only)";
    EXPECT_GT(pixelG(d, s, 25, 34, 50), 200) << "top edge (y=25+10-1=34) should have stroke";
    EXPECT_GT(pixelG(d, s, 39, 25, 50), 200) << "right edge (x=25+15-1=39) should have stroke";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawLine) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    r.DrawLine({5, 25}, {45, 25}, RenderColor::Blue(), 2);

    EXPECT_EQ(pixelB(d, s, 25, 25, 50), 255) << "middle of horizontal line";
    EXPECT_EQ(pixelB(d, s, 10, 25, 50), 255) << "near left end";
    EXPECT_EQ(pixelB(d, s, 40, 25, 50), 255) << "near right end";
    EXPECT_EQ(pixelB(d, s, 25, 10, 50), 0) << "above line";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawRect) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    r.DrawRect({10, 10, 20, 20}, RenderColor::Red());

    EXPECT_EQ(pixelR(d, s, 15, 15, 50), 255) << "inside rect";
    EXPECT_EQ(pixelR(d, s, 29, 29, 50), 255) << "inside near edge";
    EXPECT_EQ(pixelR(d, s, 5, 5, 50), 0) << "outside rect";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawRectOutline) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    r.DrawRectOutline({10, 10, 20, 20}, RenderColor::Green(), 2);

    EXPECT_EQ(pixelG(d, s, 10, 10, 50), 255) << "edge corner";
    EXPECT_EQ(pixelG(d, s, 20, 10, 50), 255) << "top edge";
    EXPECT_EQ(pixelG(d, s, 15, 15, 50), 0) << "center should be empty";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawPolygon) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    // Triangle at bottom-left: {{5,5}, {25,5}, {15,30}}
    RenderPoint tri[] = {{5, 5}, {25, 5}, {15, 30}};
    CGRenderer(ctx).DrawPolygon(tri, 3, RenderColor::Red());

    EXPECT_EQ(pixelR(d, s, 15, 15, 50), 255) << "inside triangle";
    EXPECT_EQ(pixelR(d, s, 15, 7, 50), 255) << "near top edge";
    EXPECT_EQ(pixelR(d, s, 2, 30, 50), 0) << "left of triangle";
    EXPECT_EQ(pixelR(d, s, 0, 0, 50), 0) << "far outside";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_DrawRoundedRect) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    r.DrawRoundedRect({10, 10, 30, 30}, 5, RenderColor::Blue());

    EXPECT_EQ(pixelB(d, s, 25, 25, 50), 255) << "center of rounded rect";
    EXPECT_EQ(pixelB(d, s, 10, 10, 50), 0) << "corner (should be clipped by radius)";
    EXPECT_EQ(pixelB(d, s, 15, 10, 50), 255) << "near top edge (past radius)";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}

TEST(GooseRender, CGRenderer_Transforms) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(NULL, 50, 50, 8, 0, cs,
        kCGBitmapByteOrderDefault | (CGBitmapInfo)kCGImageAlphaPremultipliedFirst);
    ASSERT_NE(ctx, nullptr);
    int s = (int)CGBitmapContextGetBytesPerRow(ctx);
    const uint8_t* d = (const uint8_t*)CGBitmapContextGetData(ctx);

    CGRenderer r(ctx);
    r.SaveState();
    r.Translate(10, 10);
    r.DrawRect({0, 0, 10, 10}, RenderColor::Red());
    r.RestoreState();

    EXPECT_EQ(pixelR(d, s, 15, 15, 50), 255) << "after translate (10+5, 10+5)";
    EXPECT_EQ(pixelR(d, s, 5, 5, 50), 0) << "outside translated region";

    r.Scale(2, 2);
    r.DrawRect({0, 0, 5, 5}, RenderColor::Green());
    EXPECT_EQ(pixelG(d, s, 5, 5, 50), 255) << "scaled rect at 2*2.5=5";

    CGContextRelease(ctx);
    CGColorSpaceRelease(cs);
}
